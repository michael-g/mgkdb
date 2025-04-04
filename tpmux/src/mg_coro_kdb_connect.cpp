
#include <utility> // std::exchange
#include <string.h>

#include "mg_io.h"
#include "mg_coro_task.h"
#include "mg_coro_epoll.h"
#include "mg_fmt_defs.h"

namespace mg7x {

extern 
Task<int> tcp_connect(EpollCtl & epoll, const char *host, const char *service);

Task<int> kdb_connect(EpollCtl & epoll, const char * host, const char * service, const char * user)
{
  Task<int> task = tcp_connect(epoll, host, service);

  DBG_PRINT(YEL "kdb_connect" RST ": awaiting tcp_connect Task");

  int sock_fd = co_await task; // std::move(task);

  if (-1 == sock_fd) {
    WRN_PRINT(YEL "kdb_connect" RST ": sock_fd is {}", sock_fd);
    co_return sock_fd;
  }

  DBG_PRINT(YEL "kdb_connect" RST ": beginning kdb-handshake using sock_fd {}", sock_fd);

  std::array<int8_t,128> creds{};
  size_t off = 0;
  size_t len = strlen(user);

  if (len > 0) {
    std::copy(user, user + len, &creds[0]); 
    off = len;
  }
  creds[off++] = ':';
  creds[off++] = 3;
  creds[off++] = 0;

  {
    EpollCtl::Awaiter awaiter{sock_fd};
    int err = epoll.add_interest(sock_fd, EPOLLOUT, awaiter);
    if (-1 == err) {
      ERR_PRINT(YEL "kdb_connect" RST ": failed in .add_interest, exiting");
      std::terminate();
    }

    int retries = 0;
    len = off;
    off = 0;

  retry:
    DBG_PRINT(YEL "kdb_connect" RST ": writing credentials (attempt {})", retries);
    // ssize_t wrt = write(sock_fd, &creds[off], len);
    ssize_t wrt = IO::write(sock_fd, {&creds[off], len});
    if (-1 == wrt) {
      if (EWOULDBLOCK == errno || EAGAIN == errno) {
        if (retries++ < 3) { 
          // TODO [[unused-variable]]
          auto [events, fd] = co_await awaiter;
          // TODO test events
          goto retry;
        }
        ERR_PRINT(YEL "kdb_connect" RST ": can't retry again: giving up.");
        std::terminate();
      }
      ERR_PRINT(YEL "kdb_connect" RST ": failed in write: {}", strerror(errno));
      std::terminate();
    }
    off += wrt;
    if (off < len) {
      goto retry;
    }
  }

  {
    EpollCtl::Awaiter awaiter{sock_fd};
  
    int err = epoll.mod_interest(sock_fd, EPOLLIN, awaiter);
    if (-1 == err) {
      ERR_PRINT(YEL "kdb_connect" RST ": failed in epoll.mod_interest");
      // TODO failure handling
    }

    DBG_PRINT(YEL "kdb_connect" RST ": awaiting read-event on sock_fd");
    auto [fd, events] = co_await awaiter;
    // TODO check events for errors
  }

  std::span<int8_t> buf{&creds[0], 1};

  ssize_t red = IO::read(sock_fd, buf);
  if (-1 == red) {
    // TOOD this should probably check for EAGAIN, probably not EWOULDBLOCK (given the size);
    ERR_PRINT(YEL "kdb_connect" RST ": while reading login-response: {}", strerror(errno));
    std::terminate();
  }
  if (1 == red) {
    INF_PRINT(YEL "kdb_connect" RST ": logged in as user '{}', agreed IPC version is {}", user, creds[0]);
  }
  else {
    WRN_PRINT(YEL "kdb_connect" RST ": login failed, read {} bytes", red);
    // TODO close sock_fd
  }

  epoll.clr_interest(sock_fd);

  TRA_PRINT(YEL "kdb_connect" RST ": co_return sock_fd is {}", sock_fd);
  co_return sock_fd;

}
}
