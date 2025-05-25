#include <cstddef>
#include <exception>
#ifndef __mg_coro_task__H__
#define __mg_coro_task__H__

#include <coroutine> // also std::hash
#include <utility> // std::exchange
#include <exception> // std::current_exception, std::exception_ptr
#include <vector>
#include <stdexcept> // std::logic_error

#include "mg_fmt_defs.h"


namespace mg7x {

struct SuspendAlways
{
  bool await_ready() const noexcept {
    TRA_PRINT(MAG "SuspendAlways" RST "::await_ready");
    return false;
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    TRA_PRINT(MAG "SuspendAlways" RST "::await_suspend; address {}", h.address());
  }

  void await_resume() const noexcept {
    TRA_PRINT(MAG "SuspendAlways" RST "::await_resume");
  }
};

struct SuspendNever
{
  bool await_ready() const noexcept {
    TRA_PRINT(MAG "SuspendNever" RST "::await_ready");
    return true;
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    TRA_PRINT(MAG "SuspendNever" RST "::await_suspend; address {}", h.address());
  }

  void await_resume() const noexcept {
    TRA_PRINT(MAG "SuspendNever" RST "::await_resume");
  }
};

}; // end namespace mg7x

#if 1

namespace mg7x {

class BrokenPromise : public std::logic_error
{
public:
  BrokenPromise()
   : std::logic_error("Not yet awaited")
  { }
};


template<typename T>
class Task
{
public:
  enum class ResultType
  {
    UNSET, EXCEPTION, RESULT
  };

  struct Promise
  {
    struct FinalAwaiter {

      bool await_ready() noexcept {
        TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_ready, returning {}", false);
        return false;
      }
      // See https://stackoverflow.com/q/78399968/322304 for discussion of different return types
      std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) {
        if (ResultType::EXCEPTION == h.promise().p_result_type) {
          WRN_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_suspend; rethrowing exception");
          std::rethrow_exception(h.promise().p_exception);
        }
        if (nullptr != h.promise().p_cont) {
          TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_suspend; h ? {}, h.done? {}, h.address {}, h.promise.p_cont.address {}", !!h, h && h.done(), h.address(), h.promise().p_cont.address());
          return h.promise().p_cont;
        }
        WRN_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::" RED "FinalAwaiter" RST "::await_suspend; NULL continuation, throwing protective exception; perhaps an exception should be raised instead of co_returning?");
        throw BrokenPromise{};
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
    SuspendAlways initial_suspend() noexcept {
      TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::initial_suspend; from_promise().address {}, p_cont.address {}", std::coroutine_handle<Promise>::from_promise(*this).address(), p_cont.address());
      return {};
    }
    void return_value(T && value) {
      TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST ":: from_promise().address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
      p_result = std::move(value);
      p_result_type = ResultType::RESULT;
    }
    // template<typename VALUE, typename = std::enable_if_t<std::is_convertible_v<VALUE&&,T>>>
    // void return_value(VALUE&& value) noexcept(std::is_nothrow_constructible_v<T, VALUE&&>)
    // {
    //   // Shamelessly understood and repeated from Lewis Baker's and thence Andreas Buhr's cppcoro::detail::task_promise::unhandled_exception
    //   // url = git@github.com:andreasbuhr/cppcoro.git
    //   // 817121cb438fb75e0652d8f7819f4855df3bcd2c
    //   TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::return_value {}, from_promise().address {}", value, std::coroutine_handle<Promise>::from_promise(*this).address());
    //   ::new (static_cast<void*>(std::addressof(p_result))) T(std::forward<VALUE>(value));
    //   p_result_type = ResultType::RESULT;
    // }
    void unhandled_exception() noexcept {
      WRN_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::unhandled_exception; from_promise().address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
      // Shamelessly understood and repeated from Lewis Baker's and thence Andreas Buhr's cppcoro::detail::task_promise::unhandled_exception
      // url = git@github.com:andreasbuhr/cppcoro.git
      // 817121cb438fb75e0652d8f7819f4855df3bcd2c
      ::new (static_cast<void*>(std::addressof(p_exception))) std::exception_ptr(std::current_exception());
      p_result_type = ResultType::EXCEPTION;
    }
    FinalAwaiter final_suspend() noexcept {
      TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::final_suspend; from_promise().address {}, result {}", std::coroutine_handle<Promise>::from_promise(*this).address(), static_cast<int>(p_result_type));
      return {};
    }

    Promise() noexcept
    {}

    ~Promise()
    {
      switch (p_result_type) {
        case ResultType::RESULT:
          // p_result.~T(); // C.f. the return_value implementation, using placement-new
          TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::~Promise; address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
          break;
        case ResultType::EXCEPTION:
          TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::~Promise; p_result_type is EXCEPTION, address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
          p_exception.~exception_ptr();
          break;
        default:
          TRA_PRINT(MAG "Task" RST "::" YEL "Promise" RST "::~Promise; p_result_type is EMPTY, address {}", std::coroutine_handle<Promise>::from_promise(*this).address());
          break;
      }
    }

    ResultType p_result_type;
    T p_result;
    std::exception_ptr p_exception;
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
    TRA_PRINT(MAG "Task" RST "::" RED "~" MAG "Task" RST ": t_handle? {}, t_handle.done? {}, t_handle.address() {}", !!t_handle, t_handle && t_handle.done(), t_handle.address());
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

  T value() {
    if (!done())
      return {};

    return t_handle.promise().p_result;
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
      TRA_PRINT(YEL MAG "Task" RST "::" CYN "Awaiter" RST "::await_ready" RST "; address {}, returning false", a_handle.address());
      return false;
    }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> cont) noexcept {
      TRA_PRINT(YEL MAG "Task" RST "::" CYN "Awaiter" RST "::await_suspend:" RST " a_handle? {}, a_handle.done()? {}, a_handle.address {}, cont? {}, cont.done()? {}, cont.address {}",
                           !!a_handle, a_handle && a_handle.done(), a_handle.address(), !!cont, cont && cont.done(), cont.address());
      a_handle.promise().p_cont = cont;
      return a_handle;
    }
    T await_resume() {
      if (nullptr == a_handle || nullptr == a_handle.promise().p_cont) {
        WRN_PRINT(MAG "Task" RST "::" CYN "Awaiter" RST "::await_resume" RST "; a_handle? {}, a_handle.done()? {}, a_handle.address {}, p_cont.address() (nil), raising BrokenPromise", !!a_handle, a_handle && a_handle.done(), a_handle.address());
        throw new BrokenPromise();
      }
      TRA_PRINT(MAG "Task" RST "::" CYN "Awaiter" RST "::await_resume" RST "; a_handle? {}, a_handle.done()? {}, a_handle.address {}, p_cont.address() {}", !!a_handle, a_handle && a_handle.done(), a_handle.address(), a_handle.promise().p_cont.address());
      return std::move(a_handle.promise().p_result);
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



}; // end namespace mg7x

#define TASK_TYPE ::mg7x::Task

#else /* begin alternate task-definition */

#include "cppcoro/task.hpp"
#define TASK_TYPE ::cppcoro::task

#endif /* end alternate task-definition */

namespace mg7x {

template<typename T>
struct TopLevelTask
{
  struct Policy
  {
    TopLevelTask get_return_object() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::get_return_object: m_handle.address {}", hdl.address());
      return TopLevelTask{hdl};
    }
    SuspendNever initial_suspend() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::initial_suspend: m_handle.address {}", hdl.address());
      return {};
    }
    SuspendNever final_suspend() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::" RED "final_suspend" RST ": m_handle.address {}", hdl.address());
      return {};
    }
    void return_value(T val) noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      TRA_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::return_void: m_handle.address {}", hdl.address());
      p_result = val;
    }
    void unhandled_exception() noexcept {
      auto hdl = std::coroutine_handle<Policy>::from_promise(*this);
      WRN_PRINT(CYN "TopLevelTask" RST "::" YEL "Policy" RST "::unhandled_exception: m_handle.address {}", hdl.address());
    }

    T p_result;
  };
  using promise_type = Policy;

  explicit TopLevelTask(std::coroutine_handle<Policy> h) noexcept
    : m_handle{h}
  {
    TRA_PRINT(CYN "TopLevelTask" RST "::" CYN "TopLevelTask" RST "(handle): m_handle.address {}", m_handle.address());
  }
  explicit TopLevelTask(TopLevelTask && rhs) noexcept
    : m_handle{std::exchange(rhs.m_handle, {})}
  {
    TRA_PRINT(CYN "TopLevelTask" RST "::" CYN "TopLevelTask" RST "(" RED "&&" RST "): m_handle.address {}", m_handle.address());
  }

  TopLevelTask& operator=(TopLevelTask && rhs) noexcept
  {
    TRA_PRINT(CYN "TopLevelTask" RST "::" CYN "operator" RST "=(" RED "&&" RST "): m_handle.address {}", m_handle.address());
    m_handle = std::exchange(rhs.m_handle, {});
    return *this;
  }

  ~TopLevelTask() {
    TRA_PRINT(CYN "TopLevelTask" RST "::" CYN "~TopLevelTask" RST ": m_handle.address {}", m_handle.address());
    if (m_handle) {
      m_handle.destroy();
    }
  }

  static
  TopLevelTask<T> await(TASK_TYPE<T> & task) noexcept {
    TRA_PRINT(CYN "TopLevelTask" RST "::await");
    T result = co_await task;
    TRA_PRINT(CYN "TopLevelTask" RST "::await: after co_await: result {}", result);
    co_return result;
  }

  bool done() const noexcept {
    TRA_PRINT(CYN "TopLevelTask" RST "::done: m_handle.done? {}", m_handle.done());
    return m_handle.done();
  }

  T value() {
    if (!done())
      return {};
    return m_handle.promise().p_result;
  }

  std::coroutine_handle<Policy> m_handle;
}; // end class TopLevelTask


template<typename T>
class TaskContainer
{
  std::vector<TopLevelTask<T>> m_tasks;

public:

  void add(TASK_TYPE<T> & task) noexcept
  {
    m_tasks.push_back(TopLevelTask<T>::await(task));
  }

  bool complete() noexcept
  {
    TRA_PRINT(YEL "TaskContainer" RST "::complete: ENTER");
    bool all_done = true;
    for (auto it0 = m_tasks.begin() ; it0 != m_tasks.end() ; ) {
      bool done = it0->done();
      if (done) {
        TRA_PRINT(YEL "TaskContainer" RST "::complete: removing TopLevelTask from vector: {}", it0->m_handle.address());
        it0 = m_tasks.erase(it0);
      }
      else {
        it0++;
        all_done = false;
      }
    }
    TRA_PRINT(YEL "TaskContainer" RST "::complete: EXIT");
    return all_done;
  }

}; // end class TaskContainer

}; // end namespace mg7x

#endif /* end #ifndef __mg_coro_task__H__ */
