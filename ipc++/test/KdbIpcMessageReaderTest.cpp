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

#include "KdbType.h"
#include <memory>
#include <sys/stat.h>  // stat
#include <fcntl.h>     // open
#include <string.h>    // strerror
#include <errno.h>     // errno
#include <unistd.h>    // read

#include <gtest/gtest.h>

#include "../src/KdbType.cpp"

using namespace mg7x;
using namespace std;

namespace mg7x::test {

std::unique_ptr<int8_t[]> fromHex(const char *hex, uint64_t *len)
{
	std::string hex_str{hex};
	if (0 != (hex_str.length() % 2))
		throw std::runtime_error("Non-even string length");

	const auto pos = hex_str.find("0x", 0, 2);
	if (string::npos != pos)
		hex_str = std::string(hex_str.substr(2));

	*len = hex_str.length() / 2;
	auto dst = make_unique<int8_t[]>(*len);
	for (size_t i = 0 ; i < hex_str.length() ; i += 2) {
		dst[i/2] = (int8_t)std::stoi(hex_str.substr(i, 2), nullptr, 16);
	}
	return dst;
}

void expectRqdFromHex(const uint64_t exp, const char *hex)
{
	uint64_t len = 0;
	std::unique_ptr<int8_t[]> src = fromHex(hex, &len);

	ReadBuf buf{src.get(), len};

	std::ignore = buf.read<int32_t>();
	int32_t ipc_len = buf.read<int32_t>();
	EXPECT_EQ(ipc_len, len) << "Incoherent test-IPC data: embedded length does not match array length";
	int32_t msg_len = buf.read<int32_t>();

	KdbIpcDecompressor kid{static_cast<uint64_t>(ipc_len), static_cast<uint64_t>(msg_len), std::make_unique<int8_t[]>(msg_len)};
	uint64_t rqd = kid.blockReadSz(buf);
	EXPECT_EQ(exp, rqd) << "Failed for input " << hex;
}

TEST(KdbTypeTest, TestKdbIpcMessageReaderReadMsg)
{
	// -8!"Hello, world!"
	const char *hex = "0x010000001b0000000a000d00000048656c6c6f2c20776f726c6421";
	uint64_t len;
	std::unique_ptr<int8_t[]> src = fromHex(hex, &len);

	KdbIpcMessageReader reader{};

	ReadMsgResult result{};
	bool complete = reader.readMsg(src.get(), len, result);
	EXPECT_TRUE(complete);
	EXPECT_EQ(ReadResult::RD_OK, result.result);
	EXPECT_EQ(len, result.bytes_consumed);
	EXPECT_EQ(len, reader.getInputBytesConsumed());
	EXPECT_EQ(len, reader.getMsgBytesDeserialized() + SZ_MSG_HDR);
	EXPECT_EQ(len, reader.getIpcLength());

	KdbCharVector *vec = dynamic_cast<KdbCharVector*>(result.message.get());
	EXPECT_EQ(std::string_view{"Hello, world!"}, vec->getString());
}

TEST(KdbIpcMessageReaderTest, TestKdbIpcDecompressorBlockReadSz)
{
	// Function builds the _compressed_ IPC for small messages because KDB refuses to do so:
	// q)z:{[M] -9!0N!0x01000100,(reverse 0x0 vs 6h$8 + 4 + 1 + count i),(reverse 0x0 vs 6h$8 + count i),0x00,i:8_-8!M }
	// q)z 1b
	// 0x010001000f0000000a00000000ff01
	// 1b

	// While these have zero as the key-byte, we can test the logic around the end-of-message truncation
	expectRqdFromHex( 3, "0x010001000f0000000a000000" "00" "ff00");             // z 0b
	expectRqdFromHex( 4, "0x01000100100000000b000000" "00" "f53400");           // z `4
	expectRqdFromHex( 5, "0x01000100110000000c000000" "00" "f5343500");         // z `45
	expectRqdFromHex( 6, "0x01000100120000000d000000" "00" "f534353600");       // z `456
	expectRqdFromHex( 7, "0x01000100130000000e000000" "00" "f53435363700");     // z `4567
	expectRqdFromHex( 8, "0x01000100140000000f000000" "00" "0a000100000061");   // z enlist"a"
	expectRqdFromHex( 9, "0x010001001500000010000000" "00" "0a00020000006162"); // z "ab"
	// raze -2# (sums 0 12 1 8 1 8 1 8 1 8 1 8 1 9 1 11 1 11 1 12 1 10 1 11 1 11 1 11 1 10 1 11 1 11 1 11 1 10 1) cut -18!10000 #.Q.a
	// this final block has a "natural" requirement for 13 bytes, but because the
	// IPC length is shorter, is actually 12

	// extract the ipc-len and msg-len from the compressed message so we can spoof that we're reading the final block
	// q)0x0 sv/:reverse each ((sums 0 4 4 4) cut ipc:-18!10000 #.Q.a) 1 2
	// 221 10014i
	// expectRqdFromHex(12,  "0x74" "5103ff72737403ff72077e");
	// expectRqdFromHex(1+8, "0xf0" "0102030405060708");   // 1111.0000 thus 1,{2,3},{4,5},{6,7},{8,9}
	// expectRqdFromHex(1+9, "0xf0" "010203040506070809"); // 1111.0000 thus 1,{2,3},{4,5},{6,7},{8,9},10
}

void rdIpc(const char *path, std::shared_ptr<int8_t> & ptr, ssize_t *sz)
{
	struct stat nfo = {0};
	int err = stat(path, &nfo);
	if (-1 == err) {
		FAIL() << "Failed to stat file " << path;
	}
	void *mem = malloc(nfo.st_size);
	if (nullptr == mem) {
		FAIL() << "Failed to allocate " << nfo.st_size << " bytes for " << path;
	}

	int fd = open(path, O_RDONLY);
	if (-1 == fd) {
		FAIL() << "Failed to open file " << path << ": " << strerror(errno);
		goto err_open;
	}

	*sz = read(fd, mem, nfo.st_size);
	if (-1 == *sz) {
		FAIL() << "Failed to read file " << path << ": " << strerror(errno);
		goto err_read;
	}

	ptr.reset(static_cast<int8_t*>(mem));

	return;

err_read:
err_open:
	free(mem);
}

TEST(KdbIpcMessageReaderTest, TestRead10kQa)
{
	std::shared_ptr<int8_t> ipc{};
	ssize_t len = -1;
	// Assumes you run the tests from the build/test directory
	rdIpc("../../test/tenKQa.zipc", ipc, &len);

	if (-1 == len)
		FAIL() << "Failed to read file tenKQa.zipc";

	ReadBuf buf{ipc.get(), static_cast<uint64_t>(len)};
	std::ignore = buf.read<int32_t>();          // Read the endian byte and next three
	uint64_t ipc_len = buf.read<uint32_t>();    // Read the total byte count
	uint64_t msg_len = buf.read<uint32_t>();    // Read the expanded message size

	KdbIpcDecompressor kid{ipc_len, msg_len, std::make_unique<int8_t[]>(msg_len)};
	uint64_t neu = kid.uncompress(buf);

	EXPECT_EQ(msg_len, neu + SZ_MSG_HDR) << "Reported wrong number of newly-decompressed bytes";
}

TEST(KdbIpcMessageReaderTest, TestRead10kQaViaMsgReader)
{
	std::shared_ptr<int8_t> ipc{};
	ssize_t len = -1;
	// Assumes you run the tests from the build/test directory
	rdIpc("../../test/tenKQa.zipc", ipc, &len);

	if (-1 == len)
		FAIL() << "Failed to read file tenKQa.zipc";

	KdbIpcMessageReader rdr{};
	
	ReadMsgResult result = {};
	bool complete = rdr.readMsg(ipc.get(), static_cast<uint64_t>(len), result);
	EXPECT_TRUE(complete);
	EXPECT_EQ(ReadResult::RD_OK, result.result);

	EXPECT_EQ(len, result.bytes_consumed);
	EXPECT_EQ(len, rdr.getInputBytesConsumed());
	// Read the uncompressed message length from the stream rather than hardcoding it. Use a throwaway
	// ReadBuf for this, skipping the first  eight bytes.
	ReadBuf tmp{ipc.get(), static_cast<uint64_t>(len)};
	std::ignore = tmp.read<int64_t>();
	EXPECT_EQ(tmp.read<int32_t>() - SZ_MSG_HDR, rdr.getMsgBytesDeserialized());

	if (!!result.message) {
		KdbCharVector *vec = reinterpret_cast<KdbCharVector*>(result.message.get());
		EXPECT_EQ(10000, vec->count());

		for (uint32_t i = 0 ; i < vec->count() ; i++) {
			EXPECT_EQ('a' + i % 26, vec->getChar(i));
		}
	}
	else {
		FAIL() << "Message was null";
	}
	
}

} // end namespace mg7x::test

int main(int argc, char *argv[]) {
	using namespace mg7x::test;
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
