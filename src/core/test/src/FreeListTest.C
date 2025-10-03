#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdint.h>

#include "MgFreeList.H"
#include "MgDebug.H"


namespace mg7x::test
{
using namespace mg7x;
using namespace testing;

TEST(MgFreeListTest, Test_ergonomics)
{
  
  FreeList subj{};
  int err = FreeList::alloc(subj, 4);
  ASSERT_EQ(0, err) << "FreeList::alloc failed: " << err;

  EXPECT_EQ(4, subj.capacity());
  EXPECT_EQ(0, subj.size());

  // Here the free-list is initialised and contains slots 0, 1, 2, 3
  // in that order
  EXPECT_EQ(0, subj.next_slot()); // pop 0, leaving 1, 2, 3
  EXPECT_EQ(1, subj.size());

  EXPECT_EQ(1, subj.next_slot()); // pop 1, leaving 2, 3
  EXPECT_EQ(2, subj.size());

  EXPECT_EQ(2, subj.next_slot()); // pop 2, leaving 3
  EXPECT_EQ(3, subj.size());

  EXPECT_EQ(3, subj.next_slot()); // pop 3, list is empty
  EXPECT_EQ(4, subj.size());

  WRN_PRINT("Expecting message that FreeList is full:");
  EXPECT_EQ(UINT32_MAX, subj.next_slot()); // signal it's all bad
  EXPECT_EQ(4, subj.size());

  EXPECT_EQ(0, subj.release(2)); // push 2: list contains 2
  EXPECT_EQ(3, subj.size());

  EXPECT_EQ(0, subj.release(1)); // push 1, list contains 1, 2
  EXPECT_EQ(2, subj.size());

  EXPECT_EQ(1, subj.next_slot()); // pop 1, list contains 2
  EXPECT_EQ(2, subj.next_slot()); // pop 2, list is empty

  EXPECT_EQ(0, subj.release(0)); // push 0, list contains 0
  EXPECT_EQ(0, subj.release(1)); // push 1, list contains 1, 0
  EXPECT_EQ(0, subj.release(2)); // .. list contains 2, 1, 0
  EXPECT_EQ(0, subj.release(3)); // .. list contains 3, 1, 0
  EXPECT_EQ(0, subj.size());
}

} // end namespace mg7x::testing
