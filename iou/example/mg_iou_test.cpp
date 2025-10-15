/*
This file is part of the Mg IO Uring Test Library (hereinafter "The Library").

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

#include <stdlib.h>     // EXIT_FAILURE
#include <string.h>     // memcpy, strlen
#include <stdint.h>     // uint16_t
#include <netinet/in.h> // sockaddr_in
#include <errno.h>

#include <liburing.h>

#include <print>
#include <expected>
#include <memory>       // unique_ptr, make_unique

#include "MgIoDefs.h"
#include "MgCircularBuffer.h"

using namespace mg7x;

static
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

	const char *msg = "michaelg:\3\0";

	std::expected<ssize_t,int> sz_res = io::write(fd, msg, 1 + strlen(msg));
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

	std::print(" INFO: connected on port {} as FD {}\n", port, fd);

	return fd;
}

static
std::unique_ptr<int8_t[]> from_hex(const char *hex, int64_t *len)
{
	std::string hex_str{hex};
	if (0 != (hex_str.length() % 2)) {
		exit(EXIT_FAILURE);
	}

	const auto pos = hex_str.find("0x", 0, 2);
	if (std::string::npos != pos) {
		hex_str = std::string(hex_str.substr(2));
	}

	*len = hex_str.length() / 2;
	auto dst = std::make_unique<int8_t[]>(*len);
	for (size_t i = 0 ; i < hex_str.length() ; i += 2) {
		dst[i/2] = (int8_t)std::stoi(hex_str.substr(i, 2), nullptr, 16);
	}
	return dst;
}

/*
https://lwn.net/Articles/776703/

"There are, however, some more features worthy of note in this interface. One of those is the ability to map a program's
I/O buffers into the kernel. This mapping normally happens with each I/O operation so that data can be copied into or
out of the buffers; the buffers are unmapped when the operation completes. If the buffers will be used many times over
the course of the program's execution, it is far more efficient to map them once and leave them in place."

Note this isn't really borne out by the man-page, which simply says:

"Registered  buffers is an optimization that is useful in conjunction with O_DIRECT reads and writes, where it maps
the specified range into the kernel once when the buffer is registered rather than doing a map and unmap for  each
IO every time IO is performed to that region. Additionally, it also avoids manipulating the page reference counts
for each IO."

The LWM article continues with more interesting details, do go back and re-read the Advanced Features section, e.g.:

"The buffers will remain mapped for as long as the initial file descriptor remains open, unless the program explicitly
unmaps them with IORING_UNREGISTER_BUFFERS. Mapping buffers in this way is essentially locking memory into RAM, so the
usual resource limit that applies to mlock() applies here as well."

"There is also an IORING_REGISTER_FILES operation that can be used to optimize situations where many operations will be
performed on the same file(s)."
*/

int main(int argc, char *arvg[])
{
	// open a connection to a kdb+ instance and send handshake bytes
	// write some data to the buffer so it's in a wrapped-around disposition
	// use io_uring to write the buffer to the fd
	// check the client receives coherent data

	// connect_and_login (being a test-function) simply exits if it can't connect
	int fd0 = connect_and_login(30095);
	int fd1 = connect_and_login(30094);

	std::array<int,2> fds{fd0, fd1};

	std::expected<CircBufPtr,std::string> circ_buf_res = init_circ_buffer(4096);
	if (!circ_buf_res) {
		std::print("ERROR: while creating circular buffer: {}", circ_buf_res.error());
		return EXIT_FAILURE;
	}
	std::unique_ptr<CircularBuffer> buf{};
	std::swap(buf, circ_buf_res.value());

	buf->set_appended(4090);
	buf->set_consumed(4090);

	int64_t len = 0;
	// -8!"0N!\"Hello, world!\""
	std::unique_ptr<int8_t[]> ipc = from_hex("0x01000000200000000a0012000000304e212248656c6c6f2c20776f726c642122", &len);

	memcpy(buf->write_ptr(), ipc.get(), static_cast<size_t>(len));

	buf->set_appended(len);

	struct io_uring ring = {0};

	int err = io_uring_queue_init(32, &ring, 0);
	if (err != 0) {
		std::print("ERROR: failed in io_uring_setup: {}\n", strerror(std::abs(err)));
		return EXIT_FAILURE;
	}
	std::print("DEBUG: ring FD is {}\n", ring.ring_fd);

	struct iovec meta = { .iov_base = buf->map_base(), .iov_len = buf->magic_len() };

	err = io_uring_register_buffers(&ring, &meta, 1);
	if (0 != err) {
		std::print("ERROR: failed in io_uring_register: {}\n", strerror(std::abs(err)));
		return EXIT_FAILURE;
	}

	// Fill in the parameters required for the read or write operation
	for (unsigned i = 0 ; i < fds.size() ; i++) {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		std::print("DEBUG: constructing write-task for FD {}, len {}, off {}, buf_idx 0\n", fds[i], buf->readable(), buf->pos());
		// note that we use read_ptr as the *buf argument, and not map_base(); setting a non-zero offset
		// value causes an ESPIPE "Illegal Seek" error.
		io_uring_prep_write_fixed(sqe, fds[i], buf->read_ptr(), buf->readable(), 0, 0);
		io_uring_sqe_set_data(sqe, nullptr);
	}
	std::print("DEBUG: submitting two write-commands to the SQE\n");

	io_uring_submit(&ring);

	struct io_uring_cqe *cqe;

	std::print("DEBUG: awaiting first completion event\n");

	err = io_uring_wait_cqe(&ring, &cqe);
	if (0 != err) {
		std::print("ERROR: during io_uring_wait_cqe: {}\n", strerror(std::abs(err)));
		return EXIT_FAILURE;
	}

	std::print("DEBUG: have completion event\n");
	if (cqe->res < 0) {
		std::print("ERROR: {}\n", strerror(std::abs(cqe->res)));
	}

	return EXIT_SUCCESS;
}

