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
Task<std::expected<io::TcpConn,int>> tcp_connect(EpollCtl & epoll, std::string_view host, std::string_view service);

extern
Task<int8_t> kdb_connect(EpollCtl & epoll, const io::TcpConn & conn, std::string_view user);

extern
Task<int> kdb_subscribe_and_replay(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub);

extern
Task<int> kdb_subscribe(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub);

Task<int> subscribe(EpollCtl & epoll, Subscription sub)
{
  auto ret = co_await tcp_connect(epoll, "localhost", sub.service());
  if (!ret.has_value()) {
    ERR_PRINT("failed to establish TCP connection");
    int err = ret.error();
    co_return err;
  }
  io::TcpConn conn = ret.value();

  auto ipc_lvl = co_await kdb_connect(epoll, conn, sub.username());
  if (ipc_lvl < 0) {
    ERR_PRINT("failed during handshake with KDB");
    co_return -1;
  }

  auto res = co_await kdb_subscribe_and_replay(epoll, conn, sub);
  if (-1 == res) {
    co_return res;
  }

  res = co_await kdb_subscribe(epoll, conn, sub);
  if (-1 == res) {
    co_return res;
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

  Task<int> task98 = subscribe(ctl, sub98);
  tasks.add(task98);
  Task<int> task99 = subscribe(ctl, sub99);
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
