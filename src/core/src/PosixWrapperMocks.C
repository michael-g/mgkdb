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

#include "MgIoDefs.H"
#include "MockMgIoDefs.H"

#include <expected>

namespace mg7x::test
{

std::unique_ptr<MockIoPalette> m_palette;

void set_palette(std::unique_ptr<MockIoPalette> && ptr)
{
  m_palette = std::move(ptr);
}
std::unique_ptr<MockIoPalette> & get_palette()
{
  return m_palette;
}
void reset_palette()
{
  m_palette.reset();
}

} // end namespace mg7x::test

namespace mg7x::io
{

using namespace mg7x::test;
std::expected<ssize_t,int> write(int fd, const void *buf, size_t count) noexcept
{
  return m_palette->m_write_uptr->target(fd, buf, count);
}

std::expected<ssize_t,int> read(int fd, void *buf, size_t count) noexcept
{
  return m_palette->m_read_uptr->target(fd, buf, count);
}

std::expected<void*,int> mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) noexcept
{
  return m_palette->m_mmap_uptr->target(addr, length, prot, flags, fd, offset);
}

std::expected<int,int> memfd_create(const char *name, unsigned int flags) noexcept
{
  return m_palette->m_memfd_create_uptr->target(name, flags);
}

std::expected<int,int> munmap(void *addr, size_t length) noexcept
{
  return m_palette->m_munmap_uptr->target(addr, length);
}

std::expected<int,int> ftruncate(int fd, off_t length) noexcept
{
  return m_palette->m_ftruncate_uptr->target(fd, length);
}

std::expected<int,int> open(const char *pathname, int flags) noexcept
{
  return m_palette->m_open2_uptr->target(pathname, flags);
}

std::expected<int,int> open(const char *pathname, int flags, mode_t mode) noexcept
{
  return m_palette->m_open3_uptr->target(pathname, flags, mode);
}

std::expected<int,int> close(int fd) noexcept
{
  return m_palette->m_close_uptr->target(fd);
}

std::expected<int,int> fstat(int fd, struct stat *statbuf) noexcept
{
  return m_palette->m_fstat_uptr->target(fd, statbuf);
}

std::expected<off_t,int> lseek(int fd, off_t offset, int whence) noexcept
{
  return m_palette->m_lseek_uptr->target(fd, offset, whence);
}
  
} // end namespace mg7x::io

