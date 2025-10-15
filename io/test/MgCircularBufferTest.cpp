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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstddef>       // size_t
#include <sys/types.h>   // off_t

#include "MgIoMockDefs.h"
#include "../src/MgCircularBuffer.cpp"

#include <sys/mman.h>

namespace mg7x::test {

static void* to_vp(uint64_t base, size_t off = 0)
{
	return reinterpret_cast<int8_t*>(base) + off;
}

TEST(CircularBufferTest, TestInitCircBufferCompletesAsExpected)
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

	// Here we're asserting that the CircularBuffer destructor, and avoiding segfaults when it
	// calls these functions, which we have to stub-out! We may as well assert that it's closing
	// the resources properly. We can assert order here since this is given by CicularBuffer, not
	// the two RAII gadgets.
	EXPECT_CALL(munmap_mock, call(to_vp(base_addr, BUF_LEN), BUF_LEN));
	EXPECT_CALL(munmap_mock, call(p_base_addr, BUF_LEN));
	EXPECT_CALL(close_mock, call(mfd));

	auto maybe_buf = mg7x::init_circ_buffer(BUF_LEN);

}

TEST(CircularBufferTest, TestInitCircBufferFailsIfMMapCallFails)
{
	MMapMock mmap_mock;
	MUnMapMock munmap_mock;
	CloseMock close_mock;

	mmap_call = &mmap_mock;
	munmap_call = &munmap_mock;
	close_call = &close_mock;

	const size_t BUF_LEN = 4096;

	testing::InSequence marker;

	EXPECT_CALL(mmap_mock, call(nullptr, 2 * BUF_LEN, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))
		.WillOnce(testing::Return(std::unexpected<int>{ENOMEM}));

	// Do some checking to ensure the ResourcCleanup RAII helper hasn't been involved, and
	// that its destructor hasn't called the munmap/close proxies
	EXPECT_CALL(munmap_mock, call(testing::_, BUF_LEN)).Times(0);
	EXPECT_CALL(close_mock, call(testing::_)).Times(0);

	auto maybe_buf = mg7x::init_circ_buffer(BUF_LEN);
	// Expect TRUE, but coerce to bool using operator!
	EXPECT_TRUE(!maybe_buf);

	// Assert we returned the first error messge, not worrying about the exact text
	// for ENOMEM
	EXPECT_THAT(maybe_buf.error().c_str(), testing::StartsWith("Failed to reserve private address range: "));

}

TEST(CircularBufferTest, TestInitCircBufferFirstRAIIDestructorCalled)
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

	EXPECT_CALL(mmap_mock, call(nullptr, 2 * BUF_LEN, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))
		.WillOnce(testing::Return(p_base_addr));

	EXPECT_CALL(memfd_create_mock, call("mg_circ_buf", 0))
		.WillOnce(testing::Return(std::unexpected<int>{ENOMEM}));

	// Although these are "in sequence" (and we can't really predict the destructor order ...
	// I don't think), since the second of the two asserts that the call is not made at all,
	// we should be OK
	EXPECT_CALL(munmap_mock, call(p_base_addr, BUF_LEN * 2)).Times(1);
	EXPECT_CALL(close_mock, call(testing::_)).Times(0);

	auto maybe_buf = mg7x::init_circ_buffer(BUF_LEN);

	// Assert the error branch was taken, and the string was returned
	EXPECT_TRUE(!maybe_buf);

	// Assert the stem of the error-message matches
	EXPECT_THAT(maybe_buf.error().c_str(), testing::StartsWith("Failed in memfd_create: "));
}

TEST(CircularBufferTest, TestInitCircBufferBothRAIIDestructorsCalled)
{
	const size_t BUF_LEN = 4096;

	const int MFD = 4;
	const uint64_t BASE_ADDR = 0x7f001000L;
	void *p_base_addr = to_vp(BASE_ADDR); // (void*)base_addr;

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

	// Here we're asserting that the CircularBuffer destructor, and avoiding segfaults when it
	// calls these functions, which we have to stub-out! We may as well assert that it's closing
	// the resources properly.
	EXPECT_CALL(munmap_mock, call(p_base_addr, 2 * BUF_LEN)).Times(1);
	EXPECT_CALL(close_mock, call(MFD)).Times(1);

	testing::InSequence marker;

	// We're going to test the scenario where for some idiopathic reason the final mmap doesn't
	// return the expected base-address and the sanity check returns an error message
	EXPECT_CALL(mmap_mock, call(nullptr, 2 * BUF_LEN, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))
		.WillOnce(testing::Return(p_base_addr));

	EXPECT_CALL(memfd_create_mock, call(testing::_, 0))
		.WillOnce(testing::Return(MFD));

	EXPECT_CALL(ftrunc_mock, call(MFD, BUF_LEN))
		.WillOnce(testing::Return(0));

	EXPECT_CALL(mmap_mock, call(p_base_addr, BUF_LEN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, MFD, 0))
		.WillOnce(testing::Return(p_base_addr));

	// cause the error by returning the wrong base address
	EXPECT_CALL(mmap_mock, call(to_vp(BASE_ADDR, BUF_LEN), BUF_LEN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, MFD, 0))
		.WillOnce(testing::Return(to_vp(BASE_ADDR, BUF_LEN / 2)));

	auto maybe_buf = mg7x::init_circ_buffer(BUF_LEN);

	EXPECT_TRUE(!maybe_buf);

	// Assert the stem of the error-message matches, actual value is
	// "expected MAP_FIXED addr and response to be the same, have addr=0x7f001000 and result=0x7f001800"
	EXPECT_THAT(maybe_buf.error().c_str(), testing::MatchesRegex("expected MAP_FIXED addr and response to be the same, have addr=0x[0-9a-f]+ and result=0x[0-9a-f]+"));
}

}; // end namespace mg7x::test

int main(int argc, char **argv)
{
	using namespace mg7x::test;
	testing::InitGoogleMock(&argc, argv);
	// testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
