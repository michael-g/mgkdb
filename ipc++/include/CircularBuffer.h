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

#ifndef __mg7x_CircularBuffer__H__
#define __mg7x_CircularBuffer__H__
#include "KdbIoDefs.h"
#pragma once

#include <stddef.h> // size_t
#include <memory> // unique_ptr

namespace mg7x {

class CircularBuffer
{
	void    *m_base{nullptr};
	uint64_t m_buf_len;
	int      m_mem_fd;
	uint64_t m_pos{0};
	uint64_t m_len{0};

public:
	CircularBuffer(void *base, uint64_t buf_len, int mem_fd)
	 : m_base(base)
	 , m_buf_len(buf_len)
	 , m_mem_fd(mem_fd)
	{}

	~CircularBuffer() {
		if (m_base) {
			std::ignore = io::munmap(static_cast<int8_t*>(m_base) + m_buf_len, m_buf_len);
			std::ignore = io::munmap(m_base, m_buf_len);
			std::ignore = io::close(m_mem_fd);
		}
		m_base = nullptr;
		m_mem_fd = 0;
	}
	bool is_mapped() const noexcept {
		return nullptr != m_base;
	}
	void* map_base() const noexcept {
		return m_base;
	}
	void* read_ptr() const noexcept {
		return static_cast<int8_t*>(m_base) + m_pos;
	}
	void* write_ptr() const noexcept {
		return static_cast<int8_t*>(m_base) + m_pos + m_len;
	}
	uint64_t pos() const noexcept {
		return m_pos;
	}
	uint64_t readable() const noexcept {
		return m_len;
	}
	uint64_t writeable() const noexcept {
		return m_buf_len - m_len;
	}
	void clear() noexcept {
		m_pos = 0;
		m_len = 0;
	}
	void set_consumed(uint64_t count) noexcept {
		// std::print(" CIRC: .set_consumed: count {}, m_pos {}/{}/{}, m_len {}/{}\n", count, m_pos, m_pos + count, (m_pos + count >= m_buf_len ? (m_pos + count) - m_buf_len : m_pos), m_len, m_len - count);
		m_pos += count;
		m_len -= count;
		if (m_pos >= m_buf_len) {
			m_pos -= m_buf_len;
		}
	}
	void set_appended(uint64_t count) noexcept{
		// std::print(" CIRC: .set_appended: count {}, m_len {}, resulting m_len {}\n", count, m_len, m_len + count);
		m_len += count;
	}
};

using CircBufPtr = std::unique_ptr<CircularBuffer>;

/*
  Implements one of those "magic ciruclar buffers" that's discussed in a number of places online.
  Essentially, we grab a contiguous chunk of virtual memory using `mmap` and creates two more
  anonymous file-mappings, each of half the underlying size, one directly following the other
  in the address range. A write which spills over the end of the first then results in the
  overflowing bytes being written to the head of that mapped region. Very useful, for exmaple,
  so you don't have to compact bytes in a read-buffer.

  @param buf_len the capacity of the buffer; in other words, the length of the snake before it eats
  its tail. MUST BE a multiple of the page size.
 */
std::expected<CircBufPtr,std::string> init_circ_buffer(const size_t buf_len);

} // end namespace mg7x
#endif // ifndef __mg7x_CircularBuffer__H__
