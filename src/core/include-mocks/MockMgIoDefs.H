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
#ifndef MG_INC_MOCK_MG_IO_DEFS_H
#define MG_INC_MOCK_MG_IO_DEFS_H
#pragma once

#include <sys/types.h> // size_t
#include <expected>

#include <gmock/gmock.h>

#include <memory>

namespace mg7x::test
{

struct ReadCall
{
  virtual ~ReadCall() = default;
  virtual std::expected<ssize_t,int> target(int fd, void *buf, size_t count) noexcept = 0;
};

struct ReadMock final : public ReadCall
{
  MOCK_METHOD((std::expected<ssize_t,int>), target, (int fd, void *buf, size_t count), (noexcept, override));
};

struct WriteCall
{
  virtual ~WriteCall() = default;
  virtual std::expected<ssize_t,int> target(int fd, const void *buf, size_t count) noexcept = 0;
};

struct WriteMock final : public WriteCall
{
  MOCK_METHOD((std::expected<ssize_t,int>), target, (int fd, const void *buf, size_t count), (noexcept, override));
};

struct MMapCall
{
  virtual ~MMapCall() = default;
  virtual std::expected<void*,int> target(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;
};

struct MMapMock final : public MMapCall
{
  MOCK_METHOD((std::expected<void*,int>), target, (void *addr, size_t length, int prot, int flags, int fd, off_t offset), (override));
};

struct MUnMapCall
{
  virtual ~MUnMapCall() = default;
  virtual std::expected<int,int> target(void *addr, size_t length) = 0;
};

struct MUnMapMock final : public MUnMapCall
{
  MOCK_METHOD((std::expected<int,int>), target, (void *addr, size_t length), (override));
};

struct MemFDCreateCall
{
  virtual ~MemFDCreateCall() = default;
  virtual std::expected<int,int> target(const char *name, unsigned int flags) = 0;
};

struct MemFDCreateMock final : public MemFDCreateCall
{
  MOCK_METHOD((std::expected<int,int>), target, (const char *name, unsigned int flags), (override));
};

struct FTruncateCall
{
  virtual ~FTruncateCall() = default;
  virtual std::expected<int,int> target(int fd, off_t length) noexcept = 0;
};

struct FTruncateMock final : public FTruncateCall
{
  MOCK_METHOD((std::expected<int,int>), target, (int fd, off_t length), (noexcept, override));
};

struct Open2Call
{
  virtual ~Open2Call() = default;
  virtual std::expected<int,int> target(const char *pathname, int flags) noexcept = 0;
};

struct Open2Mock final : public Open2Call
{
  MOCK_METHOD((std::expected<int,int>), target, (const char *pathname, int flags), (noexcept, override));
};

struct Open3Call
{
  virtual ~Open3Call() = default;
  virtual std::expected<int,int> target(const char *pathname, int flags, mode_t mode) noexcept = 0;
};

struct Open3Mock final : public Open3Call
{
  MOCK_METHOD((std::expected<int,int>), target, (const char *pathname, int flags, mode_t mode), (noexcept, override));
};

struct CloseCall
{
  virtual ~CloseCall() = default;
  virtual std::expected<int,int> target(int fd) noexcept = 0;
};

struct CloseMock final : public CloseCall
{
  MOCK_METHOD((std::expected<int,int>), target, (int fd), (noexcept, override));
};

struct FstatCall
{
  virtual ~FstatCall() = default;
  virtual std::expected<int,int> target(int fd, struct stat *statbuf) noexcept = 0;
};

struct FstatMock final : public FstatCall
{
  MOCK_METHOD((std::expected<int,int>), target, (int fd, struct stat *statbuf), (noexcept, override));
};

struct LSeekCall
{
  virtual ~LSeekCall() = default;
  virtual std::expected<off_t,int> target(int fd, off_t offset, int whence) = 0;
};

struct LSeekMock final : public LSeekCall
{
  MOCK_METHOD((std::expected<off_t,int>), target, (int fd, off_t offset, int whence), (override));
};

#include <sys/mman.h>

struct MockIoPalette
{
  std::unique_ptr<ReadMock>        m_read_uptr{};
  std::unique_ptr<WriteMock>       m_write_uptr{};
  std::unique_ptr<MMapMock>        m_mmap_uptr{};
  std::unique_ptr<MemFDCreateMock> m_memfd_create_uptr{};
  std::unique_ptr<MUnMapMock>      m_munmap_uptr{};
  std::unique_ptr<FTruncateMock>   m_ftruncate_uptr{};
  std::unique_ptr<Open2Mock>       m_open2_uptr{};
  std::unique_ptr<Open3Mock>       m_open3_uptr{};
  std::unique_ptr<CloseMock>       m_close_uptr{};
  std::unique_ptr<LSeekMock>       m_lseek_uptr{};
  std::unique_ptr<FstatMock>       m_fstat_uptr{};

};

void set_palette(std::unique_ptr<MockIoPalette> && ptr);
std::unique_ptr<MockIoPalette> & get_palette();
void reset_palette();

struct MockIoTest : public testing::Test
{
  void SetUp() override
  {
    set_palette(std::make_unique<MockIoPalette>());
    install_defaults();
  }
  void TearDown() override
  {
    reset_palette();
  }

  void set(std::unique_ptr<ReadMock> && arg) { get_palette()->m_read_uptr = std::move(arg); }
  void set(std::unique_ptr<WriteMock> && arg) { get_palette()->m_write_uptr = std::move(arg); }
  void set(std::unique_ptr<MMapMock> && arg) { get_palette()->m_mmap_uptr = std::move(arg); }
  void set(std::unique_ptr<MemFDCreateMock> && arg) { get_palette()->m_memfd_create_uptr = std::move(arg); }
  void set(std::unique_ptr<MUnMapMock> && arg) { get_palette()->m_munmap_uptr = std::move(arg); }
  void set(std::unique_ptr<FTruncateMock> && arg) { get_palette()->m_ftruncate_uptr = std::move(arg); }
  void set(std::unique_ptr<Open2Mock> && arg) { get_palette()->m_open2_uptr = std::move(arg); }
  void set(std::unique_ptr<Open3Mock> && arg) { get_palette()->m_open3_uptr = std::move(arg); }
  void set(std::unique_ptr<CloseMock> && arg) { get_palette()->m_close_uptr = std::move(arg); }
  void set(std::unique_ptr<LSeekMock> && arg) { get_palette()->m_lseek_uptr = std::move(arg); }
  void set(std::unique_ptr<FstatMock> && arg) { get_palette()->m_fstat_uptr = std::move(arg); }
  
  template<typename T>
  auto & install() {
    set(std::make_unique<T>());
    return get<T>();
  }

  template<typename T>
  void install_default() {
    auto & ptr = install<T>();
    EXPECT_CALL(*ptr.get(), target).Times(0);
  }

  void install_defaults() noexcept
  {
    install_default<ReadMock>();
    install_default<WriteMock>();
    install_default<MMapMock>();
    install_default<MemFDCreateMock>();
    install_default<MUnMapMock>();
    install_default<FTruncateMock>();
    install_default<Open2Mock>();
    install_default<Open3Mock>();
    install_default<CloseMock>();
    install_default<LSeekMock>();
    install_default<FstatMock>();
  }

  template<typename T>
  auto & get() {
    if constexpr (std::is_same_v<T, ReadMock>) return get_palette()->m_read_uptr;
    else if constexpr (std::is_same_v<T, WriteMock>) return get_palette()->m_write_uptr;
    else if constexpr (std::is_same_v<T, MMapMock>) return get_palette()->m_mmap_uptr;
    else if constexpr (std::is_same_v<T, MemFDCreateMock>) return get_palette()->m_memfd_create_uptr;
    else if constexpr (std::is_same_v<T, MUnMapMock>) return get_palette()->m_munmap_uptr;
    else if constexpr (std::is_same_v<T, FTruncateMock>) return get_palette()->m_ftruncate_uptr;
    else if constexpr (std::is_same_v<T, Open2Mock>) return get_palette()->m_open2_uptr;
    else if constexpr (std::is_same_v<T, Open3Mock>) return get_palette()->m_open3_uptr;
    else if constexpr (std::is_same_v<T, CloseMock>) return get_palette()->m_close_uptr;
    else if constexpr (std::is_same_v<T, LSeekMock>) return get_palette()->m_lseek_uptr;
    else if constexpr (std::is_same_v<T, FstatMock>) return get_palette()->m_fstat_uptr;
    else static_assert(0 == sizeof(T), "Unknown mock");
  }

  template<typename T, typename N, typename A>
  void expect(N count, A action)
  {
    EXPECT_CALL(*get<T>().get(), target)
      .Times(count)
      .WillRepeatedly(action);
  }
  template<typename T, typename N, typename A>
  void expect(N count, testing::Sequence & s, A action)
  {
    EXPECT_CALL(*get<T>().get(), target)
      .Times(count)
      .InSequence(s)
      .WillRepeatedly(action);
  }
  template<typename T, typename N, typename A, typename... Args>
  void expect(N count, A action, Args... args)
  {
    EXPECT_CALL(*get<T>().get(), target(args...))
      .Times(count)
      .WillRepeatedly(action);
  }
  template<typename T, typename N, typename A, typename... Args>
  void expect(N count, testing::Sequence & s, A action, Args... args)
  {
    EXPECT_CALL(*get<T>().get(), target(args...))
      .Times(count)
      .InSequence(s)
      .WillRepeatedly(action);
  }
};


} // end namespace mg7x::test
#endif
