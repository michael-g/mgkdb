#include <sys/epoll.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <string_view>
#include <vector>
#include <expected>

#include "mg_io.h"
#include "mg_coro_domain_obj.h"
#include "mg_fmt_defs.h"
#include "mg_coro_epoll.h"
#include "mg_coro_task.h"


#include <stdint.h>


namespace mg7x {

extern
TASK_TYPE<std::expected<io::TcpConn,int>>
  tcp_connect(EpollCtl & epoll, std::string_view host, std::string_view service);

extern
TASK_TYPE<std::expected<KdbIpcLevel,ErrnoMsg>>
  kdb_connect(EpollCtl & epoll, const io::TcpConn & conn, std::string_view user);

extern
TASK_TYPE<std::expected<int,ErrnoMsg>>
  kdb_subscribe_and_replay(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub, TpMsgCounts & counts);

extern
TASK_TYPE<std::expected<int,ErrnoMsg>>
  kdb_read_tcp_messages(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub, TpMsgCounts & counts);

TASK_TYPE<int> subscribe(EpollCtl & epoll, Subscription sub, TpMsgCounts & counts)
{
  auto ret = co_await tcp_connect(epoll, "localhost", sub.service());
  if (!ret.has_value()) {
    ERR_PRINT(YEL "subscribe" RST ": failed to establish TCP connection to port {}", sub.service());
    int err = ret.error();
    co_return err;
  }
  io::TcpConn conn = ret.value();

  auto res_kc = co_await kdb_connect(epoll, conn, sub.username());
  if (!res_kc.has_value()) {
    ERR_PRINT(YEL "subscribe" RST ": failed during handshake with KDB listening on port {}", sub.service());
    co_return -1;
  }

  auto res_ksr = co_await kdb_subscribe_and_replay(epoll, conn, sub, counts);
  if (!res_ksr.has_value()) {
    ERR_PRINT(YEL "subscribe" RST ": failed during subscribe/journal-replay for KDB instance listening on port {}", sub.service());
    co_return -1;
  }

  auto res_rtm = co_await kdb_read_tcp_messages(epoll, conn, sub, counts);
  if (!res_rtm.has_value()) {
    ERR_PRINT(YEL "subscribe" RST ": failed while in steady-state TCP-receive (port {})", sub.service());
    co_return -1;
  }

  co_return -1;
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

  Subscription sub98{"30098", "michaelg", {"position"}, dst_jnl};
  Subscription sub99{"30099", "michaelg", {"trade"}, dst_jnl};

  TaskContainer<int> tasks{};

  TpMsgCounts count98{0, 0};
  TpMsgCounts count99{0, 0};

  TASK_TYPE<int> task98 = subscribe(ctl, sub98, count98);
  tasks.add(task98);
  TASK_TYPE<int> task99 = subscribe(ctl, sub99, count99);
  tasks.add(task99);

  // cppcoro::sync_wait(task98);
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

int main(int argc, char **argv)
{
	return mg7x::tpmux_main(argc, argv);
}
