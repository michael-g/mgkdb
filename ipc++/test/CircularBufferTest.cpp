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

#include "CircularBuffer.h"
#include <memory> // std::unique_ptr
#include <utility> // std::swap
#include <print> // TODO REMOVEME

#include "../src/CircularBuffer.cpp"

namespace mg7x::test {

TEST(CircularBufferTest, TestBasicCircBufferOps)
{
	const size_t BUF_LEN = 4096;
	auto maybe_buf = mg7x::init_circ_buffer(BUF_LEN);
	if (!maybe_buf) {
		FAIL() << "in mg7x::init_circ_buffer: " << maybe_buf.error();
	}
	std::unique_ptr<CircularBuffer> buf{};
	std::swap(buf, maybe_buf.value());

	std::print("Before assertions\n");

	EXPECT_TRUE(buf->is_mapped());
	EXPECT_EQ(0, buf->pos());
	EXPECT_EQ(0, buf->readable());
	EXPECT_EQ(BUF_LEN, buf->writeable());

	const size_t WRITE_LEN = 32;

	int8_t *pos = static_cast<int8_t*>(buf->map_base()) + (BUF_LEN - WRITE_LEN / 2);

	for (unsigned i = 0 ; i < WRITE_LEN ; i++) {
		pos[i] = i+1;
	}

	for (int8_t i = 0 ; i < WRITE_LEN ; i++) {
		EXPECT_EQ(static_cast<int8_t>(1) + i, pos[i]);
	}

	pos = static_cast<int8_t*>(buf->map_base());

	for (int8_t i = 0 ; i < WRITE_LEN / 2 ; i++) {
		EXPECT_EQ(static_cast<int8_t>(1 + WRITE_LEN / 2) + i , pos[i]);
	}

	buf.reset();
}

}; // end namespace mg7x::test

int main(int argc, char **argv)
{
	using namespace mg7x::test;
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
