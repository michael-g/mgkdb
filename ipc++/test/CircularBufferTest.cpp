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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstddef> // size_t
#include <sys/types.h> // off_t

#include "KdbIoMockDefs.h"
#include "../src/CircularBuffer.cpp"

namespace mg7x::test {

static void* to_vp(uint64_t base, size_t off = 0)
{
	return reinterpret_cast<int8_t*>(base) + off;
}

TEST(CircularBufferTest, TestInitCircBuffer1)
{
	const size_t BUF_LEN = 4096;

	const int mfd = 4;
	const uint64_t base_addr = 0x7f001000L;
	void *p_base_addr = to_vp(base_addr); // (void*)base_addr;

	MMapMock mmap_mock;
	MemFDCreateMock memfd_create_mock;
	FTruncateMock ftrunc_mock;
	MUnMapMock munmap_mock;
	CloseMock close_mock;

	mmap_call = &mmap_mock;
	memfd_create_call = &memfd_create_mock;
	ftruncate_call = &ftrunc_mock;
	munmap_call = &munmap_mock;
	close_call = &close_mock;

	testing::InSequence marker;

	// We expect there to have been three mmap calls:
	// 1. the first call allocates twice the buf-len argument with PRIVATE/ANON backing
	// 2. the second, which is RW and SHARED/FIXED, and
	// 3. the third, which is also RW and SHARED/FIXED but at the contiguous second segment address
	EXPECT_CALL(mmap_mock, call(nullptr, 2 * BUF_LEN, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))
		.WillOnce(testing::Return(p_base_addr));

	EXPECT_CALL(memfd_create_mock, call(testing::_, 0))
		.WillOnce(testing::Return(mfd));

	EXPECT_CALL(ftrunc_mock, call(mfd, BUF_LEN))
		.WillOnce(testing::Return(0));

	EXPECT_CALL(mmap_mock, call(p_base_addr, BUF_LEN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, 0))
		.WillOnce(testing::Return(p_base_addr));

	EXPECT_CALL(mmap_mock, call(to_vp(base_addr, BUF_LEN), BUF_LEN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, 0))
		.WillOnce(testing::Return(to_vp(base_addr, BUF_LEN)));

	auto maybe_buf = mg7x::init_circ_buffer(BUF_LEN);

	// Here we're asserting that the CircularBuffer destructor, and avoiding segfaults when it
	// calls these functions, which we have to stub-out! We may as well assert that it's closing
	// the resources properly.
	EXPECT_CALL(munmap_mock, call(to_vp(base_addr, BUF_LEN), BUF_LEN));
	EXPECT_CALL(munmap_mock, call(p_base_addr, BUF_LEN));
	EXPECT_CALL(close_mock, call(mfd));

}

}; // end namespace mg7x::test

int main(int argc, char **argv)
{
	using namespace mg7x::test;
	testing::InitGoogleMock(&argc, argv);
	// testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
