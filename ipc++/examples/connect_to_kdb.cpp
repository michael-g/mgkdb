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

/*
	This example shows how to use the library to connect and write some data to a
	local KDB instance on Linux using blocking comms.
 */
#include <sys/socket.h> // socket, connect
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons, htonl
#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h> // read
#include <string.h> // strerror
#include <stdint.h> // uint16_t
#include <errno.h> // errno

#include <memory> // unique_ptr
#include <algorithm> // copy
#include <vector>
#include <print>

#include "KdbType.h"

using namespace ::mg7x;

int main(void)
{
	int err = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == err) {
		std::print("ERROR: failed to create socket: {}", strerror(errno));
		exit(EXIT_FAILURE);
	}

	int fd = err;
	uint16_t port = 30098;

	// Probably ... don't do this. Use `getaddrinfo` like a good chap with a safe pair of hands
	struct sockaddr_in dest{0};
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl(0x7f000001);
	dest.sin_port = htons(static_cast<uint16_t>(port));

	err = connect(fd, (struct sockaddr*)&dest, sizeof(dest));
	if (-1 == err) {
		std::print("ERROR: failed to connect to {}: {}\n", port, strerror(errno));
		exit(EXIT_FAILURE);
	}

	std::unique_ptr<char> msg{};

	size_t msg_len = KdbUtil::writeLoginMsg("michaelg", "", msg);

	ssize_t wrz = write(fd, msg.get(), msg_len);
	if (wrz < 0) {
		std::print("ERROR: while writing login-bytes: {}\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	int8_t res_buf[1];
	ssize_t rdz = read(fd, res_buf, 1);
	if (-1 == rdz) {
		std::print("ERROR: while reading login response: {}\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	std::print(" INFO: connected on port {}\n", port);

	KdbCharVector cmd{"{0N!(`Hello;,;\"World\";!)}[]"};
	KdbIpcMessageWriter writer{KdbMsgType::SYNC, cmd};

	std::vector<int8_t> buf(writer.bytesRemaining());
	buf.resize(0);

	WriteResult wrr = writer.write(buf.data(), writer.bytesRemaining());
	if (WriteResult::WR_OK != wrr) {
		std::print("ERROR: failed to encode message: {}\n", wrr);
		exit(EXIT_FAILURE);
	}

	wrz = write(fd, buf.data(), buf.capacity());
	if (-1 == wrz) {
		std::print("ERROR: failed to write message to KDB: {}\n", strerror(errno));
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
		rdz = read(fd, rd_buf.data() + data_len, rd_buf.capacity() - data_len);
		if (-1 == rdz) {
			std::print("ERROR: failed during read: {}\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		data_len += rdz;

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

	return 0;
}
