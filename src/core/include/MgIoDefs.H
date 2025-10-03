/* This file is part of the "Mg kdb+ Library" (hereinafter "The Library").
 *
 * The Library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * The Library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License along with The
 * Library. If not, see https://www.gnu.org/licenses/agpl.txt.
 */
#ifndef MG_INC_MG_IO_DEFS_H
#define MG_INC_MG_IO_DEFS_H

#pragma once

#include <stdint.h>
#include <stddef.h> // size_t
#include <sys/types.h> // ssize_t
#include <sys/stat.h> // struct stat actually defined in bits/struct_stat.h
#include <sys/socket.h> // socklen_t

#include <expected>

namespace mg7x::io {

std::expected<int,int> eventfd(unsigned int initval, int flags) noexcept;

std::expected<int,int> epoll_ctl(int epfd, int op, int fd, struct epoll_event *events) noexcept;

std::expected<int,int> getaddrinfo_a(int mode, struct gaicb** list, int nitems, struct sigevent *sevp) noexcept;

std::expected<int,int> socket(int domain, int type, int protocol) noexcept;

std::expected<int,int> connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) noexcept;

std::expected<int,int> getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) noexcept;

std::expected<ssize_t,int> write(int fd, const void *buf, size_t count) noexcept;

std::expected<ssize_t,int> read(int fd, void *buf, size_t count) noexcept;

std::expected<int,int> open(const char *pathname, int flags) noexcept;

std::expected<int,int> open(const char *pathname, int flags, mode_t mode) noexcept;

std::expected<int,int> close(int fd) noexcept;

std::expected<int,int> fstat(int fd, struct stat *statbuf) noexcept;

std::expected<ssize_t,int> sendfile(int out_fd, int in_fd, off_t * offset, size_t count) noexcept;

std::expected<void*,int> mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) noexcept;

std::expected<int,int> memfd_create(const char *name, unsigned int flags) noexcept;

std::expected<int,int> munmap(void *addr, size_t length) noexcept;

std::expected<off_t,int> lseek(int fd, off_t offset, int whence) noexcept;

std::expected<int,int> ftruncate(int fd, off_t length) noexcept;

std::expected<ssize_t,int> read_fully(int fd, void *buf, size_t len) noexcept;

std::expected<ssize_t,int> write_fully(int fd, void *buf, size_t len) noexcept;

} // end namepace mg7x::io

#include "MgCore.H"
#include <string>

namespace mg7x
{

class FileDesc
{
  std::string m_path{};
  int m_fd{-1};
  uint64_t m_sz{0};

  inline
  void move_from(FileDesc && rhs) noexcept;

public:
  FileDesc() {}
  FileDesc(std::string const & path) : m_path{path} {}
  ~FileDesc() { close(); }

  FileDesc(FileDesc && rhs) { move_from(std::move(rhs)); }
  void operator=(FileDesc && rhs) noexcept { move_from(std::move(rhs)); }

  FileDesc(FileDesc const &) = delete;
  void operator=(FileDesc const &) = delete;

  int fd() const noexcept { return m_fd; }
  std::string const & path() const noexcept { return m_path; }
  uint64_t size64() const noexcept { return m_sz; }

  int open() noexcept;
  void close() noexcept;
  ssize_t stat_size() noexcept;
  int grow_to(Extent sz) noexcept;
  int set_size(Extent sz) noexcept;
  
}; // end class FileDesc

void FileDesc::move_from(FileDesc && rhs) noexcept
{
  m_path = rhs.m_path;
  m_fd = rhs.m_fd;
  m_sz = rhs.m_sz;
  rhs.m_path.clear();
  rhs.m_fd = -1;
  rhs.m_sz = 0;
}

} // end namepace mg7x

#endif
