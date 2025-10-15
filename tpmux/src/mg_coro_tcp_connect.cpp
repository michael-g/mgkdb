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

#include <errno.h>
#include <stdint.h>
#include <signal.h> // struct sigevent
#include <sys/eventfd.h> // args to eventfd()
#include <sys/socket.h> // args to socket, struct addrinfo
#include <netdb.h> // struct gaicb
#include <arpa/inet.h> // inet_ntop, sockaddr_in
#include <string.h>
#include <errno.h>

#include <expected>
#include <utility>

#include "mg_coro_domain_obj.h"
#include "mg_fmt_defs.h"
#include "KdbIoDefs.h"
#include "mg_io.h"
#include "mg_coro_task.h"
#include "mg_coro_epoll.h"


static void mg_sigev_notify(union sigval arg) {
	int64_t val = 1;
	TRA_PRINT(YEL "mg_sigev_notify" RST ": calling ::mg7x::io::write({}, &val, {})", arg.sival_int, sizeof(val));

	std::expected<ssize_t,int> io_res = ::mg7x::io::write(arg.sival_int, &val, sizeof(val));
	if (!io_res.has_value()) {
		ERR_PRINT(YEL "mg_sigev_notify" RST ": failed to write 1 byte to m_eventfd {}: {}", arg.sival_int, strerror(io_res.error()));
	}
	else {
		TRA_PRINT(YEL "mg_sigev_notify" RST ": wrote signal value to event-fd in mg_sigev_notify");
	}
}

static void* get_in_addr(struct sockaddr *sa)
{
	// https://stackoverflow.com/a/9212542/322304
	if (AF_INET == sa->sa_family)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static void mg_print_addrinfo(struct addrinfo *nfo)
{
	char buf[INET6_ADDRSTRLEN] = {0};
	if (nullptr == inet_ntop(nfo->ai_family, get_in_addr(static_cast<struct sockaddr*>(nfo->ai_addr)), buf, sizeof buf)) {
		ERR_PRINT("		in inet_ntop: {}", strerror(errno));
	}
	else {
		DBG_PRINT("		addrinfo is {}", buf);
	}
}

static std::expected<int,int> create_event_fd()
{
	std::expected<int,int> result = ::mg7x::io::eventfd(0, EFD_NONBLOCK|EFD_SEMAPHORE);
	if (!result.has_value()) {
		ERR_PRINT("failed in eventfd: {}", strerror(result.error()));
	}
	return result;
}

namespace mg7x {

TASK_TYPE<std::expected<io::TcpConn,ErrnoMsg>> tcp_connect(EpollCtl & epoll, std::string_view host, std::string_view service)
{
	DBG_PRINT(GRN "tcp_connect" RST ": beginning tcp-connection to {}:{}", host, service);
	std::expected<int,int> result = create_event_fd();
	if (!result.has_value()) {
		// already logged
		co_return std::unexpected(ErrnoMsg{result.error(), "Failed to create event_fd"});
	}

	const int event_fd = result.value();

	DBG_PRINT(GRN "tcp_connect" RST ": created event_fd {}", event_fd);

	struct addrinfo addrinfo;
	struct gaicb gai_req;
	struct sigevent sevp;
	struct gaicb *gai_reqs[1];

	gai_req.ar_name = host.data();
	gai_req.ar_service = service.data();
	gai_req.ar_request = &addrinfo;

	addrinfo.ai_family = AF_INET;
	addrinfo.ai_socktype = SOCK_STREAM;

	sevp.sigev_notify = SIGEV_THREAD;
	sevp.sigev_value.sival_int = event_fd;
	sevp.sigev_notify_function = mg_sigev_notify;

	gai_reqs[0] = &gai_req;

	{
		EpollCtl::Awaiter awaiter{event_fd};
		result = epoll.add_interest(event_fd, EPOLLIN, awaiter);
		if (!result.has_value()) {
			ERR_PRINT(GRN "tcp_connect" RST ": failed in EpollCtl::add_interest");
			co_return std::unexpected(ErrnoMsg{result.error(), "Could not .add_interest"});
		}

		DBG_PRINT(GRN "tcp_connect" RST ": calling getaddrinfo_a(..)");

		result = ::mg7x::io::getaddrinfo_a(GAI_NOWAIT, gai_reqs, 1, &sevp);
		if (!result.has_value()) {
			ERR_PRINT(GRN "tcp_connect" RST ": failed in getaddrinfo_a: {}", strerror(result.error()));
			co_return std::unexpected(ErrnoMsg{result.error(), "Failed in getaddrinfo_a"});
		}

		DBG_PRINT(GRN "tcp_connect" RST ": co_await EpollCtl::Awaiter{{event_fd = {}}}", event_fd);
		auto [fd, events] = co_await awaiter;
		if (0 != (events & EPOLLERR)) {
			ERR_PRINT("Non-zero error-flags reported by epoll: getaddrinfo lookup probably bad");
			co_return std::unexpected(ErrnoMsg{0, "EPOLLERR detected after getadrrinfo_a op"});
		}

		DBG_PRINT(GRN "tcp_connect" RST ": return from co_await EpollCtl::Awaiter, fd {}, events {}", fd, events);
	}

	struct addrinfo *res = gai_req.ar_result;

	result = epoll.clr_interest(event_fd);
	if (!result.has_value()) {
		ERR_PRINT(GRN "tcp_connect" RST ": failed in EpollCtl::clr_interest");
		co_return std::unexpected(ErrnoMsg{result.error(), "Could not .clr_interest"});
	}

	TRA_PRINT(GRN "tcp_connect" RST ": closing event_fd {}", event_fd);
	result = ::mg7x::io::close(event_fd);
	if (!result.has_value()) {
		WRN_PRINT(GRN "tcp_connect" RST ": incomplete close(eventfd) signalled: {}", strerror(result.error()));
	}

	for ( ; nullptr != res ; res = res->ai_next) {
		TRA_PRINT(GRN "tcp_connect" RST ": opening socket (port {})", service);
		mg_print_addrinfo(res);
		result = ::mg7x::io::socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK, res->ai_protocol);
		if (!result.has_value()) {
			WRN_PRINT(GRN "tcp_connect" RST ": failed to create socket({}, {}, {}): {}", res->ai_family, res->ai_socktype, res->ai_protocol, strerror(result.error()));
			continue;
		}

		int sock_fd = result.value();
		TRA_PRINT(GRN "tcp_connect" RST ": created socket fd {}", sock_fd);

		TRA_PRINT(GRN "tcp_connect" RST ": calling connect; fd {}, port {}", sock_fd, service);
		result = ::mg7x::io::connect(sock_fd, res->ai_addr, res->ai_addrlen);
		// EINPROGRESS for SOCK_STREAM, EAGAIN for UNIX domain sockets
		if (!result.has_value() && EINPROGRESS != result.error() && EAGAIN != result.error()) {
			ERR_PRINT(GRN "tcp_connect" RST ": failed in connect for FD {} to {}:{}: {}", sock_fd, host, service, strerror(result.error()));
			close(sock_fd);
			continue;
		}

		if (result.has_value() && 0 == result.value()) { // already connected
			INF_PRINT(GRN "tcp_connect" RST ": connected on FD {} to {}:{}", sock_fd, host, service);
			freeaddrinfo(gai_req.ar_result);
			co_return io::TcpConn{sock_fd, host, service};
		}

		{
			EpollCtl::Awaiter awaiter{sock_fd};
			result = epoll.add_interest(sock_fd, EPOLLOUT, awaiter);
			if (!result.has_value()) {
				ERR_PRINT(GRN "tcp_connect" RST ": failed in EpollCtl::add_interest");
				close(sock_fd);
				co_return std::unexpected(ErrnoMsg{result.error(), "Could not EpollCtl::add_interest"});
			}

			TRA_PRINT(GRN "tcp_connect" RST ": co_await EpollCtl::Awaiter{{sock_fd = {}}}", sock_fd);
			auto [fd, revts] = co_await awaiter;

			if (0 != (EPOLLERR & revts)) {
				ERR_PRINT(GRN "tcp_connect" RST ": non-zero error-flags reported by epoll: probable error during connection; clearing epoll-interest and quitting");
				result = epoll.clr_interest(sock_fd);
				if (!result.has_value()) {
					ERR_PRINT(GRN "tcp_connect" RST ": error reported while clearing epoll-interest");
				}
				close(sock_fd);
				co_return std::unexpected(ErrnoMsg{0, "EPOLLERR detected after connect-op"});
			}
			TRA_PRINT(GRN "tcp_connect" RST ": return from co_await EpollCtl::Awaiter, fd {}, events {}", fd, revts);
		}

		result = epoll.clr_interest(sock_fd);
		if (!result.has_value()) {
			ERR_PRINT(GRN "tcp_connect" RST ": failed in EpollCtl::clr_interest");
			close(sock_fd);
			co_return std::unexpected(ErrnoMsg{result.error(), "Could not EpollCtl::add_interest"});
		}

		int sk_errno = 0;
		socklen_t sk_optlen = sizeof sk_errno;

		TRA_PRINT(GRN "tcp_connect" RST ": calling getsockopt");
		result = io::getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &sk_errno, &sk_optlen);
		if (!result.has_value()) {
			ERR_PRINT(GRN "tcp_connect" RST ": failed in getsockopt for FD {} to {}:{}: {}", sock_fd, host, service, strerror(result.error()));
			close(sock_fd);
		}
		else if (0 != sk_errno) {
			WRN_PRINT(GRN "tcp_connect" RST ": connection to {}:{} failed: {}", host, service, strerror(sk_errno));
			close(sock_fd);
		}
		else {
			INF_PRINT(GRN "tcp_connect" RST ": connection established on FD {} to {}:{}", sock_fd, host, service);
			co_return io::TcpConn{sock_fd, host, service};
		}
	}

	WRN_PRINT(GRN "tcp_connect" RST ": connect failed; " RED "co_return" RST " -1");
	co_return std::unexpected(ErrnoMsg{0, "Could not establish TCP connection"});
}
}; // end namespace mg7x
