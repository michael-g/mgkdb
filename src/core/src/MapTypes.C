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
#include <fcntl.h> // O_CREAT, O_RDWR etc

#include "MgDebug.H"

#include "MgIoDefs.H"
#include "MgMapTypes.H"

namespace mg7x
{
void Mem::move_from(Mem && rhs) noexcept
{
  m_mem = rhs.m_mem;
  m_sz = rhs.m_sz;
  rhs.m_mem = nullptr;
  rhs.m_sz = 0;
}

int Mem::alloc(Mem & dst, Extent sz, size_t alignment) noexcept
{
  void *mem = alignment ? aligned_alloc(alignment, sz.ext64()) :
                            malloc(sz.ext64());
  if (nullptr == mem) {
    ERR_PRINT("malloc({}): {}", sz.ext64(), strerror(errno));
    return -1;
  }
  dst = Mem{mem, sz};
  return 0;
}

void Mem::release() noexcept
{
  free(m_mem);
  m_mem = nullptr;
}

using namespace mg7x::io;

int Mapping::unmap() noexcept
{
  if (m_pages.bytes64() > 0) {
    auto res = io::munmap(m_addr.as_ptr<void*>(), m_pages.bytes64());
    if (!res) {
      ERR_PRINT("munmap({}, {}): {}", m_addr.as_ptr<void*>(), m_pages.bytes64(), strerror(res.error()));
      return -1;
    }
    m_addr = Address{nullptr};
    m_pages = PageCount{};
  }
  return 0;
}

int Mapping::create_reservation(Mapping & dst, PageCount pg_ct) noexcept
{
  size_t len = pg_ct.bytes64();
  constexpr int prot = PROT_NONE;
  constexpr int flags = MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE;
  constexpr int fd = -1;
  constexpr off_t offset = 0;
  DBG_PRINT("requesting mmap(<nil>, {}, {:#x}, {:#x}, {}, {})", len, prot, flags, fd, offset);
  auto res = io::mmap(nullptr, len, prot, flags, fd, offset);
  if (!res) {
    ERR_PRINT("mmap(<nil>, {}, {:#x}, {:#x}, {}, {}): {}", len, prot, flags, fd, offset, strerror(res.error()));
    return -1;
  }
  dst.m_addr = Address{res.value()};
  dst.m_pages = pg_ct;
  dst.m_prot = prot;
  dst.m_flags = flags;
  dst.m_fd = fd;
  dst.m_offset = offset;

  DBG_PRINT("mmap.create {}", dst);

  return 0;
}
int Mapping::create_mapping_from_resv(Mapping & donor, Mapping & dst, PageCount pg_ct,
                                      int prot, int flags, int fd, off_t offset) noexcept
{
  void *base = donor.base_ptr<void*>();
  size_t len = pg_ct.bytes64();
  auto res = io::mmap(base, len, prot, flags, fd, offset);
  if (!res) {
    ERR_PRINT("mmap({}, {}, {:#x}, {:#x}, {}, {}): {}; donor mapping may be impaired",
                  base, len, prot, flags, fd, offset, strerror(res.error()));
    return -1;
  }
  dst.m_addr = Address{res.value()};
  dst.m_pages = pg_ct;
  dst.m_prot = prot;
  dst.m_flags = flags;
  dst.m_fd = fd;
  dst.m_offset = offset;

  donor.m_addr += pg_ct;
  donor.m_pages -= pg_ct;

  DBG_PRINT("active mapping created {}", dst);
  DBG_PRINT("donor mapping adjusted {}", donor);

  return 0;
}

int Mapping::steal_head_pages(Mapping & donor, PageCount ct) noexcept
{
  if (0 == (MAP_FIXED & m_flags)) {
    ERR_PRINT("logic error: m_flags does not have MAP_FIXED: {}", *this);
    return -1;
  }

  off_t offset = m_fd >= 0 ? m_pages.bytes64() + m_offset : 0;

  DBG_PRINT("mmap.steal({}, {}, {:#x}, {:#x}, {}, {})", donor.base_ptr<void*>(),
              ct.bytes64(), m_prot, m_flags, m_fd, offset);

  auto res = io::mmap(donor.base_ptr<void*>(), ct.bytes64(), m_prot, m_flags, m_fd, offset);
  if (!res) {
    ERR_PRINT("mmap.steal({}, {}, {:#x}, {:#x}, {}, {}): {}", donor.base_ptr<void*>(),
              ct.bytes64(), m_prot, m_flags, m_fd, offset, strerror(res.error()));
    return -1;
  }
  donor.m_addr += ct;
  donor.m_pages -= ct;

  m_pages += ct;

  DBG_PRINT("steal_head_pages.from: {}", donor);
  DBG_PRINT("steal_head_pages.to  : {}", *this);

  return 0;
}

int ReservedMapping::reserve_and_alloc(ReservedMapping & pair, PageCount resv, PageCount alloc, int prot, int flags, int fd, off_t off) noexcept
{
  int err = Mapping::create_reservation(pair.m_resv, resv);
  if (0 != err) {
    return err;
  }
  err = Mapping::create_mapping_from_resv(pair.m_resv, pair.m_actv, alloc, prot, MAP_FIXED|flags, fd, off);
  if (0 != err) {
    pair.m_resv.unmap();
    return err;
  }
  
  return 0;
}

int ReservedMapping::extend_active_to_include(Address addr) noexcept
{
  if (m_actv.end_addr().u64() >= addr.u64()) {
    return 0;
  }
  Extent diff = m_actv.end_addr().difference(addr);
  PageCount pages = PageCount::from_bytes_align_up(diff);

  return m_actv.steal_head_pages(m_resv, pages);
}

void ReservedMapping::close() noexcept
{

}

} // end namespace mg7x
