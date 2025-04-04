#ifndef __mg_io__H__
#define __mg_io__H__

#include <stdint.h>
#include <fcntl.h> // open
#include <unistd.h> // write, read, lseek
#include <sys/stat.h> // fstat
#include <sys/socket.h> // socket
#include <sys/eventfd.h> // eventfd
#include <sys/sendfile.h> // sendfile
#include <sys/mman.h> // mmap

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // getaddrinfo_a
#endif
#include <netdb.h> // getaddrinfo_a

#include <span> // std::span

namespace mg7x {

struct IO
{
  static
  int eventfd(unsigned int initval, int flags)
  {
    return ::eventfd(initval, flags);
  }

  static
  int getaddrinfo_a(int mode, struct gaicb** list, int nitems, struct sigevent *sevp)
  {
    return ::getaddrinfo_a(mode, list, nitems, sevp);
  }

  static
  int socket(int domain, int type, int protocol)
  {
    return ::socket(domain, type, protocol);
  }

  static
  int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
  {
    return ::connect(sockfd, addr, addrlen);
  }

  static
  ssize_t write(int fd, const std::span<uint8_t> & src)
  {
    return ::write(fd, src.data(), src.size_bytes());
  }

  static
  ssize_t write(int fd, const std::span<int8_t> & src)
  {
    return ::write(fd, src.data(), src.size_bytes());
  }

  static
  ssize_t read(int fd, std::span<int8_t> & dst)
  {
    return ::read(fd, dst.data(), dst.size_bytes());
  }

  static
  ssize_t read(int fd, std::span<uint8_t> & dst)
  {
    return ::read(fd, dst.data(), dst.size_bytes());
  }

  static
  int open(const char *pathname, int flags)
  {
    return ::open(pathname, flags);
  }

  static
  int open(const char *pathname, int flags, mode_t mode)
  {
    return ::open(pathname, flags, mode);
  }

  static 
  int close(int fd)
  {
    return ::close(fd);
  }

  static
  int fstat(int fd, struct stat *statbuf)
  {
    return ::fstat(fd, statbuf);
  }

  static
  ssize_t sendfile(int out_fd, int in_fd, off_t *_Nullable offset, size_t count)
  {
    return ::sendfile(out_fd, in_fd, offset, count);

  }

  static
  void* mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
  {
    return ::mmap(addr, length, prot, flags, fd, offset);
  }

  static
  int munmap(void *addr, size_t length)
  {
    return ::munmap(addr, length);
  }

  static
  off_t lseek(int fd, off_t offset, int whence)
  {
    return ::lseek(fd, offset, whence);
  }
};

}
#endif
