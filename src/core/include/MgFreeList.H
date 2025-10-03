#ifndef MG_INC_MG_FREE_LIST_H
#define MG_INC_MG_FREE_LIST_H
#pragma once

#include <stdint.h>
#include <assert.h>

#include <utility>

namespace mg7x
{

class FreeList
{
  uint32_t  m_cap{0};
  uint32_t  m_next{0};
  uint32_t *m_idcs{nullptr};

  void move_from(FreeList && rhs) noexcept;

public:
  static
  int alloc(FreeList & list, uint32_t cap) noexcept;

  FreeList() noexcept {}
  ~FreeList() { delete[] m_idcs; }
  FreeList(FreeList const &) = delete;
  void operator=(FreeList const &) = delete;
  FreeList(FreeList && rhs) noexcept { move_from(std::move(rhs)); }
  void operator=(FreeList && rhs) noexcept { move_from(std::move(rhs)); }

  uint32_t capacity() const noexcept { return m_cap; }
  uint32_t size() const noexcept { return m_cap - m_next - 1; }

  uint32_t release(uint32_t idx) noexcept;

  uint32_t next_slot() noexcept;

};

};
#endif
