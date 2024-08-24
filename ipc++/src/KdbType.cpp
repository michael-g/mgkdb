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

#include "KdbType.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <format>

namespace chr = std::chrono;

namespace mg7x {

namespace time {

static const int64_t NANOS_IN_DAY = 86400000000000L;
static const int64_t NANOS_IN_HOUR = NANOS_IN_DAY / 24;
static const int64_t NANOS_IN_MINUTE = 60000000000L;
static const int64_t NANOS_IN_SECOND =  1000000000L;

static const int32_t MILLIS_IN_SECOND = 1000;
static const int32_t MILLIS_IN_MINUTE = 60 * MILLIS_IN_SECOND;
static const int32_t MILLIS_IN_HOUR = 60 * MILLIS_IN_MINUTE;
static const int32_t MILLIS_IN_DAY = MILLIS_IN_HOUR * 24;

static const int32_t DAYS_SINCE_0AD = 730425;
static const int32_t DAYS_SINCE_EPOCH = 10957;
static const int64_t MILLIS_SINCE_EPOCH = (int64_t)DAYS_SINCE_EPOCH * (int64_t)MILLIS_IN_DAY;

int64_t Timestamp::now()
{
	auto now = chr::time_point_cast<chr::nanoseconds>(chr::system_clock::now());
	int64_t nan = now.time_since_epoch().count();
	return nan - NANOS_IN_DAY * DAYS_SINCE_EPOCH;
}

int64_t Timestamp::getUTC(int32_t y, int32_t m, int32_t d, int32_t h, int32_t u, int32_t v, int64_t n) {
	int64_t nos = n;
	nos += v * NANOS_IN_SECOND;
	nos += u * NANOS_IN_MINUTE;
	nos += h * NANOS_IN_HOUR;
	nos += Date::getDays(y, m, d) * NANOS_IN_DAY;
	return nos;
}

BufIter<32> Timestamp::toDateNotation(BufIter<32> out, int64_t nanos)
{
	long g, y, ddd, mi, mm, dd;
	g = DAYS_SINCE_0AD + nanos / NANOS_IN_DAY;
	y = (10000L * g + 14780L)/3652425L;
	ddd = g - (365*y + y/4 - y/100 + y/400);
	if (ddd < 0) {
		y = y - 1;
		ddd = g - (365*y + y/4 - y/100 + y/400);
	}
	mi = (100*ddd + 52)/3060;
	mm = (mi + 2) % 12 + 1;
	y = y + (mi + 2)/12;
	dd = ddd - (mi*306 + 5)/10 + 1;
	return std::format_to(std::move(out), "{:04}.{:02}.{:02}", y, mm, dd);
}

BufIter<32> Timestamp::toTimeNotation(BufIter<32> out, int64_t nanos)
{
	int64_t h, m, s, n;
	nanos = nanos % NANOS_IN_DAY;
	h = nanos / NANOS_IN_HOUR;
	m = (nanos % NANOS_IN_HOUR) / NANOS_IN_MINUTE;
	s = (nanos % NANOS_IN_MINUTE) / NANOS_IN_SECOND;
	n = nanos % NANOS_IN_SECOND;
	return std::format_to(std::move(out), "{:02}:{:02}:{:02}.{:09}", h, m, s, n);
}

BufIter<32> Timestamp::format_to(BufIter<32> out, int64_t nanos, bool suffix)
{
	if (NULL_LONG == nanos)
		return std::format_to(std::move(out), "0N{}", suffix ? "p": "");

	out = toDateNotation(std::move(out), nanos);
	out = std::format_to(std::move(out), "D");
	return toTimeNotation(std::move(out), nanos);
}

BufIter<16> Month::format_to(BufIter<16> out, int32_t mth, bool suffix)
{
	if (NULL_INT == mth)
		return std::format_to(std::move(out), "0N{}", suffix ? "m" : "");
	return std::format_to(std::move(out), "{:04}.{:02}{}", 2000 + mth / 12, (mth % 12) + 1, suffix ? "m" : "");
}

static constexpr int32_t Y2K_DAYS_OFF = 730425; // 0 A.D. to 1970.01.01 - y2k offset =  719468 - 6h$1970.01.01

int32_t Date::getDays(int32_t y, int32_t m, int32_t d)
{
	// C.f. http://web.archive.org/web/20170507133619/https://alcor.concordia.ca/~gpkatch/gdate-algorithm.html
	int32_t month = (m + 9) % 12;    //mar=0, feb=11
	int32_t year = y - month / 10;   //if Jan/Feb, year--
	return (year * 365 + year / 4 - year / 100 + year / 400 + (month * 306 + 5) / 10 + (d - 1)) - Y2K_DAYS_OFF;
}

BufIter<16> Date::format_to(BufIter<16> out, int32_t date, bool suffix)
{
	if (NULL_INT == date)
		return std::format_to(std::move(out), "0N{}", suffix ? "d" : "");

	int64_t g, y, ddd, mi, mm, dd;
	g = DAYS_SINCE_0AD + date;
	y = (10000L * g + 14780L)/3652425L;
	ddd = g - (365*y + y/4 - y/100 + y/400);
	if (ddd < 0) {
		y = y - 1;
		ddd = g - (365*y + y/4 - y/100 + y/400);
	}
	mi = (100*ddd + 52)/3060;
	mm = (mi + 2) % 12 + 1;
	y = y + (mi + 2)/12;
	dd = ddd - (mi*306 + 5)/10 + 1;

	return std::format_to(std::move(out), "{:4}.{:02}.{:02}", y, mm, dd);
}

int64_t Timespan::now(const std::chrono::time_zone * loc)
{
	auto now = chr::time_point_cast<chr::nanoseconds>(chr::system_clock::now());
	chr::zoned_time const current{loc, now};
	chr::zoned_time const midnight{loc, floor<chr::days>(current.get_local_time())};
	chr::duration const elapsed = current.get_local_time() - midnight.get_local_time();
	return elapsed.count();
}

BufIter<24> Timespan::format_to(BufIter<24> out, int64_t nanos, bool suffix)
{
	if (NULL_LONG == nanos)
		return std::format_to(std::move(out), "0N{}", suffix ? "n" : "");

	int64_t d, h, m, s, n;
	d = nanos / NANOS_IN_DAY;
	nanos = nanos % NANOS_IN_DAY;
	h = nanos / NANOS_IN_HOUR;
	m = (nanos % NANOS_IN_HOUR) / NANOS_IN_MINUTE;
	s = (nanos % NANOS_IN_MINUTE) / NANOS_IN_SECOND;
	n = nanos % NANOS_IN_SECOND;

	return std::format_to(std::move(out), "{:0}D{:02d}:{:02d}:{:02d}.{:09d}", d, h, m, s, n);
}

BufIter<16> Minute::format_to(BufIter<16> out, int32_t val, bool suffix)
{
	if (NULL_INT == val)
		return std::format_to(std::move(out), "0N{}", suffix ? "u" : "");
	return std::format_to(std::move(out), "{:02d}:{:02d}", val / 60, val %60);
}

BufIter<16> Second::format_to(BufIter<16> out, int32_t secs, bool suffix)
{
	if (NULL_INT == secs)
		return std::format_to(std::move(out), "0N{}", suffix ? "v" : "");
	
	int32_t hours, mins;
	hours = secs / 3600;
	secs -= hours * 3600;
	mins = secs / 60;
	secs -= mins * 60;

	return std::format_to(std::move(out), "{:02d}:{:02d}:{:02d}", hours, mins, secs);
}

BufIter<16> Time::format_to(BufIter<16> out, int32_t time, bool suffix)
{
	if (NULL_INT == time)
		return std::format_to(std::move(out), "0N{}", suffix ? "t" : "");
	
	int32_t hours, mins, secs;
	hours = time / MILLIS_IN_HOUR;
	time -= hours * MILLIS_IN_HOUR;
	mins = time / MILLIS_IN_MINUTE;
	time -= mins * MILLIS_IN_MINUTE;
	secs = time / MILLIS_IN_SECOND;
	time -= secs * MILLIS_IN_SECOND;

	return std::format_to(std::move(out), "{:02}:{:02}:{:02}.{:03}", hours, mins, secs, time);
}

} // end namespace mg7x::time

//--------------------------------------------------------------------------------------- ReadBuf
ReadBuf::ReadBuf(const int8_t *src, uint64_t len)
 : ReadBuf(src, len, 0)
{}

ReadBuf::ReadBuf(const int8_t *src, uint64_t len, int64_t csr)
 : m_src(src)
 , m_len(len)
 , m_csr(csr)
{
}

size_t ReadBuf::adj(size_t adj)
{
	m_off += adj;
	m_csr += adj;
	return adj;
}

template<typename T>
T ReadBuf::read()
{
	T const *src = reinterpret_cast<T const*>(m_src + m_off);
	T val{*src};
	adj(sizeof(T));
	return val;
}

template<typename T>
T ReadBuf::peek() const
{
	T const *src = reinterpret_cast<T const*>(m_src + m_off);
	return *src;
}

template<typename T>
uint64_t ReadBuf::read(T ary[], uint64_t count) {
	const uint64_t cpy = count * sizeof(T);
	memcpy(ary, &m_src[m_off], cpy);
	return adj(cpy);
}

template<typename T>
uint64_t ReadBuf::read(std::vector<T> & vec, const uint64_t count)
{
	size_t rqd = sizeof(T) * count;
	if (remaining() < rqd)
		return 0;

	const T* src = reinterpret_cast<const T*>(m_src + m_off);
	vec.insert(vec.begin() + vec.size(), src, src + count);
	adj(sizeof(T) * count);
	return count;
}

ReadResult ReadBuf::readSym(std::string & str) {
	const uint64_t rem = remaining();
	if (rem > 0) {
		const char *src = reinterpret_cast<const char*>(m_src + m_off);
		size_t str_len = strnlen(src, rem);
		if (rem == str_len)
			return ReadResult::RD_INCOMPLETE;
		str.assign(src, str_len);
		adj(str_len + SZ_BYTE);
		return ReadResult::RD_OK;
	}
	return ReadResult::RD_INCOMPLETE;
}

ReadResult ReadBuf::readSyms(size_t rqd, std::vector<struct LocInfo> & locs, std::vector<char> & data)
{
	uint64_t rem = remaining();
	if (rem > 0) {
		for (size_t i = 0 ; i < rqd ; i++) {
			const char *src = reinterpret_cast<const char*>(m_src + m_off);
			size_t str_len = strnlen(src, rem);
			if (rem == str_len)
				return ReadResult::RD_INCOMPLETE;

			// don't include the null byte in locs.c_len so you have an easier time making a string_view
			locs.emplace_back(data.size(), str_len + SZ_BYTE);
			data.insert(data.end(), src, src + str_len + SZ_BYTE);

			rem -= str_len + SZ_BYTE;
			adj(str_len + SZ_BYTE);
		}
		return ReadResult::RD_OK;
	}
	return ReadResult::RD_INCOMPLETE;
}

//--------------------------------------------------------------------------------------- WriteBuf
WriteBuf::WriteBuf(void *dst, uint64_t cap)
 : WriteBuf(dst, cap, 0)
{
}

WriteBuf::WriteBuf(void *dst, uint64_t cap, int64_t csr)
 : m_dst(static_cast<int8_t*>(dst))
 , m_cap(cap)
 , m_csr(csr)
{}

size_t WriteBuf::adj(size_t bytes)
{
	m_off += bytes;
	m_csr += bytes;
	return bytes;
}

bool WriteBuf::writeTyp(KdbType typ)
{
	return write<int8_t>(static_cast<int8_t>(typ));
}

bool WriteBuf::writeAtt(KdbAttr att)
{
	return write<int8_t>(static_cast<int8_t>(att));
}

bool WriteBuf::writeHdr(KdbType typ, KdbAttr att, int32_t m_len)
{
	if (remaining() < SZ_VEC_HDR)
		return false;
	write<int8_t>(static_cast<int8_t>(typ));
	write<int8_t>(static_cast<int8_t>(att));
	write<int32_t>(m_len);
	return true;
}

template <typename T>
bool WriteBuf::write(T val)
{
	const size_t SZ = sizeof(T);
	if (remaining() < SZ)
		return false;

	T *dst = reinterpret_cast<T*>(m_dst + m_off);
	*dst = val;
	adj(SZ);
	return true;
}

bool WriteBuf::writeSym(const std::string_view sv)
{
	const size_t RQD = sv.length() + SZ_BYTE;
	if (remaining() < RQD)
		return false;

	char *dst = reinterpret_cast<char*>(m_dst + m_off);
	std::copy(sv.data(), sv.data() + sv.length(), dst);

	adj(sv.length());

	m_dst[m_off++] = 0;
	m_csr += 1;

	return true;
}

template<typename T>
size_t WriteBuf::writeAry(const T ary[], size_t off, size_t cap)
{
	if (off >= cap)
		return 0;

	const size_t elm = std::min(cap - off, remaining() / sizeof(T));

	T *dst = reinterpret_cast<T*>(m_dst + m_off);

	std::copy(ary + off, ary + elm, dst);

	adj(elm * sizeof(T));

	return elm;
}

//--------------------------------------------------------------------------------------- newInstance
template<class T>
ReadResult newAtomFromType(ReadBuf & buf, KdbBase **ptr)
{
	std::ignore = buf.read<int8_t>();
	*ptr = new T{};
	return ReadResult::RD_OK;
}

template<class T>
ReadResult newVec32(ReadBuf & buf, KdbBase **ptr)
{
	if (!buf.canRead(SZ_VEC_HDR))
		return ReadResult::RD_INCOMPLETE;
	
	std::ignore = buf.read<int8_t>();
	int8_t att = buf.read<int8_t>();
	int32_t len = buf.read<int32_t>();

	*ptr = new T{static_cast<uint64_t>(len), static_cast<KdbAttr>(att)};
	return ReadResult::RD_OK;
}

ReadResult newInstance(ReadBuf & buf, KdbBase **ptr)
{
	if (buf.cursorActive()) {
		if (!buf.canRead(SZ_BYTE))
			return ReadResult::RD_INCOMPLETE;

		const int8_t typ_i8 = buf.peek<int8_t>();
		const KdbType typ = static_cast<KdbType>(typ_i8);

		switch (typ) {
			case KdbType::EXCEPTION:           return newAtomFromType<KdbException>(buf, ptr);
			case KdbType::TIME_ATOM:           return newAtomFromType<KdbTimeAtom>(buf, ptr);
			case KdbType::SECOND_ATOM:         return newAtomFromType<KdbSecondAtom>(buf, ptr);
			case KdbType::MINUTE_ATOM:         return newAtomFromType<KdbMinuteAtom>(buf, ptr);
			case KdbType::TIMESPAN_ATOM:       return newAtomFromType<KdbTimespanAtom>(buf, ptr);
			case KdbType::DATE_ATOM:           return newAtomFromType<KdbDateAtom>(buf, ptr);
			case KdbType::MONTH_ATOM:          return newAtomFromType<KdbMonthAtom>(buf, ptr);
			case KdbType::TIMESTAMP_ATOM:      return newAtomFromType<KdbTimestampAtom>(buf, ptr);
			case KdbType::SYMBOL_ATOM:         return newAtomFromType<KdbSymbolAtom>(buf, ptr);
			case KdbType::CHAR_ATOM:           return newAtomFromType<KdbCharAtom>(buf, ptr);
			case KdbType::FLOAT_ATOM:          return newAtomFromType<KdbFloatAtom>(buf, ptr);
			case KdbType::REAL_ATOM:           return newAtomFromType<KdbRealAtom>(buf, ptr);
			case KdbType::LONG_ATOM:           return newAtomFromType<KdbLongAtom>(buf, ptr);
			case KdbType::INT_ATOM:            return newAtomFromType<KdbIntAtom>(buf, ptr);
			case KdbType::SHORT_ATOM:          return newAtomFromType<KdbShortAtom>(buf, ptr);
			case KdbType::BYTE_ATOM:           return newAtomFromType<KdbByteAtom>(buf, ptr);
			case KdbType::GUID_ATOM:           return newAtomFromType<KdbGuidAtom>(buf, ptr);
			case KdbType::BOOL_ATOM:           return newAtomFromType<KdbBoolAtom>(buf, ptr);
			case KdbType::LIST:                return newVec32<KdbList>(buf, ptr);
			case KdbType::BOOL_VECTOR:         return newVec32<KdbBoolVector>(buf, ptr);
			case KdbType::GUID_VECTOR:         return newVec32<KdbGuidVector>(buf, ptr);
			case KdbType::BYTE_VECTOR:         return newVec32<KdbByteVector>(buf, ptr);
			case KdbType::SHORT_VECTOR:        return newVec32<KdbShortVector>(buf, ptr);
			case KdbType::INT_VECTOR:          return newVec32<KdbIntVector>(buf, ptr);
			case KdbType::LONG_VECTOR:         return newVec32<KdbLongVector>(buf, ptr);
			case KdbType::REAL_VECTOR:         return newVec32<KdbRealVector>(buf, ptr);
			case KdbType::FLOAT_VECTOR:        return newVec32<KdbFloatVector>(buf, ptr);
			case KdbType::CHAR_VECTOR:         return newVec32<KdbCharVector>(buf, ptr);
			case KdbType::SYMBOL_VECTOR:       return newVec32<KdbSymbolVector>(buf, ptr);
			case KdbType::TIMESTAMP_VECTOR:    return newVec32<KdbTimestampVector>(buf, ptr);
			case KdbType::MONTH_VECTOR:        return newVec32<KdbMonthVector>(buf, ptr);
			case KdbType::DATE_VECTOR:         return newVec32<KdbDateVector>(buf, ptr);
			case KdbType::TIMESPAN_VECTOR:     return newVec32<KdbTimespanVector>(buf, ptr);
			case KdbType::MINUTE_VECTOR:       return newVec32<KdbMinuteVector>(buf, ptr);
			case KdbType::SECOND_VECTOR:       return newVec32<KdbSecondVector>(buf, ptr);
			case KdbType::TIME_VECTOR:         return newVec32<KdbTimeVector>(buf, ptr);
			case KdbType::TABLE:               return KdbTable::alloc(buf, ptr);
			case KdbType::DICT:                return newAtomFromType<KdbDict>(buf, ptr);
			case KdbType::FUNCTION:            return KdbFunction::alloc(buf, ptr);
			case KdbType::UNARY_PRIMITIVE:     return newAtomFromType<KdbUnaryPrimitive>(buf, ptr);
			case KdbType::BINARY_PRIMITIVE:    return newAtomFromType<KdbBinaryPrimitive>(buf, ptr);
			case KdbType::TERNARY_PRIMITIVE:   return newAtomFromType<KdbTernaryPrimitive>(buf, ptr);
			case KdbType::PROJECTION:          return KdbProjection::alloc(buf, ptr);

			default:
				std::cerr << "'nyi atom-type " << static_cast<int32_t>(typ_i8);
				return ReadResult::RD_ERR_LOGIC;
		}
		return ReadResult::RD_OK;
	}
	if (nullptr == *ptr)
		return ReadResult::RD_ERR_LOGIC; // TODO log
	return (*ptr)->read(buf);
}
//--------------------------------------------------------------------------------------- new-sequence
static KdbBase* new1dSequence(KdbType typ, uint64_t cap, KdbAttr attr = KdbAttr::NONE)
{
	switch (typ) {
		case KdbType::LIST:                return new KdbList(cap, attr);
		case KdbType::BOOL_VECTOR:         return new KdbBoolVector{cap, attr};
		case KdbType::GUID_VECTOR:         return new KdbGuidVector{cap, attr};
		case KdbType::BYTE_VECTOR:         return new KdbByteVector{cap, attr};
		case KdbType::SHORT_VECTOR:        return new KdbShortVector{cap, attr};
		case KdbType::INT_VECTOR:          return new KdbIntVector{cap, attr};
		case KdbType::LONG_VECTOR:         return new KdbLongVector{cap, attr};
		case KdbType::REAL_VECTOR:         return new KdbRealVector{cap, attr};
		case KdbType::FLOAT_VECTOR:        return new KdbFloatVector{cap, attr};
		case KdbType::CHAR_VECTOR:         return new KdbCharVector{cap, attr};
		case KdbType::SYMBOL_VECTOR:       return new KdbSymbolVector(cap, attr);
		case KdbType::TIMESTAMP_VECTOR:    return new KdbTimestampVector{cap, attr};
		case KdbType::MONTH_VECTOR:        return new KdbMonthVector{cap, attr};
		case KdbType::DATE_VECTOR:         return new KdbDateVector{cap, attr};
		case KdbType::TIMESPAN_VECTOR:     return new KdbTimespanVector{cap, attr};
		case KdbType::MINUTE_VECTOR:       return new KdbMinuteVector{cap, attr};
		case KdbType::SECOND_VECTOR:       return new KdbSecondVector{cap, attr};
		case KdbType::TIME_VECTOR:         return new KdbTimeVector(cap, attr);
		default:
			throw std::format("'nyi sequence-type {}h", static_cast<int>(typ));
	}
}

static KdbBase* new1dSequence(char typ, uint64_t cap, KdbAttr attr = KdbAttr::NONE)
{
	switch (typ) {
		case ' ': [[fallthrough]];
		case '*': return new1dSequence(KdbType::LIST, cap, attr);
		case 'b': return new1dSequence(KdbType::BOOL_VECTOR, cap, attr);
		case 'g': return new1dSequence(KdbType::GUID_VECTOR, cap, attr);
		case 'x': return new1dSequence(KdbType::BYTE_VECTOR, cap, attr);
		case 'h': return new1dSequence(KdbType::SHORT_VECTOR, cap, attr);
		case 'i': return new1dSequence(KdbType::INT_VECTOR, cap, attr);
		case 'j': return new1dSequence(KdbType::LONG_VECTOR, cap, attr);
		case 'e': return new1dSequence(KdbType::REAL_VECTOR, cap, attr);
		case 'f': return new1dSequence(KdbType::FLOAT_VECTOR, cap, attr);
		case 'c': return new1dSequence(KdbType::CHAR_VECTOR, cap, attr);
		case 's': return new1dSequence(KdbType::SYMBOL_VECTOR, cap, attr);
		case 'p': return new1dSequence(KdbType::TIMESTAMP_VECTOR, cap, attr);
		case 'm': return new1dSequence(KdbType::MONTH_VECTOR, cap, attr);
		case 'd': return new1dSequence(KdbType::DATE_VECTOR, cap, attr);
		case 'n': return new1dSequence(KdbType::TIMESPAN_VECTOR, cap, attr);
		case 'u': return new1dSequence(KdbType::MINUTE_VECTOR, cap, attr);
		case 'v': return new1dSequence(KdbType::SECOND_VECTOR, cap, attr);
		case 't': return new1dSequence(KdbType::TIME_VECTOR, cap, attr);
		default:
			throw std::format("'nyi sequence-type '{}'", typ);
	}
}

//-------------------------------------------------------------------------------- Constraints
template <typename T>
	concept KdbObj = requires(T typ, ReadBuf & buf)
{
	// { typ.m_typ } -> std::same_as<const KdbType&>;
	// C.f. https://stackoverflow.com/a/74474681
	requires std::same_as<decltype(typ.m_typ), const KdbType>;
	// from C++20: {auto(typ.m_typ)}->std::same_as<const KdbType>;
	{ typ.read(buf) } -> std::same_as<ReadResult>;
};

template <typename T>
	concept IsKdbSequence = KdbObj<T> &&
requires(T typ)
{
	{ typ.isSequence() } -> std::same_as<bool>;
	{ true == typ.isSequence() };
	{ typ.count() } -> std::same_as<size_t>;
};

template <typename T>
	concept KdbAtom = KdbObj<T> && 
requires(T typ)
{
	{ typ.isSequence() } -> std::same_as<bool>;
	{ true != typ.isSequence() };
};

template <typename T>
	concept KdbReadableAtom = requires(T typ, ReadBuf & buf)
{
	requires std::same_as<decltype(typ.m_typ), const KdbType>; // from C++20: {auto(typ.m_typ)}->std::same_as<const KdbType>;
	std::is_integral<decltype(typ.m_val)>::value;
	{ typ.read(buf) } -> std::same_as<ReadResult>;
};

template <typename T>
	concept KdbWritableAtom = requires(const T typ, WriteBuf & buf)
{
	requires std::same_as<decltype(typ.m_typ), const KdbType>;
	{ typ.write(buf) } -> std::same_as<WriteResult>;
	std::is_integral<const decltype(typ.m_val)>::value;
};

template <typename T>
	concept KdbVectorReader = requires(T typ, ReadBuf & buf)
{
	// requires std::is_base_of<KdbBase, T>::value;
	requires std::same_as<decltype(typ.m_typ), const KdbType>;
	// std::same_as<std::vector<E>, decltype(typ.m_vec)>;
	requires std::same_as<std::vector<typename std::decay<decltype(*typ.m_vec.begin())>::type>, decltype(typ.m_vec)>;
	{ typ.read(buf) } -> std::same_as<ReadResult>;
};

template <typename T>
	concept KdbVectorWriter = requires(const T typ, WriteBuf & buf)
{
	requires std::same_as<decltype(typ.m_typ), const KdbType>;
	requires std::same_as<decltype(typ.m_attr), KdbAttr>;
	requires std::same_as<std::vector<typename std::decay<decltype(*typ.m_vec.begin())>::type>, decltype(typ.m_vec)>;
	{ typ.write(buf) } -> std::same_as<WriteResult>;
};
//-------------------------------------------------------------------------------- read/write RegularAtom
template <KdbReadableAtom T>
ReadResult readIntAtom(T & typ, ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE);

	if (buf.cursorActive()) {
		if (buf.remaining() < sizeof(typ.m_val))
			return ReadResult::RD_INCOMPLETE;
		typ.m_val = buf.read<decltype(typ.m_val)>();
	}
	else {
		buf.ffwd(sizeof(typ.m_val));
	}
	return ReadResult::RD_OK;
}

template <KdbWritableAtom T>
WriteResult writeIntAtom(T & typ, WriteBuf &buf)
{
	using E = decltype(typ.m_val);

	const size_t SZ_ATM = SZ_BYTE + sizeof(E);
	if (!buf.cursorActive()) {
		buf.ffwd(SZ_ATM);
	}
	else if (!buf.canWrite(SZ_ATM)) {
		return WriteResult::WR_INCOMPLETE;
	}
	else {
		buf.writeTyp(typ.m_typ);
		buf.write<E>(typ.m_val);
	}
	return WriteResult::WR_OK;
}
//-------------------------------------------------------------------------------- read/write RegularVector
template<KdbVectorReader T>
ReadResult readIntVector(T & typ, ReadBuf & buf)
{
	using E = std::decay<decltype(*typ.m_vec.begin())>::type; // https://stackoverflow.com/a/53259979/322304

	if (!buf.cursorActive())
		buf.ffwd(SZ_VEC_HDR);

	const size_t cap = typ.m_vec.capacity();
	const size_t len = typ.m_vec.size();
	const uint64_t rqd = cap * sizeof(E);

	buf.ffwd(sizeof(E) * len);
	if (len == cap)
		return ReadResult::RD_OK;	

	const uint64_t elm = std::min(cap - len, buf.remaining() / sizeof(E));

	if (elm > 0) {
		if (elm != buf.read(typ.m_vec, elm))
			return ReadResult::RD_INCOMPLETE;
	}

	return (cap == typ.m_vec.size()) ? ReadResult::RD_OK : ReadResult::RD_INCOMPLETE;
}

template <KdbVectorWriter T>
WriteResult writeIntVector(const T & typ, WriteBuf & buf)
{
	using E = std::decay<decltype(*typ.m_vec.begin())>::type;

	if (buf.cursorActive()) {
		if (!buf.writeHdr(typ.m_typ, typ.m_attr, typ.m_vec.size()))
			return WriteResult::WR_INCOMPLETE;
	}
	else {
		buf.ffwd(SZ_VEC_HDR);
	}

	int64_t off = buf.cursorOff();
	size_t skp = off < 0 ? -off / sizeof(E) : 0;
	if (skp > 0) {
		buf.ffwd(skp * sizeof(E));
	}

	if (typ.m_vec.size() > skp) {
		size_t rit = buf.writeAry(typ.m_vec.data(), skp, typ.m_vec.size());
		if (skp + rit < typ.m_vec.size())
			return WriteResult::WR_INCOMPLETE;
	}

	return WriteResult::WR_OK;
}
//-------------------------------------------------------------------------------- KdbBase
KdbBase::~KdbBase()
{
}

//-------------------------------------------------------------------------------- KdbException
ReadResult KdbException::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE);

	if (buf.cursorActive())
		return buf.readSym(m_msg);

	buf.ffwd(m_msg.length() + SZ_BYTE);
	return ReadResult::RD_OK;
}

WriteResult KdbException::write(WriteBuf & buf) const
{
	const size_t SZ_ATM = SZ_BYTE + m_msg.length() + SZ_BYTE;
	if (buf.cursorActive()) {
		if (!buf.canWrite(SZ_ATM))
			return WriteResult::WR_INCOMPLETE;
		buf.writeTyp(m_typ);
		buf.writeSym(m_msg);
	}
	else {
		buf.ffwd(SZ_ATM);
	}
	return WriteResult::WR_OK;
}
//-------------------------------------------------------------------------------- KdbTimeAtom
ReadResult KdbTimeAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbTimeAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbSecondAtom
ReadResult KdbSecondAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbSecondAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbMinuteAtom
ReadResult KdbMinuteAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbMinuteAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbTimespanAtom
ReadResult KdbTimespanAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbTimespanAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbDateAtom
ReadResult KdbDateAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbDateAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbMonthAtom
ReadResult KdbMonthAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbMonthAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbTimestampAtom
ReadResult KdbTimestampAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbTimestampAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbSymbolAtom
ReadResult KdbSymbolAtom::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE);

	if (buf.cursorActive())
		return buf.readSym(m_val);

	buf.ffwd(m_val.length() + SZ_BYTE);
	return ReadResult::RD_OK;
}

WriteResult KdbSymbolAtom::write(WriteBuf & buf) const
{
	const size_t SZ_ATM = SZ_BYTE + m_val.length() + SZ_BYTE;
	if (!buf.cursorActive()) {
		buf.ffwd(SZ_ATM);
	}
	else if (!buf.canWrite(SZ_ATM)) {
		return WriteResult::WR_INCOMPLETE;
	}
	else {
		buf.writeTyp(m_typ);
		buf.writeSym(m_val);
	}
	return WriteResult::WR_OK;
}
//-------------------------------------------------------------------------------- KdbCharAtom
ReadResult KdbCharAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbCharAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbFloatAtom
ReadResult KdbFloatAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbFloatAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbRealAtom
ReadResult KdbRealAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbRealAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbLongAtom
ReadResult KdbLongAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbLongAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbIntAtom
ReadResult KdbIntAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbIntAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbShortAtom
ReadResult KdbShortAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbShortAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbByteAtom
ReadResult KdbByteAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbByteAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbGuidAtom
ReadResult KdbGuidAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbGuidAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbBoolAtom
ReadResult KdbBoolAtom::read(ReadBuf & buf)
{
	return readIntAtom<decltype(*this)>(*this, buf);
}

WriteResult KdbBoolAtom::write(WriteBuf & buf) const
{
	return writeIntAtom<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbList
KdbList::KdbList(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::LIST)
 , m_attr(attr)
 , m_vals(cap)
{
	m_vals.resize(0);
}

KdbList::~KdbList()
{
	m_vals.clear();
}

void KdbList::push(std::unique_ptr<KdbBase> val)
{
	m_vals.emplace_back(val.release());
}

void KdbList::push(KdbBase & val)
{
	m_vals.emplace_back(val);
}

const KdbBase* KdbList::getObj(size_t idx) const
{
	return m_vals[idx].m_ptr;
}

KdbType KdbList::typeAt(size_t idx) const
{
	return m_vals[idx].m_ptr->m_typ;
}

uint64_t KdbList::wireSz() const
{
	uint64_t sz = SZ_VEC_HDR;
	for (size_t i = 0 ; i < m_vals.size() ; i++) {
		sz += m_vals[i].m_ptr->wireSz();
	}
	return sz;
}

ReadResult KdbList::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE + SZ_VEC_META);

	ReadResult rr;
	for (size_t i = 0 ; i < m_vals.size() ; i++) {
		rr = m_vals[i].m_ptr->read(buf);
		if (ReadResult::RD_OK != rr)
			return rr;
	}

	const size_t cap = m_vals.capacity();
	for (size_t i = m_vals.size() ; i < cap ; i++) {
		KdbBase *obj;
		rr = newInstance(buf, &obj);
		if (ReadResult::RD_OK != rr)
			return rr;
		m_vals.push_back(obj);
		rr = m_vals[i].m_ptr->read(buf);
		if (rr != ReadResult::RD_OK)
			return rr;
	}
	return ReadResult::RD_OK;
}

WriteResult KdbList::write(WriteBuf & buf) const
{
	if (buf.cursorActive()) {
		if (!buf.writeHdr(m_typ, m_attr, m_vals.size()))
			return WriteResult::WR_INCOMPLETE;
	}
	else {
		buf.ffwd(SZ_VEC_HDR);
	}

	for (uint64_t i = 0 ; i < m_vals.size() ; i++) {
		WriteResult wr = m_vals[i].m_ptr->write(buf);
		if (WriteResult::WR_OK != wr)
			return wr;
	}

	return WriteResult::WR_OK;
}

//-------------------------------------------------------------------------------- Vector functions
template <typename T> void _vec_set_value(T & vec, uint64_t idx, typename T::value_type val)
{
	if (idx > vec.size())
		throw std::format("idx.oob: cannot insert {} at {}, vec.size is {}", val, idx, vec.size());
	if (idx == vec.size())
		vec.insert(vec.end(), val);
	else
		vec[idx] = val;
}
template <typename T> uint64_t _vec_wire_sz(T vec)
{
	return SZ_VEC_HDR + vec.size() * sizeof(typename decltype(vec)::value_type);
}
//-------------------------------------------------------------------------------- KdbBoolVector
KdbBoolVector::KdbBoolVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::BOOL_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbBoolVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbBoolVector::setBool(uint64_t idx, bool val)
{
	_vec_set_value(m_vec, idx, static_cast<int8_t>(val ? 1 : 0));
}

ReadResult KdbBoolVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbBoolVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//--------------------------------------------------------------------------------------- KdbGuidVector
KdbGuidVector::KdbGuidVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::GUID_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbGuidVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbGuidVector::setGuid(uint64_t idx, const GuidType & val)
{
	if (idx > m_vec.size())
		throw "idx.oob";

	if (idx == m_vec.size()) {
		m_vec.push_back(val);
	}
	else {
		memcpy(&m_vec[idx], val.data(), val.size());
	}
}

ReadResult KdbGuidVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbGuidVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbByteVector
KdbByteVector::KdbByteVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::BYTE_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbByteVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbByteVector::setByte(uint64_t idx, int8_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbByteVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbByteVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbShortVector
KdbShortVector::KdbShortVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::SHORT_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbShortVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbShortVector::setShort(uint64_t idx, int16_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbShortVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbShortVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbIntVector
KdbIntVector::KdbIntVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::INT_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

KdbIntVector::KdbIntVector(const std::vector<int32_t> & vals, KdbAttr attr)
 : KdbBase(KdbType::INT_VECTOR)
 , m_attr(attr)
 , m_vec(vals)
{
}

uint64_t KdbIntVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbIntVector::setInt(uint64_t idx, int32_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbIntVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbIntVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbLongVector
KdbLongVector::KdbLongVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::LONG_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbLongVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbLongVector::setLong(uint64_t idx, int64_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbLongVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbLongVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbRealVector
KdbRealVector::KdbRealVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::REAL_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbRealVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbRealVector::setReal(uint64_t idx, float val)
{
	_vec_set_value(m_vec, idx, std::bit_cast<int32_t>(val));
}

ReadResult KdbRealVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbRealVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbFloatVector
KdbFloatVector::KdbFloatVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::FLOAT_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbFloatVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbFloatVector::setFloat(uint64_t idx, double val)
{
	_vec_set_value(m_vec, idx, std::bit_cast<int64_t>(val));
}

ReadResult KdbFloatVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbFloatVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbCharVector
KdbCharVector::KdbCharVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::CHAR_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

KdbCharVector::KdbCharVector(const std::string_view & src)
 : KdbBase(KdbType::CHAR_VECTOR)
 , m_attr(KdbAttr::NONE)
 , m_vec{src.begin(), src.end()}
{
}

uint64_t KdbCharVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbCharVector::setChar(uint64_t idx, char val)
{
	_vec_set_value(m_vec, idx, std::bit_cast<uint8_t>(val));
}

void KdbCharVector::setString(const std::string_view & val)
{
	m_vec.reserve(val.size());
	m_vec.resize(0);
	std::copy(val.begin(), val.end(), std::back_inserter(m_vec));
}

std::string_view KdbCharVector::getString() const
{
	return std::string_view{reinterpret_cast<const char*>(m_vec.data()), m_vec.size()};
}

ReadResult KdbCharVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbCharVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbSymbolVector
KdbSymbolVector::KdbSymbolVector(const std::vector<std::string_view> & vals)
 : KdbBase(KdbType::SYMBOL_VECTOR)
 , m_locs(vals.size())
 , m_data([](const std::vector<std::string_view> & v) {
		size_t sz = 0;
		for (uint64_t i = 0 ; i < v.size() ; i++) {
			sz += v[i].size() + SZ_BYTE;
		}
		return sz;
 }(vals))
{
	m_locs.resize(0);
	m_data.resize(0);
	for (uint64_t i = 0 ; i < vals.size() ; i++) {
		size_t off = m_data.size();
		size_t len = vals[i].size() + SZ_BYTE;
		m_locs.emplace_back(off, len);
		m_data.insert(m_data.end(), vals[i].data(), vals[i].data() + vals[i].size());
		// since we can't be sure that each string_view is null-terminated:
		m_data.push_back('\0');
	}
}
KdbSymbolVector::KdbSymbolVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::SYMBOL_VECTOR)
 , m_attr(attr)
 , m_locs(cap)
 , m_data(cap * 8)
{
	m_locs.resize(0);
	m_data.resize(0);
}

const std::string_view KdbSymbolVector::getString(size_t idx) const
{
	if (idx >= m_locs.size())
		throw "idx oob";
	return std::string_view{m_data.data() + m_locs[idx].c_off, m_locs[idx].c_len - 1};
}

int32_t KdbSymbolVector::indexOf(const std::string_view & col) const
{
	for (size_t i = 0 ; i < m_locs.size() ; i++) {
		std::string_view sv{m_data.data() + m_locs[i].c_off, m_locs[i].c_len - 1};
		if (sv == col) {
			return i;
		}
	}
	return -1;
}

void KdbSymbolVector::push(const std::string_view & sym)
{
	const size_t off = m_data.size();
	const size_t len = sym.size() + SZ_BYTE;
	m_locs.emplace_back(off, len);
	m_data.insert(m_data.end(), sym.data(), sym.data() + sym.size());
	m_data.push_back('\0');
}

uint64_t KdbSymbolVector::wireSz() const
{
	return SZ_VEC_HDR + m_data.size();
}

ReadResult KdbSymbolVector::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE + SZ_VEC_META);
	
	const size_t cap = m_locs.capacity();
	const size_t len = m_locs.size();
	const size_t ncd = m_data.size();

	// invariant: if len < cap then 'off' should be >= 0
	const size_t rqd = cap - len;

	buf.ffwd(ncd);
	if (len == cap)
		return ReadResult::RD_OK;

	return buf.readSyms(rqd, m_locs, m_data);
}

WriteResult KdbSymbolVector::write(WriteBuf & buf) const
{
	if (buf.cursorActive()) {
		if (!buf.writeHdr(m_typ, m_attr, m_locs.size()))
			return WriteResult::WR_INCOMPLETE;
	}
	else {
		buf.ffwd(SZ_VEC_HDR);
	}
	
	size_t skp = 0;

	if (int64_t off = buf.cursorOff(); off < 0) {
		skp = std::min(static_cast<size_t>(-off), m_data.size());
		buf.ffwd(skp);
	}

	if (skp < m_data.size()) {
		size_t rit = buf.writeAry(m_data.data(), skp, m_data.size());
		if (rit + skp == m_data.size())
			return WriteResult::WR_OK;
	}
	return WriteResult::WR_OK;
}

//-------------------------------------------------------------------------------- KdbTimestampVector
KdbTimestampVector::KdbTimestampVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::TIMESTAMP_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbTimestampVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbTimestampVector::setTimestamp(uint64_t idx, int64_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbTimestampVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbTimestampVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbMonthVector
KdbMonthVector::KdbMonthVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::MONTH_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbMonthVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbMonthVector::setMonth(uint64_t idx, int32_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbMonthVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbMonthVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbDateVector
KdbDateVector::KdbDateVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::DATE_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbDateVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbDateVector::setDate(uint64_t idx, int32_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbDateVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbDateVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbTimespanVector
KdbTimespanVector::KdbTimespanVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::TIMESPAN_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbTimespanVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbTimespanVector::setTimespan(uint64_t idx, int64_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbTimespanVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbTimespanVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbMinuteVector
KdbMinuteVector::KdbMinuteVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::MINUTE_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbMinuteVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbMinuteVector::setMinute(uint64_t idx, int32_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbMinuteVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbMinuteVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbSecondVector
KdbSecondVector::KdbSecondVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::SECOND_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbSecondVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbSecondVector::setSecond(uint64_t idx, int32_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbSecondVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbSecondVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbTimeVector
KdbTimeVector::KdbTimeVector(uint64_t cap, KdbAttr attr)
 : KdbBase(KdbType::TIME_VECTOR)
 , m_attr(attr)
 , m_vec(cap)
{
	m_vec.resize(0);
}

uint64_t KdbTimeVector::wireSz() const
{
	return _vec_wire_sz(m_vec);
}

void KdbTimeVector::setMillis(uint64_t idx, int32_t val)
{
	_vec_set_value(m_vec, idx, val);
}

ReadResult KdbTimeVector::read(ReadBuf & buf)
{
	return readIntVector<decltype(*this)>(*this, buf);
}

WriteResult KdbTimeVector::write(WriteBuf & buf) const
{
	return writeIntVector<decltype(*this)>(*this, buf);
}

//-------------------------------------------------------------------------------- KdbTable

ReadResult KdbTable::alloc(ReadBuf & buf, KdbBase **ptr)
{
	if (!buf.canRead(SZ_BYTE + SZ_BYTE + SZ_BYTE))
		return ReadResult::RD_INCOMPLETE;
	std::ignore = buf.read<int8_t>();
	std::ignore = buf.read<int8_t>(); // some sort of attr
	std::ignore = buf.read<int8_t>();
	*ptr = new KdbTable{};
	return ReadResult::RD_OK;
}

KdbTable::KdbTable(const std::string_view & typs, const std::vector<std::string_view> & cols)
 : KdbBase(KdbType::TABLE)
 , m_cols(std::make_unique<KdbSymbolVector>(cols.size()))
 , m_vals(std::make_unique<KdbList>(cols.size()))
{
	if (typs.size() != cols.size()) {
		throw "types.length != cols.length";
	}
	if (cols.size() == 0) {
		throw "names.size == 0";
	}

	// TODO what if any([0>x for x in typs])
	
	for (size_t i = 0 ; i < cols.size() ; i++) {
		m_cols->push(cols[i]);
		m_vals->push(std::unique_ptr<KdbBase>(new1dSequence(typs[i], 0)));
	}
}

uint64_t KdbTable::count() const
{
	if (!m_vals)
		return 0;
	return m_vals->count() == 0 ? 0 : m_vals->getObj(0)->count();
}

uint64_t KdbTable::wireSz() const
{
	return SZ_BYTE + SZ_BYTE + SZ_BYTE + m_cols->wireSz() + m_vals->wireSz();
}

ReadResult KdbTable::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE + SZ_BYTE + SZ_BYTE);

	ReadResult rr;
	if (buf.cursorActive()) {
		KdbBase *ptr;
		rr = newInstance(buf, &ptr);
		if (ReadResult::RD_OK != rr)
			return rr;
		if (ptr->m_typ != KdbType::SYMBOL_VECTOR)
			return ReadResult::RD_ERR_IPC; // TODO log why
		m_cols.reset(dynamic_cast<KdbSymbolVector*>(ptr));
	}

	rr = m_cols->read(buf);
	if (ReadResult::RD_OK != rr)
		return rr;
	
	if (buf.cursorActive()) {
		KdbBase *ptr;
		rr = newInstance(buf, &ptr);
		if (ReadResult::RD_OK != rr)
			return rr;
		if (ptr->m_typ != KdbType::LIST)
			return ReadResult::RD_ERR_IPC; // TODO log
		m_vals.reset(dynamic_cast<KdbList*>(ptr));
	}

	return m_vals->read(buf);
}

WriteResult KdbTable::write(WriteBuf & buf) const
{
	const size_t SZ_TBL_HDR = SZ_BYTE + SZ_BYTE + SZ_BYTE;
	if (buf.cursorActive()) {
		if (!buf.canWrite(SZ_TBL_HDR))
			return WriteResult::WR_INCOMPLETE;
		buf.writeTyp(m_typ);
		buf.writeAtt(KdbAttr::NONE);
		buf.writeTyp(KdbType::DICT);
	}
	else {
		buf.ffwd(SZ_TBL_HDR);
	}

	if (WriteResult wr = m_cols->write(buf); WriteResult::WR_OK != wr)
		return wr;

	return m_vals->write(buf);
}

//-------------------------------------------------------------------------------- KdbDict
uint64_t KdbDict::count() const
{
	return !m_keys ? 0 : m_keys->count();
}

uint64_t KdbDict::wireSz() const
{
	return SZ_BYTE + m_keys->wireSz() + m_vals->wireSz();
}

std::optional<KdbType> KdbDict::getKeyType() const
{
	if (!m_keys)
		return {};
	return std::make_optional<KdbType>(m_keys.get()->m_typ);
}

std::optional<KdbType> KdbDict::getValueType() const
{
	if (!m_vals)
		return {};
	return std::make_optional<KdbType>(m_vals.get()->m_typ);
}

ReadResult KdbDict::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE);

	ReadResult rr;
	if (buf.cursorActive()) {
		KdbBase *k;
		rr = newInstance(buf, &k);
		if (ReadResult::RD_OK != rr)
			return rr;
		m_keys.reset(k);
	}

	rr = m_keys->read(buf);
	if (ReadResult::RD_OK != rr)
		return rr;
	
	if (buf.cursorActive()) {
		KdbBase *v;
		rr = newInstance(buf, &v);
		if (ReadResult::RD_OK != rr)
			return rr;
		m_vals.reset(v);
	}

	return m_vals->read(buf);
}

WriteResult KdbDict::write(WriteBuf & buf) const
{
	if (buf.cursorActive()) {
		if (!buf.writeTyp(KdbDict::kdb_type))
			return WriteResult::WR_INCOMPLETE;
	}
	else {
		buf.ffwd(SZ_BYTE);
	}

	if (WriteResult wr = m_keys->write(buf); WriteResult::WR_OK != wr)
		return wr;

	return m_vals->write(buf);
}

//-------------------------------------------------------------------------------- KdbFunction

ReadResult KdbFunction::alloc(ReadBuf & buf, KdbBase **ptr)
{
	size_t SZ_PROLOGUE = SZ_BYTE + SZ_BYTE + SZ_VEC_HDR;
	if (buf.cursorActive()) {
		if (!buf.canRead(SZ_PROLOGUE))
			return ReadResult::RD_INCOMPLETE;
		std::ignore = buf.read<int8_t>();                       // 0x64
		std::ignore = buf.read<int8_t>();                       //     00
		int8_t ctyp = buf.read<int8_t>();                       //       0a
		std::ignore = buf.read<int8_t>();                       //         00
		const int32_t len = buf.read<int32_t>();                //           01020304
		if (static_cast<KdbType>(ctyp) != KdbType::CHAR_VECTOR || len < 0)
			return ReadResult::RD_ERR_IPC; // TODO log the error
		*ptr = new KdbFunction{static_cast<uint64_t>(len)};
	}
	else {
		buf.ffwd(SZ_PROLOGUE);
	}
	return ReadResult::RD_OK;
}

KdbFunction::KdbFunction(uint64_t cap)
 : KdbBase(KdbType::FUNCTION)
 , m_vec(cap)
{
	m_vec.resize(0);
}

KdbFunction::KdbFunction(std::string_view fun)
 : KdbBase(KdbType::FUNCTION)
 , m_vec(fun.size())
{
	m_vec.insert(m_vec.begin() + m_vec.size(), fun.data(), fun.data() + fun.size());
}

uint64_t KdbFunction::wireSz() const
{
	return SZ_BYTE + SZ_BYTE + SZ_VEC_HDR + m_vec.size();
}

// q)(sums 0 2 6) cut 8_-8!{x+y}
// 0x6400
// 			0a0005000000
// 									7b782b797d
// q)10h$ last (sums 0 2 6) cut 8_-8!{x+y}
// "{x+y}"
ReadResult KdbFunction::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE + SZ_BYTE + SZ_VEC_HDR);

	buf.ffwd(m_vec.size());
	if (m_vec.size() == m_vec.capacity())
		return ReadResult::RD_OK;

	const uint64_t rqd = m_vec.capacity() - m_vec.size();
	const uint64_t elm = std::min(rqd, buf.remaining());

	if (elm > 0 && rqd == buf.read(m_vec, elm))
		return ReadResult::RD_OK;

	return ReadResult::RD_INCOMPLETE;
}

WriteResult KdbFunction::write(WriteBuf & buf) const
{
	size_t SZ_PROLOGUE = SZ_BYTE + SZ_BYTE + SZ_VEC_HDR;
	if (buf.cursorActive()) {
		if (!buf.canWrite(SZ_PROLOGUE))
			return WriteResult::WR_INCOMPLETE;
		buf.writeTyp(KdbFunction::kdb_type);
		buf.writeAtt(KdbAttr::NONE);
		buf.writeHdr(KdbType::CHAR_VECTOR, KdbAttr::NONE, static_cast<int32_t>(m_vec.size()));
	}
	else {
		buf.ffwd(SZ_PROLOGUE);
	}

	int64_t off = buf.cursorOff();
	size_t skp = off >= 0 ? 0 : std::min(static_cast<size_t>(-off), m_vec.size());

	size_t rit = buf.writeAry(m_vec.data(), skp, m_vec.capacity());
	if (skp + rit == m_vec.size())
		return WriteResult::WR_OK;

	return WriteResult::WR_INCOMPLETE;
}

//--------------------------------------------------------------------------------------- KdbPrimitive
template<typename T>
ReadResult KdbPrimitive<T>::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE);

	if (buf.cursorActive()) {
		if (!buf.canRead(SZ_BYTE))
			return ReadResult::RD_INCOMPLETE;
		m_val = static_cast<T>(buf.read<uint8_t>());
	}
	else {
		buf.ffwd(SZ_BYTE);
	}
	return ReadResult::RD_OK;
}

template<typename T>
WriteResult KdbPrimitive<T>::write(WriteBuf & buf) const
{
	if (buf.cursorActive()) {
		if (!buf.canWrite(SZ_BYTE + SZ_BYTE))
			return WriteResult::WR_INCOMPLETE;
		buf.writeTyp(m_typ);
		buf.write<int8_t>(static_cast<int8_t>(m_val));
	}
	else {
		buf.ffwd(SZ_BYTE + SZ_BYTE);
	}
	return WriteResult::WR_OK;
}

//--------------------------------------------------------------------------------------- KdbProjection

ReadResult KdbProjection::alloc(ReadBuf & buf, KdbBase **ptr)
{
	if (!buf.canRead(SZ_BYTE + SZ_INT))
		return ReadResult::RD_INCOMPLETE;

	std::ignore = buf.read<int8_t>();
	int32_t len = buf.read<int32_t>();
	if (len < 0)
		return ReadResult::RD_ERR_IPC;

	*ptr = new KdbProjection{static_cast<uint64_t>(len)};

	return ReadResult::RD_OK;
}

KdbProjection::KdbProjection(uint64_t cap)
 : KdbBase(KdbType::PROJECTION)
 , m_vec(cap)
{
	m_vec.resize(0);
}

KdbProjection::KdbProjection(std::string_view fun, size_t n_args)
 : KdbBase(KdbType::PROJECTION)
 , m_vec(n_args + 1)
{
	m_vec.resize(0);
	m_vec.emplace_back(new KdbFunction(fun));
	for (uint64_t i = 0 ; i < n_args ; i++) {
		m_vec.emplace_back(new KdbUnaryPrimitive(KdbUnary::ELIDED_VALUE));
	}
}

void KdbProjection::setArg(uint32_t idx, std::unique_ptr<KdbBase> val)
{
	m_vec[idx + 1].reset(val.release());
}

uint64_t KdbProjection::wireSz() const
{
	uint64_t len = SZ_BYTE + SZ_INT;
	for (size_t i = 0 ; i < m_vec.size() ; i++) {
		len += m_vec[i]->wireSz();
	}
	return len;
}

// This is just a container for stuff, but not a list, e.g. we can rewrite the IPC so that instead
// of its first element being a function, it can also be a projection, so a projection within
// a projection.
// q)(sums 0 1 4 1 4 2 6 5 2)cut 8_ (0x01000000,reverse 0x0 vs 6h$ 8+count ipc),ipc:((5#prj),prj:8_-8!{x+y}"a"),0x65ff
// 0x68                                                      - projection type-byte
//     02000000                                              - projection element-count
//             68                                            - 1st element: projection type-byte
//               02000000                                                   projection element-count
//                       6400                                               nested 1st element: function type-byte and empty byte
//                           0a0005000000                                                       char-vec header
//                                       7b782b797d                                             char-vec data
//                                                 f661                     nested 2nd element: "a"
//                                                     65ff  - 2nd element: elided-argument "null"
// 
// We can even execute the resulting function (because of the elided argument)
//
// q)(-9!(0x01000000,reverse 0x0 vs 6h$ 8+count ipc),ipc:((5#prj),prj:8_-8!{x,y}"a"),0x65ff)`
// "a"
// `
// q)
ReadResult KdbProjection::read(ReadBuf & buf)
{
	if (!buf.cursorActive())
		buf.ffwd(SZ_BYTE + SZ_INT);

	ReadResult rr;
	for (size_t i = 0 ; i < m_vec.size() ; i++) {
		rr = m_vec[i]->read(buf);
		if (ReadResult::RD_OK != rr)
			return rr;
	}

	const size_t cap = m_vec.capacity();

	for (size_t i = m_vec.size() ; i < cap ; i++) {
		KdbBase *obj;
		rr = newInstance(buf, &obj);
		if (ReadResult::RD_OK != rr)
			return rr;
		m_vec.push_back(std::unique_ptr<KdbBase>(obj));
		rr = m_vec[i]->read(buf);
		if (ReadResult::RD_OK != rr)
			return rr;
	}
	return ReadResult::RD_OK;
}

WriteResult KdbProjection::write(WriteBuf & buf) const
{
	const size_t SZ_HDR = SZ_BYTE + SZ_INT;
	if (buf.cursorActive()) {
		if (!buf.canWrite(SZ_HDR))
			return WriteResult::WR_INCOMPLETE;
		buf.writeTyp(KdbProjection::kdb_type);
		buf.write<int32_t>(static_cast<int32_t>(m_vec.size()));
	}
	else {
		buf.ffwd(SZ_HDR);
	}

	for (uint64_t i = 0 ; i < m_vec.size() ; i++) {
		WriteResult wr = m_vec[i]->write(buf);
		if (WriteResult::WR_OK != wr)
			return wr;
	}
	return WriteResult::WR_OK;
}

//-------------------------------------------------------------------------------- KdbIpcDecompressor
KdbIpcDecompressor::KdbIpcDecompressor(uint64_t ipc_len, uint64_t msg_len, std::unique_ptr<int8_t[]> dst)
 : m_ipc_len(ipc_len - SZ_MSG_HDR - SZ_INT)
 , m_msg_len(msg_len - SZ_MSG_HDR)
 , m_dst(std::move(dst))
{
}

uint64_t KdbIpcDecompressor::blockReadSz(ReadBuf & buf) const
{
	uint64_t rem = buf.remaining();
	if (0 == rem)
		return UINT64_MAX;

	uint8_t set = buf.peek<uint8_t>();

	// while there are still IPC bytes to read
	// for each bit in 'set'
	//   if bit == 1
	//     require 2 bytes
	//   else if bit == 0
	//     require 1 byte
	uint64_t count = static_cast<uint64_t>(std::popcount(set));
	uint64_t rqd = 1 + count * 2 + std::min(8 - count, m_ipc_len - m_rdx - count * 2 - 1);

	return rqd;
}

uint64_t KdbIpcDecompressor::uncompress(ReadBuf & buf)
{
	const uint64_t pos = m_off;
	uint32_t rdn = 0;

	while (m_off < m_msg_len) {

		if (0 == m_idx) {
			if (blockReadSz(buf) > buf.remaining())
				break;
			m_bit = static_cast<uint32_t>(buf.read<uint8_t>());
			m_rdx += 1;
			m_idx = 1;
		}

		if (0 != (m_bit & m_idx)) {
			uint32_t off = static_cast<uint32_t>(buf.read<uint8_t>());
			m_rdx += 1;
			uint32_t old = m_lbh[off];
			m_dst[m_off++] = m_dst[old++];
			m_dst[m_off++] = m_dst[old++];
			rdn = static_cast<uint32_t>(buf.read<uint8_t>());
			m_rdx += 1;
			for (uint32_t m = 0 ; m < rdn ; m++) {
				m_dst[m_off + m] = m_dst[old + m];
			}
		}
		else {
			m_dst[m_off++] = buf.read<int8_t>(); // copy a plaintext byte
			m_rdx += 1;
		}

		// 1. Since m_off is always >= 1 here, m_off - 1 is safe for an unsigned type
		while (m_chx < (m_off - 1)) {
			int32_t lhs = 0xFF & m_dst[m_chx];
			int32_t rhs = 0xFF & m_dst[m_chx+1];
			m_lbh[lhs ^ rhs] = m_chx++;
		}

		if (0 != (m_bit & m_idx)) {
			m_chx = m_off += rdn;
		}

		m_idx *= 2;
		if (256 == m_idx) {
			m_idx = 0;
		}

	}
	return m_off - pos;
}

bool KdbIpcDecompressor::isComplete() const
{
	return m_off == m_msg_len;
}

uint64_t KdbIpcDecompressor::getUsedInputCount() const
{
	return m_rdx;
}

ReadBuf KdbIpcDecompressor::getReadBuf(int64_t csr) const
{
	return ReadBuf{m_dst.get(), m_off, csr};
}
//-------------------------------------------------------------------------------- KdbIpcMessageReader

bool KdbIpcMessageReader::readMsgHdr(ReadBuf & buf, ReadMsgResult & result)
{
	if (0 == m_ipc_len) {
		const uint64_t pos = buf.offset();
		if (buf.remaining() < 8) {
			result.result = ReadResult::RD_INCOMPLETE, result.bytes_consumed = 0;
			return false;
		}
		const int8_t end = buf.read<int8_t>();
		if (1 != end) {
			result.result = ReadResult::RD_ERR_IPC, result.bytes_consumed = 0;
			return false;
		}

		m_msg_typ = static_cast<KdbMsgType>(buf.read<int8_t>());
		m_compressed = 1 == buf.read<int8_t>();
		std::ignore = buf.read<int8_t>();
		m_ipc_len = buf.read<int32_t>();

		if (m_compressed) {
			if (!buf.canRead(SZ_INT)) {
				m_ipc_len = 0;
				// NB: the ReadBuf is internal and we want the client to present the same IPC bytes again
				// when more are available, hence g_used <- 0
				result.result = ReadResult::RD_INCOMPLETE, result.bytes_consumed = 0;
				return false;
			}
			m_msg_len = buf.read<int32_t>();
			int8_t *dst = new int8_t[m_msg_len - SZ_MSG_HDR];
			if (nullptr == dst) {
				// again: we want the caller to present the same IPC bytes again so we say we haven't used
				// any yet, although TBH, if the allocation fails, is the client really going to continue?
				result.result = ReadResult::RD_ERR_ALLOC, result.bytes_consumed = 0;
				return false;
			}
			m_inflater = std::make_unique<KdbIpcDecompressor>(m_ipc_len, m_msg_len, std::unique_ptr<int8_t[]>(static_cast<int8_t*>(dst)));
		}
		else {
			m_msg_len = m_ipc_len;
		}
		m_byt_usd = buf.offset() - pos;
		m_byt_dez = 0;
	}
	else {
		buf.ffwd((SZ_MSG_HDR + m_compressed) ? SZ_INT : 0);
	}
	return true;
}

bool KdbIpcMessageReader::readMsgData1(ReadBuf & buf, ReadMsgResult & result)
{
	if (!m_msg) {
		KdbBase *obj;
		ReadResult rr = newInstance(buf, &obj);
		if (ReadResult::RD_OK != rr) {
			result.result = ReadResult::RD_INCOMPLETE;
			return false;
		}
		m_msg.reset(obj);
	}

	ReadResult rr = m_msg->read(buf);

	if (ReadResult::RD_OK != rr) {
		result.result = rr;
		return false;
	}

	KdbBase *obj = m_msg.release();
	result.result = ReadResult::RD_OK;
	result.message.reset(obj);
	return true;
}

bool KdbIpcMessageReader::readMsgData(ReadBuf & buf, ReadMsgResult & result)
{
	const uint64_t pos = buf.offset();
	bool complete = readMsgData1(buf, result);
	m_byt_dez += buf.offset() - pos;
	return complete;
}

bool KdbIpcMessageReader::readMsg(const void *src, uint64_t len, ReadMsgResult & result)
{
	ReadBuf buf{static_cast<const int8_t*>(src), len, -static_cast<int64_t>(m_byt_dez)};

	const uint64_t hdr_pos = buf.offset();
	if (!readMsgHdr(buf, result))
		return false;
	const uint64_t hdr_usd = buf.offset() - hdr_pos;

	const uint64_t src_pos = buf.offset();

	bool complete = false;
	if (!m_compressed)  {
		complete = readMsgData(buf, result);
	}
	else {
		std::ignore = m_inflater->uncompress(buf);
		ReadBuf zBuf = m_inflater->getReadBuf(m_byt_dez);
		complete = readMsgData(zBuf, result);
	}
	m_byt_usd += buf.offset() - src_pos;
	result.bytes_consumed = hdr_usd + buf.offset() - src_pos;
	return complete;
}

uint64_t KdbIpcMessageReader::getIpcLength() const
{
	return m_ipc_len;
}

uint64_t KdbIpcMessageReader::getInputBytesConsumed() const
{
	return m_byt_usd;
}

uint64_t KdbIpcMessageReader::getMsgBytesDeserialized() const
{
	return m_byt_dez;
}

//--------------------------------------------------------------------------------------- KdbUtil
size_t KdbUtil::writeLoginMsg(std::string_view usr, std::string_view pwd, std::unique_ptr<char> & ptr)
{
	size_t len = usr.length() + SZ_BYTE + pwd.length() + SZ_BYTE + SZ_BYTE;
	char *dst = static_cast<char*>(malloc(len));
	if (nullptr == dst) {
		return 0;
	}
	size_t off = 0;
	memcpy(dst + off, usr.data(), usr.length());
	off += usr.length();
	dst[off++] = ':';
	if (pwd.length() > 0) {
		memcpy(dst + off, pwd.data(), pwd.length());
		off += pwd.length();
	}
	dst[off++] = 3;
	dst[off++] = 0;
	ptr.reset(dst);
	return len;
}

//--------------------------------------------------------------------------------------- KdbIpcMessageWriter
KdbIpcMessageWriter::KdbIpcMessageWriter(KdbMsgType msg_typ, std::shared_ptr<KdbBase> msg)
 : m_msg_typ(msg_typ)
 , m_ipc_len(msg->wireSz() + SZ_MSG_HDR)
 , m_root(msg)
{
	m_byt_rem = m_ipc_len;
}

size_t KdbIpcMessageWriter::bytesRemaining() const
{
	return m_byt_rem;
}

WriteResult KdbIpcMessageWriter::write(void *dst, size_t cap)
{
	WriteBuf buf{dst, cap, -static_cast<int64_t>(m_ipc_len - m_byt_rem)};

	if (buf.cursorActive()) {
		if (!buf.canWrite(SZ_MSG_HDR))
			return WriteResult::WR_INCOMPLETE;

		buf.write<int8_t>(1);                              // little endian
		buf.write<int8_t>(static_cast<int8_t>(m_msg_typ)); // msg type
		buf.write<int8_t>(0);                              // ?
		buf.write<int8_t>(0);                              // ?
		buf.write<int32_t>(m_ipc_len);
	}
	else {
		buf.ffwd(SZ_MSG_HDR);
	}

	WriteResult wr = m_root->write(buf);
	m_byt_rem -= buf.cursorOff();
	return wr;
}

//-------------------------------------------------------------------------------- end namespace mg7x
}
