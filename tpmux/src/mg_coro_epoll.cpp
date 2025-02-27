#include <coroutine>
#include <string.h>

#include "mg_fmt_defs.h"
#include "mg_coro_epoll.h"

namespace mg7x {

EpollCtl::Awaiter::Awaiter(int fd)
 : m_fd{fd}
{
  TRA_PRINT("EpollCtl::" ULB "Awaiter" RST " constructor: fd {}", fd);
}

bool EpollCtl::Awaiter::await_ready() {
  // a_handle.address is nil at this point
  TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_ready; address {}", m_handle.address());
  return false;
}

void EpollCtl::Awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
  TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_suspend; h? {}, h.done()? {}, h.address 0x{}", !!h, h && h.done(), h.address());
  m_handle = h;
}

std::pair<int,int> EpollCtl::Awaiter::await_resume() noexcept {
  TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_resume: fd {}, events {}, m_handle {}, m_handle.done {}, m_handle.address {}",
                  m_fd, m_events, !!m_handle, m_handle && m_handle.done(), m_handle.address());
  return {m_fd, m_events};
}

void EpollCtl::Awaiter::onEvent(int events) {
  TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::onEvent: fd {}, events {}, m_handle {}, m_handle.done {}, m_handle.address {}",
                  m_fd, events, !!m_handle, m_handle && m_handle.done(), m_handle.address());
  m_events = events;
  if (m_handle && !m_handle.done()) {
    m_handle.resume();
  }
}

int EpollCtl::epoll_upd(int fd, int events, int action, Awaiter * awaiter) {
  struct epoll_event ev;
  ev.events = events;
  if (nullptr != awaiter) {
    ev.data.ptr = &awaiter->m_callback;
  }
  //TRA_PRINT("EpollCtl::epoll_upd({}, {}, {}, {})", fd, events, action, awaiter);
  if (epoll_ctl(m_epollfd, action, fd, &ev) == -1) {
    ERR_PRINT("EpollCtl::epoll_upd in epoll_ctl: {}", strerror(errno));
    return -1;
  }
  return 0;
}

EpollCtl::EpollCtl(int epoll_fd)
  : m_epollfd{epoll_fd}
{ }

int EpollCtl::mod_interest(int fd, int events, Awaiter & awaiter) {
  TRA_PRINT("EpollCtl::mod_interest, fd = {}, events = {}", fd, events);
  return epoll_upd(fd, events, EPOLL_CTL_MOD, &awaiter);
}

int EpollCtl::add_interest(int fd, int events, Awaiter & awaiter) {
  TRA_PRINT("EpollCtl::add_interest, fd = {}, events = {}", fd, events);
  return epoll_upd(fd, events, EPOLL_CTL_ADD, &awaiter);
}

int EpollCtl::clr_interest(int fd) {
  TRA_PRINT("EpollCtl::clr_interest, fd = {}", fd);
  return epoll_upd(fd, 0, EPOLL_CTL_DEL);
}

};
