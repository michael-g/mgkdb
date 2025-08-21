/*
This file is part of the Mg KDB-IPC C++ Library (hereinafter "The Library").

The Library is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

The Library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Affero Public License for more details.

You should have received a copy of the GNU Affero Public License along with The
Library. If not, see https://www.gnu.org/licenses/agpl.txt.
*/

#ifndef __mg7x_KdbIoDefs__H__
#define __mg7x_KdbIoDefs__H__

#pragma once

#include <stdint.h>
#include <stddef.h> // size_t
#include <sys/types.h> // ssize_t
#include <fcntl.h> // open
#include <unistd.h> // write, read, lseek, close
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
#include <errno.h>

#include <expected>

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
void setup_sockaddr_in_ipv4_localhost_port(struct sockaddr_in & dest, uint16_t port)
{
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl(0x7f000001);
	dest.sin_port = htons(static_cast<uint16_t>(port));
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
std::expected<int,int> memfd_create(const char *name, unsigned int flags)
{
	int mfd = ::memfd_create(name, flags);
	if (-1 == mfd) {
		return std::unexpected(errno);
	}
	return mfd;
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

inline
std::expected<int,int> ftruncate(int fd, off_t length)
{
	if (0 != ::ftruncate(fd, length)) {
		return std::unexpected(errno);
	}
	return 0;
}

} // end namepace mg7x::io

#endif
