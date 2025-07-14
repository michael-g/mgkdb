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

#ifndef __mg_coro_epoll__H__
#define __mg_coro_epoll__H__

#include <sys/epoll.h>
#include <coroutine>
#include <utility> // std::pair
#include <functional> // std::function, std::bind
#include <expected>

namespace mg7x {

using EpollFunc = std::function<void(int)>;

class EpollCtl
{

public:
	struct Awaiter
	{
		EpollFunc m_callback = std::bind(&EpollCtl::Awaiter::onEvent, this, std::placeholders::_1);
		int m_fd;
		int m_events;
		std::coroutine_handle<> m_handle;

		explicit Awaiter(int fd);

		bool await_ready();

		void await_suspend(std::coroutine_handle<> h) noexcept;

		std::pair<int,int> await_resume() noexcept;

		void onEvent(int events);

	};

private:

	int m_epollfd;

	std::expected<int,int> epoll_upd(int fd, int events, int action, Awaiter * awaiter = nullptr);

public:
	explicit EpollCtl(int epoll_fd);

	std::expected<int,int> mod_interest(int fd, int events, Awaiter & awaiter);

	std::expected<int,int> add_interest(int fd, int events, Awaiter & awaiter);

	std::expected<int,int> clr_interest(int fd);

};

};

#endif // __mg_coro_epoll__H__
