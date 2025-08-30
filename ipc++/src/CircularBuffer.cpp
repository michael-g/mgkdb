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

#include <stdint.h>     //
#include <string.h>     // memset, strnlen
#include <sys/mman.h>   // mmap memfd_create

#include <memory>       // unique_ptr
#include <expected>
#include <format>
#include <tuple>        // std::ignore
#include <utility>      // std::forward

#include "CircularBuffer.h"

namespace mg7x {

struct MMapCleanup
{
	void   *m_base;
	size_t  m_len;

	MMapCleanup(void *base, size_t len)
	 : m_base(base)
	 , m_len(len)
	{}
	~MMapCleanup()
	{
		if (nullptr != m_base && 0 < m_len) {
			std::ignore = io::munmap(m_base, m_len);
		}
	}
	void disarm()
	{
		m_base = nullptr;
		m_len = 0;
	}
};

struct MemFdCleanup
{
	int m_fd;
	MemFdCleanup(int fd) : m_fd{fd} {}
	~MemFdCleanup()
	{
		if (m_fd) {
			std::ignore = io::close(m_fd);
		}
	}
	void disarm() {
		m_fd = 0;
	}

};

std::expected<CircBufPtr,std::string> init_circ_buffer(const size_t buf_len)
{
	std::expected<void*,int> res_map = io::mmap(nullptr, 2 * buf_len, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (!res_map) {
		return std::unexpected(std::format("Failed to reserve private address range: {}", strerror(res_map.error())));
	}

	void *base = res_map.value();
	int mfd;

	MMapCleanup root_cleaner{base, buf_len * 2};

	std::expected<int,int> io_res = io::memfd_create("mg_circ_buf", 0);
	if (!io_res) {
		return std::unexpected(std::format("Failed in memfd_create: {}", strerror(io_res.error())));
	}

	mfd = io_res.value();

	MemFdCleanup mfd_closer{mfd};

	io_res = io::ftruncate(mfd, buf_len);
	if (!io_res) {
		return std::unexpected(std::format("Failed in ftruncate: {}", strerror(io_res.error())));
	}

	res_map = io::mmap(base, buf_len, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, 0);
	if (!res_map) {
		return std::unexpected(std::format("Failed in mmap(base, len={}, .. MAP_FIXED): {}", buf_len, strerror(res_map.error())));
	}

	void *tmp = res_map.value();

	if (tmp != base) {
		return std::unexpected(std::format("expected MAP_FIXED addr and response to be the same, have addr={} and result={}", base, tmp));
	}

	res_map = io::mmap(static_cast<int8_t*>(base) + buf_len, buf_len, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, 0);
	if (!res_map) {
		return std::unexpected(std::format("Failed in mmap(base + buf_len, len={}, .. MAP_FIXED): {}", buf_len, strerror(res_map.error())));
	}

	tmp = res_map.value();

	if (tmp != (static_cast<int8_t*>(base) + buf_len)) {
		return std::unexpected(std::format("expected MAP_FIXED addr and response to be the same, have addr={} and result={}", base, tmp));
	}

	root_cleaner.disarm();
	mfd_closer.disarm();

	return std::make_unique<CircularBuffer>(base, buf_len, mfd);
}

} // end namespace mg7x
