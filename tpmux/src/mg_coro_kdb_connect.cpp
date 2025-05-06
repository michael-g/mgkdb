
#include <string.h>

#include "mg_coro_domain_obj.h"
#include "mg_io.h"
#include "mg_coro_task.h"
#include "mg_coro_epoll.h"
#include "mg_fmt_defs.h"


namespace mg7x {

Task<std::expected<KdbIpcLevel,ErrnoMsg>> kdb_connect(EpollCtl & epoll, const io::TcpConn & conn, std::string_view user)
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

  std::expected<int,int> result;
  std::expected<ssize_t,int> io_res;
  {
    EpollCtl::Awaiter awaiter{conn.sock_fd()};
    result = epoll.add_interest(conn.sock_fd(), EPOLLOUT, awaiter);
    if (!result.has_value()) {
      ERR_PRINT(YEL "kdb_connect" RST ": failed in EpollCtl::add_interest");
      co_return std::unexpected(ErrnoMsg{result.error(), "Failed to EpollCtl::add_interest"});
    }

    int retries = 0;
    len = off;
    off = 0;

  retry:
    DBG_PRINT(YEL "kdb_connect" RST ": writing credentials (attempt {})", retries);

    io_res = ::mg7x::io::write(conn.sock_fd(), &creds[off], len);
    if (!io_res.has_value()) {
      int err_num = io_res.error();
      if (EWOULDBLOCK == err_num || EAGAIN == err_num) {
        if (retries++ < 3) {
          auto [fd, revts] = co_await awaiter;
          if (0 != (EPOLLERR  & revts)) {
            ERR_PRINT("Non-zero error-flags reported by epoll: while waiting for write to clear");
            co_return std::unexpected(ErrnoMsg{0, "EPOLLERR detected while waiting for write"});
          }
          goto retry;
        }
        ERR_PRINT(YEL "kdb_connect" RST ": can't retry again: giving up.");
        co_return std::unexpected(ErrnoMsg{err_num, "Failed attempting to write credentials"});
      }
      ERR_PRINT(YEL "kdb_connect" RST ": failed in write: {}", strerror(err_num));
      co_return std::unexpected(ErrnoMsg{err_num, "Failed while writing credentials"});
    }
    off += io_res.value();
    if (off < len) {
      goto retry;
    }
  }

  {
    EpollCtl::Awaiter awaiter{conn.sock_fd()};

    result = epoll.mod_interest(conn.sock_fd(), EPOLLIN, awaiter);
    if (!result.has_value()) {
      ERR_PRINT(YEL "kdb_connect" RST ": failed in EpollCtl::mod_interest");
      co_return std::unexpected(ErrnoMsg{result.error(), "Failed in EpollCtl::mod_interest"});
    }

    DBG_PRINT(YEL "kdb_connect" RST ": awaiting read-event on sock_fd");
    auto [fd, revts] = co_await awaiter;
    if (0 != (EPOLLERR  & revts)) {
      ERR_PRINT("Non-zero error-flags reported by epoll: while waiting for read-event");
      co_return std::unexpected(ErrnoMsg{0, "EPOLLERR detected while waiting for write"});
    }
  }

  io_res = ::mg7x::io::read(conn.sock_fd(), &creds[0], 1);
  if (!io_res.has_value()) {
    // TOOD this should probably check for EAGAIN, probably not EWOULDBLOCK (given the size);
    ERR_PRINT(YEL "kdb_connect" RST ": while reading login-response: {}", strerror(io_res.error()));
    co_return std::unexpected(ErrnoMsg{io_res.error(), "Failed during read"});
  }

  if (1 == io_res.value()) {
    INF_PRINT(YEL "kdb_connect" RST ": logged in as user '{}', agreed IPC version is {}", user, creds[0]);
  }
  else {
    WRN_PRINT(YEL "kdb_connect" RST ": login failed, read {} bytes", io_res.value());
    co_return std::unexpected(ErrnoMsg{0, "Failed in kdb IPC-handshake"});
  }

  result = epoll.clr_interest(conn.sock_fd());
  if (!result.has_value()) {
    ERR_PRINT(YEL "kdb_connect" RST ": failed in EpollCtl::clr_interest");
    co_return std::unexpected(ErrnoMsg{result.error(), "Failed in EpollCtl::clr_interest"});
  }

  int8_t ipc_lvl = creds[0];

  TRA_PRINT(YEL "kdb_connect" RST ": co_return ipc-level {}", ipc_lvl);
  co_return KdbIpcLevel{ipc_lvl};

}
}
