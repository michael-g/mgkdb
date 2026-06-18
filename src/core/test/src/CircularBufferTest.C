/* This file is part of the "Mg kdb+ Library" (hereinafter "The Library").
 *
 * The Library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * The Library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License along with The
 * Library. If not, see https://www.gnu.org/licenses/agpl.txt.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stddef.h>      // size_t
#include <sys/types.h>   // off_t
#include <sys/mman.h>

#include "MgCircularBuffer.H"
#include "MockMgIoDefs.H"


namespace mg7x::test {

static void* to_vp(uint64_t base, size_t off = 0)
{
  return reinterpret_cast<int8_t*>(base) + off;
}

struct CircularBufferTest : public MockIoTest
{

};

TEST_F(CircularBufferTest, TestInitCircBufferCompletesAsExpected)
{
  using testing::_;

  const PageCount MAP_PGS{1u};
  const size_t BUF_LEN = MAP_PGS.bytes64();

  const int mfd = 4;
  const uint64_t base_addr = 0x7f001000L;
  void *p_base_addr = to_vp(base_addr); // (void*)base_addr;
  void *p_seam_addr = to_vp(base_addr, BUF_LEN);

  install<MMapMock>();
  install<MemFDCreateMock>();
  install<FTruncateMock>();
  install<MUnMapMock>();
  install<CloseMock>();

  testing::Sequence seq{};

  // We expect there to have been three mmap calls:
  // 1. the first call allocates twice the buf-len argument with PRIVATE/ANON backing
  // 2. the second, which is RW and SHARED/FIXED, and
  // 3. the third, which is also RW and SHARED/FIXED but at the contiguous second segment address
  auto mock_mmap_1 = [=]() {return p_base_addr;};
  expect<MMapMock>(1, seq, mock_mmap_1, nullptr, 2 * BUF_LEN, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  expect<MMapMock>(1, seq, mock_mmap_1, p_base_addr, BUF_LEN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, (off_t)0);
  expect<MMapMock>(1, seq, [=](){return p_seam_addr;}, p_seam_addr, BUF_LEN, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, (off_t)0);

  expect<MemFDCreateMock>(1, [](){return mfd;}, _, 0);
  expect<FTruncateMock>(1, [](){return 0;}, mfd, BUF_LEN);

  // We assert that the CircularBuffer destructor closes the mapping
  // and the mem_fd
  expect<MUnMapMock>(1, [](){return 0;}, p_base_addr, BUF_LEN * 2);
  expect<CloseMock>(1, []() -> std::expected<int,int>{return 0;}, mfd);

  {
    auto maybe_buf = mg7x::init_circ_buffer(MAP_PGS);
    EXPECT_FALSE(!maybe_buf);
  }
}

TEST_F(CircularBufferTest, TestInitCircBufferFailsIfMMapCallFails)
{
  install<MMapMock>();
  install<MUnMapMock>();
  install<CloseMock>();

  const PageCount MAP_PGS{1u};
  const size_t BUF_LEN = MAP_PGS.bytes64();

  testing::Sequence marker;

  expect<MMapMock>(1, [](){return std::unexpected(ENOMEM);}, nullptr, 2 * BUF_LEN, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

  auto maybe_buf = mg7x::init_circ_buffer(MAP_PGS);
  // Expect TRUE, but coerce to bool using operator!
  EXPECT_TRUE(!maybe_buf);

  // Assert we returned the first error messge, not worrying about the exact text
  // for ENOMEM
  EXPECT_THAT(maybe_buf.error().c_str(), testing::StartsWith("Failed to reserve private address range: "));

}

TEST_F(CircularBufferTest, TestInitCircBufferFirstRAIIDestructorCalled)
{
  const PageCount MAP_PGS{1u};
  const size_t BUF_LEN = MAP_PGS.bytes64();

  const uint64_t base_addr = 0x7f001000L;
  void *p_base_addr = to_vp(base_addr); // (void*)base_addr;

  install<MMapMock>();
  install<MemFDCreateMock>();
  install<FTruncateMock>();
  install<MUnMapMock>();
  install<CloseMock>();

  auto mmap_fun = [p_base_addr](){return p_base_addr;};
  expect<MMapMock>(1, mmap_fun, nullptr, 2 * BUF_LEN, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

  expect<MemFDCreateMock>(1, [](){return std::unexpected<int>{ENOMEM};}, testing::StrEq("mg_circ_buf"), 0);

  expect<MUnMapMock>(1, [](){return 0;}, p_base_addr, BUF_LEN * 2);

  auto maybe_buf = mg7x::init_circ_buffer(MAP_PGS);

  // Assert the error branch was taken, and the string was returned
  EXPECT_TRUE(!maybe_buf);

  // Assert the stem of the error-message matches
  EXPECT_THAT(maybe_buf.error().c_str(), testing::StartsWith("Failed in memfd_create: "));
}

TEST_F(CircularBufferTest, TestInitCircBufferBothRAIIDestructorsCalled)
{
  const PageCount MAP_PGS{1u};
  const size_t BUF_LEN = MAP_PGS.bytes64();

  const int MFD = 4;
  const uint64_t BASE_ADDR = 0x7f001000L;
  void *p_base_addr = to_vp(BASE_ADDR); // (void*)base_addr;

  install<MMapMock>();
  install<MemFDCreateMock>();
  install<FTruncateMock>();
  install<MUnMapMock>();
  install<CloseMock>();

  // Here we're asserting that the CircularBuffer destructor is closing the
  // resources properly.
  expect<MUnMapMock>(1, [](){return 0;}, p_base_addr, 2 * BUF_LEN);
  expect<CloseMock>(1, [](){return 0;}, MFD);

  // We're going to test the scenario where for some idiopathic reason the final mmap
  // doesn't return the expected base-address and the sanity check returns an error
  // message
  expect<MMapMock>(1, [p_base_addr](){return p_base_addr;}, nullptr, 2 * BUF_LEN,
                       PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

  expect<MemFDCreateMock>(1, [](){return MFD;}, testing::_, 0);

  expect<FTruncateMock>(1, [](){return 0;}, MFD, BUF_LEN);

  expect<MMapMock>(1, [p_base_addr](){return p_base_addr;}, p_base_addr, BUF_LEN,
                       PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, MFD, 0);

  // cause the error by returning the wrong base address
  void *addr = to_vp(BASE_ADDR, BUF_LEN);
  void *rtn = to_vp(BASE_ADDR, BUF_LEN / 2);
  expect<MMapMock>(1, [rtn](){return rtn;}, addr, BUF_LEN, PROT_READ|PROT_WRITE,
                       MAP_SHARED|MAP_FIXED, MFD, 0);

  auto maybe_buf = mg7x::init_circ_buffer(MAP_PGS);

  EXPECT_TRUE(!maybe_buf);

  // Assert the stem of the error-message matches, actual value is
  // "expected MAP_FIXED addr and response to be the same, have addr=0x7f001000 and result=0x7f001800"
  EXPECT_THAT(maybe_buf.error().c_str(), testing::MatchesRegex("expected MAP_FIXED addr "
              "and response to be the same, have addr=0x[0-9a-f]+ and result=0x[0-9a-f]+"));
}

}; // end namespace mg7x::test

