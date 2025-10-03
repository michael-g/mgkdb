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
#include "MgCore.H"
#include "MgIoDefs.H"
#include "MgDebug.H"

#include <fcntl.h> // O_RDWR etc

namespace mg7x::io
{

std::expected<ssize_t,int> read_fully(int fd, void *buf, size_t len) noexcept
{
  ssize_t tot = 0;
  uint32_t tries = 0;

  while (tot < static_cast<ssize_t>(len)) {
    auto res = mg7x::io::read(fd, static_cast<uint8_t*>(buf) + tot, len - tot);
    if (!res) {
      if (EINTR == res.error() && ++tries < 3) {
        continue;
      }
      return std::unexpected(res.error());
    }
    if (0 == res.value()) {
      //WRN_PRINT("read_fully: EOF; FD %i, len %lu, tot %li", fd, len, tot);
      break;
    }
    tot += res.value();
    tries = 0;
  }
  return tot;
}

std::expected<ssize_t,int> write_fully(int fd, void *buf, size_t len) noexcept
{
  ssize_t tot = 0;
  uint32_t tries = 0;

  while (tot < static_cast<ssize_t>(len)) {
    auto res = mg7x::io::write(fd, static_cast<uint8_t*>(buf) + tot, len - tot);
    if (!res) {
      if (EINTR == res.error() && ++tries < 3) {
        continue;
      }
      return std::unexpected(res.error());
    }
    if (0 == res.value()) {
      if (++tries < 3) {
        continue;
      }
      return std::unexpected(EIO);
    }
    tot += res.value();
    tries = 0;
  }
  return tot;
}

} // end namespace mg7x::io
namespace mg7x
{

int FileDesc::open() noexcept
{
  auto res = io::open(m_path.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (!res) {
    ERR_PRINT("open failed: {}; path {}", strerror(res.error()), m_path);
    return -1;
  }
  m_fd = res.value();
  DBG_PRINT("file {} opened as FD {}", m_path, m_fd);
  if (stat_size() < 0) {
    return -1;
  }
  return 0;
}

void FileDesc::close() noexcept
{
  if (-1 != m_fd) {
    auto res = io::close(m_fd);
    if (!res) {
      ERR_PRINT("close: {}", strerror(res.error()));
    }
    m_fd = -1;
  }
}

ssize_t FileDesc::stat_size() noexcept
{
  struct stat sb{};
  auto res = io::fstat(m_fd, &sb);
  if (!res) {
    ERR_PRINT("fstat({}): {}", m_fd, strerror(res.error()));
    return -1;
  }
  m_sz = (uint64_t)sb.st_size;
  DBG_PRINT("stat({}) size = {}", m_fd, m_sz);
  return sb.st_size;
}

int FileDesc::set_size(Extent size) noexcept
{
  auto res = io::ftruncate(m_fd, size.ext64());
  if (!res) {
    ERR_PRINT("ftruncate({}, {}): {}", m_fd, size.ext64(), strerror(res.error()));
    return -1;
  }
  m_sz = size.ext64();
  DBG_PRINT("file size is {}; FD {}", m_sz, m_fd);
  return 0;
}

int FileDesc::grow_to(Extent size) noexcept
{
  const uint64_t sz = size.ext64();
  if (m_sz < sz) {
    return set_size(Extent{sz});
  }
  return 0;
}

} // end namespace mg7x
