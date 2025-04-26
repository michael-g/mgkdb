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

namespace mg7x {

namespace io {

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

inline
int eventfd(unsigned int initval, int flags)
{
  return ::eventfd(initval, flags);
}

inline
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *events)
{
  return ::epoll_ctl(epfd, op, fd, events);
}

inline
int getaddrinfo_a(int mode, struct gaicb** list, int nitems, struct sigevent *sevp)
{
  return ::getaddrinfo_a(mode, list, nitems, sevp);
}

inline
int socket(int domain, int type, int protocol)
{
  return ::socket(domain, type, protocol);
}

inline
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  return ::connect(sockfd, addr, addrlen);
}

inline
ssize_t write(int fd, const void *buf, size_t count)
{
  return ::write(fd, buf, count);
}

inline
ssize_t read(int fd, void *buf, size_t count)
{
  return ::read(fd, buf, count);
}

inline
int open(const char *pathname, int flags)
{
  return ::open(pathname, flags);
}

inline
int open(const char *pathname, int flags, mode_t mode)
{
  return ::open(pathname, flags, mode);
}

inline 
int close(int fd)
{
  return ::close(fd);
}

inline
int fstat(int fd, struct stat *statbuf)
{
  return ::fstat(fd, statbuf);
}

inline
ssize_t sendfile(int out_fd, int in_fd, off_t * offset, size_t count)
{
  return ::sendfile(out_fd, in_fd, offset, count);

}

inline
void* mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  return ::mmap(addr, length, prot, flags, fd, offset);
}

inline
int munmap(void *addr, size_t length)
{
  return ::munmap(addr, length);
}

inline
off_t lseek(int fd, off_t offset, int whence)
{
  return ::lseek(fd, offset, whence);
}

} // end namespace io

} // end namepace mg7x
#endif
