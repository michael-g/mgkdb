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
#ifndef MG_INC_CORE_H
#define MG_INC_CORE_H
#pragma once

#include <stdint.h>
#include <string.h> // memcpy

#include <type_traits>

#define MG_PACKED __attribute__((packed))

#ifdef Z
#error "Bad Zeee. You may have included k.h ahead of this."
#endif

namespace mg7x
{
constexpr uint32_t PAGE_SIZE_32 = 4096;
constexpr uint64_t PAGE_SIZE_64 = PAGE_SIZE_32;
constexpr uint64_t PAGE_SIZE = PAGE_SIZE_64;

template<typename Z>
static inline
bool is_aligned(Z val, Z align)
{
  return 0 == (val & (static_cast<Z>(align) - 1));
}

template<typename Z>
static inline
bool is_page_aligned(Z val)
{
  return is_aligned<Z>(val, PAGE_SIZE_64);
}

constexpr
static bool is_power_of_two(uint64_t val)
{
  return val && !(val & (val - 1));
}

template<typename D, typename S>
static inline D ptr_cvt(S src)
{
  static_assert(sizeof(D) == sizeof(S));
  D dst;
  S _src = src;
  memcpy(&dst, &_src, sizeof(D));
  return dst;
}

template<uint64_t Z>
static inline
constexpr uint64_t align_up(uint64_t value)
{
  value += Z - 1;
  value &= ~(Z - 1);
  return value;
}

template<uint64_t Z>
static inline
constexpr uint64_t align_to_next_mltpl(uint64_t value)
{
  value += Z;
  value &= ~(Z - 1);
  return value;
}

class Extent
{
  uint64_t m_size{0};
public:
  Extent() {}
  template<typename T>
  Extent(T sz) noexcept { static_assert(!std::is_same_v<T,uint64_t>  && "Bad type"); }
  Extent(uint64_t sz) noexcept : m_size{sz} {}

  uint64_t ext64() const noexcept { return m_size; }

  template<uint64_t Z> [[nodiscard]]
  Extent align_up_cp() const noexcept { return Extent{mg7x::align_up<Z>(m_size)}; }

  template<uint64_t Z> [[nodiscard]]
  Extent align_to_next_mltpl_cp() const noexcept { return Extent{mg7x::align_to_next_mltpl<Z>(m_size)}; }

  Extent difference(Extent rhs) const noexcept { return Extent{rhs.ext64() > m_size ? rhs.ext64() - m_size : m_size - rhs.ext64()}; }

  void operator+=(Extent rhs) noexcept { m_size += rhs.m_size; }
  void operator+=(uint64_t rhs) noexcept { m_size += rhs; }
  Extent operator+(Extent rhs) const noexcept { return Extent{m_size + rhs.m_size}; }
  Extent operator+(uint64_t rhs) const noexcept { return Extent{m_size + rhs}; }
  bool operator<(Extent rhs) const noexcept { return m_size < rhs.m_size; }
  bool operator<=(Extent rhs) const noexcept { return m_size <= rhs.m_size; }
  bool operator>(Extent rhs) const noexcept { return m_size > rhs.m_size; }
  bool operator>=(Extent rhs) const noexcept { return m_size >= rhs.m_size; }
  bool operator==(Extent rhs) const noexcept { return m_size == rhs.m_size; }
};

class PageCount
{
  static constexpr uint64_t PG_SZ = 4096;
  uint64_t m_count;
public:
  static
  PageCount from_bytes(Extent ext) noexcept { return PageCount{ ext.ext64() / PG_SZ }; }
  static
  PageCount from_bytes_align_up(Extent ext) noexcept { return PageCount{ (PG_SZ + ext.ext64()) / PG_SZ }; }

  PageCount() noexcept : m_count{0} {}
  // template<typename T>
  // PageCount(T count) noexcept : m_count{count} {static_assert(!std::is_signed<T>::value && "Bad type"); }
  explicit
  PageCount(uint64_t pg_ct) noexcept : m_count{pg_ct} {}

  uint64_t pages64() const noexcept { return m_count; }
  uint64_t bytes64() const noexcept { return m_count * PG_SZ; }

  void operator-=(PageCount ct) noexcept { m_count -= ct.pages64(); }
  void operator+=(PageCount ct) noexcept { m_count += ct.pages64(); }
};

class Address
{
  uint64_t m_addr{0};

public:
  Address() {}
  explicit Address(void *addr) : m_addr{ptr_cvt<uint64_t>(addr)} {}
  explicit Address(uint64_t addr) : m_addr{addr} {}

  uint64_t u64() const noexcept { return m_addr; }
  bool is_nil() const noexcept { return m_addr == 0; }
  template<uint64_t Z>
  Address align_up_cp() const noexcept { return Address{mg7x::align_up<Z>(m_addr)}; }
  template<typename D>
  D as_ptr() const noexcept { return ptr_cvt<D>(m_addr); }

  Extent difference(Address rhs) const noexcept { return Extent{rhs.u64() > m_addr ? rhs.u64() - m_addr : m_addr - rhs.u64()}; }
  Address operator+(Extent sz) const noexcept { return Address{m_addr + sz.ext64()}; }
  Address operator-(Extent sz) const noexcept { return m_addr < sz.ext64() ? Address{nullptr} : Address{m_addr - sz.ext64()}; }
  void operator+=(PageCount ct) noexcept { m_addr += ct.bytes64(); }
  bool operator>(Address rhs) const noexcept { return m_addr > rhs.m_addr; }
  bool operator>=(Address rhs) const noexcept { return m_addr >= rhs.m_addr; }
  bool operator<(Address rhs) const noexcept { return m_addr < rhs.m_addr; }
  bool operator<=(Address rhs) const noexcept { return m_addr <= rhs.m_addr; }
};

} // end namespace mg7x

#endif // end _MG_CORE_H_
