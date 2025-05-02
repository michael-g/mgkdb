
#include <sys/socket.h> // struct sockaddr, socklen_t
#include <errno.h>
#ifndef _GNU_SOURCE
#define _GN_SOURCE
#endif
#include <netdb.h> // getadrinfo_a structs
#include <sys/epoll.h> // struct epoll_event

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
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *events)
{
  using namespace mg7x::test::io::epoll_ctl;
  calls.emplace_back(epfd, op, fd, events);
  return pop_set_err_and_return(outputs);
}

inline
int getaddrinfo_a(int mode, struct gaicb** list, int nitems, struct sigevent *sevp)
{
  using namespace mg7x::test::io::getaddrinfo_a;
  calls.emplace_back(mode, list, nitems, sevp);
  return pop_set_err_and_return(outputs);
}

inline
int socket(int domain, int type, int protocol)
{
  using namespace mg7x::test::io::socket;
  calls.emplace_back(domain, type, protocol);
  return pop_set_err_and_return(outputs);
}

inline
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  using namespace mg7x::test::io::connect;
  calls.emplace_back(sockfd, addr, addrlen);
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
