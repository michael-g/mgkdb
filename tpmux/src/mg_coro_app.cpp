#include <sys/epoll.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <string_view>
#include <vector>

#include "mg_fmt_defs.h"
#include "mg_coro_epoll.h"
#include "mg_coro_task.h"

namespace mg7x {

extern
Task<int> kdb_subscribe(EpollCtl & epoll, const char * service, const char * user, std::vector<std::string_view> & tables, const char *dst_path);

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

  // std::vector<std::string_view> tbs98{"position",};
  // Task<int> task98 = kdb_subscribe(ctl, "30098", "michaelg", tbs98, dst_jnl);

  TaskContainer<int> tasks{};
  tasks.add(task99);
  // tasks.add(task98);

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
