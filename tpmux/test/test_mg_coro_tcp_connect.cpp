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

// lock-out the inclusion of the mg_io.h header and its inline definitions.
#include "mg_io.h"
#include "mg_coro_epoll.h"
#include "test_mg_io.cpp"

#include "mg_coro_epoll.cpp"
#include "mg_coro_tcp_connect.cpp"

namespace mg7x::test {

TopLevelTask<::mg7x::io::TcpConn> coro_wrapper(EpollCtl &ctl)
{
  auto task = tcp_connect(ctl, "localhost", "30098");
  ::mg7x::io::TcpConn val = co_await task;
  co_return val;
}

TEST(MgCoroTcpConnectTest, Test_tcp_connect)
{
  // create_event_fd
  io::eventfd::outputs.emplace_back(-1, ENOMEM);
  EpollCtl ctl{-1};
  Task<::mg7x::io::TcpConn> task = tcp_connect(ctl, "localhost", "30098");
  auto tlt = TopLevelTask<::mg7x::io::TcpConn>::await(task);
  DBG_PRINT("Testing ... 1, 2, 3");

  ::mg7x::io::TcpConn conn = tlt.value();

  EXPECT_TRUE(tlt.done());
  EXPECT_EQ(-1, conn.sock_fd());
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
