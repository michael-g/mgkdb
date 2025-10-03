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
// alias re='cmake --build build && (cd build && ctest --output-on-failure)'
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MgCore.H"
#include "MgIoDefs.H"
#include "MockMgIoDefs.H"

#include <sys/mman.h> // MAP_FAILED
#include <fcntl.h> // O_CREAT etc

namespace mg7x::test
{

using namespace mg7x;
using namespace testing;

TEST(CoreTest, Test_Extent)
{
  Extent ext{100uL};
  EXPECT_EQ(4096, ext.align_up_cp<4096>().ext64()); // assert align-up works as expected
  EXPECT_EQ(42, ext.difference(Extent{142uL}).ext64()); // check a larger RHS value
  EXPECT_EQ(42, ext.difference(Extent{58uL}).ext64()); // check a smaller RHS value
  ext += 1;
  EXPECT_EQ(101, ext.ext64());
  ext += Extent{1uL};
  EXPECT_EQ(102, ext.ext64());
  EXPECT_EQ(103, (ext + 1).ext64());
  EXPECT_EQ(103, (ext + Extent{1uL}).ext64());
  EXPECT_TRUE(ext == Extent{102uL});
  EXPECT_FALSE(ext > Extent{102uL});
  EXPECT_TRUE(ext > Extent{100uL});
  EXPECT_TRUE(ext >= Extent{102uL});
  EXPECT_FALSE(ext < Extent{102uL});
  EXPECT_TRUE(ext <= Extent{102uL});
  EXPECT_TRUE(ext <= Extent{103uL});
}

TEST(CoreTest, Test_Address)
{
  const uint64_t base = 0x7f000000;
  Address addr{base};
  EXPECT_EQ(base + 42, (addr + Extent{42uL}).u64());
  EXPECT_EQ(base - 42, (addr - Extent{42uL}).u64());
  EXPECT_EQ(42, addr.difference(Address{base - 42}).ext64());
  EXPECT_EQ(42, addr.difference(Address{base + 42}).ext64());

  EXPECT_TRUE(addr > Address{base - 1});
  EXPECT_FALSE(addr > addr);

  EXPECT_TRUE(addr >= Address{base - 1});
  EXPECT_TRUE(addr >= addr);
  EXPECT_FALSE(addr >= Address{base + 1});

  EXPECT_FALSE(addr < addr);
  EXPECT_TRUE(addr < Address{base + 1});

  EXPECT_TRUE(addr <= addr);
  EXPECT_TRUE(addr <= Address{base + 1});
  EXPECT_FALSE(addr <= Address{base - 1});

  EXPECT_FALSE(addr.is_nil());
  EXPECT_TRUE(Address{nullptr}.is_nil());
}


// Tests the difference function in class Address to assert the unsigned subtraction.
TEST(CoreTest, Test_Address_difference)
{
  Address rhs{8192};
  Address lhs{4096};
  EXPECT_EQ(4096, lhs.difference(rhs).ext64());
  EXPECT_EQ(4096, rhs.difference(lhs).ext64());
}

struct CoreIoTest : public MockIoTest
{};

// Tests that the FileDesc::open function works correctly and that it 
// sets the size-value as expected. It's POSIX call mockery all the way
// down.
TEST_F(CoreIoTest, Test_FileDesc_open_and_thus_stat_size)
{
  const int FD = 4;
  const char *PATH = "/some/path";
  
  install<Open3Mock>();
  install<FstatMock>();
  install<CloseMock>();

  expect<Open3Mock>(1, [](){return FD;}, StrEq(PATH), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  expect<FstatMock>(1, [](int fd, struct stat *sb) { (void)fd; sb->st_size = 42; return 0; }, FD, _);
  expect<CloseMock>(1, [](){return 0;}, FD);
  {
    FileDesc fd{"/some/path"};
    EXPECT_EQ(std::string(PATH), fd.path());   // assert the path is good

    int res = fd.open();
    EXPECT_EQ(0, res);                         // assert the return value
    EXPECT_EQ(42, fd.size64());                // assert the size has be recorded
    EXPECT_EQ(FD, fd.fd());                    // assert the FD is correct
  }
}

} // end namespace mg7x::test


