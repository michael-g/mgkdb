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

#include <sys/epoll.h>	// struct epoll_event
#include <coroutine>
#include <string.h>

#include "KdbIoDefs.h"
#include "mg_fmt_defs.h"
#include "mg_coro_epoll.h"

namespace mg7x {

EpollCtl::Awaiter::Awaiter(int fd)
 : m_fd{fd}
{
	TRA_PRINT("EpollCtl::" ULB "Awaiter" RST " constructor: fd {}", fd);
}

bool EpollCtl::Awaiter::await_ready() {
	// a_handle.address is nil at this point
	TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_ready; address {}", m_handle.address());
	return false;
}

void EpollCtl::Awaiter::await_suspend(std::coroutine_handle<> h) noexcept {
	TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_suspend; h? {}, h.done()? {}, h.address 0x{}", !!h, h && h.done(), h.address());
	m_handle = h;
}

std::pair<int,int> EpollCtl::Awaiter::await_resume() noexcept {
	TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::await_resume: fd {}, events {}, m_handle {}, m_handle.done {}, m_handle.address {}",
	              m_fd, m_events, !!m_handle, m_handle && m_handle.done(), m_handle.address());
	return {m_fd, m_events};
}

void EpollCtl::Awaiter::onEvent(int events) {
	TRA_PRINT("EpollCtl::" ULB "Awaiter" RST "::onEvent: fd {}, events {}, m_handle {}, m_handle.done {}, m_handle.address {}",
	              m_fd, events, !!m_handle, m_handle && m_handle.done(), m_handle.address());
	m_events = events;
	if (m_handle && !m_handle.done()) {
		m_handle.resume();
	}
}

std::expected<int,int> EpollCtl::epoll_upd(int fd, int events, int action, Awaiter * awaiter) {
	struct epoll_event ev;
	ev.events = events;
	if (nullptr != awaiter) {
		ev.data.ptr = &awaiter->m_callback;
	}
	auto ret = ::mg7x::io::epoll_ctl(m_epollfd, action, fd, &ev);
	if (ret.has_value()) {
		return ret;
	}
	ERR_PRINT("EpollCtl::epoll_upd in epoll_ctl: {}", strerror(errno));
	return std::unexpected(ret.error());
}

EpollCtl::EpollCtl(int epoll_fd)
 : m_epollfd{epoll_fd}
{ }

std::expected<int,int> EpollCtl::mod_interest(int fd, int events, Awaiter & awaiter) {
	TRA_PRINT("EpollCtl::mod_interest, fd = {}, events = {}", fd, events);
	return epoll_upd(fd, events, EPOLL_CTL_MOD, &awaiter);
}

std::expected<int,int> EpollCtl::add_interest(int fd, int events, Awaiter & awaiter) {
	TRA_PRINT("EpollCtl::add_interest, fd = {}, events = {}", fd, events);
	return epoll_upd(fd, events, EPOLL_CTL_ADD, &awaiter);
}

std::expected<int,int> EpollCtl::clr_interest(int fd) {
	TRA_PRINT("EpollCtl::clr_interest, fd = {}", fd);
	return epoll_upd(fd, 0, EPOLL_CTL_DEL);
}

};
