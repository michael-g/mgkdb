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
#ifndef MG_INC_MG_MAP_TYPES_H
#define MG_INC_MG_MAP_TYPES_H
#pragma once

#include <stdint.h>
#include <sys/mman.h> // mmap constants

#include <format>

#include "MgCore.H"

namespace mg7x
{

class Mem
{
  void *m_mem;
  uint64_t m_sz;

  void move_from(Mem && rhs) noexcept;

public:
  static int alloc(Mem & dst, Extent sz, size_t alignment = 0) noexcept;

public:
  ~Mem() noexcept { release(); }
  Mem() noexcept : m_mem{nullptr}, m_sz{0} {}
  Mem(void *mem, Extent sz) noexcept : m_mem{mem}, m_sz{sz.ext64()} { } 
  Mem(Mem const & rhs) = delete;
  Mem(Mem && rhs) noexcept { move_from(std::move(rhs)); }
  
  void operator=(Mem && rhs) noexcept { move_from(std::move(rhs)); }
  void operator=(Mem const & rhs) = delete;

  template<typename D>
  D base_ptr() const noexcept { return ptr_cvt<D>(m_mem); }
  Address addr() const noexcept { return Address{m_mem}; }
  Extent extent() const noexcept { return m_sz; }
  void release() noexcept;
};

class Mapping
{
  Address   m_addr{};
  PageCount m_pages{};
  int       m_prot{0};
  int       m_flags{0};
  int       m_fd{-1};
  off_t     m_offset{0};

  void move_from(Mapping && rhs) noexcept;

public:
  static
  int create_reservation(Mapping & dst, PageCount page_ct) noexcept;
  static
  int create_mapping_from_resv(Mapping & donor, Mapping & dst, PageCount page_ct,
                               int prot, int flags, int fd, off_t offset) noexcept;
public:
  ~Mapping() noexcept { unmap(); }
  Mapping() noexcept {}
  Mapping(Address addr, PageCount pg_ct, int prot = 0, int flags = 0, int fd = -1, off_t off = 0) noexcept
  : m_addr{addr}, m_pages{pg_ct}, m_prot{prot}, m_flags{flags}, m_fd{fd}, m_offset{off}
  {}
  Mapping(Mapping && rhs) noexcept { move_from(std::move(rhs)); }
  Mapping(Mapping const &) = delete;

  void operator=(Mapping && rhs) noexcept { move_from(std::move(rhs)); }
  void operator=(Mapping const &) = delete;

  template<typename D>
  D base_ptr() const noexcept { return m_addr.as_ptr<D>(); }
  Address base_addr() const noexcept { return m_addr; }
  Address end_addr() const noexcept { return m_addr.is_nil() ? Address{} : (m_addr + m_pages.bytes64()); }
  PageCount page_count() const noexcept { return m_pages; }
  Extent extent() const noexcept { return m_pages.bytes64(); }
  int unmap() noexcept;
  int prot() const noexcept { return m_prot; }
  int flags() const noexcept { return m_flags; }
  int fd() const noexcept { return m_fd; }
  off_t offset() const noexcept { return m_offset; }
  int steal_head_pages(Mapping & donor, PageCount ct) noexcept;
};

inline
void Mapping::move_from(Mapping && rhs) noexcept
{
  m_addr = rhs.m_addr;
  m_pages = rhs.m_pages;
  m_prot = rhs.m_prot;
  m_flags = rhs.m_flags;
  m_fd = rhs.m_fd;
  m_offset = rhs.m_offset;
  rhs.m_addr = Address{nullptr};
  rhs.m_pages = PageCount{0uL};
  rhs.m_fd = -1;
  rhs.m_prot = 0;
  rhs.m_flags = 0;
  rhs.m_offset = 0;
}

class ReservedMapping
{
  Mapping m_resv;
  Mapping m_actv;

  void move_from(ReservedMapping && rhs) noexcept;

public: static 
  int reserve_and_alloc(ReservedMapping & pair, PageCount resv, PageCount alloc,
                        int prot, int flags, int fd, off_t off) noexcept;

public:
  ReservedMapping() noexcept {}
  ReservedMapping(ReservedMapping && rhs) noexcept { move_from(std::move(rhs)); }
  ReservedMapping(ReservedMapping const &) = delete;

  void operator=(ReservedMapping && rhs) noexcept { move_from(std::move(rhs)); }
  void operator=(ReservedMapping const &) = delete;

  Mapping & mapping() noexcept { return m_actv; }
  
  int extend_active_to_include(Address addr) noexcept;

  void close() noexcept;
};

inline
void ReservedMapping::move_from(ReservedMapping && rhs) noexcept
{
  m_resv = std::move(rhs.m_resv);
  m_actv = std::move(rhs.m_actv);
}

}; // end namespace mg7x

template<>
struct std::formatter<mg7x::Mapping>
{
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
  template<class FormatContext> auto format(const mg7x::Mapping & map, FormatContext & ctx) const
  {
    return std::format_to(ctx.out(), "Mapping {{ {}-{}, page_count {}, bytes {}, prot {:#x}, "
                          "flags {:#x}, fd {}, off {} }}", map.base_addr().as_ptr<void*>(),
                          map.end_addr().as_ptr<void*>(), map.page_count().pages64(),
                          map.extent().ext64(), map.prot(), map.flags(), map.fd(), map.offset());
  }
};

#endif
