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
#include <stdlib.h> // EXIT_FAILURE
#include <stdint.h> // uint16_t

#include "KdbType.h"
#include "KdbIoDefs.h"
#include "CircularBuffer.h"

#include <algorithm> // std::copy
#include <tuple> // std::ignore
#include <vector>
#include <print>

int connect_and_login(const uint16_t port);

using namespace ::mg7x;

int main(void)
{
	int fd = connect_and_login(static_cast<uint16_t>(30098));

	KdbFunction func{"{x}"};

	const uint64_t tbl_len = 1000;

	KdbTimeVector time{tbl_len};
	KdbSymbolVector sym{};
	KdbLongVector size{tbl_len};

	const int32_t eight_am = 8 * 60 * 60 * 1000;
	for (uint64_t i = 0 ; i < tbl_len ; i++) {
		time.setMillis(i, eight_am + 1000 * static_cast<int32_t>(i));
		sym.push("VOD.L");
		size.setLong(i, static_cast<int64_t>(i));
	}

	KdbTable tbl{ColDef{"time", time}, ColDef{"sym", sym}, ColDef{"size", size}};

	KdbList cmd{};
	cmd.push(func);
	cmd.push(tbl);

	KdbIpcMessageWriter writer{KdbMsgType::SYNC, cmd};

	// std::expected<CircBufPtr,std::string>
	auto circ_buf_res =  init_circ_buffer(4096);
	if (!circ_buf_res) {
		std::print("ERROR: while creating circular buffer: {}", circ_buf_res.error());
		return EXIT_FAILURE;
	}
	std::unique_ptr<CircularBuffer> buf{};
	std::swap(buf, circ_buf_res.value());

	const size_t ipc_len = writer.ipcLength();

	do {
		// We figure out how many bytes were serialised as part of the writer.write call by taking
		// the "before" number and subtracting the "after" number. I suppose it would be nice if this
		// were returned somehow, but it's not too onerous for the client.
		uint64_t remaining = writer.bytesRemaining();
		// the result is either "OK" or "INCOMPLETE"; we're going to track completion
		// via the bytes-remaining
		std::ignore = writer.write(buf->write_pos(), buf->writeable());

		// Calculate how many bytes were encoded to the buffer
		uint64_t encoded = remaining - writer.bytesRemaining();

		// Update the buffer's internal pointers so things work-out
		buf->set_appended(encoded);

		// We were probably able to write 100% of the available bytes to the network buffer, given
		// how small the message actually is; this is unlikely to return a value smaller than
		// buf->readable
		std::expected<ssize_t,int> sz_res = io::write(fd, buf->read_off(), buf->readable());
		if (!sz_res) {
			std::print("ERROR: failed during write: {}", strerror(sz_res.error()));
			return EXIT_FAILURE;
		}
		// Again, update the buffer's internal pointers. We don't need to "compact" the buffer static_cast
		// it's one of those magic circular buffer gadgets.
		buf->set_consumed(sz_res.value());

	} while (writer.bytesRemaining() > 0);

	std::print(" INFO: wrote {} bytes to the remote kdb+ process\n", ipc_len);

	return EXIT_SUCCESS;
}
