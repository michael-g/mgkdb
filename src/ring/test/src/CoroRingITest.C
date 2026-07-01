#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <coroutine>
#include <liburing/io_uring.h>
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#include <liburing.h>
#include <fcntl.h>  // open-constants
#include <string.h> // memset
#include <sys/stat.h> //fstat

#include "MgCore.H"
#include "MgDebug.H"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <coroutine>
#include <expected>
#include <exception>

namespace mg7x
{

#define COR_PRINT(fmt, ...)\
do { if (MG_LOG_ERR_ENABLED) MG_LOG(stderr, YEL " PROM" RST, fmt RST,##__VA_ARGS__);} while (0)
#define TSK_PRINT(fmt, ...)\
do { if (MG_LOG_ERR_ENABLED) MG_LOG(stderr, GRN " TASK" RST, fmt RST,##__VA_ARGS__);} while (0)

struct SetCqeDataAndResume
{
  struct io_uring_sqe *m_sqe;
  SetCqeDataAndResume(struct io_uring_sqe *sqe) : m_sqe(sqe) {}
};

struct SetCqeDataAndWait
{
  struct io_uring_sqe *m_sqe;
  SetCqeDataAndWait(struct io_uring_sqe *sqe) : m_sqe(sqe) {}
};

struct CqeAwaitEvt
{
};

struct CqeBatchEvt
{
  std::vector<struct io_uring_sqe*> m_vec{};
  std::vector<int32_t> m_results{};
  void push_sqe(struct io_uring_sqe *sqe) { m_vec.push_back(sqe); }
  void push_result(int32_t result) { m_results.push_back(result); }
  uint32_t size() const { return m_vec.size(); }
};

template<typename R>
class CoTask;


class IoContext
{
  struct io_uring *m_ring;
  struct io_uring_cqe *m_cqe;

public:
  IoContext(struct io_uring *ring)
  : m_ring{ring}
  {}

  std::expected<io_uring_sqe*,int>
  get_sqe(bool submit = false);
  int submit();
  int await_and_dispatch(uint32_t nr);
  int submit_and_await_dispatch(uint32_t nr);
  // C.f. asyncpp-uring/include/asyncpp/uring/io_service.h
  // Can't easily copy because of the flexible struct member big_cqe, IIRC,
  // so we have to point-at rather than copy, so we're left with this "get
  // current" approach.
  constexpr struct io_uring_cqe*
  current_cqe();

  template<typename R>
  CoTask<R> gen_await_task_for(struct io_uring_sqe *sqe);

  template<typename R>
  CoTask<R> gen_await_task_for(CqeBatchEvt & evt);
  // void retire_cqe(struct io_uring_cqe *cqe){ io_uring_cqe_seen(m_ring, cqe); }
};

struct CqeConduitAwaiter
{
  IoContext & m_ctx;
  bool await_ready() { return false; } // always suspend
  void await_suspend(std::coroutine_handle<>) { }
  struct io_uring_cqe* await_resume() { return m_ctx.current_cqe(); }
};

template<typename T, typename R>
struct MyNthPromiseType
{
  using handle_type = std::coroutine_handle<MyNthPromiseType<T,R>>;
  std::exception_ptr m_exception;
  R m_result;
  IoContext & m_ctx;

  template<typename ... Args>
  MyNthPromiseType(IoContext & ctx, Args &&...)
  : m_ctx{ctx}
  {}

  T get_return_object()
  {
    COR_PRINT("");
    return T{handle_type::from_promise(*this)};
  }

  std::suspend_never initial_suspend()
  {
    COR_PRINT("");
    return {};
  }

  void return_value(R val)
  {
    m_result = val;
  }

  void unhandled_exception()
  {
    m_exception = std::current_exception();
    COR_PRINT("ERROR: unhandled_exception");
  }

  std::suspend_always final_suspend() noexcept
  {
    COR_PRINT("");
    return {};
  }

  CqeConduitAwaiter await_transform(CqeAwaitEvt const &)
  {
    COR_PRINT("");
    return {m_ctx};
  }

  void attach_sqe(io_uring_sqe *sqe)
  {
    void *addr = handle_type::from_promise(*this).address();
    DBG_PRINT("Setting promise address on SQE.data; SQE address {}, promise.address {}", (void*)sqe, addr);
    io_uring_sqe_set_data(sqe, addr);
  }

  // std::suspend_always await_transform(SetCqeDataAndWait const & evt)
  // {
  //   COR_PRINT("SeqCqeDataAndWait");
  //   attach_sqe(evt.m_sqe);
  //   return {};
  // }

  std::suspend_never await_transform(SetCqeDataAndResume const & evt)
  {
    COR_PRINT("SetCqeDataAndResume");
    attach_sqe(evt.m_sqe);
    return {};
  }

};

template<typename R>
class CoTask
{
  friend struct MyNthPromiseType<CoTask<R>,R>;

public:
  using promise_type = MyNthPromiseType<mg7x::CoTask<R>,R>;

private:
  using handle_type = std::coroutine_handle<promise_type>;

  handle_type m_handle;

  CoTask(handle_type h)
   : m_handle{h}
  {
    TSK_PRINT("constructor");
  }

  CoTask(const CoTask &) = delete;
  CoTask &operator=(const CoTask &) = delete;

public:

  CoTask(CoTask &&rhs)
   : m_handle{rhs.m_handle}
  {
    TSK_PRINT("move-constructor");
    rhs.m_handle = nullptr;
  }

  CoTask &operator=(CoTask && rhs) {
    TSK_PRINT("&=");
    if (m_handle) {
      m_handle.destroy();
    }
    m_handle = rhs.m_handle;
    rhs.m_handle = nullptr;
    return *this;
  }

  ~CoTask() {
    TSK_PRINT("~");
    if (m_handle)
      m_handle.destroy();
  }

  bool has_exception()
  {
    return !!m_handle.promise().m_exception;
  }

  std::exception_ptr & get_exception()
  {
    return m_handle.promise().m_exception;
  }

  R final_result()
  {
    // caller can check beforehand if they don't want it thrown...[?]
    if (has_exception()) {
      std::rethrow_exception(get_exception());
    }
    TSK_PRINT("");
    return m_handle.promise().m_result;
  }
};

std::expected<io_uring_sqe*,int>
IoContext::get_sqe(bool submit)
{
  struct io_uring_sqe *sqe = io_uring_get_sqe(m_ring);
  if (!sqe && submit) {
    int err = io_uring_submit(m_ring);
    if (err < 0) {
      sqe = nullptr;
      ERR_PRINT("bad io_uring_submit: {}", strerror(-err));
      return std::unexpected(-1);
    }
    sqe = io_uring_get_sqe(m_ring);
    if (!sqe) {
      sqe = nullptr;
      ERR_PRINT("no free io_uring_sqe");
      return std::unexpected(-1);
    }
  }
  DBG_PRINT("Reserved SQE {}", (void*)sqe);
  memset(sqe, 0, sizeof(io_uring_sqe));
  return sqe;
}

constexpr
struct io_uring_cqe*
IoContext::current_cqe()
{
  int err = io_uring_peek_cqe(m_ring, &m_cqe);
  if (0 != err) {
    ERR_PRINT("Ooops, it returned -EAGAIN: {}", strerror(-err));
    return nullptr;
  }
  return m_cqe;
}

int IoContext::submit()
{
  DBG_PRINT("Calling io_uring_submit");
  int no_evts = io_uring_submit(m_ring);
  if (no_evts < 0) {
    ERR_PRINT("io_uring_submit failed: {}", strerror(-no_evts));
  }
  return no_evts;
}

int IoContext::await_and_dispatch(uint32_t nr)
{
  struct io_uring_cqe *cqe = nullptr;
  DBG_PRINT("Awaiting CQEs");
  int err;
  if (0 == nr) {
    // wait indefinitely
    err = io_uring_wait_cqes(m_ring, &cqe, 0, nullptr, 0);
  }
  else {
    err = io_uring_wait_cqe_nr(m_ring, &cqe, nr);
  }
  if (err < 0) {
    ERR_PRINT("bad io_uring_wait_cqe{{s,_nr}}: {}", strerror(-err));
    return -1;
  }
  struct io_uring_cqe_iter iter = io_uring_cqe_iter_init(m_ring);

  // C.f. macro io_uring_for_each_cqe
  while (io_uring_cqe_iter_next(&iter, &cqe)) {
    void* ptr = ptr_cvt<void*>(io_uring_cqe_get_data(cqe));
    if (ptr) {
      DBG_PRINT("Have CQE; cqe->data is {}", ptr);
      std::coroutine_handle<>::from_address(ptr).resume();
    }
    else {
      ERR_PRINT("No cqe->data field");
      io_uring_cqe_seen(m_ring, cqe);
      return -1; 
    }
    DBG_PRINT("Releasing CQE");
    io_uring_cqe_seen(m_ring, cqe);
  }
  return 0;
}

int IoContext::submit_and_await_dispatch(uint32_t nr)
{
  int no_evts = submit();
  if (no_evts < 0) {
    return -1;
  }
  int err = await_and_dispatch(nr);
  if (err < 0) {
    return err;
  }
  return no_evts;
}

template<>
CoTask<int> IoContext::gen_await_task_for(struct io_uring_sqe *sqe)
{
  co_await SetCqeDataAndResume{sqe};

  struct io_uring_cqe *cqe = co_await CqeAwaitEvt{};

  int32_t res = cqe->res;

  // retire_cqe(cqe);

  co_return res;
}


template<>
CoTask<int> IoContext::gen_await_task_for(CqeBatchEvt & evt)
{
  const auto size = evt.size();
  for (decltype(evt.m_vec)::size_type i = 0 ; i < size ; i++) {
    if (i > 0) {
      evt.m_vec.at(i)->flags |= IOSQE_IO_LINK;
    }
    DBG_PRINT("co-awaiting SetCqeDataAndResume for {} of {}", i+1, evt.m_vec.size());
    co_await SetCqeDataAndResume{evt.m_vec.at(i)};
  }

  for (decltype(evt.m_vec)::size_type i = 0 ; i < size ; i++) {
    DBG_PRINT("co-awaiting CqeAwaitEvt for {} of {}", i+1, size);
    struct io_uring_cqe *cqe = co_await CqeAwaitEvt{};
    evt.push_result(cqe->res);
    DBG_PRINT("io_uring_job {} of {} returned {}", 1+i, size, cqe->res);
  }
  co_return 0;
}

TEST(CoroRingITest, TestMgTask)
{
  struct io_uring ring{};
  struct io_uring_params params{};
  params.cq_entries = 128; // power-of-two
  int err = io_uring_queue_init_params(64, &ring, &params);
  if (0 != err) {
    FAIL() << "bad init: " << err;
  }
  IoContext ctx{&ring};

  const pid_t pid = getpid();
  std::string f_name{"/tmp/coro_ring_ops_i_test."};
  auto spid = std::stringstream{} << pid;
  f_name += spid.str();

  struct io_uring_sqe *sqe = nullptr;

  //--------------------------------------------------- Open
  auto sqe_res = ctx.get_sqe();
  if (!sqe_res) {
    FAIL() << "No SQE.1";
  }
  sqe = sqe_res.value();
  io_uring_prep_open(sqe, f_name.c_str(), O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);

  INF_PRINT("Creating await-task for open call");
  CoTask<int> open_task = ctx.gen_await_task_for<int>(sqe);
  int no_evts = ctx.submit_and_await_dispatch(1);
  if (no_evts < 0) {
    FAIL() << "submit_and_dispatch";
  }
  err = open_task.final_result();
  if (err < 0) {
    ERR_PRINT("open failed: {}", strerror(-err));
    FAIL() << "open.error";
  }

  const int fd = err;
  INF_PRINT("Opened file {} as FD {}", f_name, fd);

  //--------------------------------------------------- Ftruncate
  sqe_res = ctx.get_sqe();
  if (!sqe_res) {
    FAIL() << "No SQE.2";
  }
  sqe = sqe_res.value();

  io_uring_prep_ftruncate(sqe, fd, 4096);
  INF_PRINT("Creating await-task for ftruncate call");
  CoTask<int> ftrunc_task = ctx.gen_await_task_for<int>(sqe);
  no_evts = ctx.submit_and_await_dispatch(1);
  if (no_evts < 0) {
    FAIL() << "submit_and_dispatch";
  }
  err = ftrunc_task.final_result();
  if (err < 0) {
    ERR_PRINT("ftruncate failed: {}", strerror(-err));
    FAIL() << "ftruncate.error";
  }
  INF_PRINT("ftruncate complete");

  //--------------------------------------------------- Check ftruncate
  struct stat sb{};
  err = fstat(fd, &sb);
  if (0 != err) {
    ERR_PRINT("fstat failed: {}", strerror(errno));
    FAIL() << "fstat.error";
  }
  INF_PRINT("truncated file FD {} to {}", fd, sb.st_size);

  //--------------------------------------------------- Get the two SQEs we'll need
  sqe_res = ctx.get_sqe();
  if (!sqe_res) {
    FAIL() << "No SQE.3";
  }
  struct io_uring_sqe *hdr_sqe = sqe_res.value();

  sqe_res = ctx.get_sqe();
  if (!sqe_res) {
    FAIL() << "No SQE.4";
  }
  struct io_uring_sqe *tlr_sqe = sqe_res.value();

  //--------------------------------------------------- Set-up the write of the file header
  DBG_PRINT("Setting up header write");

  struct KFileHdr { int8_t m, a, t, u; int32_t r; };
  KFileHdr hdr{.m=(int8_t)0xfd, .a=0x20, .t=0, .u=0, .r=0};
  io_uring_prep_write(hdr_sqe, fd, &hdr, sizeof(KFileHdr), 0);
  
  //--------------------------------------------------- Set-up the linked write of the file-tailer
  DBG_PRINT("Setting up tailer write");

  struct KHdr { int8_t m, a, t, u; int32_t r; int64_t n;};
  KHdr tlr{.m=(int8_t)0xfd, .a=0, .t=0x4d, .u=0, .r=0, .n=0};
  io_uring_prep_write(tlr_sqe, fd, &tlr, sizeof(KHdr), 0xff0);

  CqeBatchEvt batch{};
  batch.push_sqe(hdr_sqe);
  batch.push_sqe(tlr_sqe);

  CoTask<int> batch_task = ctx.gen_await_task_for<int>(batch);

  no_evts = ctx.submit_and_await_dispatch(2);
  if (no_evts < 0) {
    FAIL() << "submit_and_dispatch";
  }
  err = batch_task.final_result();
  if (err < 0) {
    ERR_PRINT("io_uring error processing writes");
    FAIL() << "write.error";
  }
  for (size_t i = 0 ; i < batch.m_results.size() ; i++) {
    INF_PRINT("Wrote {} bytes in op {}", batch.m_results.at(i), i);
  }
}

} // end namespace mg7x

