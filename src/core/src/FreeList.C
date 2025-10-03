#include "MgFreeList.H"
#include "MgDebug.H"


namespace mg7x
{

void FreeList::move_from(FreeList && rhs) noexcept
{
  if (nullptr != m_idcs) {
    delete[] m_idcs;
  }
  m_cap = rhs.m_cap;
  m_next = rhs.m_next;
  m_idcs = rhs.m_idcs;
  rhs.m_cap = 0;
  rhs.m_next = UINT32_MAX;
  rhs.m_idcs = nullptr;
}

int FreeList::alloc(FreeList & list, uint32_t cap) noexcept
{
  if (0 == cap) {
    ERR_PRINT("illegal capacity");
    return -1;
  }

  uint32_t *idcs{nullptr};
  try {
    idcs = new uint32_t[cap];
  }
  catch (...) {
    ERR_PRINT("new uint32_t[]");
    return -1;
  }

  if (nullptr != list.m_idcs) {
    delete[] list.m_idcs;
  }
  list.m_idcs = idcs;
  list.m_cap = cap;
  list.m_next = cap - 1;

  for (uint32_t i = 0 ; i < cap ; i++) {
    list.m_idcs[i] = cap - 1 - i;
  }
  return 0;
}

[[nodiscard]]
uint32_t FreeList::release(uint32_t idx) noexcept
{
  if (m_next == m_cap) {
    ERR_PRINT("illegal state: m_next == m_cap == {}", m_cap);
    return UINT32_MAX;
  }
  //DBG_PRINT("Released index {}, occupancy is now {}", idx, size()-1);
  m_idcs[++m_next] = idx;
  return 0;
}

[[nodiscard]]
uint32_t FreeList::next_slot() noexcept
{
  if (UINT32_MAX == m_next) {
    WRN_PRINT("FreeList full (cap {})", m_cap);
    return UINT32_MAX;
  }
  uint32_t idx = m_idcs[m_next];
  //DBG_PRINT("Assigning UINT32_MAX to m_idcs[{}], returning idx {}", m_next, idx);
  m_idcs[m_next--] = UINT32_MAX;
  return idx;
}

} // end namespace mg7x
