#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h> // EXIT_SUCCESS
#include <errno.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <netdb.h> // getaddrinfo_a, getaddrinfo
#include <sys/socket.h> // getaddrinfo
#include <sys/types.h> // getaddrinfo
#include <arpa/inet.h> // inet_ntop
#include <signal.h> // sigev

#include <coroutine>
#include <exception>
#include <functional> // std::function, std::bind
#include <utility> // std::pair, std::exchange, std::forward
#include <print> // used by the logging macros
#include <stdio.h> // used by the logging macros for stdout/stderr
#include "mg_defs.h"

using Func = std::function<void(int)>;

class EpollCtl
{

public:
  struct Awaiter
  {
    Func m_callback = std::bind(&EpollCtl::Awaiter::onEvent, this, std::placeholders::_1);
    int m_fd;
    int m_events;
    std::coroutine_handle<> m_handle;

    explicit Awaiter(int fd)
     : m_fd{fd}
    {
      TRA_PRINT("EpollCtl::" ULB "Awaiter" RST " constructor: fd {}", fd);
    }

    bool await_ready() {
      // a_handle.address is nil at this point
      TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_ready; address {}", m_handle.address());
      return false;
    }

    void await_suspend(std::coroutine_handle<> h) noexcept {
      TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_suspend; h? {}, h.done()? {}, h.address 0x{}", !!h, h && h.done(), h.address());
      m_handle = h;
    }

    std::pair<int,int> await_resume() noexcept {
      TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_resume: fd {}, events {}, m_handle {}, m_handle.done {}, m_handle.address {}",
                     m_fd, m_events, !!m_handle, m_handle && m_handle.done(), m_handle.address());
      return {m_fd, m_events};
    }

    void onEvent(int events) {
      TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::onEvent: fd {}, events {}, m_handle {}, m_handle.done {}, m_handle.address {}",
                     m_fd, events, !!m_handle, m_handle && m_handle.done(), m_handle.address());
      m_events = events;
      if (m_handle && !m_handle.done()) {
        m_handle.resume();
      }
    }

  };

private:
  int m_epollfd;

  int epoll_upd(int fd, int events, int action, Awaiter * awaiter = nullptr) {
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

public:
  explicit EpollCtl(int epoll_fd)
   : m_epollfd{epoll_fd}
  { }

  int mod_interest(int fd, int events, Awaiter & awaiter) {
    TRA_PRINT("EpollCtl::mod_interest, fd = {}, events = {}", fd, events);
    return epoll_upd(fd, events, EPOLL_CTL_MOD, &awaiter);
  }

  int add_interest(int fd, int events, Awaiter & awaiter) {
    TRA_PRINT("EpollCtl::add_interest, fd = {}, events = {}", fd, events);
    return epoll_upd(fd, events, EPOLL_CTL_ADD, &awaiter);
  }

  int clr_interest(int fd) {
    TRA_PRINT("EpollCtl::clr_interest, fd = {}", fd);
    return epoll_upd(fd, 0, EPOLL_CTL_DEL);
  }

};

template<typename T>
class Task
{
public:
  struct Promise
  {
    struct FinalAwaiter {

      bool await_ready() noexcept {
        TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_ready");
        return false;
      }
      std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept {
      // bool await_suspend(std::coroutine_handle<Promise> h) noexcept {
        TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_suspend; h ? {}, h.done? {}, h.address {}, h.promise.p_cont.address {}", !!h, h && h.done(), h.address(), h.promise().p_cont.address());
        if (nullptr == h.promise().p_cont) {
          WRN_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_suspend; NULL continuation");
          return {};
        }
        return h.promise().p_cont;
      }
      void await_resume() const noexcept {
        TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_resume; nop");
      }
    };

    Task<T> get_return_object() noexcept {
      auto hdl = std::coroutine_handle<Promise>::from_promise(*this);
      TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::get_return_object; from_promise().address {}", hdl.address());
      return Task<T>{hdl};
    }
    std::suspend_never initial_suspend() noexcept {
      TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::initial_suspend; from_promise().address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
      return {};
    }
    void return_value(T i) {
      TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::return_value {}, from_promise().address {}", i, std::coroutine_handle<Promise>::from_promise(*this).address());
      p_result = i;
    }
    void unhandled_exception() noexcept {
      WRN_PRINT(MAG "Task" RST "::Promise:unhandled_exception; from_promise().address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
      std::terminate();
    }
    FinalAwaiter final_suspend() noexcept {
      TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::final_suspend; from_promise().address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
      return {};
    }

    T p_result;
    std::coroutine_handle<> p_cont;
  };
  using promise_type = Promise;

  Task() = delete;
  Task(Task<T> &) = delete;
  Task(const Task<T> &) = delete;

  Task(Task<T> && rhs) noexcept
   : t_handle{std::exchange(rhs.t_handle, {})}
  {
    ERR_PRINT(RED "Task" RST "::" RED "Task " RED "&&" RST ": t_handle.address {}", t_handle.address());
  }

  ~Task() {
    ERR_PRINT(MAG "Task" RST "::" RED "~" MAG "Task" RST ": t_handle? {}, t_handle.done? {}, t_handle.address() {}", !!t_handle, t_handle && t_handle.done(), t_handle.address());
    if (t_handle) {
      t_handle.destroy();
    }
  }

  bool done() {
    TRA_PRINT(MAG "Task" RST ".done: t_handle? {}, t_handle.done? {}, t_handle.address {}", !!t_handle, t_handle && t_handle.done(), t_handle.address());
    return t_handle && t_handle.done();
  }

  void* address() {
    return t_handle ? t_handle.address() : nullptr;
  }

  Task<T> operator=(const Task<T>) = delete;
  Task<T>& operator=(const Task<T> &) = delete;
  Task<T>& operator=(Task<T> &) = delete;
  Task<T>& operator=(const Task<T> &&) = delete;

  Task<T> & operator=(Task<T> && rhs) noexcept
  {
    ERR_PRINT(RED "Task" RST "::" RED "operator" RST "=(Task && rhs), rhs.address {}", rhs.t_handle.address());
    t_handle = std::exchange(rhs.t_handle, {});
    return *this;
  }

  class Awaiter
  {
  public:
    bool await_ready() noexcept {
      TRA_PRINT(YEL MAG "Task" RST "::" CYN "Awaiter" RST "::await_ready" RST "; address {}", a_handle.address());
      return false;
    }
    void await_suspend(std::coroutine_handle<> cont) noexcept {
      TRA_PRINT(YEL MAG "Task" RST "::" CYN "Awaiter" RST "::await_suspend:" RST " a_handle? {}, a_handle.done()? {}, a_handle.address {}, cont? {}, cont.done()? {}, cont.address {}",
                           !!a_handle, a_handle && a_handle.done(), a_handle.address(), !!cont, cont && cont.done(), cont.address());
      a_handle.promise().p_cont = cont;
    }
    T await_resume() noexcept {
      if (a_handle && a_handle.promise().p_cont) {
        TRA_PRINT(MAG "Task" RST "::" CYN "Awaiter" RST "::await_resume" RST "; p_result {}, a_handle? {}, a_handle.done()? {}, a_handle.address {}, p_cont.address() {}",
                     a_handle.promise().p_result, !!a_handle, a_handle && a_handle.done(), a_handle.address(), a_handle.promise().p_cont.address());
      }
      else {
        WRN_PRINT(MAG "Task" RST "::" CYN "Awaiter" RST "::await_resume" RST "; p_result {}, a_handle? {}, a_handle.done()? {}, a_handle.address {}, p_cont.address() (nil)", a_handle.promise().p_result, !!a_handle, a_handle && a_handle.done(), a_handle.address());
      }
      return a_handle.promise().p_result;
    }
  private:
    friend Task<T>;
    explicit Awaiter(std::coroutine_handle<Promise> h) noexcept
     : a_handle{h}
    {
      TRA_PRINT(MAG "Task" RST "::" CYN "Awaiter" RST " constructor" RST "; h.address {}", h.address());
    }
    std::coroutine_handle<Promise> a_handle;
  };

  Awaiter operator co_await() noexcept {
    TRA_PRINT(MAG "Task" RST "::operator co_await()" RST "; t_handle? {}, t_handle.done()? {}, address {}", !!t_handle, t_handle && t_handle.done(), t_handle.address());
    return Awaiter{t_handle};
  }

private:
  std::coroutine_handle<Promise> t_handle;
  explicit Task(std::coroutine_handle<Promise> h) noexcept
   : t_handle{h}
  {
    TRA_PRINT(MAG "Task" RST "::" MAG "Task" RST ": t_handle.address {}", h.address());
  }

};

struct TopLevelTask
{
  struct Policy
  {
    TopLevelTask get_return_object() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::get_return_object: m_handle.address {}", hdl.address());
      return TopLevelTask{hdl};
    }
    std::suspend_never initial_suspend() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::initial_suspend: m_handle.address {}", hdl.address());
      return {};
    }
    std::suspend_always final_suspend() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::" RED "final_suspend" RST ": m_handle.address {}", hdl.address());
      return {};
    }
    void return_void() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::return_void: m_handle.address {}", hdl.address());
    }
    void unhandled_exception() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::unhandled_exception: m_handle.address {}", hdl.address());
      std::terminate();
    }
  };
  using promise_type = Policy;

  explicit TopLevelTask(std::coroutine_handle<Policy> h) noexcept
   : m_handle{h}
  {
    TRA_PRINT(CYN "TopLevelTask" RST "::" CYN "TopLevelTask" RST ": m_handle.address {}", m_handle.address());
  }

  TopLevelTask(TopLevelTask & rhs) = delete;
  TopLevelTask(const TopLevelTask & rhs) = delete;

  TopLevelTask(TopLevelTask && rhs) noexcept
   : m_handle{std::exchange(rhs.m_handle, {})}
  {
    TRA_PRINT(CYN "TopLevelTask" RST "::" CYN "TopLevelTask " RED "&&" RST ": m_handle.address {}", m_handle.address());
  }

  TopLevelTask operator=(TopLevelTask) = delete;
  TopLevelTask& operator=(TopLevelTask&) = delete;
  TopLevelTask& operator=(const TopLevelTask&) = delete;

  ~TopLevelTask() {
    ERR_PRINT(CYN "TopLevelTask" RST "::" CYN "~TopLevelTask" RST ": m_handle.address {}", m_handle.address());
    if (m_handle) {
      m_handle.destroy();
    }
  }

  template<typename T>
  static
  TopLevelTask await(Task<T> & task) {
    TRA_PRINT(CYN "TopLevelTask" RST "::await: task.address {}", task.address());
    co_await task; // std::move(task);
  }

  bool done() {
    TRA_PRINT(CYN "TopLevelTask" RST "::done: m_handle.done? {}", m_handle.done());
    return m_handle.done();
  }

  std::coroutine_handle<Policy> m_handle;
};


static void mg_sigev_notify(union sigval arg) {
  int64_t val = 1;
  TRA_PRINT(YEL "mg_sigev_notify" RST ": calling write({}, &val, {})", arg.sival_int, sizeof(val));
  int err = write(arg.sival_int, &val, sizeof(val)); 
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
    return &(((struct sockaddr_in*)sa)->sin_addr);
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

static int create_event_fd()
{
  int ret = eventfd(0, EFD_NONBLOCK|EFD_SEMAPHORE);
  if (-1 == ret) {
    ERR_PRINT("failed in eventfd: {}", strerror(errno));
  }
  return ret;
}

Task<int> tcp_connect(EpollCtl & epoll, const char *host, const char *service)
{
  DBG_PRINT(GRN "tcp_connect" RST ": beginning tcp-connection to {}:{}", host, service);
  int event_fd = create_event_fd();
  if (-1 == event_fd) {
    // already logged
   	co_return -1;
  }

  DBG_PRINT(GRN "tcp_connect" RST ": created event_fd {}", event_fd);

  struct addrinfo addrinfo;
  struct gaicb gai_req;
  struct sigevent sevp;
  struct gaicb *gai_reqs[1];

  gai_req.ar_name = host;
  gai_req.ar_service = service;
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
      std::terminate();
    }

    DBG_PRINT(GRN "tcp_connect" RST ": calling getaddrinfo_a(..)");

    getaddrinfo_a(GAI_NOWAIT, gai_reqs, 1, &sevp);

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
    int sock_fd = socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK, res->ai_protocol);
    if (-1 == sock_fd) {
      WRN_PRINT(GRN "tcp_connect" RST ": failed to create socket({}, {}, {}): {}", res->ai_family, res->ai_socktype, res->ai_protocol, strerror(errno));
      continue;
    }
    TRA_PRINT(GRN "tcp_connect" RST ": calling connect");
    int err = connect(sock_fd, res->ai_addr, res->ai_addrlen);
    if (-1 == err && EINPROGRESS != errno && EAGAIN != errno) { // EINPROGRESS for SOCK_STREAM, EAGAIN for UNIX domain sockets
      ERR_PRINT(GRN "tcp_connect" RST ": signalled by 'connect', errno is {}: {}", errno, strerror(errno));
      close(sock_fd);
      continue;
    }
    
    if (0 == err) { // already connected
      INF_PRINT(GRN "tcp_connect" RST ": connected to {}", gai_req.ar_name);
      freeaddrinfo(gai_req.ar_result);
      co_return sock_fd;
    }

    {
      EpollCtl::Awaiter awaiter{sock_fd};
      int err = epoll.add_interest(sock_fd, EPOLLOUT, awaiter);
      if (-1 == err) {
        ERR_PRINT(GRN "tcp_connect" RST ": failed in .add_interest, exiting");
        std::terminate();
      }

      DBG_PRINT(GRN "tcp_connect" RST ": co_await EpollCtl::Awaiter{{sock_fd = {}}}", sock_fd);
      auto [fd, events] = co_await awaiter;
      // TODO check events for errors
      DBG_PRINT(GRN "tcp_connect" RST ": return from co_await EpollCtl::Awaiter, fd {}, events {}", fd, events);
    }

    int sk_errno = 0;
    socklen_t sk_optlen = sizeof sk_errno;

    TRA_PRINT(GRN "tcp_connect" RST ": calling getsockopt");
    err = getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &sk_errno, &sk_optlen);

    epoll.clr_interest(sock_fd);

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
      co_return sock_fd;
    }
  }

  WRN_PRINT(GRN "tcp_connect" RST ": connect failed; " RED "co_return" RST " -1");
  co_return -1;

}

Task<int> kdb_connect(EpollCtl & epoll, const char * host, const char * service, const char * user)
{
  Task<int> task = tcp_connect(epoll, host, service);

  DBG_PRINT(YEL "kdb_connect" RST ": awaiting tcp_connect Task");

  int sock_fd = co_await task; // std::move(task);


  if (-1 == sock_fd) {
    WRN_PRINT(YEL "kdb_connect" RST ": sock_fd is {}", sock_fd);
    co_return sock_fd;
  }

  DBG_PRINT(YEL "kdb_connect" RST ": beginning kdb-handshake using sock_fd {}", sock_fd);

  std::array<char,128> creds{};
  size_t off = 0;
  size_t len = strlen(user);

  if (len > 0) {
    std::copy(user, user + len, &creds[0]); 
    off = len;
  }
  creds[off++] = ':';
  creds[off++] = 3;
  creds[off++] = 0;

  {
    EpollCtl::Awaiter awaiter{sock_fd};
    int err = epoll.add_interest(sock_fd, EPOLLOUT, awaiter);
    if (-1 == err) {
      ERR_PRINT(YEL "kdb_connect" RST ": failed in .add_interest, exiting");
      std::terminate();
    }

    int retries = 0;
    len = off;
    off = 0;

  retry:
    DBG_PRINT(YEL "kdb_connect" RST ": writing credentials (attempt {})", retries);
    ssize_t wrt = write(sock_fd, &creds[off], len);
    if (-1 == wrt) {
      if (EWOULDBLOCK == errno || EAGAIN == errno) {
        if (retries++ < 3) {
          auto [events, fd] = co_await awaiter;
          // TODO test events
          goto retry;
        }
        ERR_PRINT(YEL "kdb_connect" RST ": can't retry again: giving up.");
        std::terminate();
      }
      ERR_PRINT(YEL "kdb_connect" RST ": failed in write: {}", strerror(errno));
      std::terminate();
    }
    off += wrt;
    if (off < len) {
      goto retry;
    }
  }

  {
    EpollCtl::Awaiter awaiter{sock_fd};
  
    int err = epoll.mod_interest(sock_fd, EPOLLIN, awaiter);

    DBG_PRINT(YEL "kdb_connect" RST ": awaiting read-event on sock_fd");
    auto [fd, events] = co_await awaiter;
    // TODO check events for errors
  }

  ssize_t red = read(sock_fd, &creds[0], 1);
  if (-1 == red) {
    // TOOD this should probably check for EAGAIN, probably not EWOULDBLOCK (given the size);
    ERR_PRINT(YEL "kdb_connect" RST ": while reading login-response: {}", strerror(errno));
    std::terminate();
  }
  if (1 == red) {
    INF_PRINT(YEL "kdb_connect" RST ": logged in as user '{}', agreed IPC version is {}", user, creds[0]);
  }
  else {
    WRN_PRINT(YEL "kdb_connect" RST ": login failed, read {} bytes", red);
    // TODO close sock_fd
  }

  TRA_PRINT(YEL "kdb_connect" RST ": co_return sock_fd is {}", sock_fd);
  co_return sock_fd;

}

Task<int> kdb_subscribe(EpollCtl & epoll, const char * service, const char * user)
{
  Task<int> task = kdb_connect(epoll, "localhost", service, user);

  DBG_PRINT(CYN "kdb_subscribe" RST ": awaiting kdb_connect Task");

  int sock_fd = co_await task; // std::move(task);

  if (-1 == sock_fd) {
    WRN_PRINT(YEL "kdb_connect" RST ": co_return sock_fd is {}", sock_fd);
    co_return sock_fd;
  }
  DBG_PRINT(CYN "kdb_subscribe" RST ": co_return sock_fd {}", sock_fd);

  co_return sock_fd;
}

#define MAX_EVENTS 10

int main(int argc, char **argv)
{
  int epollfd = epoll_create1(0);
  if (-1 == epollfd) {
    ERR_PRINT("main: epoll_create1 failed: {}", strerror(errno));
    return EXIT_FAILURE;
  }

  EpollCtl ctl{epollfd};

  Task<int> task = kdb_subscribe(ctl, "30098", "michaelg");
  TopLevelTask tlt = TopLevelTask::await(task);

  struct epoll_event events[MAX_EVENTS];
  while (!tlt.done()) {
    TRA_PRINT("main: calling epoll_wait");
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (-1 == nfds) {
      ERR_PRINT("main: in epoll_wait: {}", strerror(errno));
    }
    else {
      DBG_PRINT("main: have {} epoll fds", nfds);
      for (int i = 0 ; i < nfds ; i++) {
        Func *ptr = static_cast<Func*>(events[i].data.ptr);
        if (nullptr != ptr) {
          (*ptr)(events[i].events);
        }
        else {
          WRN_PRINT("have nullptr for event[{}]", i);
        }
      }
    }
  }

  tlt.done();

  return EXIT_SUCCESS;
}
#undef MAX_EVENTS