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

#include "MgIoDefs.h"
#include "KdbType.h"

#include "mg_coro_epoll.h"
#include "mg_coro_task.h"
#include "mg_fmt_defs.h"
#include "mg_coro_domain_obj.h"

#include <expected>


namespace mg7x {

TASK_TYPE<std::expected<mg7x::ReadMsgResult,ErrnoMsg>>
	kdb_read_message(EpollCtl & epoll, int fd, std::vector<int8_t> & ary)
{
	INF_PRINT(YEL "kdb_read_message" RST ": have sock_fd {}", fd);
	if (ary.capacity() < mg7x::SZ_MSG_HDR) {
		ary.reserve(mg7x::SZ_MSG_HDR);
	}
	TRA_PRINT(YEL "kdb_read_message" RST ": ary.capacity is {}", ary.capacity());
	ary.clear();

	EpollCtl::Awaiter awaiter{fd};
	std::expected<int,int> result = epoll.add_interest(fd, EPOLLIN, awaiter);
	if (!result.has_value()) {
		ERR_PRINT(YEL "kdb_read_message" RST ": failed in EpollCtl::add_interest: {}", strerror(result.error()));
		co_return std::unexpected(ErrnoMsg{0, "Failed in EpollCtl::add_interest"});
	}

	ssize_t tot = 0;
	std::expected<ssize_t,int> io_res;

	while (tot < mg7x::SZ_MSG_HDR) {
		TRA_PRINT(YEL "kdb_read_message" RST ": awaiting read-signal for header bytes");
		auto [_fd, rev] = co_await awaiter;
		if (rev & EPOLLERR) {
			ERR_PRINT("error signal from epoll: revents is {}; as yet unhandled, exiting", rev);
			co_return std::unexpected(ErrnoMsg{0, "Error reported via revents"});
		}
		TRA_PRINT(YEL "kdb_read_message" RST ": have read-signal");

		io_res = ::mg7x::io::read(fd, ary.data(), mg7x::SZ_MSG_HDR);
		if (!io_res.has_value()) {
			ERR_PRINT("failed during read: {}; exiting.", strerror(io_res.error()));
			// TODO check EINTR
			co_return std::unexpected(ErrnoMsg{io_res.error(), "Failed during read"});
		}
		TRA_PRINT(YEL "kdb_read_message" RST ": read {} bytes", io_res.value());
		tot += io_res.value();
		TRA_PRINT(YEL "kdb_read_message" RST ": read {} bytes in total", tot);
	}

	mg7x::KdbIpcMessageReader rdr{};
	mg7x::ReadMsgResult res{};

	bool dun = rdr.readMsg(ary.data(), mg7x::SZ_MSG_HDR, res);
	TRA_PRINT(YEL "kdb_read_message" RST ": IPC length is {}, input-bytes consumed {}, msg_type is {}", rdr.getIpcLength(), rdr.getInputBytesConsumed(), res.msg_typ);
	size_t off = 0;
	// doesn't matter for v.3 IPC whether it's compressed or not, bytes [4-7] are the wire-size
	int8_t *dst = ary.data();
	int32_t num_nfrs = 0; // non-fatal retries
	while (!dun) {
		size_t len = std::min(ary.capacity() - off, rdr.getInputBytesRemaining());
		TRA_PRINT(YEL "kdb_read_message" RST ": requesting read of up to {} bytes at offset {}", len, off);

		io_res = ::mg7x::io::read(fd, dst + off, len);

		if (!io_res.has_value()) {
			if (EAGAIN == errno) {
				// would have blocked
				if (num_nfrs++ < 3) {
					auto [_fd, rev] = co_await awaiter;
					if (0 != (EPOLLERR & rev)) {
						ERR_PRINT("Non-zero error-flags reported by epoll: while waiting read signal");
						co_return std::unexpected(ErrnoMsg{io_res.error(), "EPOLLERR detected"});
					}
					continue;
				}
				ERR_PRINT("Got EAGAIN, non-fatal retry-count exceeded", num_nfrs);
				co_return std::unexpected(ErrnoMsg{result.error(), "Too many retries"});
			}
			else if (EINTR == errno) {
				if (num_nfrs++ < 3) {
					continue;
				}
				ERR_PRINT("Got EINTR, non-fatal retry-count exceeded", num_nfrs);
				co_return std::unexpected(ErrnoMsg{io_res.error(), "Too many retries"});
			}
			else {
				ERR_PRINT("failed during read: {}; exiting.", strerror(errno));
				co_return std::unexpected(ErrnoMsg{io_res.error(), "Failed during read"});
			}
		}

		num_nfrs = 0;
		TRA_PRINT(YEL "kdb_read_message" RST ": read {} bytes", io_res.value());
		size_t hav = off + io_res.value();
		size_t pre = rdr.getInputBytesConsumed();

		TRA_PRINT(YEL "kdb_read_message" RST ": offering .readMsg {} bytes", hav);
		dun = rdr.readMsg(dst, hav,	res);

		TRA_PRINT(YEL "kdb_read_message" RST ": this read consumed {}, total input bytes used {}, complete? {}", rdr.getInputBytesConsumed() - pre, rdr.getInputBytesConsumed(), dun);

		if (!dun) {
			size_t rem = hav - (rdr.getInputBytesConsumed() - pre);
			if (rem > 0) {
				TRA_PRINT(YEL "kdb_read_message" RST ": compacting data; rem {}, (hav-rem) {}, hav {}", rem, hav-rem, hav);
				// compact
				std::copy(dst + hav - rem, dst + hav, dst);
				off = rem;
			}
			else {
				off = 0;
			}
		}
	}

	TRA_PRINT(YEL "kdb_read_message" RST ": message-read complete, have:\n		" CYN "{}\n		" RST, *res.message.get());

	result = epoll.clr_interest(fd);
	if (!result.has_value()) {
		ERR_PRINT(YEL "kdb_read_message" RST ": failed in EpollCtl::clr_interest");
		co_return std::unexpected(ErrnoMsg{result.error(), "Failed in EpollCtl::clr_interest"});
	}

	TRA_PRINT(YEL "kdb_read_message" RST ": co_returning {}", res);

	co_return std::move(res);
}

}; // end namespace mg7x
