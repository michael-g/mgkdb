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

#include <algorithm> // copy
#include <vector>
#include <print>

int connect_and_login(const uint16_t port);

using namespace ::mg7x;

int main(void)
{
	int fd = connect_and_login(static_cast<uint16_t>(30098));

	KdbCharVector cmd{"{0N!(`Hello;,;\"World\";!)}[]"};
	KdbIpcMessageWriter writer{KdbMsgType::SYNC, cmd};

	std::vector<int8_t> buf(writer.bytesRemaining());
	buf.resize(0);

	WriteResult wrr = writer.write(buf.data(), writer.bytesRemaining());
	if (WriteResult::WR_OK != wrr) {
		std::print("ERROR: failed to encode message: {}\n", wrr);
		exit(EXIT_FAILURE);
	}

	std::expected<ssize_t,int> sz_res = io::write(fd, buf.data(), buf.capacity());
	if (!sz_res) {
		std::print("ERROR: failed to write message to KDB: {}\n", strerror(!sz_res.error()));
		exit(EXIT_FAILURE);
	}

	// obviously, at this point we should check that all the bytes were in fact written to KDB
	KdbIpcMessageReader reader{};
	ReadMsgResult rmr{};

	uint64_t data_len = 0;

	// Use a paltry buffer to illustrate how we account for bytes left in the buffer after each
	// message-parsing pass
	std::vector<int8_t> rd_buf(24);
	int count = 0;
	do {
		sz_res = io::read(fd, rd_buf.data() + data_len, rd_buf.capacity() - data_len);
		if (!sz_res) {
			std::print("ERROR: failed during read: {}\n", strerror(sz_res.error()));
			exit(EXIT_FAILURE);
		}
		data_len += sz_res.value();

		uint64_t off_0 = reader.getInputBytesConsumed();
		bool complete = reader.readMsg(rd_buf.data(), data_len, rmr);
		if (complete) {
			break;
		}
		uint64_t off_1 = reader.getInputBytesConsumed();
		uint64_t used = off_1 - off_0;
		if (0 == used) {
			continue;
		}

		if (used < data_len) {
			std::copy(rd_buf.data() + used, rd_buf.data() + data_len, rd_buf.data());
			data_len = data_len - used;
		}
		else if (used > data_len) {
			std::print("ERROR: coding-error: used > data_len!\n");
			exit(EXIT_FAILURE);
		}
		else {
			data_len = 0;
		}
	} while (count++ < 10);

	std::print(" INFO: received response: {}\n", *(rmr.message.get()));

	return EXIT_SUCCESS;
}
