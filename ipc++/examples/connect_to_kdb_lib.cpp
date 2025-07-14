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

#include <netinet/in.h> // sockaddr_in
#include <stdlib.h> // EXIT_FAILURE
#include <string.h> // strerror
#include <stdint.h> // uint16_t

#include "KdbType.h"
#include "KdbIoDefs.h"

#include <memory> // unique_ptr
#include <print>

using namespace ::mg7x;

int connect_and_login(const uint16_t port)
{
	// being explicit in this first instance about the type of the returned value and the full
	// path to the imported mg7x::io functions. In fugure, I'll just use io::...
	std::expected<int,int> io_res = ::mg7x::io::socket(AF_INET, SOCK_STREAM, 0);
	if (!io_res) {
		std::print("ERROR: failed to create socket: {}", strerror(io_res.error()));
		exit(EXIT_FAILURE);
	}

	int fd = io_res.value();

	struct sockaddr_in dest{0};
	// You should probably use getaddrinfo but since this is a localhost, IPV4 connection
	// we can set the port-value directly and use 127.0.0.1 without the lookup gubbins.
	io::setup_sockaddr_in_ipv4_localhost_port(dest, port);

	io_res = io::connect(fd, (struct sockaddr*)&dest, sizeof(dest));
	if (!io_res) {
		std::print("ERROR: failed to connect to {}: {}\n", port, strerror(io_res.error()));
		exit(EXIT_FAILURE);
	}

	std::unique_ptr<char> msg{};

	size_t msg_len = KdbUtil::writeLoginMsg("michaelg", "", msg);

	std::expected<ssize_t,int> sz_res = io::write(fd, msg.get(), msg_len);
	if (!sz_res) {
		std::print("ERROR: while writing login-bytes: {}\n", strerror(sz_res.error()));
		exit(EXIT_FAILURE);
	}

	int8_t res_buf[1];
	sz_res = io::read(fd, res_buf, 1);
	if (!sz_res) {
		std::print("ERROR: while reading login response: {}\n", strerror(sz_res.error()));
		exit(EXIT_FAILURE);
	}
	if (1 != sz_res.value()) {
		// would have to be zero if it's not 1
		std::print("ERROR: expected to read 1 byte in login-response; read instead {}\n", sz_res.value());
	}

	std::print(" INFO: connected on port {}\n", port);

	return fd;
}
