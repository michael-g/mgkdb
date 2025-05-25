#include <stdlib.h> // EXIT_SUCCESS

#include <stdint.h>
#include <fcntl.h> // open flags
#include <sys/stat.h> // struct stat
#include <sys/mman.h> // MAP_FAILED
#include <unistd.h> // lseek, write
#include <string.h>

#include <unordered_set> // std::unordered_set
#include <filesystem>
#include <utility> // std::pair

#include "mg_coro_domain_obj.h"
#include "mg_fmt_defs.h"
#include "mg_io.h"
#include "mg_coro_epoll.h"
#include "mg_coro_task.h"

#include "KdbType.h"
#include "KdbIoDefs.h"

namespace mg7x {

extern
TASK_TYPE<std::expected<mg7x::ReadMsgResult,ErrnoMsg>>
  kdb_read_message(EpollCtl & epoll, int fd, std::vector<int8_t> & ary);

static int monitor_close(int fd)
{
  std::expected<int,int> result = ::mg7x::io::close(fd);
  if (!result.has_value()) {
    ERR_PRINT(CYN "kdb_subscribe" RST ": failed to close FD {}: {}", fd, strerror(result.error()));
  }
  return -1;
}

static auto bad_structure(int fd)
{
  monitor_close(fd);
  return std::unexpected(ErrnoMsg{0, "Bad repsonse struture"});
}

TASK_TYPE<std::expected<int,ErrnoMsg>> kdb_subscribe_and_replay(EpollCtl & epoll, const io::TcpConn & conn, KdbJournal & dst_jnl, const Subscription & sub, TpMsgCounts & counts)
{
  DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": have sock_fd {}", conn.sock_fd());

  mg7x::KdbSymbolAtom fun{".u.sub"};
  mg7x::KdbSymbolVector tbl{sub.tables()};
  mg7x::KdbSymbolAtom sym{};
  mg7x::KdbList msg{3};
  msg.push(fun);
  msg.push(tbl);
  msg.push(sym);

  const size_t ipc_len = mg7x::KdbUtil::ipcMessageLen(msg);
  std::vector<int8_t> ary(512); // we assume 512 is greater than ipc_len
  ary.resize(0);

  DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": subscription message is {}", msg);

  mg7x::KdbIpcMessageWriter writer{mg7x::KdbMsgType::SYNC, msg};

  mg7x::WriteResult res = writer.write(ary.data(), ary.capacity());
  TRA_PRINT(CYN "kdb_subscribe_and_replay" RST ": serialized subscription message, result is {}", res);

  if (mg7x::WriteResult::WR_OK != res) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed inexplicably to serialise message; closing FD {}", conn.sock_fd());
    monitor_close(conn.sock_fd());
    co_return std::unexpected(ErrnoMsg{0, "Failed to serialise subscribe-message"});
  }

  ssize_t wrt = 0;
  std::expected<ssize_t,int> wr_res;
  do {
    // we assume it's writable here, Lord, it's only just been created...
    wr_res = ::mg7x::io::write(conn.sock_fd(), ary.data() + wrt, ipc_len - wrt);
    if (!wr_res.has_value()) {
      // TODO check for EINTR etc
      ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed in write: {}; closing FD {}", strerror(errno), conn.sock_fd());
      monitor_close(conn.sock_fd());
      co_return std::unexpected(ErrnoMsg{0, "Failure reported during write"});
    }
    wrt += wr_res.value();
    DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": Wrote {} bytes of {} to FD {}, awaiting kdb_read_message", wr_res.value(), ipc_len, conn.sock_fd());
  } while (wrt < ipc_len);

  std::expected<mg7x::ReadMsgResult,ErrnoMsg> rd_msg_res = co_await kdb_read_message(epoll, conn.sock_fd(), ary);

  if (!rd_msg_res.has_value()) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed to complete read; closing FD {}", conn.sock_fd());
    monitor_close(conn.sock_fd());
    co_return std::unexpected(rd_msg_res.error());
  }
  if (mg7x::KdbMsgType::RESPONSE != rd_msg_res.value().msg_typ) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": unexpected response to .u.sub; closing FD {}", conn.sock_fd());
    co_return bad_structure(conn.sock_fd());
  }
  KdbBase *sub_ptr = rd_msg_res.value().message.get();

  if (mg7x::KdbType::LIST != sub_ptr->m_typ) {
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": expected response of type LIST but received {}: {}; closing FD {}", sub_ptr->m_typ, *sub_ptr, conn.sock_fd());
    co_return bad_structure(conn.sock_fd());
  }
  mg7x::KdbList *lst = dynamic_cast<mg7x::KdbList*>(sub_ptr);
  if (mg7x::KdbType::LONG_ATOM != lst->typeAt(0)) {
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": expected LONG_ATOM at index [0], found {}; closing FD {}", lst->typeAt(0), conn.sock_fd());
    co_return bad_structure(conn.sock_fd());
  }
  if (3 != lst->count()) {
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": expected 3 elements but found {}; closing FD {}", lst->count(), conn.sock_fd());
    co_return bad_structure(conn.sock_fd());
  }
  if (mg7x::KdbType::SYMBOL_ATOM != lst->typeAt(1)) {
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": expected SYMBOL_ATOM at index [1], found {}; closing FD {}", lst->typeAt(1), conn.sock_fd());
    co_return bad_structure(conn.sock_fd());
  }
  if (mg7x::KdbType::LIST != lst->typeAt(2)) {
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": expected LIST at index [2], found {}; closing FD {}", lst->typeAt(2), conn.sock_fd());
    co_return bad_structure(conn.sock_fd());
  }

  const mg7x::KdbLongAtom *msg_count = dynamic_cast<const mg7x::KdbLongAtom*>(lst->getObj(0));
  const mg7x::KdbSymbolAtom *path = dynamic_cast<const mg7x::KdbSymbolAtom*>(lst->getObj(1));

  if (msg_count->m_val < 0) {
    monitor_close(conn.sock_fd());
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": illegal .u.i message count is {} for FD {}", msg_count->m_val, conn.sock_fd());
    co_return std::unexpected(ErrnoMsg{0, "illegal .u.i value upon subscription"});
  }

  if (msg_count->m_val < counts.m_num_msg_total) {
    monitor_close(conn.sock_fd());
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": reported .u.i message count is {}, alleged message-count previously replayed is {}, FD is {}", msg_count->m_val, counts.m_num_msg_total, conn.sock_fd());
    co_return std::unexpected(ErrnoMsg{0, "fewer messages in src-journal than we have apparently alread read"});
  }

  INF_PRINT(CYN "kdb_subscribe_and_replay" RST ": now subscribed; TP message-count is {}, journal is {}, will skip {} messages", msg_count->m_val, path->m_val, counts.m_num_msg_total);

  size_t pth_off = 0;
  if (':' == path->m_val.at(0) && path->m_val.length() > 1)
    pth_off = path->m_val.find_first_not_of(":");

  // unpack the journal location, open, scan for tables, rewrite into ours
  const std::string_view src_path{path->m_val.c_str() + pth_off};

  struct KdbJournal::Options opts{
    .read_only = true,
    .validate_and_count_upon_init = false,
  };

  std::expected<KdbJournal,std::string> jnl_res = KdbJournal::init(std::filesystem::path{src_path}, opts);
  if (!jnl_res) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed while reading remote TP log {}: {}", src_path, jnl_res.error());
    monitor_close(conn.sock_fd());
    co_return std::unexpected(ErrnoMsg{0, "Failed while reading remote journal"});
  }

  KdbJournal src_jnl = jnl_res.value();
  const int dst_fd = dst_jnl.jnl_fd();

  std::string_view fn_name{"upd"};
  const std::unordered_set<std::string_view> names{sub.tables().begin(), sub.tables().end()};

  auto scribe = [&src_path, dst_fd](const int8_t *src, uint64_t len) -> int {
    std::expected<ssize_t,int> res_zi = ::mg7x::io::write(dst_fd, src, len);
    if (!res_zi) {
      ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed while copying into local journal ({}): {}", src_path, strerror(res_zi.error()));
      return -1;
    }
    return 1;
  };
  const uint64_t skip = counts.m_num_msg_total;
  auto filter = KdbJournal::mk_upd_tbl_filter(skip, fn_name, names, scribe);

  std::expected<std::pair<uint64_t,uint64_t>,std::string> res_ps = src_jnl.filter_msgs(static_cast<uint64_t>(msg_count->m_val), filter);
  if (!res_ps) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed while filtering {}: {}", src_path, res_ps.error());
    monitor_close(conn.sock_fd());
    co_return std::unexpected(ErrnoMsg{0, "Failed while filtering remote journal"});
  }

  counts.m_num_msg_total = res_ps.value().first;
  counts.m_num_msg_included += res_ps.value().second;

  INF_PRINT(CYN "kdb_subscribe_and_replay" RST ": replayed {} messages from {}: skipped {}, copied {}; total from this TP in our journal now {}", counts.m_num_msg_total, src_path, skip, res_ps.value().second, counts.m_num_msg_included);

  co_return 0;
}

}; // end namespace mg7x
