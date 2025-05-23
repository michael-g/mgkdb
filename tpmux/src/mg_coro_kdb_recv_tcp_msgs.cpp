#include <stdlib.h> // EXIT_SUCCESS

#include <stdint.h>
#include <fcntl.h> // open flags
#include <sys/stat.h> // struct stat
#include <sys/mman.h> // MAP_FAILED
#include <unistd.h> // lseek, write
#include <string.h>

#include <unordered_set> // std::unordered_set
#include <algorithm> // std::copy

#include "mg_coro_domain_obj.h"
#include "mg_fmt_defs.h"
#include "mg_io.h"
#include "mg_coro_epoll.h"
#include "mg_coro_task.h"

#include "KdbType.h"

namespace mg7x {

static int monitor_close(int fd)
{
  std::expected<int,int> result = ::mg7x::io::close(fd);
  if (!result.has_value()) {
    ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": failed to close FD {}: {}", fd, strerror(result.error()));
  }
  return -1;
}

TASK_TYPE<std::expected<int,ErrnoMsg>> kdb_read_tcp_messages(EpollCtl & epoll, const io::TcpConn & conn, const Subscription & sub, TpMsgCounts & counts)
{
  DBG_PRINT(GRN "kdb_read_tcp_messages" RST ": have sock_fd {}", conn.sock_fd());

  // We expect each TP message to be smaller than 256 Kb, amend here if you've got unusual requirements
  std::vector<int8_t> ary(256 * 1024);
  ary.resize(0);

  EpollCtl::Awaiter awaiter{conn.sock_fd()};
  std::expected<int,int> result = epoll.add_interest(conn.sock_fd(), EPOLLIN, awaiter);
  if (!result.has_value()) {
    ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": failed in EpollCtl::add_interest");
    co_return std::unexpected(ErrnoMsg{result.error(), "Failed in EpollCtl::add_interest"});
  }

  size_t rd_off = 0;
  size_t wr_off = 0;
  uint32_t msg_sz = -1;

  const std::unordered_set<std::string_view> tbl_names{sub.tables().begin(), sub.tables().end()};
  const std::string_view fn_name{"upd"};

  do {
    auto [_fd, rev] = co_await awaiter;
    if (rev & EPOLLERR) {
      ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": error signal from epoll: revents is {:#8x}", rev);
      co_return std::unexpected(ErrnoMsg{0, "error signal from epoll-revents"});
    }
    else if (0 != (rev & ~EPOLLIN)) {
      DBG_PRINT(GRN "kdb_read_tcp_messages" RST ": epoll revents is not just EPOLLIN: {:#8x}", rev & ~EPOLLIN);
    }

    ssize_t rdz = 0;
    std::expected<ssize_t,int> io_res = ::mg7x::io::read(conn.sock_fd(), ary.data() + wr_off, ary.capacity() - wr_off);
    if (!io_res.has_value()) {
      if (EAGAIN == io_res.error()) {
        TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": have EAGAIN on FD {}, nothing further to read", conn.sock_fd());
        rdz = 0;
      }
      else {
        if (EINTR == io_res.error()) {
          // TODO how many times?
          continue;
        }
        else {
          ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": while reading from FD {}: {}; closing socket", conn.sock_fd(), strerror(io_res.error()));
          co_return monitor_close(conn.sock_fd());
        }
      }
    }
    wr_off += io_res.value();
    while (wr_off - rd_off > 0) {
      int64_t len = KdbUpdMsgFilter::filter_msg(ary.data() + rd_off, wr_off - rd_off, fn_name, tbl_names);
      if (len > 0) {
        // worth just pondering here that if the calling function sent us a `counts` object with positive values in
        // each of its message-count fields, that these _must_ exist within the tickerplant log file; therefore we
        // would have replayed these from the file. If TP is doing something funky with log-rotation to avoid each
        // getting "too big", then you won't be able to use this library without making some serious changes to
        // ensure each is filtered and replayed in-order.
        counts.m_num_msg_included += 1;
        counts.m_num_msg_total += 1;
        TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": have matching message: len {}, rd_off {}, wr_off {}, rem {}", len, rd_off, wr_off, wr_off - rd_off);
        // TODO: table match; complete message: copy to output
        rd_off += len;
      }
      else if (len < -2) {
        counts.m_num_msg_total += 1;
        TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": skipping non-matching message: len {}, rd_off {}, wr_off {}, rem {}", len, rd_off, wr_off, wr_off - rd_off);
        rd_off += std::abs(len);
      }
      else if (-2 == len) { // insufficent data
        TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": insufficient data remain: rd_off {}, wr_off {}, rem {}", rd_off, wr_off, wr_off - rd_off);
        break;
      }
      else if (-1 == len) { // parse error
        ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": IPC parse-error reported while reading from FD {}; closing socket", conn.sock_fd());
        co_return monitor_close(conn.sock_fd());
      }
    }
    if (rd_off == wr_off) {
      TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": buffer empty: resetting rd_off, wr_off");
      rd_off = wr_off = 0;
    }
    else if (rd_off > wr_off) {
      ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": bad maths, crossed cursors: rd_off {}, wr_off {}", rd_off, wr_off);
      // TODO ensure message-count is coherent with published-count
      co_return monitor_close(conn.sock_fd());
    }
    else {
      // twiddle this ratio to vary the level at which it will compact
      // perhaps add other heuristics like amount-to-copy, or min/max/avg-message-size-over-time
      if ((ary.capacity() - wr_off) < (ary.capacity() / 4)) {
        TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": compacting buffer; rd_off {}, wr_off {}, capacity {}, load {}", rd_off, wr_off, ary.capacity(), ary.capacity() / static_cast<double>(wr_off));
        std::copy(ary.data() + rd_off, ary.data() + wr_off, ary.data());
        wr_off -= rd_off;
        rd_off = 0;
      }
      else {
        TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": not compacting buffer: more than 1/4 capacity remaining; rd_off {}, wr_off {}, capacity {}, load {}", rd_off, wr_off, ary.capacity(), ary.capacity() / static_cast<double>(wr_off));
      }
    }
  }
  while (true);

  co_return conn.sock_fd();
}

};
