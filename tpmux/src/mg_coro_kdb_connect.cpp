
#include <string.h>

#include "mg_io.h"
#include "mg_coro_task.h"
#include "mg_coro_epoll.h"
#include "mg_fmt_defs.h"


namespace mg7x {

Task<int8_t> kdb_connect(EpollCtl & epoll, const io::TcpConn & conn, std::string_view user)
{
  DBG_PRINT(YEL "kdb_connect" RST ": beginning kdb-handshake using sock_fd {}", conn.sock_fd());

  std::array<int8_t,128> creds{};
  size_t off = 0;
  size_t len = user.length();

  if (len > 0) {
    std::copy(user.begin(), user.end(), &creds[0]);
    off = len;
  }
  creds[off++] = ':';
  creds[off++] = 3;
  creds[off++] = 0;

  {
    EpollCtl::Awaiter awaiter{conn.sock_fd()};
    int err = epoll.add_interest(conn.sock_fd(), EPOLLOUT, awaiter);
    if (-1 == err) {
      ERR_PRINT(YEL "kdb_connect" RST ": failed in .add_interest");
      throw io::EpollError("Failed to .add_interest");
    }

    int retries = 0;
    len = off;
    off = 0;

  retry:
    DBG_PRINT(YEL "kdb_connect" RST ": writing credentials (attempt {})", retries);

    ssize_t wrt = ::mg7x::io::write(conn.sock_fd(), &creds[off], len);
    if (-1 == wrt) {
      if (EWOULDBLOCK == errno || EAGAIN == errno) {
        if (retries++ < 3) {
          auto [fd, events] = co_await awaiter;
          if (0 != (EPOLLERR  & events)) {
            ERR_PRINT(YEL "kdb_connect" RST ": error-flag set in Epoll.revents");
            throw io::EpollError("Error reported via revents");
          }
          goto retry;
        }
        ERR_PRINT(YEL "kdb_connect" RST ": can't retry again: giving up.");
        throw io::IoError("Failed in write: too many retries");
      }
      ERR_PRINT(YEL "kdb_connect" RST ": failed in write: {}", strerror(errno));
      throw io::IoError("Failed in write");
    }
    off += wrt;
    if (off < len) {
      goto retry;
    }
  }

  {
    EpollCtl::Awaiter awaiter{conn.sock_fd()};

    int err = epoll.mod_interest(conn.sock_fd(), EPOLLIN, awaiter);
    if (-1 == err) {
      ERR_PRINT(YEL "kdb_connect" RST ": failed in epoll.mod_interest");
      throw io::EpollError("Failed in .mod_interest");
    }

    DBG_PRINT(YEL "kdb_connect" RST ": awaiting read-event on sock_fd");
    auto [fd, events] = co_await awaiter;
    if (0 != (EPOLLERR  & events)) {
      ERR_PRINT(YEL "kdb_connect" RST ": error-flag set in Epoll.revents");
      throw io::EpollError("Error reported via revents");
    }
  }

  ssize_t red = ::mg7x::io::read(conn.sock_fd(), &creds[0], 1);
  if (-1 == red) {
    // TOOD this should probably check for EAGAIN, probably not EWOULDBLOCK (given the size);
    ERR_PRINT(YEL "kdb_connect" RST ": while reading login-response: {}", strerror(errno));
    throw io::IoError("Failed in read");
  }
  if (1 == red) {
    INF_PRINT(YEL "kdb_connect" RST ": logged in as user '{}', agreed IPC version is {}", user, creds[0]);
  }
  else {
    WRN_PRINT(YEL "kdb_connect" RST ": login failed, read {} bytes", red);
    throw io::IoError("Failed in kdb IPC-handshake"); // TODO consider error-type
  }

  int err = epoll.clr_interest(conn.sock_fd());
  if (-1 == err) {
    ERR_PRINT(YEL "kdb_connect" RST ": failed in epoll.clr_interest");
    throw io::EpollError("Failed in .clr_interest");
  }

  int8_t ipc_lvl = creds[0];

  TRA_PRINT(YEL "kdb_connect" RST ": co_return ipc-level {}", ipc_lvl);
  co_return ipc_lvl;

}
}
