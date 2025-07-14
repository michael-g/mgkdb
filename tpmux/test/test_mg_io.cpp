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

#include <sys/socket.h> // struct sockaddr, socklen_t
#include <errno.h>
#ifndef _GNU_SOURCE
#define _GN_SOURCE
#endif
#include <netdb.h> // getadrinfo_a structs
#include <sys/epoll.h> // struct epoll_event

#include <expected>
#include <vector>
#include <deque>

#include <gtest/gtest.h>

namespace mg7x::test::io {

  template<typename T>
  struct Outputs
  {
    T return_val;
    int errno_val;
  };

  template<typename T>
  T pop_set_err_and_return(std::deque<struct Outputs<T>> deq) {
    EXPECT_FALSE(deq.empty()) << "Return and errno values are required";
    auto obj = deq.front();
    deq.pop_front();
    errno = obj.errno_val;
    return obj.return_val;
  }

namespace eventfd {
  struct Args
  {
    unsigned int initval;
    int flags;
  };
  std::vector<struct Args> calls{};
  std::deque<struct Outputs<int>> outputs;
}

namespace epoll_ctl {
  struct Args {
    int epfd;
    int op;
    int fd;
    struct epoll_event *events;
  };
  std::vector<struct Args> calls{};
  std::deque<struct Outputs<int>> outputs;
}

namespace getaddrinfo_a {
  struct Args {
    int mode;
    struct gaicb** list;
    int nitems;
    struct sigevent *sevp;
  };
  std::vector<struct Args> calls{};
  std::deque<struct Outputs<int>> outputs;
}

namespace socket {
  struct Args
  {
    int domain;
    int type;
    int protocol;
  };
  std::vector<struct Args> calls{};
  std::deque<struct Outputs<int>> outputs{};
}

namespace connect {
  struct Args
  {
    int sockfd;
    const struct sockaddr *addr;
    socklen_t addrlen;
  };
  std::vector<struct Args> calls{};
  std::deque<struct Outputs<int>> outputs{};
}

namespace getsockopt {
  struct Args
  {
    int sockfd;
    int level;
    int optname;
    void *optval;
    socklen_t *optlen;
  };
  std::vector<struct Args> calls{};
  std::deque<struct Outputs<int>> outputs{};
}

namespace write {
  struct Args
  {
    int fd;
    const void *buf;
    size_t count;
  };
  std::vector<struct Args> calls{};
  std::deque<struct Outputs<int>> outputs{};
}

} // close namespace mg7x::test::io

namespace mg7x::io {

inline
int eventfd(unsigned int initval, int flags)
{
  using namespace mg7x::test::io::eventfd;
  calls.emplace_back(initval, flags);
  return pop_set_err_and_return(outputs);
}

inline
std::expected<int,int> epoll_ctl(int epfd, int op, int fd, struct epoll_event *events)
{
  using namespace mg7x::test::io::epoll_ctl;
  calls.emplace_back(epfd, op, fd, events);
  return pop_set_err_and_return(outputs);
}

inline
std::expected<int,int> getaddrinfo_a(int mode, struct gaicb** list, int nitems, struct sigevent *sevp)
{
  using namespace mg7x::test::io::getaddrinfo_a;
  calls.emplace_back(mode, list, nitems, sevp);
  return pop_set_err_and_return(outputs);
}

inline
std::expected<int,int> socket(int domain, int type, int protocol)
{
  using namespace mg7x::test::io::socket;
  calls.emplace_back(domain, type, protocol);
  return pop_set_err_and_return(outputs);
}

inline
std::expected<int,int> connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  using namespace mg7x::test::io::connect;
  calls.emplace_back(sockfd, addr, addrlen);
  return pop_set_err_and_return(outputs);
}

inline
std::expected<int,int> getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
  using namespace mg7x::test::io::getsockopt;
  calls.emplace_back(sockfd, level, optname, optval, optlen);
  return pop_set_err_and_return(outputs);

}

inline
ssize_t write(int fd, const void *buf, size_t count)
{
  using namespace mg7x::test::io::write;
  calls.emplace_back(fd, buf, count);
  return pop_set_err_and_return(outputs);
}

}// end namespace mg7x::io
