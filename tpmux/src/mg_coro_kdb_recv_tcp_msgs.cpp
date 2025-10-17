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

#include <stdlib.h> // EXIT_SUCCESS

#include <stdint.h>
#include <fcntl.h> // open flags
#include <sys/stat.h> // struct stat
#include <sys/mman.h> // MAP_FAILED
#include <unistd.h> // lseek, write
#include <string.h>

#include <unordered_set> // std::unordered_set
#include <algorithm> // std::copy
#include <ranges> // views::join_with

#include "MgIoDefs.h"
#include "KdbType.h"

#include "mg_coro_domain_obj.h"
#include "mg_fmt_defs.h"
#include "mg_io.h"
#include "mg_coro_epoll.h"
#include "mg_coro_task.h"


namespace mg7x {

static int handle_close(EpollCtl & epoll, int fd)
{
	std::expected<int,int> result = epoll.clr_interest(fd);
	if (!result) {
		ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": failed to remove FD {} from epoll: {}", fd, strerror(result.error()));
	}
	result = ::mg7x::io::close(fd);
	if (!result) {
		ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": failed to close FD {}: {}", fd, strerror(result.error()));
	}
	return -1;
}

TASK_TYPE<std::expected<int,ErrnoMsg>> kdb_read_tcp_messages(EpollCtl & epoll, const io::TcpConn & conn, KdbJournal & jnl, const Subscription & sub, TpMsgCounts & counts)
{
	DBG_PRINT(GRN "kdb_read_tcp_messages" RST ": have sock_fd {}, table-filter: {}, counts.included {}, counts.total", conn.sock_fd(), sub.tables() | std::views::join_with(',') | std::ranges::to<std::string>(), counts.m_num_msg_included, counts.m_num_msg_total);

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

		TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": reading from FD {}", conn.sock_fd());

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
					co_return handle_close(epoll, conn.sock_fd());
				}
			}
		}
		if (0 == io_res.value()) {
			WRN_PRINT(GRN "kdb_read_tcp_messages" RST ": received EOF on FD {}; closing socket. msg_include is {}, msg_total is {}", conn.sock_fd(), counts.m_num_msg_included, counts.m_num_msg_total);
			co_return handle_close(epoll, conn.sock_fd());
		}
		wr_off += io_res.value();
		DBG_PRINT(GRN "kdb_read_tcp_messages" RST ": read {} bytes on FD {}, wr_off is {}, remaining is {}", io_res.value(), conn.sock_fd(), wr_off, wr_off - rd_off);
		while ((wr_off - rd_off) > 0) {
			int64_t len = KdbUpdMsgFilter::filter_msg(ary.data() + rd_off, wr_off - rd_off, fn_name, tbl_names);
			if (len > 0) {
				// worth just pondering here that if the calling function sent us a `counts` object with positive values in
				// each of its message-count fields, that these _must_ exist within the tickerplant log file; therefore we
				// would have replayed these from the file. If TP is doing something funky with log-rotation to avoid each
				// getting "too big", then you won't be able to use this library without making some serious changes to
				// ensure each is filtered and replayed in-order.
				counts.m_num_msg_included += 1;
				counts.m_num_msg_total += 1;
				TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": have matching message: len {}, rd_off {}, wr_off {}, rem {}, msg_included {}, msg_total {}", len, rd_off, wr_off, wr_off - rd_off, counts.m_num_msg_included, counts.m_num_msg_total);
				// table match: copy to output

				// copy just the payload to the journal, without the 8-byte header:
				io_res = ::mg7x::io::write(jnl.jnl_fd(), ary.data() + SZ_MSG_HDR + rd_off, len - SZ_MSG_HDR);
				if (!io_res.has_value()) {
					ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": error writing to journal: %s", strerror(io_res.error()));
					handle_close(epoll, conn.sock_fd());
				}
				// TODO: forward the matching message to any connected subscribers
				rd_off += len;


			}
			else if (len < -2) {
				counts.m_num_msg_total += 1;
				TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": skipping non-matching message: len {}, rd_off {}, wr_off {}, rem {}, msg_total {}", len, rd_off, wr_off, wr_off - rd_off, counts.m_num_msg_total);
				rd_off += std::abs(len);
			}
			else if (-2 == len) { // insufficent data
				TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": insufficient data remain: rd_off {}, wr_off {}, rem {}", rd_off, wr_off, wr_off - rd_off);
				break;
			}
			else if (-1 == len) { // parse error
				ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": IPC parse-error reported while reading from FD {}; closing socket", conn.sock_fd());
				co_return handle_close(epoll, conn.sock_fd());
			}
		} // end inner while (wr_off - rd_off > 0)
		if (rd_off == wr_off) {
			TRA_PRINT(GRN "kdb_read_tcp_messages" RST ": buffer empty: resetting rd_off, wr_off");
			rd_off = wr_off = 0;
		}
		else if (rd_off > wr_off) {
			ERR_PRINT(GRN "kdb_read_tcp_messages" RST ": bad maths, crossed cursors: rd_off {}, wr_off {}", rd_off, wr_off);
			// TODO ensure message-count is coherent with published-count
			co_return handle_close(epoll, conn.sock_fd());
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
