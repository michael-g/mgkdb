#include <stdint.h>
#include <signal.h> // struct sigevent
#include <sys/eventfd.h> // args to eventfd()
#include <sys/socket.h> // args to socket, struct addrinfo
#include <netdb.h> // struct gaicb
#include <arpa/inet.h> // inet_ntop
#include <string.h>
#include <errno.h>

// #include <system_error>
#include <expected>
#include <utility>

#include "mg_fmt_defs.h"
#include "mg_io.h"
#include "mg_coro_task.h"
#include "mg_coro_epoll.h"


static void mg_sigev_notify(union sigval arg) {
  int64_t val = 1;
  TRA_PRINT(YEL "mg_sigev_notify" RST ": calling ::mg7x::io::write({}, &val, {})", arg.sival_int, sizeof(val));

  int err = ::mg7x::io::write(arg.sival_int, &val, sizeof(val));
  if (-1 == err) {
    ERR_PRINT(YEL "mg_sigev_notify" RST ": failed to write 1 byte to m_eventfd {}: {}", arg.sival_int, strerror(errno));
  }
  else {
    TRA_PRINT(YEL "mg_sigev_notify" RST ": wrote signal value to event-fd in mg_sigev_notify");
  }
}

static void* get_in_addr(struct sockaddr *sa)
{
  // https://stackoverflow.com/a/9212542/322304
  if (AF_INET == sa->sa_family)
    return &(((struct sockaddr_in*)sa)->sin_addr)
    ;
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static void mg_print_addrinfo(struct addrinfo *nfo)
{
  char buf[INET6_ADDRSTRLEN] = {0};
  if (nullptr == inet_ntop(nfo->ai_family, get_in_addr(static_cast<struct sockaddr*>(nfo->ai_addr)), buf, sizeof buf)) {
    ERR_PRINT("    in inet_ntop: {}", strerror(errno));
  }
  else {
    DBG_PRINT("    addrinfo is {}", buf);
  }
}

static std::pair<int,int> create_event_fd()
{
  int ret = ::mg7x::io::eventfd(0, EFD_NONBLOCK|EFD_SEMAPHORE);
  if (-1 == ret) {
    const int err = errno;
    ERR_PRINT("failed in eventfd: {}", strerror(errno));
    return {ret, err};
  }
  return {0, ret};
}

namespace mg7x {

Task<std::expected<io::TcpConn,int>> tcp_connect(EpollCtl & epoll, std::string_view host, std::string_view service)
// Task<io::TcpConn> tcp_connect(EpollCtl & epoll, std::string_view host, std::string_view service)
{
  DBG_PRINT(GRN "tcp_connect" RST ": beginning tcp-connection to {}:{}", host, service);
  auto efd = create_event_fd();
  if (-1 == efd.first) {
    // already logged
    throw std::system_error(efd.second, std::system_category(), "Failed to create event_fd");
    // co_return std::unexpected(-1);
  }

  const int event_fd = efd.second;

  DBG_PRINT(GRN "tcp_connect" RST ": created event_fd {}", event_fd);

  struct addrinfo addrinfo;
  struct gaicb gai_req;
  struct sigevent sevp;
  struct gaicb *gai_reqs[1];

  gai_req.ar_name = host.data();
  gai_req.ar_service = service.data();
  gai_req.ar_request = &addrinfo;

  addrinfo.ai_family = AF_INET;
  addrinfo.ai_socktype = SOCK_STREAM;

  sevp.sigev_notify = SIGEV_THREAD;
  sevp.sigev_value.sival_int = event_fd;
  sevp.sigev_notify_function = mg_sigev_notify;

  gai_reqs[0] = &gai_req;

  {
    EpollCtl::Awaiter awaiter{event_fd};
    int err = epoll.add_interest(event_fd, EPOLLIN, awaiter);
    if (-1 == err) {
      ERR_PRINT(GRN "tcp_connect" RST ": failed in .add_interest, exiting");
      throw io::IoError("Could not .add_interest");
    }

    DBG_PRINT(GRN "tcp_connect" RST ": calling getaddrinfo_a(..)");

    ::mg7x::io::getaddrinfo_a(GAI_NOWAIT, gai_reqs, 1, &sevp);

    DBG_PRINT(GRN "tcp_connect" RST ": co_await EpollCtl::Awaiter{{event_fd = {}}}", event_fd);
    auto [fd, events] = co_await awaiter;
    // TODO check events for errors
    DBG_PRINT(GRN "tcp_connect" RST ": return from co_await EpollCtl::Awaiter, fd {}, events {}", fd, events);
  }

  struct addrinfo *res = gai_req.ar_result;

  epoll.clr_interest(event_fd);

  TRA_PRINT(GRN "tcp_connect" RST ": closing event_fd {}", event_fd);
  if (-1 == close(event_fd)) {
    ERR_PRINT(GRN "tcp_connect" RST ": failed in close(eventfd): {}", strerror(errno));
  }

  for ( ; nullptr != res ; res = res->ai_next) {
    TRA_PRINT(GRN "tcp_connect" RST ": opening socket");
    mg_print_addrinfo(res);
    int sock_fd = ::mg7x::io::socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK, res->ai_protocol);
    if (-1 == sock_fd) {
      WRN_PRINT(GRN "tcp_connect" RST ": failed to create socket({}, {}, {}): {}", res->ai_family, res->ai_socktype, res->ai_protocol, strerror(errno));
      continue;
    }
    TRA_PRINT(GRN "tcp_connect" RST ": calling connect");
    int err = ::mg7x::io::connect(sock_fd, res->ai_addr, res->ai_addrlen);
    if (-1 == err && EINPROGRESS != errno && EAGAIN != errno) { // EINPROGRESS for SOCK_STREAM, EAGAIN for UNIX domain sockets
      ERR_PRINT(GRN "tcp_connect" RST ": signalled by 'connect', errno is {}: {}", errno, strerror(errno));
      close(sock_fd);
      continue;
    }

    if (0 == err) { // already connected
      INF_PRINT(GRN "tcp_connect" RST ": connected to {}", gai_req.ar_name);
      freeaddrinfo(gai_req.ar_result);
      co_return io::TcpConn{sock_fd, host, service};
    }

    {
      EpollCtl::Awaiter awaiter{sock_fd};
      err = epoll.add_interest(sock_fd, EPOLLOUT, awaiter);
      if (-1 == err) {
        ERR_PRINT(GRN "tcp_connect" RST ": failed in .add_interest");
        throw io::EpollError("Could not .add_interest");
      }

      DBG_PRINT(GRN "tcp_connect" RST ": co_await EpollCtl::Awaiter{{sock_fd = {}}}", sock_fd);
      auto [fd, events] = co_await awaiter;
      // TODO check events for errors
      DBG_PRINT(GRN "tcp_connect" RST ": return from co_await EpollCtl::Awaiter, fd {}, events {}", fd, events);
    }

    err = epoll.clr_interest(sock_fd);
    if (-1 == err) {
      ERR_PRINT(GRN "tcp_connect" RST ": failed in .clr_interest");
      throw io::EpollError("Could not .add_interest");
    }

    int sk_errno = 0;
    socklen_t sk_optlen = sizeof sk_errno;

    TRA_PRINT(GRN "tcp_connect" RST ": calling getsockopt");
    err = getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &sk_errno, &sk_optlen);

    if (-1 == err) {
      ERR_PRINT(GRN "tcp_connect" RST ": in getsockopt: {}", strerror(errno));
      close(sock_fd);
    }
    else if (0 != sk_errno) {
      WRN_PRINT(GRN "tcp_connect" RST ": connection failed: {}", strerror(sk_errno));
      close(sock_fd);
    }
    else {
      INF_PRINT(GRN "tcp_connect" RST ": connection established on FD {}, calling " RED "co_return" RST, sock_fd);
      co_return io::TcpConn{sock_fd, host, service};
    }
  }

  WRN_PRINT(GRN "tcp_connect" RST ": connect failed; " RED "co_return" RST " -1");
  throw io::IoError{"connect failed"};
}
}; // end namespace mg7x
