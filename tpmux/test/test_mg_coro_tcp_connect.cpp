#include <gtest/gtest.h>

#include "mg_coro_epoll.h"
#include "test_mg_io.cpp"

#include "mg_coro_epoll.cpp"
#include "mg_coro_tcp_connect.cpp"

namespace mg7x::test {

TopLevelTask<int> coro_wrapper(EpollCtl &ctl) 
{
  Task<int> task = tcp_connect(ctl, "localhost", "30098");
  int val = co_await task;
  co_return val;
}

TEST(MgCoroTcpConnectTest, Test_tcp_connect)
{
  // create_event_fd
  io::eventfd::outputs.emplace_back(-1, ENOMEM);
  EpollCtl ctl{-1};
  Task<int> task = tcp_connect(ctl, "localhost", "30098");
  auto tlt = TopLevelTask<int>::await(task);
  DBG_PRINT("Testing ... 1, 2, 3");
  
  EXPECT_TRUE(tlt.done());
  EXPECT_EQ(-1, tlt.value());
  EXPECT_EQ(1, io::eventfd::calls.size());
  auto rgs = io::eventfd::calls.front();
  EXPECT_EQ(0, rgs.initval);
  EXPECT_EQ(EFD_NONBLOCK|EFD_SEMAPHORE, rgs.flags);
}

}

int main(int argc, char **argv)
{
  using namespace mg7x::test;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
