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
#include "MgMapTypes.H"
#include "MockMgIoDefs.H"

#include <sys/mman.h> // MAP_FAILED
#include <fcntl.h> // O_CREAT etc

#include <expected>

namespace mg7x::test
{

using namespace mg7x;
using namespace testing;


struct MapTypesIoTest : public MockIoTest
{};

TEST_F(MapTypesIoTest, Test_Mapping_move_from)
{
  Address base{0x7f000000};

  install<MUnMapMock>();
  // The moved-to Mapping, 'lhs', will call munmap in its destructor
  // as it goes out-of-scope, but the 'rhs' instance will not as its 
  // ::unmap function sees that the extent is zero.
  expect<MUnMapMock>(1, [](){return 0;}, base.as_ptr<void*>(), 4096);

  {
    Mapping rhs{base, PageCount{1uL}};
    Mapping lhs{std::move(rhs)};
    EXPECT_EQ(base.u64(), lhs.base_addr().u64());
    EXPECT_EQ(4096, lhs.extent().ext64());
    EXPECT_EQ(0, rhs.base_addr().u64());
    EXPECT_EQ(0, rhs.extent().ext64());
  }
}

// Tests the difference function in class Address to assert the unsigned subtraction.
TEST(MgMapTypesTest, Test_Address_difference)
{
  Address rhs{8192};
  Address lhs{4096};
  EXPECT_EQ(4096, lhs.difference(rhs).ext64());
  EXPECT_EQ(4096, rhs.difference(lhs).ext64());
}

// Tests that ::create_mapping returns a filled-out Mapping according to the parameters.
// Tests the success case.
TEST_F(MapTypesIoTest, Test_Mapping_create_reservation)
{
  Address addr{0x7f000000};
  PageCount pg_ct{1uL};
  Mapping mapping{};

  install<MMapMock>();
  install<MUnMapMock>();
  expect<MMapMock>(1, [addr](){return addr.as_ptr<void*>();});
  expect<MUnMapMock>(1, [](){return 0;});

  int res = Mapping::create_reservation(mapping, pg_ct);

  EXPECT_EQ(0, res);
  EXPECT_EQ(addr.u64(), mapping.base_addr().u64());
  EXPECT_EQ(pg_ct.bytes64(), mapping.extent().ext64());
  EXPECT_EQ(PROT_NONE, mapping.prot());
  EXPECT_EQ(MAP_ANONYMOUS|MAP_NORESERVE|MAP_PRIVATE, mapping.flags());
  EXPECT_EQ(-1, mapping.fd());
  EXPECT_EQ(0, mapping.offset());

}

// Tests that if the mmap call fails that -1 is returned and that the Mapping object's
// properties remain at their default values.
TEST_F(MapTypesIoTest, Test_Mapping_create_reservation_fails)
{
  install<MMapMock>();
  expect<MMapMock>(1, [](){return std::unexpected(1);});

  Address addr{0x7f000000};
  PageCount pg_ct{1uL};
  Mapping mapping{};

  int res = Mapping::create_reservation(mapping, pg_ct);

  EXPECT_EQ(-1, res);
  EXPECT_EQ(0, mapping.base_addr().u64());
  EXPECT_EQ(0, mapping.extent().ext64());
  EXPECT_EQ(0, mapping.prot());
  EXPECT_EQ(0, mapping.flags());
  EXPECT_EQ(-1, mapping.fd());
  EXPECT_EQ(0, mapping.offset());

}

// Tests the Mapping::extend_to function where the call to mmap fails
TEST_F(MapTypesIoTest, Test_Mapping_steal_head_pages_failure)
{
  const uint64_t base = 0x7f000000;
  
  Address addr_start{base};
  PageCount resv_ct{10uL};
  PageCount actv_ct{1uL};

  PageCount pg_1{1uL};

  Sequence map_seq;

  install<MUnMapMock>();
  expect<MUnMapMock>(2, [](){return 0;});

  install<MMapMock>();
  expect<MMapMock>(1, map_seq, [](){return (void*)base;}, nullptr, resv_ct.bytes64(), PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE, -1, 0);

  expect<MMapMock>(1, map_seq, [](){return (void*)base;}, (void*)base, actv_ct.bytes64(), PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PRIVATE, 4, 0);

  expect<MMapMock>(1, map_seq, [](){return std::unexpected(2);}, (void*)(base + actv_ct.bytes64()), pg_1.bytes64(), PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PRIVATE, 4, (off_t)actv_ct.bytes64());

  // We're using this instead of two Mapping::create_mapping calls as this will set
  // MAP_FIXED on the active mapping, and it just seems more coherent to use this 
  // to make the two Mapping::create_mapping calls than doing it independnetly here.
  ReservedMapping resv{};
  int res = ReservedMapping::reserve_and_alloc(resv, resv_ct, actv_ct, PROT_READ|PROT_WRITE, MAP_PRIVATE, 4, 0);
  EXPECT_EQ(0, res);

  res = resv.extend_active_to_include(Address{base + actv_ct.bytes64() + 1});
  EXPECT_EQ(-1, res);

}

// Tests the Mapping::extend_to function success-case. Really similar to the test
// for the failure-mode, but there we go.

TEST_F(MapTypesIoTest, Test_Mapping_extend_active_to_include_success)
{
  const uint64_t base = 0x7f000000;
  
  Address addr_start{base};
  PageCount resv_ct{10uL};
  PageCount actv_ct{1uL};

  PageCount pg_1{1uL};

  Sequence map_seq;

  install<MUnMapMock>();
  expect<MUnMapMock>(2, [](){return 0;});

  install<MMapMock>();

  expect<MMapMock>(1, map_seq, [](){return (void*)base;}, nullptr, resv_ct.bytes64(), PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE, -1, 0);

  expect<MMapMock>(1, map_seq, [](){return (void*)base;}, (void*)base, actv_ct.bytes64(), PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PRIVATE, 4, 0);

  expect<MMapMock>(1, map_seq, [](){return (void*)base;}, (void*)(base + actv_ct.bytes64()), pg_1.bytes64(), PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PRIVATE, 4, (off_t)actv_ct.bytes64());

  {
    // We're using this instead of two Mapping::create_mapping calls as this will set
    // MAP_FIXED on the active mapping, and it just seems more coherent to use this 
    // to make the two Mapping::create_mapping calls than doing it independnetly here.
    ReservedMapping resv{};
    int res = ReservedMapping::reserve_and_alloc(resv, resv_ct, actv_ct, PROT_READ|PROT_WRITE, MAP_PRIVATE, 4, 0);
    EXPECT_EQ(0, res);

    res = resv.extend_active_to_include(Address{base + actv_ct.bytes64() + 1});
    EXPECT_EQ(0, res);
  }
}

// Tests the call to munmap. Subtly also tests that it is not called when size == 0, by
// asserting that munmap is called only once: the destructor will call ::unmap again when
// it goes out-of-scope, and would therefore fail the .Times assertion if the size-check
// were inoperative.
TEST_F(MapTypesIoTest, Test_Mapping_unmap_success)
{
  const uint64_t base = 0x7f000000;
  Address addr_start{base};
  PageCount pg_ct{1uL};
  Mapping mapping{};

  install<MMapMock>();
  expect<MMapMock>(1, [](){return (void*)base;});

  install<MUnMapMock>();
  expect<MUnMapMock>(1, [](){return 0;}, addr_start.as_ptr<void*>(), pg_ct.bytes64());

  int res = Mapping::create_reservation(mapping, pg_ct);

  EXPECT_EQ(0, res);

  res = mapping.unmap();
  EXPECT_EQ(0, res);
  EXPECT_EQ(nullptr, mapping.base_ptr<void*>());
  EXPECT_EQ(0, mapping.extent().ext64());
}

TEST_F(MapTypesIoTest, Test_Mapping_steal_head_pages)
{
  uint64_t base = 0x7f000000;
  PageCount rsv_ct{9uL};
  Address dst_addr{base + PAGE_SIZE_64};
  Address src_addr{base};
  Mapping src{dst_addr, rsv_ct};
  Mapping dst{src_addr, PageCount{1uL}, PROT_WRITE, MAP_FIXED|MAP_SHARED, 4, 0};

  EXPECT_EQ(9, src.page_count().pages64());
  EXPECT_EQ(1, dst.page_count().pages64());

  install<MMapMock>();
  auto map_rtn = [dst_addr](){return dst_addr.as_ptr<void*>();};
  expect<MMapMock>(1, map_rtn, dst_addr.as_ptr<void*>(), 4096, PROT_WRITE, MAP_SHARED|MAP_FIXED, 4, 4096);

  install<MUnMapMock>();
  expect<MUnMapMock>(2, [](){return 0;});

  int err = dst.steal_head_pages(src, PageCount{1uL});
  EXPECT_EQ(0, err);

  // We constructed src and dst with dst at "zero", src already at +1 page
  // Assert that the donor 'src' is now at +2 pages
  EXPECT_EQ(base + 2 * PAGE_SIZE_64, src.base_addr().u64());
  // Assert that the donor's extent has shrunk from 9 to 8 (let's say that it
  // started at 10, and immediately gave 1 to 'dst' upon construction).
  EXPECT_EQ(8, src.page_count().pages64());
  // Assert that the actlive 'dst' is still at "zero"
  EXPECT_EQ(base, dst.base_addr().u64());
  // Assert that the active mapping now has two pages' extent
  EXPECT_EQ(2, dst.page_count().pages64());

}

} // end namespace mg7x::test

