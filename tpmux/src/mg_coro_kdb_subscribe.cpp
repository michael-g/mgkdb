#include <stdlib.h> // EXIT_SUCCESS

#include <stdint.h>
#include <fcntl.h> // open
#include <sys/sendfile.h> // sendfile
#include <sys/stat.h> // fstat
#include <sys/mman.h> // mmap
#include <unistd.h> // lseek, write
#include <string.h>
#include <errno.h>

#include <unordered_set> // std::unordered_set
#include <algorithm> // std::copy

#include "mg_fmt_defs.h"
#include "mg_coro_epoll.h"
#include "mg_coro_task.h"

#include "KdbType.h"

namespace mg7x {

extern 
Task<int> kdb_connect(EpollCtl & epoll, const char * host, const char * service, const char * user);

extern
Task<mg7x::ReadMsgResult> kdb_read_message(EpollCtl & epoll, int fd, std::vector<int8_t> & ary);

static int monitor_close(int fd)
{
  if (-1 == close(fd)) {
    ERR_PRINT(CYN "kdb_subscribe" RST ": failed to close FD {}: {}", fd, strerror(errno));
  }
  return -1;
}

static int cp_sgmt(const int dst_fd, const int src_fd, const uint64_t src_off, const size_t len)
{
  off_t l_src_off = static_cast<off_t>(src_off);
  ssize_t wrz = sendfile(dst_fd, src_fd, &l_src_off, len);
  if (-1 == wrz) {
    ERR_PRINT("failed in sendfile; dst_fd {}, src_fd {}, src_off: {}, len: {}: {}", dst_fd, src_fd, src_off, len, strerror(errno));
    return -1;
  }
  if (static_cast<size_t>(wrz) != len) {
    // doubt we'll hit this...
    ERR_PRINT("mismatch between length reported by sendfile and write-length requested; {} != {}", wrz, len);
    return -1;
  }
  return 0;
}

static int64_t check_match(const int8_t *ptr, size_t rem)
{
  return 0;
}

static int filter_msgs(const int src_fd, const uint64_t msg_count, const int jnl_fd, const std::unordered_set<std::string_view> & tbls)
{
  struct stat sbuf{};
  int err = fstat(src_fd, &sbuf);
  if (-1 == err) {
    ERR_PRINT("failed in fstat over source-journal on FD {}: {}", src_fd, strerror(errno));
    return -1;
  }

  void *const addr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, src_fd, 0);
  if (MAP_FAILED == addr) {
    ERR_PRINT("failed in mmap; size {}, FD {}: {}", sbuf.st_size, src_fd, strerror(errno));
    return -1;
  }

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
    JNL_LOG("check_match({}, {})", (void*)(ptr + off + len),  sbuf.st_size - (off + len));
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

  munmap(addr, sbuf.st_size);
  return 0;

err_jnl_ipc:
err_cp:
  munmap(addr, sbuf.st_size);
  return -1;
}

static int init_jnl(const std::string_view & path)
{
  char buf[path.length() + 1];
  std::copy(path.begin(), path.end(), buf);
  buf[path.length()] = 0;
  int jfd = open(buf, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (-1 == jfd) {
    ERR_PRINT("failed to open/create file {}: {}", path, strerror(errno));
    return -1;
  }

  DBG_PRINT("opened dest-journal {} as FD {}", path, jfd);

  struct stat sbuf{};
  int err = fstat(jfd, &sbuf);
  if (-1 == err) {
    ERR_PRINT("failed in fstat: {}", strerror(errno));
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
    ssize_t wsz = write(jfd, &header, sizeof(header));
    if (-1 == wsz) {
      ERR_PRINT("failed to write journal-header: {}", strerror(errno));
      goto err_write;
    }
    DBG_PRINT("wrote dest-journal-header, size {}", sizeof(header));
  }
  else {
    DBG_PRINT("dest-journal size is {}", sbuf.st_size);
    off_t off = lseek(jfd, sbuf.st_size, SEEK_SET);
    if (-1 == off) {
      ERR_PRINT("failed in lseek: {}", strerror(errno));
      goto err_lseek;
    }
  }

  return jfd;

err_write:
err_lseek:
err_fstat:
  close(jfd);
  return -1;
}

Task<int> kdb_subscribe(EpollCtl & epoll, const char * service, const char * user, std::vector<std::string_view> & tables, const char *dst_path)
{
  Task<int> task = kdb_connect(epoll, "localhost", service, user);

  INF_PRINT(CYN "kdb_subscribe" RST ": awaiting kdb_connect Task");

  int sock_fd = co_await task;

  if (-1 == sock_fd) {
    WRN_PRINT(YEL "kdb_connect" RST ": co_return sock_fd is {}", sock_fd);
    co_return sock_fd;
  }
  DBG_PRINT(CYN "kdb_subscribe" RST ": have sock_fd {}", sock_fd);

  mg7x::KdbSymbolAtom fun{".u.sub"};
  mg7x::KdbSymbolVector tbl{tables};
  mg7x::KdbSymbolAtom sym{};
  mg7x::KdbList msg{3};
  msg.push(fun);
  msg.push(tbl);
  msg.push(sym);

  const size_t ipc_len = mg7x::KdbUtil::ipcMessageLen(msg);
  std::vector<int8_t> ary(512); // we assume 512 is greater than ipc_len
  ary.resize(0);

  INF_PRINT(CYN "kdb_subscribe" RST ": subscription message is {}", msg);

  mg7x::KdbIpcMessageWriter writer{mg7x::KdbMsgType::SYNC, msg};

  mg7x::WriteResult res = writer.write(ary.data(), ary.capacity());
  TRA_PRINT(CYN "kdb_subscribe" RST ": serialized subscription message, result is {}", res);

  if (mg7x::WriteResult::WR_OK != res) {
    ERR_PRINT(CYN "kdb_subscribe" RST ": failed inexplicably to serialise message; closing FD {}", sock_fd);
    co_return monitor_close(sock_fd);
  }

  ssize_t wrt = 0;
  do {
    // we assume it's writable here, Lord, it's only just been created...
    ssize_t wrz = write(sock_fd, ary.data() + wrt, ipc_len - wrt);
    if (-1 == wrz) {
      // TODO check for EINTR etc
      ERR_PRINT(CYN "kdb_subscribe" RST ": failed in write: {}; closing FD {}", strerror(errno), sock_fd);
      co_return monitor_close(sock_fd);
    }
    wrt += wrz;
    DBG_PRINT(CYN "kdb_subscribe" RST ": Wrote {} bytes of {} to FD {}, awaiting kdb_read_message", wrz, ipc_len, sock_fd);
  } while (wrt < ipc_len);


  mg7x::ReadMsgResult rmr = co_await kdb_read_message(epoll, sock_fd, ary);
  
  if (mg7x::ReadResult::RD_OK != rmr.result) {
    ERR_PRINT(CYN "kdb_subscribe" RST ": failed to complete read; closing FD {}", sock_fd);
    co_return monitor_close(sock_fd);
  }
  if (mg7x::KdbMsgType::RESPONSE != rmr.msg_typ) {
    WRN_PRINT(CYN "kdb_subscribe" RST ": unexpected response to .u.sub; closing FD {}", sock_fd);
    co_return monitor_close(sock_fd);
  }
  //std::unique_ptr<mg7x::KdbBase> sub_ptr{rmr.message.release()};
  KdbBase *sub_ptr = rmr.message.get();

  if (mg7x::KdbType::LIST != sub_ptr->m_typ) {
    WRN_PRINT(CYN "kdb_subscribe" RST ": expected response of type LIST but received {}; closing FD {}", sub_ptr->m_typ, sock_fd);
    co_return monitor_close(sock_fd);
  }
  mg7x::KdbList *lst = dynamic_cast<mg7x::KdbList*>(sub_ptr);
  if (mg7x::KdbType::LONG_ATOM != lst->typeAt(0)) {
    WRN_PRINT(CYN "kdb_subscribe" RST ": expected LONG_ATOM at index [0], found {}; closing FD {}", lst->typeAt(0), sock_fd);
    co_return monitor_close(sock_fd);
  }
  if (3 != lst->count()) {
    WRN_PRINT(CYN "kdb_subscribe" RST ": expected 3 elements but found {}; closing FD {}", lst->count(), sock_fd);
    co_return monitor_close(sock_fd);
  }
  if (mg7x::KdbType::SYMBOL_ATOM != lst->typeAt(1)) {
    WRN_PRINT(CYN "kdb_subscribe" RST ": expected SYMBOL_ATOM at index [1], found {}; closing FD {}", lst->typeAt(1), sock_fd);
    co_return monitor_close(sock_fd);
  }
  if (mg7x::KdbType::LIST != lst->typeAt(2)) {
    WRN_PRINT(CYN "kdb_subscribe" RST ": expected LIST at index [2], found {}; closing FD {}", lst->typeAt(2), sock_fd);
    co_return monitor_close(sock_fd);
  }

  const mg7x::KdbLongAtom *msg_count = dynamic_cast<const mg7x::KdbLongAtom*>(lst->getObj(0));
  const mg7x::KdbSymbolAtom *path = dynamic_cast<const mg7x::KdbSymbolAtom*>(lst->getObj(1));

  INF_PRINT(CYN "kdb_subscribe" RST ": TP message-count is {}, journal is at {}", msg_count->m_val, path->m_val);
  
  size_t pth_off = 0;
  if (':' == path->m_val.at(0) && path->m_val.length() > 1) 
    pth_off = path->m_val.find_first_not_of(":");

  // unpack the journal location, open, scan for tables, rewrite into ours
  int jfd = init_jnl(dst_path);
  if (-1 == jfd) {
    ERR_PRINT("failed to open journal {}; closing FD {}", *path, sock_fd);
    co_return monitor_close(sock_fd);
  }

  DBG_PRINT(CYN "kdb_subscribe" RST ": co_return {}", sock_fd);

  const std::string_view src_path{path->m_val.c_str() + pth_off};
  int src_fd = open(src_path.data(), O_RDONLY);
  if (-1 == src_fd) {
    ERR_PRINT("failed in open; path was '{}': {}", path->m_val.c_str(), strerror(errno));
    co_return monitor_close(sock_fd);
  }
  DBG_PRINT("opened src-journal {} as FD {}", src_path, src_fd);

  const std::unordered_set<std::string_view> names{tables.begin(), tables.end()};
  if (msg_count->m_val > 0) {
    int err = filter_msgs(src_fd, static_cast<uint64_t>(msg_count->m_val), jfd, names);
    if (-1 == err) {
      ERR_PRINT("while filtering journal messages; closing FDs {} and {}", jfd, sock_fd);
      close(jfd);
      co_return monitor_close(sock_fd);
    }
  }

  co_return sock_fd;
}

#define MAX_EVENTS 10

int tpmux_main(int argc, char **argv)
{
  int epollfd = epoll_create1(0);
  if (-1 == epollfd) {
    ERR_PRINT("main: epoll_create1 failed: {}", strerror(errno));
    return EXIT_FAILURE;
  }

  EpollCtl ctl{epollfd};
  const char *dst_jnl = "/home/michaelg/tmp/dst.journal";

  std::vector<std::string_view> tbs99{"trade",};
  Task<int> task99 = kdb_subscribe(ctl, "30099", "michaelg", tbs99, dst_jnl);

  std::vector<std::string_view> tbs98{"position",};
  Task<int> task98 = kdb_subscribe(ctl, "30098", "michaelg", tbs98, dst_jnl);

  TaskContainer tasks{};
  tasks.add(task99);
  tasks.add(task98);
  // TopLevelTask tlt = TopLevelTask::await(task99);

  struct epoll_event events[MAX_EVENTS];
  while (!tasks.complete()) {
    TRA_PRINT("main: calling epoll_wait");
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (-1 == nfds) {
      ERR_PRINT("main: in epoll_wait: {}", strerror(errno));
    }
    else {
      DBG_PRINT("main: have {} epoll fds", nfds);
      for (int i = 0 ; i < nfds ; i++) {
        EpollFunc *ptr = static_cast<EpollFunc*>(events[i].data.ptr);
        if (nullptr != ptr) {
          (*ptr)(events[i].events);
        }
        else {
          WRN_PRINT("have nullptr for event[{}]", i);
        }
      }
    }
  }

  // tasks.done();

  return EXIT_SUCCESS;
}

}; // end namespace mg7x

#undef MAX_EVENTS