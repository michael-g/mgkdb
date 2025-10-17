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

#ifndef __mg7x_MgIoMockDefs__H__
#define __mg7x_MgIoMockDefs__H__
#pragma once

// define the MgIoDefs.h guard here to prevent its evaluation
#define __mg7x_MgIoDefs__H__

#include <expected>

#include <gmock/gmock.h>

namespace mg7x::test {

	struct MMapCall
	{
		virtual
		std::expected<void*,int> call(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;
	};

	struct MMapMock : public MMapCall
	{
		MOCK_METHOD6(call, std::expected<void*,int>(void *addr, size_t length, int prot, int flags, int fd, off_t offset));
	};

	struct MemFDCreateCall
	{
		virtual
		std::expected<int,int> call(const char *name, unsigned int flags) = 0;
	};

	struct MemFDCreateMock : public MemFDCreateCall
	{
		MOCK_METHOD2(call, std::expected<int,int>(const char *name, unsigned int flags));
	};

	struct MUnMapCall
	{
		virtual
		std::expected<int,int> call(void *addr, size_t length) = 0;
	};

	struct MUnMapMock : public MUnMapCall
	{
		MOCK_METHOD2(call, std::expected<int,int>(void *addr, size_t length));
	};

	struct FTruncateCall
	{
		virtual
		std::expected<int,int> call(int fd, off_t length) = 0;
	};

	struct FTruncateMock : public FTruncateCall
	{
		MOCK_METHOD2(call, std::expected<int,int>(int fd, off_t length));
	};

	struct CloseCall
	{
		virtual
		std::expected<int,int> call(int fd) = 0;
	};

	struct CloseMock : public CloseCall
	{
		MOCK_METHOD1(call, std::expected<int,int>(int fd));
	};

	MMapCall *mmap_call;
	MemFDCreateCall *memfd_create_call;
	MUnMapCall *munmap_call;
	FTruncateCall *ftruncate_call;
	CloseCall *close_call;
}

namespace mg7x::io {

	using namespace mg7x::test;

	inline
	std::expected<void*,int> mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
	{
		return mmap_call->call(addr, length, prot, flags, fd, offset);
	}

	inline
	std::expected<int,int> memfd_create(const char *name, unsigned int flags)
	{
		return memfd_create_call->call(name, flags);
	}

	inline
	std::expected<int,int> munmap(void *addr, size_t length)
	{
		return munmap_call->call(addr, length);
	}

	inline
	std::expected<int,int> ftruncate(int fd, off_t length)
	{
		return ftruncate_call->call(fd, length);
	}

	inline
	std::expected<int,int> close(int fd)
	{
		return close_call->call(fd);
	}
};

#endif
