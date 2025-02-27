#ifndef __mg_coro_epoll__H__

#include <sys/epoll.h>
// #include <string.h>
#include <coroutine>
#include <utility> // std::pair
#include <functional> // std::function, std::bind

namespace mg7x {

using EpollFunc = std::function<void(int)>;

class EpollCtl
{

public:
  struct Awaiter
  {
    EpollFunc m_callback = std::bind(&EpollCtl::Awaiter::onEvent, this, std::placeholders::_1);
    int m_fd;
    int m_events;
    std::coroutine_handle<> m_handle;

    explicit Awaiter(int fd);

    bool await_ready();

    void await_suspend(std::coroutine_handle<> h) noexcept;

    std::pair<int,int> await_resume() noexcept;

    void onEvent(int events);

  };

private:
  int m_epollfd;

  int epoll_upd(int fd, int events, int action, Awaiter * awaiter = nullptr);

public:
  explicit EpollCtl(int epoll_fd);

  int mod_interest(int fd, int events, Awaiter & awaiter);

  int add_interest(int fd, int events, Awaiter & awaiter);

  int clr_interest(int fd);

};

};

#endif // __mg_coro_epoll__H__