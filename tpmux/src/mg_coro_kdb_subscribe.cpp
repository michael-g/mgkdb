#include <stdlib.h> // EXIT_SUCCESS

#include <stdint.h>
#include <fcntl.h> // open flags
#include <sys/stat.h> // struct stat
#include <sys/mman.h> // MAP_FAILED
#include <unistd.h> // lseek, write
#include <string.h>

#include <unordered_set> // std::unordered_set
#include <algorithm> // std::copy

#include "mg_coro_domain_obj.h"
#include "mg_fmt_defs.h"
#include "mg_io.h"
#include "mg_coro_epoll.h"
#include "mg_coro_task.h"

#include "KdbType.h"

namespace mg7x {

extern
Task<std::expected<mg7x::ReadMsgResult,ErrnoMsg>>
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

static int cp_sgmt(const int dst_fd, const int src_fd, const uint64_t src_off, const size_t len)
{
  off_t l_src_off = static_cast<off_t>(src_off);
  std::expected<ssize_t,int> io_res = ::mg7x::io::sendfile(dst_fd, src_fd, &l_src_off, len);
  if (!io_res.has_value()) {
    ERR_PRINT("failed in sendfile; dst_fd {}, src_fd {}, src_off: {}, len: {}: {}", dst_fd, src_fd, src_off, len, strerror(io_res.error()));
    return -1;
  }
  if (static_cast<size_t>(io_res.value()) != len) {
    // doubt we'll hit this...
    ERR_PRINT("mismatch between length reported by sendfile and write-length requested; {} != {}", io_res.value(), len);
    return -1;
  }
  return 0;
}

static int filter_msgs(const int src_fd, const uint64_t msg_count, const int jnl_fd, const std::unordered_set<std::string_view> & tbls, TpMsgCounts & counts)
{
  struct stat sbuf{};
  std::expected<int,int> result = ::mg7x::io::fstat(src_fd, &sbuf);
  if (!result.has_value()) {
    ERR_PRINT("failed in fstat over source-journal on FD {}: {}", src_fd, strerror(result.error()));
    return -1;
  }

  std::expected<void*,int> map_res = ::mg7x::io::mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, src_fd, 0);
  if (!map_res.has_value()) {
    ERR_PRINT("failed in mmap; size {}, FD {}: {}", sbuf.st_size, src_fd, strerror(map_res.error()));
    return -1;
  }
  void *const addr = map_res.value();

  int8_t *ptr = static_cast<int8_t*>(addr);

  // skip the 8-byte header ...
  uint64_t off = mg7x::SZ_JNL_HDR;
  uint64_t len = 0;

#ifndef _MG_LOG_JNL_TESTS_
#define JNL_LOG(...)
#else
#define JNL_LOG(...) DBG_PRINT(__VA_ARGS__)
#endif

  const std::string_view fn_name{"upd"};

  for (uint64_t i = 0 ; i < msg_count ; i++) {
    // TODO implement message-skipping
    // TODO implement message counting, accumulate into 'counts'
    JNL_LOG("filter_msg({}, {}, {}, tbls)", ptr + off + len, sbuf.st_size - (off + len), fn_name);
    int64_t rtn = KdbJournalReader::filter_msg(ptr + off + len, sbuf.st_size - (off + len), fn_name, tbls);
    if (-2 == rtn) {
      goto err_jnl_ipc;
    }
    if (rtn < 0 || i == msg_count - 1) {
      if (len > 0) {
        JNL_LOG("cp_sgmt({}, {}, {}, {})", jnl_fd, src_fd, off, len);
        int err = cp_sgmt(jnl_fd, src_fd, off, len);
        if (-1 == err) {
          goto err_cp;
        }
      }
      off += len - rtn;
      len = 0;
    }
    else {
      len += rtn;
    }
  }
#undef JNL_LOG

  result = ::mg7x::io::munmap(addr, sbuf.st_size);
  if (!result.has_value()) {
    WRN_PRINT("failed in munmap: {}", strerror(result.error()));
  }
  return 0;

err_jnl_ipc:
err_cp:
  result = ::mg7x::io::munmap(addr, sbuf.st_size);
  if (!result.has_value()) {
    WRN_PRINT("failed in munmap: {}", strerror(result.error()));
  }
  return -1;
}

static std::expected<int,int> init_jnl(const std::string_view & path)
{
  char buf[path.length() + 1];
  std::copy(path.begin(), path.end(), buf);
  buf[path.length()] = 0;
  std::expected<int,int> result = ::mg7x::io::open(buf, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (!result.has_value()) {
    ERR_PRINT("failed to open/create file {}: {}", path, strerror(result.error()));
    return std::unexpected(result.error());
  }

  int jfd = result.value();

  DBG_PRINT("opened dest-journal {} as FD {}", path, jfd);

  struct stat sbuf{};
  result = ::mg7x::io::fstat(jfd, &sbuf);
  if (!result.has_value()) {
    ERR_PRINT("failed in fstat: {}", strerror(result.error()));
    goto err_fstat;
  }

  if (0 == sbuf.st_size) {
    struct {
      int8_t one = 0xff;
      int8_t two = 0x01;
      int16_t typ = 0;
      int32_t hdr = 0;
    } __attribute__((packed)) header;

    TRA_PRINT("writing dest-journal-header");
    std::expected<ssize_t,int> io_res = ::mg7x::io::write(jfd, reinterpret_cast<int8_t*>(&header), sizeof(header));
    if (!io_res.has_value()) {
      ERR_PRINT("failed to write journal-header: {}", strerror(io_res.error()));
      goto err_write;
    }
    DBG_PRINT("wrote dest-journal-header: write {}, sizeof(header) {}", io_res.value(), sizeof(header));
  }
  else {
    DBG_PRINT("dest-journal size is {}", sbuf.st_size);
    result = ::mg7x::io::lseek(jfd, sbuf.st_size, SEEK_SET);
    if (!result.has_value()) {
      ERR_PRINT("failed in lseek: {}", strerror(result.error()));
      goto err_lseek;
    }
  }

  return jfd;

err_lseek:
err_write:
err_fstat:
  close(jfd);
  return std::unexpected(result.error());
}

Task<std::expected<int,ErrnoMsg>> kdb_subscribe_and_replay(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub, TpMsgCounts & counts)
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

  INF_PRINT(CYN "kdb_subscribe_and_replay" RST ": subscription message is {}", msg);

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
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": expected response of type LIST but received {}; closing FD {}", sub_ptr->m_typ, conn.sock_fd());
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

  INF_PRINT(CYN "kdb_subscribe_and_replay" RST ": TP message-count is {}, journal is at {}", msg_count->m_val, path->m_val);

  size_t pth_off = 0;
  if (':' == path->m_val.at(0) && path->m_val.length() > 1)
    pth_off = path->m_val.find_first_not_of(":");

  // unpack the journal location, open, scan for tables, rewrite into ours
  std::expected<int,int> result = init_jnl(sub.dst_jnl());
  if (!result.has_value()) {
    ERR_PRINT("failed to open journal {}; closing FD {}", *path, conn.sock_fd());
    monitor_close(conn.sock_fd());
    co_return std::unexpected(ErrnoMsg{result.error(), "Failed to open journal"});
  }

  int jfd = result.value();

  DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": co_return {}", conn.sock_fd());

  const std::string_view src_path{path->m_val.c_str() + pth_off};
  result = ::mg7x::io::open(src_path.data(), O_RDONLY);
  if (!result.has_value()) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed in open; path was '{}': {}", path->m_val.c_str(), strerror(result.error()));
    monitor_close(conn.sock_fd());
    co_return std::unexpected(ErrnoMsg{0, "Failed during open-file op"});
  }
  int src_fd = result.value();

  DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": opened src-journal {} as FD {}", src_path, src_fd);

  const std::unordered_set<std::string_view> names{sub.tables().begin(), sub.tables().end()};
  if (msg_count->m_val > 0) {
    int err = filter_msgs(src_fd, static_cast<uint64_t>(msg_count->m_val), jfd, names, counts);
    if (-1 == err) {
      ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": while filtering journal messages; closing FDs {} and {}", jfd, conn.sock_fd());
      result = ::mg7x::io::close(jfd);
      if (!result.has_value()) {
        WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed while closing journal FD {}: {}", jfd, strerror(result.error()));
      }
      monitor_close(conn.sock_fd());
      co_return std::unexpected(ErrnoMsg{0, "Failed while filtering journaled messages"});
    }
  }

  co_return 0;
}

Task<std::expected<int,ErrnoMsg>> kdb_read_tcp_messages(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub, TpMsgCounts & counts)
{
  DBG_PRINT(GRN "kdb_subscribe" RST ": have sock_fd {}", conn.sock_fd());

  // We expect each TP message to be smaller than 256 Kb, amend here if you've got unusual requirements
  std::vector<int8_t> ary(256 * 1024);
  ary.resize(0);

  EpollCtl::Awaiter awaiter{conn.sock_fd()};
  std::expected<int,int> result = epoll.add_interest(conn.sock_fd(), EPOLLIN, awaiter);
  if (!result.has_value()) {
    ERR_PRINT(YEL "kdb_connect" RST ": failed in EpollCtl::add_interest");
    co_return std::unexpected(ErrnoMsg{result.error(), "Failed in EpollCtl::add_interest"});
  }

  size_t rd_off = 0;
  size_t wr_off = 0;
  uint32_t msg_sz = -1;

  const std::unordered_set<std::string_view> tbl_names{sub.tables().begin(), sub.tables().end()};
  const std::string_view fn_name{"upd"};

  do {
    auto [_fd, rev] = co_await awaiter;
    if (rev & EPOLLERR) {
      ERR_PRINT(GRN "kdb_subscribe" RST ": error signal from epoll: revents is {:#8x}; 'nyi, exiting", rev);
      throw io::EpollError("Error indicated by revents");
    }
    else if (0 != (rev & ~EPOLLIN)) {
      DBG_PRINT(GRN "kdb_subscribe" RST ": epoll revents is not just EPOLLIN: {:#8x}", rev & ~EPOLLIN);
    }

    ssize_t rdz = 0;
    std::expected<ssize_t,int> io_res = ::mg7x::io::read(conn.sock_fd(), ary.data() + wr_off, ary.capacity() - wr_off);
    if (!io_res.has_value()) {
      if (EAGAIN == io_res.error()) {
        TRA_PRINT(GRN "kdb_subscribe" RST ": have EAGAIN on FD {}, nothing further to read", conn.sock_fd());
        rdz = 0;
      }
      else {
        if (EINTR == io_res.error()) {
          // TODO how many times?
          continue;
        }
        else {
          ERR_PRINT(GRN "kdb_subscribe" RST ": while reading from FD {}: {}; closing socket", conn.sock_fd(), strerror(io_res.error()));
          co_return monitor_close(conn.sock_fd());
        }
      }
    }
    wr_off += io_res.value();
    while (wr_off - rd_off > 0) {
      int64_t len = KdbJournalReader::filter_msg(ary.data() + rd_off, wr_off - rd_off, fn_name, tbl_names);
      if (-2 == len) { // insufficent data
        TRA_PRINT(GRN "kdb_subscribe" RST ": insufficient data remain: rd_off {}, wr_off {}, rem {}", rd_off, wr_off, wr_off - rd_off);
        break;
      }
      else {
        // TODO increment message count
        if (len > 0) {
          TRA_PRINT(GRN "kdb_subscribe" RST ": have matching message: len {}, rd_off {}, wr_off {}, rem {}", len, rd_off, wr_off, wr_off - rd_off);
          // table match; complete message: copy to output
        }
        else {
          TRA_PRINT(GRN "kdb_subscribe" RST ": skipping non-matching message: len {}, rd_off {}, wr_off {}, rem {}", len, rd_off, wr_off, wr_off - rd_off);
        }
        rd_off += std::abs(len);
      }
    }
    if (rd_off == wr_off) {
      TRA_PRINT(GRN "kdb_subscribe" RST ": buffer empty: resetting rd_off, wr_off");
      rd_off = wr_off = 0;
    }
    else if (rd_off > wr_off) {
      ERR_PRINT(GRN "kdb_subscribe" RST ": bad maths, crossed cursors: rd_off {}, wr_off {}", rd_off, wr_off);
      // TODO ensure message-count is coherent with published-count
      co_return monitor_close(conn.sock_fd());
    }
    else {
      // twiddle this ratio to vary the level at which it will compact
      // perhaps add other heuristics like amount-to-copy, or min/max/avg-message-size-over-time
      if ((ary.capacity() - wr_off) < (ary.capacity() / 4)) {
        TRA_PRINT(GRN "kdb_subscribe" RST ": compacting buffer; rd_off {}, wr_off {}, capacity {}, load {}", rd_off, wr_off, ary.capacity(), ary.capacity() / static_cast<double>(wr_off));
        std::copy(ary.data() + rd_off, ary.data() + wr_off, ary.data());
        wr_off -= rd_off;
        rd_off = 0;
      }
      else {
        TRA_PRINT(GRN "kdb_subscribe" RST ": not compacting buffer: more than 1/4 capacity remaining; rd_off {}, wr_off {}, capacity {}, load {}", rd_off, wr_off, ary.capacity(), ary.capacity() / static_cast<double>(wr_off));
      }
    }
  }
  while (true);

  co_return conn.sock_fd();
}

}; // end namespace mg7x
