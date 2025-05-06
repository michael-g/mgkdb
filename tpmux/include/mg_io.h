#ifndef __mg_io__H__
#define __mg_io__H__

#include <stdint.h>
#include <fcntl.h> // open
#include <unistd.h> // write, read, lseek
#include <sys/stat.h> // fstat
#include <sys/socket.h> // socket
#include <sys/eventfd.h> // eventfd
#include <sys/sendfile.h> // sendfile
#include <sys/epoll.h> // epoll_ctl
#include <sys/mman.h> // mmap

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // getaddrinfo_a
#endif
#include <netdb.h> // getaddrinfo_a

#include <stdexcept> // std::logic_error
#include <expected>
#include <format> // std::formatter

namespace mg7x::io {

class EpollError : public std::logic_error
{
public:
  EpollError(const char *msg)
   : std::logic_error(msg)
  { }
};

class IoError : public std::logic_error
{
public:
  IoError(const char *msg)
   : std::logic_error(msg)
  { }
};

class TcpConn
{
  int m_sock_fd;
  std::string_view m_host;
  std::string_view m_service;
public:
  TcpConn(int sdf, std::string_view host, std::string_view service)
   : m_sock_fd(sdf)
   , m_host(host)
   , m_service(service)
   { }

   TcpConn()
    : TcpConn(-1, "", "")
   {}

   int sock_fd() const noexcept { return m_sock_fd; }

   std::string_view host() const noexcept { return m_host; }

   std::string_view service() const noexcept { return m_service; }
};

} // end namespance mg7x::io

template<>
struct std::formatter<mg7x::io::TcpConn>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::io::TcpConn & conn, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "TcpConn({}:{}, fd={})", conn.host(), conn.service(), conn.sock_fd());
	}
};

#ifndef __MG_SUPPRESS_IO_FUNC_DEFS__
namespace mg7x::io {

inline
std::expected<int,int> eventfd(unsigned int initval, int flags)
{
  int res = ::eventfd(initval, flags);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<int,int> epoll_ctl(int epfd, int op, int fd, struct epoll_event *events)
{
  int ret = ::epoll_ctl(epfd, op, fd, events);
  if (-1 == ret) {
    return std::unexpected(errno);
  }
  return ret;
}

inline
std::expected<int,int> getaddrinfo_a(int mode, struct gaicb** list, int nitems, struct sigevent *sevp)
{
  int ret = ::getaddrinfo_a(mode, list, nitems, sevp);
  if (-1 == ret) {
    return std::unexpected(errno);
  }
  return ret;
}

inline
std::expected<int,int> socket(int domain, int type, int protocol)
{
  int ret = ::socket(domain, type, protocol);
  if (-1 == ret) {
    return std::unexpected(errno);
  }
  return ret;
}

inline
std::expected<int,int> connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  int ret = ::connect(sockfd, addr, addrlen);
  if (-1 == ret) {
    return std::unexpected(errno);
  }
  return ret;
}

inline
std::expected<int,int> getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
  int ret = ::getsockopt(sockfd, level, optname, optval, optlen);
  if (-1 == ret) {
    return std::unexpected(errno);
  }
  return ret;
}

inline
std::expected<ssize_t,int> write(int fd, const void *buf, size_t count)
{
  ssize_t res = ::write(fd, buf, count);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<ssize_t,int> read(int fd, void *buf, size_t count)
{
  ssize_t res = ::read(fd, buf, count);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<int,int> open(const char *pathname, int flags)
{
  int res = ::open(pathname, flags);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<int,int> open(const char *pathname, int flags, mode_t mode)
{
  int res = ::open(pathname, flags, mode);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<int,int> close(int fd)
{
  int res = ::close(fd);
  if (-1 == res) {
    return std::unexpected(res);
  }
  return res;
}

inline
std::expected<int,int> fstat(int fd, struct stat *statbuf)
{
  int res = ::fstat(fd, statbuf);
  if (-1 == res) {
    return std::unexpected(res);
  }
  return res;
}

inline
std::expected<ssize_t,int> sendfile(int out_fd, int in_fd, off_t * offset, size_t count)
{
  ssize_t res = ::sendfile(out_fd, in_fd, offset, count);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<void*,int> mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  void *res = ::mmap(addr, length, prot, flags, fd, offset);
  if (MAP_FAILED == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<int,int> munmap(void *addr, size_t length)
{
  int res = ::munmap(addr, length);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

inline
std::expected<off_t,int> lseek(int fd, off_t offset, int whence)
{
  off_t res = ::lseek(fd, offset, whence);
  if (-1 == res) {
    return std::unexpected(errno);
  }
  return res;
}

} // end namepace mg7x::io
#endif
#endif
