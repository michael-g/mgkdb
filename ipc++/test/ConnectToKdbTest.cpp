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

#include <gtest/gtest.h>
#include <memory>

#include "KdbType.h"

#include "../src/KdbType.cpp"

using namespace mg7x;

namespace mg7x::test {

static constexpr short MG_PORT = 30098;

static int sk_open(int port);
static void sk_write(int fd, const void *buf, size_t count, ssize_t & wrt);
static void sk_read(int fd, std::unique_ptr<int8_t> & src, ssize_t & red);
static void sk_msg_read(int fd, std::unique_ptr<int8_t> & ptr, ssize_t & red);
static int sk_close(int fd);

static void hopen(int & fd, const char *usr)
{
	fd = sk_open(MG_PORT);
	if (fd <= 0)
		return;

	std::unique_ptr<char> msg{};

	size_t msg_len = KdbUtil::writeLoginMsg(usr, "", msg);

	EXPECT_NE(0, msg_len) << "ERROR: failed to allocate memory for the login bytes";
	if (0 == msg_len) {
		fd = sk_close(fd);
		return;
	}

	ssize_t wr_len, rd_len;
	sk_write(fd, msg.get(), msg_len, wr_len);

	EXPECT_EQ(msg_len, wr_len) << "ERROR: failed to write msg_len bytes to socket";
	if (msg_len != wr_len) {
		fd = sk_close(fd);
		return;
	}

	std::unique_ptr<int8_t> src{};
	sk_read(fd, src, rd_len);

	EXPECT_EQ(1, rd_len);
	if (1 != rd_len) {
		fd = sk_close(fd);
		return;
	}
}

static void open_and_write(char *mem, size_t len, const char *usr)
{
	int fd;
	hopen(fd, usr);
	if (-1 == fd) 
		FAIL() << "Could not open a connection to q";
	
	ssize_t err;
	sk_write(fd, mem, len, err);
	EXPECT_EQ(len, err);
	fd = sk_close(fd);
}

template<typename T>
static std::unique_ptr<T> open_write_and_read(const char *usr, char *src, size_t len)
{
	int fd;
	hopen(fd, usr);
	if (-1 == fd) { 
		std::cerr << "Could not open a connection to q at line " << (__LINE__ - 2) << std::endl;
		return {};
	}
	
	ssize_t err;
	sk_write(fd, src, len, err);
	EXPECT_EQ(len, err);

	ssize_t red;
	std::unique_ptr<int8_t> ipc{};
	sk_msg_read(fd, ipc, red);
	if (-1 == red) {
		std::cerr << "Failure in sk_msg_read at line " << (__LINE__ - 2) << std::endl;
		return {};
	} 

	ReadMsgResult rmr;
	KdbIpcMessageReader rdr{};
	if (!rdr.readMsg(ipc.get(), len, rmr)) {
		std::cerr << "Message reported incomplete by KdbIpcMessageReader at line " << (__LINE__ - 1) << std::endl;
		return {};
	}

	KdbBase *obj = rmr.message.release();

	fd = sk_close(fd);

	return std::unique_ptr<T>{reinterpret_cast<T*>(obj)};
}

TEST(KdbConnectedTest, TestKdbConnect)
{
	int fd;
	hopen(fd, "Basic connection test");
	fd = sk_close(fd);
}

TEST(KdbConnectedTest, TestSimpleWrite)
{
	auto unq = std::make_unique<KdbCharVector>(std::string_view{"-1\"Hello, world!\""});
	auto shr = std::shared_ptr<KdbCharVector>(unq.release());
	KdbIpcMessageWriter writer{KdbMsgType::ASYNC, shr};
	size_t rqd = writer.bytesRemaining();

	auto mem = std::make_unique<char[]>(rqd);
	WriteResult wr = writer.write(mem.get(), rqd);
	EXPECT_EQ(WriteResult::WR_OK, wr);
	// q)-8!"-1\"Hello, world!\""
	// 0x010000001f0000000a00110000002d312248656c6c6f2c20776f726c642122
	open_and_write(mem.get(), rqd, "Say 'hello, world!'");
}

TEST(KdbConnectedTest, TestTableWriter)
{
	const std::vector<std::string_view> cols{ "time", "sym", "price", "size"};
	std::unique_ptr<KdbTable> tbl = std::make_unique<KdbTable>("tsfj", cols);
	if (!tbl)
		FAIL() << "Failed in KdbTable::ctor";
	
}

} // end namespace mg7x::test

// lifted from https://www.linuxhowtos.org/C_C++/socket.htm
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


namespace mg7x::test {

static int sk_close(int fd)
{
	close(fd);
	return -1;
}

static void sk_read(int fd, std::unique_ptr<int8_t> & src, ssize_t & red)
{
	static constexpr size_t cap = 64 * 1024;
	void *dst = malloc(cap);
	if (nullptr == dst) {
		FAIL() << "ERROR: failed to allocate " << cap << " bytes";
	}

	for (int i = 0 ; i < 3 ; i++) {
		red = read(fd, dst, cap);
		if (red < 0) {
			if (EINTR == red)
				continue;
			FAIL() << "ERROR: failed in 'read' on FD " << fd << "; error was: " << strerror(errno);
		}
		src.reset(static_cast<int8_t*>(dst));
		return;
	}
	
}

static void sk_msg_read(int fd, std::unique_ptr<int8_t> & ptr, ssize_t & red)
{
	char hdr[SZ_MSG_HDR] = {0};
	size_t msg_len;
	size_t acc = 0;
	do {
		ssize_t err = read(fd, hdr, SZ_MSG_HDR);
		if (-1 == err) {
			if (EAGAIN == errno)
				continue;
			red = err;
			FAIL() << "while reading message header: " << strerror(errno);
			return;
		}
		acc += err;
	} while (acc < SZ_MSG_HDR);

	msg_len = reinterpret_cast<int32_t*>(hdr)[1];

	int8_t *msg = new int8_t[msg_len];
	memcpy(msg, hdr, SZ_MSG_HDR);
	
	do {
		ssize_t err = read(fd, msg + acc, msg_len - acc);
		if (-1 == err) {
			if (EAGAIN == err)
				continue;
			delete[] msg;
			red = err;
			FAIL() << "while reading message body: " << strerror(errno);
			return;
		}
		acc += err;
	} while (acc < msg_len);

	ptr.reset(msg);
	red = msg_len;
}

static void sk_write(int fd, const void *buf, size_t count, ssize_t & wrt)
{
	for (int i = 0 ; i < 3 ; i++) {
		wrt = write(fd, buf, count);
		if (wrt < 0) {
			if (EINTR == wrt)
				continue;
			FAIL() << "ERROR: failed to write " << count << " bytes on FD " << fd << "; error is " << strerror(errno);
		}
		return;
	}
}

static int sk_open(int port)
{
		int sockfd, n;
		struct sockaddr_in serv_addr;
		struct hostent *server;

		char buffer[256];
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		EXPECT_LT(0 , sockfd) << "Error opening socket"; 
		server = gethostbyname("localhost");
		EXPECT_NE(nullptr, server) << "ERROR, no such host";

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		serv_addr.sin_port = htons(port);
		int err = connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
		EXPECT_EQ(0, err) << "ERROR connecting";
		return sockfd;
}
}


int main(int argc, char **argv)
{
	using namespace mg7x::test;
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
