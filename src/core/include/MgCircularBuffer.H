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
#ifndef MG_INC_MG_CIRCULAR_BUFFER_H
#define MG_INC_MG_CIRCULAR_BUFFER_H
#include "MgIoDefs.H"
#include "MgMapTypes.H"
#pragma once

#include <stddef.h> // size_t
#include <memory> // unique_ptr

namespace mg7x
{

class CircularBuffer
{
	Mapping  m_mapping;
	int      m_mem_fd{-1};
	uint64_t m_pos{0};
	uint64_t m_len{0};

  void move_from(CircularBuffer && rhs) noexcept
  {
    m_mapping.unmap();
    m_mapping = std::move(rhs.m_mapping);
    m_mem_fd = std::exchange(rhs.m_mem_fd, -1);
    m_pos = std::exchange(rhs.m_pos, 0);
    m_len = std::exchange(rhs.m_len, 0);
  }

public:
	CircularBuffer(Address base, PageCount pgs, int mem_fd)
	 : m_mapping{base, pgs}
	 , m_mem_fd(mem_fd)
	{}

	~CircularBuffer() {
		if (m_mem_fd >= 0) {
			std::ignore = io::close(m_mem_fd);
		}
		m_mem_fd = -1;
	}

  CircularBuffer(CircularBuffer && rhs) noexcept { move_from(std::move(rhs)); }
  void operator=(CircularBuffer && rhs) noexcept { move_from(std::move(rhs)); }
  CircularBuffer(CircularBuffer const &) = delete;
  void operator=(CircularBuffer const &) = delete;

	bool is_mapped() const noexcept {
		return !m_mapping.base_addr().is_nil();
	}
	void* map_base() const noexcept {
		return m_mapping.base_addr().as_ptr<void*>();
	}
	uint64_t map_len() const noexcept {
		return m_mapping.extent().ext64() / 2;
	}
	uint64_t magic_len() const noexcept {
		return m_mapping.extent().ext64();
	}
	void* read_ptr() const noexcept {
		return m_mapping.addr_at(m_pos).as_ptr<void*>();
	}
	void* write_ptr() const noexcept {
    return m_mapping.addr_at(write_offset()).as_ptr<void*>();
	}
  uint64_t write_offset() const noexcept {
    return m_pos + m_len;
  }
	uint64_t pos() const noexcept {
		return m_pos;
	}
	uint64_t readable() const noexcept {
		return m_len;
	}
	uint64_t writeable() const noexcept {
		return map_len() - m_len;
	}
	void clear() noexcept {
		m_pos = 0;
		m_len = 0;
	}
	void set_consumed(uint64_t count) noexcept {
		// std::print(" CIRC: .set_consumed: count {}, m_pos {}/{}/{}, m_len {}/{}\n", count, m_pos, m_pos + count, (m_pos + count >= m_buf_len ? (m_pos + count) - m_buf_len : m_pos), m_len, m_len - count);
		m_pos += count;
		m_len -= count;
		if (m_pos >= map_len()) {
			m_pos -= map_len();
		}
	}
	void set_appended(uint64_t count) noexcept{
		// std::print(" CIRC: .set_appended: count {}, m_len {}, resulting m_len {}\n", count, m_len, m_len + count);
		m_len += count;
	}
};

using CircBufUqPtr = std::unique_ptr<CircularBuffer>;
using CircBufShPtr = std::shared_ptr<CircularBuffer>;

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
std::expected<CircBufUqPtr,std::string> init_circ_buffer(const PageCount map_pgs);

} // end namespace mg7x
#endif // ifndef __mg7x_CircularBuffer__H__
