#include <stdlib.h> // EXIT_SUCCESS

#include <stdint.h>

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


Task<int> kdb_subscribe(EpollCtl & epoll, const char * service, const char * user, std::vector<std::string_view> & tables)
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

  const mg7x::KdbSymbolAtom *path = dynamic_cast<const mg7x::KdbSymbolAtom*>(lst->getObj(1));
  INF_PRINT(CYN "kdb_subscribe" RST ": TP journal is at {}", path->m_val);
  
  // unpack the journal location, open, scan for tables, rewrite into ours

  DBG_PRINT(CYN "kdb_subscribe" RST ": co_return {}", sock_fd);

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
  std::vector<std::string_view> tbs{"trade", "quote"};
  Task<int> task = kdb_subscribe(ctl, "30099", "michaelg", tbs);
  TopLevelTask tlt = TopLevelTask::await(task);

  struct epoll_event events[MAX_EVENTS];
  while (!tlt.done()) {
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

  tlt.done();

  return EXIT_SUCCESS;
}

}; // end namespace mg7x

#undef MAX_EVENTS