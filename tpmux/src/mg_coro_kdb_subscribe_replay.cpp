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

/**
  Copies data from `dst_fd` to `src_fd` from position `src_off`, length `len`.
  The implementation uses `sendfile` to conduct the copy.
  @return `-1` in the case of error, or `0` if the copy-op succeeded.
*/
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

/**
  Scans the tickerplant log file opened as `src_fd` for messages which update the table-names in set `tbls`.

  The function will replay at most `msg_count` messages, since this was the high-water-mark returned by the
  tickerplant (_i.e._ the value given by `.u.i` in a non-batching TP). Matching messages will be copied into
  the tickerplant log file opened as `jnl_fd`.

  The `counts` object contains information about messages previously replayed or skipped. These two values
  will obviously initially be zero. It's easy to see how during a reconnection sequence these values may be
  greater than zero and identify messages that should not be copied or forwarded to the client again. The
  values in `counts` will be incremented once the high-water-mark of total-messages-seen has been reached.

  The implementation uses maps the source file into memory for ease of scanning.

  @param src_fd the FD of the source journal
  @param msg_count the maximum number of known-good messages that can be replayed, typically given by `.u.i`
  @param jnl_fd the FD of the destination journal
  @param tbls the set of table names to include into the dest-journal
  @param counts an object capable of recording the total number of messages seen and forwarded

  @return
    `-1` in the case of error, or `0` upon success
*/
static int filter_jnl(const int src_fd, const uint64_t msg_count, const int jnl_fd, const std::unordered_set<std::string_view> & tbls, TpMsgCounts & counts)
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

#ifndef _MG_LOG_JNL_TESTS_
#define JNL_LOG(...)
#else
#define JNL_LOG(...) DBG_PRINT(__VA_ARGS__)
#endif

  DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": filtering source journal, skipping {} messages", counts.m_num_msg_total);

  const std::string_view fn_name{"upd"};

  int err = 0;

  for (uint64_t i = 0 ; i < msg_count ; i++) {
    const bool skip = i < counts.m_num_msg_total;
    int64_t rtn = KdbJournalReader::filter_msg(ptr + off, sbuf.st_size - off, skip, fn_name, tbls);
    JNL_LOG(CYN "kdb_subscribe_and_replay" RST ": filter_msg({}, {}, {}, {}, tbls) = {}", off, sbuf.st_size - off, skip, fn_name, rtn);
    if (rtn > 0) {
      counts.m_num_msg_total += 1;
      if (!skip) {
        counts.m_num_msg_included += 1;
        if (-1 == cp_sgmt(jnl_fd, src_fd, off, rtn)) {
          err = -1;
          break;
        }
      }
      off += rtn;
    }
    else if (rtn < -2) {
      // message doesn't match (`upd;`table;...) OR does match and should be skipped
      counts.m_num_msg_total += 1;
      // adjust by a negative value:
      off -= rtn;
    }
    else {
      if (-2 == rtn)
        ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": bad IPC");
      else if (-1 == rtn)
        ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": insufficient data (in the journal, shouldn't happen)");
      else
        ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": illegal state, rtn is {}", rtn);
      err = -1;
      break;
    }

    // OLD: this used to do message batching, allowing `len` to grow as copyable messages were seen in a
    // contiguous stream, and which were copied as a block once a non-matching or final message was
    // encountered.
    //
    // if (rtn < 0 || i == msg_count - 1) {
    //   if (len > 0) {
    //     JNL_LOG(CYN "kdb_subscribe_and_replay" RST ": cp_sgmt({}, {}, {}, {})", jnl_fd, src_fd, off, len);
    //     int err = cp_sgmt(jnl_fd, src_fd, off, len);
    //     if (-1 == err) {
    //       goto err_cp;
    //     }
    //   }
    //   off += len - rtn;
    //   len = 0;
    // }
    // else {
    //   len += rtn;
    // }
  }
#undef JNL_LOG

  result = ::mg7x::io::munmap(addr, sbuf.st_size);
  if (!result.has_value()) {
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed in munmap: {}", strerror(result.error()));
  }

  INF_PRINT(CYN "kdb_subscribe_and_replay" RST ": journal replay complete, result {}; total-messages-seen {}, included-message-count {}", err, counts.m_num_msg_total, counts.m_num_msg_included);

  return err;
}

static std::expected<int,int> init_jnl(const std::string_view & path)
{
  char buf[path.length() + 1];
  std::copy(path.begin(), path.end(), buf);
  buf[path.length()] = 0;
  std::expected<int,int> result = ::mg7x::io::open(buf, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (!result.has_value()) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed to open/create file {}: {}", path, strerror(result.error()));
    return std::unexpected(result.error());
  }

  int jfd = result.value();

  DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": opened dest-journal {} as FD {}", path, jfd);

  struct stat sbuf{};
  result = ::mg7x::io::fstat(jfd, &sbuf);
  if (!result.has_value()) {
    ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed in fstat: {}", strerror(result.error()));
    goto err_fstat;
  }

  if (0 == sbuf.st_size) {
    struct {
      int8_t one = 0xff;
      int8_t two = 0x01;
      int16_t typ = 0;
      int32_t hdr = 0;
    } __attribute__((packed)) header;

    TRA_PRINT(CYN "kdb_subscribe_and_replay" RST ": writing dest-journal-header");
    std::expected<ssize_t,int> io_res = ::mg7x::io::write(jfd, reinterpret_cast<int8_t*>(&header), sizeof(header));
    if (!io_res.has_value()) {
      ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed to write journal-header: {}", strerror(io_res.error()));
      goto err_write;
    }
    DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": wrote dest-journal-header: write {}, sizeof(header) {}", io_res.value(), sizeof(header));
  }
  else {
    DBG_PRINT(CYN "kdb_subscribe_and_replay" RST ": dest-journal size is {}", sbuf.st_size);
    result = ::mg7x::io::lseek(jfd, sbuf.st_size, SEEK_SET);
    if (!result.has_value()) {
      ERR_PRINT(CYN "kdb_subscribe_and_replay" RST ": failed in lseek: {}", strerror(result.error()));
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

TASK_TYPE<std::expected<int,ErrnoMsg>> kdb_subscribe_and_replay(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub, TpMsgCounts & counts)
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

  if (msg_count->m_val < counts.m_num_msg_total) {
    monitor_close(conn.sock_fd());
    WRN_PRINT(CYN "kdb_subscribe_and_replay" RST ": reported .u.i message count is {}, alleged message-count previously replayed is {}", msg_count->m_val, counts.m_num_msg_total);
    co_return std::unexpected(ErrnoMsg{0, "fewer messages in src-journal than we have apparently alread read"});
  }

  INF_PRINT(CYN "kdb_subscribe_and_replay" RST ": now subscribed; TP message-count is {}, journal is {}, will skip {} messages", msg_count->m_val, path->m_val, counts.m_num_msg_total);

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
    int err = filter_jnl(src_fd, static_cast<uint64_t>(msg_count->m_val), jfd, names, counts);
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

}; // end namespace mg7x
