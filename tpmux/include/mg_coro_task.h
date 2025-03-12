#include <cstddef>
#ifndef __mg_coro_task__H__

#include <coroutine> // also std::hash
#include <utility> // std::exchange
#include <vector>

#include "mg_fmt_defs.h"

namespace mg7x {

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
      p_result = std::move(i);
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

  template<typename T>
  static
  TopLevelTask await(Task<T> & task) noexcept {
    TRA_PRINT(CYN "TopLevelTask" RST "::await: task.address {}", task.address());
    co_await task;
  }

  bool done() const noexcept {
    TRA_PRINT(CYN "TopLevelTask" RST "::done: m_handle.done? {}", m_handle.done());
    return m_handle.done();
  }

  std::coroutine_handle<Policy> m_handle;
};

class TaskContainer
{
  std::vector<TopLevelTask> m_tasks;

public:

  template<typename T>
  void add(Task<T> & task) noexcept
  {
    m_tasks.push_back(TopLevelTask::await(task));
  }

  bool complete() noexcept
  {
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

    return all_done;
  }

};

}; // end namespace mg7x

#endif // __mg_coro_task__H__
