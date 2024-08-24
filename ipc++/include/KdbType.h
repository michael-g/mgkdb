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

#ifndef __mg7x_KdbType__H__
#define __mg7x_KdbType__H__
#pragma once
#include <bit>
#include <format>
#include <iterator>

#include <vector>
#include <memory>
#include <type_traits>
#include <cstdint>
#include <chrono>
#include <cstring> // memcpy

namespace mg7x {

constexpr static int32_t SZ_BYTE     =  1;
constexpr static int32_t SZ_SHORT    =  2;
constexpr static int32_t SZ_INT      =  4;
constexpr static int32_t SZ_LONG     =  8;
constexpr static int32_t SZ_GUID     = 16;
constexpr static int32_t SZ_REAL     =  4;
constexpr static int32_t SZ_FLOAT    =  8;
constexpr static int32_t SZ_VEC_META =  5;
constexpr static int32_t SZ_VEC_HDR  =  SZ_BYTE + SZ_VEC_META;
constexpr static int32_t SZ_MSG_HDR  =  8;

constexpr int16_t NULL_SHORT     = std::bit_cast<int16_t>((unsigned short)0x8000);
constexpr int16_t POS_INF_SHORT  = std::bit_cast<int16_t>((unsigned short)0x7fff);
constexpr int16_t NEG_INF_SHORT  = std::bit_cast<int16_t>((unsigned short)0x8001);
constexpr int32_t NULL_INT       = 0x80000000;
constexpr int32_t POS_INF_INT    = 0x7fffffff;
constexpr int32_t NEG_INF_INT    = 0x80000001;
constexpr int64_t NULL_LONG      = 0x8000000000000000L;
constexpr int64_t POS_INF_LONG   = 0x7fffffffffffffffL;
constexpr int64_t NEG_INF_LONG   = 0x8000000000000001L;
constexpr int32_t NULL_REAL      = 0xffc00000;
constexpr int32_t POS_INF_REAL   = 0x7f800000;
constexpr int32_t NEG_INF_REAL   = 0xff800000;
constexpr int64_t NULL_FLOAT     = 0xfff8000000000000L;
constexpr int64_t POS_INF_FLOAT  = 0x7ff0000000000000L;
constexpr int64_t NEG_INF_FLOAT  = 0xfff0000000000000L;
constexpr uint8_t NULL_CHAR      = 0x20;

enum class KdbType
{
	EXCEPTION         = -128,
	TIME_ATOM         = -19,
	SECOND_ATOM       = -18,
	MINUTE_ATOM       = -17,
	TIMESPAN_ATOM     = -16,
	DATETIME_ATOM     = -15,
	DATE_ATOM         = -14,
	MONTH_ATOM        = -13,
	TIMESTAMP_ATOM    = -12,
	SYMBOL_ATOM       = -11,
	CHAR_ATOM         = -10,
	FLOAT_ATOM        =  -9,
	REAL_ATOM         =  -8,
	LONG_ATOM         =  -7,
	INT_ATOM          =  -6,
	SHORT_ATOM        =  -5,
	BYTE_ATOM         =  -4,
	GUID_ATOM         =  -2,
	BOOL_ATOM         =  -1,
	LIST              =   0,
	BOOL_VECTOR       = -BOOL_ATOM,
	GUID_VECTOR       = -GUID_ATOM,
	BYTE_VECTOR       = -BYTE_ATOM,
	SHORT_VECTOR      = -SHORT_ATOM,
	INT_VECTOR        = -INT_ATOM,
	LONG_VECTOR       = -LONG_ATOM,
	REAL_VECTOR       = -REAL_ATOM,
	FLOAT_VECTOR      = -FLOAT_ATOM,
	CHAR_VECTOR       = -CHAR_ATOM,
	SYMBOL_VECTOR     = -SYMBOL_ATOM,
	TIMESTAMP_VECTOR  = -TIMESTAMP_ATOM,
	MONTH_VECTOR      = -MONTH_ATOM,
	DATE_VECTOR       = -DATE_ATOM,
	DATETIME_VECTOR   = -DATETIME_ATOM,
	TIMESPAN_VECTOR   = -TIMESPAN_ATOM,
	MINUTE_VECTOR     = -MINUTE_ATOM,
	SECOND_VECTOR     = -SECOND_ATOM,
	TIME_VECTOR       = -TIME_ATOM,
	TABLE             = 98, 
	DICT              = 99,
	FUNCTION          = 100,
	UNARY_PRIMITIVE   = 101,
	BINARY_PRIMITIVE  = 102,
	TERNARY_PRIMITIVE = 103,
	PROJECTION        = 104,
};

enum class KdbMsgType
{
	ASYNC    = 0,
	SYNC     = 1,
	RESPONSE = 2
};

enum class KdbUnary
{
	NULL_IDENTITY     = 0x00, // ::
	FLIP              = 0x01, // +:
	NEG               = 0x02, // -:
	FIRST             = 0x03, // *:
	RECIPROCAL        = 0x04, // %:
	WHERE             = 0x05, // &:
	REVERSE           = 0x06, // |:
	IS_NULL           = 0x07, // ^:
	GROUP             = 0x08, // =:
	IASC              = 0x09, // <:
	HCLOSE            = 0x0a, // >:
	STRING            = 0x0b, // $:
	MONADIC_COMMA     = 0x0c, // ,:
	COUNT             = 0x0d, // #:
	FLOOR             = 0x0e, // _:
	NOT               = 0x0f, // ~:
	INV               = 0x10, // !:
	DISTINCT          = 0x11, // ?:
	TYPE              = 0x12, // @:
	GET               = 0x13, // .:
	READ0             = 0x14, // 0::
	READ1             = 0x15, // 1::
	MONADIC_TWO_COLON = 0x16, // 2::
	AVG               = 0x17, // avg
	LAST              = 0x18, // last
	SUM               = 0x19, // sum
	PRD               = 0x1a, // prd
	MIN               = 0x1b, // min
	MAX               = 0x1c, // max
	EXIT              = 0x1d, // exit
	GETENV            = 0x1e, // getenv
	ABS               = 0x1f, // abs
	SQRT              = 0x20, // sqrt
	LOG               = 0x21, // log
	EXP               = 0x22, // exp
	SIN               = 0x23, // sin
	ASIN              = 0x24, // asin
	COS               = 0x25, // cos
	ACOS              = 0x26, // acos
	TAN               = 0x27, // tan
	ATAN              = 0x28, // atan
	ENLIST            = 0x29, // enlist
	VAR               = 0x2a, // var
	ELIDED_VALUE      = 0xff, // 
};

// q)(!/) (4h$key d;value d)@\: where not `~/:value d:k!{@[-9!;0x010000000a00000066,4h$x;`]} each k:til 255
enum class KdbBinary
{
	NULL_BIN      = 0x00, // :
	PLUS          = 0x01, // +
	MINUS         = 0x02, // -
	MULTIPLY      = 0x03, // *
	DIVIDE        = 0x04, // %
	MIN_AND       = 0x05, // &
	MAX_OR        = 0x06, // |
	FILL          = 0x07, // ^
	EQUALS        = 0x08, // =
	LESS_THAN     = 0x09, // <
	GREATER_THAN  = 0x0a, // >
	CAST          = 0x0b, // $
	APPEND        = 0x0c, // ,
	TAKE          = 0x0d, // #
	DROP          = 0x0e, // _
	MATCH         = 0x0f, // ~
	DICT_OP       = 0x10, // !
	QUERY         = 0x11, // ?
	APPLY_AT      = 0x12, // @
	APPLY         = 0x13, // .
	FILE_TEXT     = 0x14, // 0:
	FILE_BINARY   = 0x15, // 1:
	DYNAMIC_LOAD  = 0x16, // 2:
	IN            = 0x17, // in
	WITHIN        = 0x18, // within
	LIKE          = 0x19, // like
	BIN           = 0x1a, // bin
	SS            = 0x1b, // ss
	INSERT        = 0x1c, // insert
	WSUM          = 0x1d, // wsum
	WAVG          = 0x1e, // wavg
	DIV           = 0x1f, // div
	XEXP          = 0x20, // xexp
	SETENV        = 0x21, // setenv
	BINR          = 0x22, // binr
	COV           = 0x23, // cov
	COR           = 0x24, // cor
};

// q)(!/) (4h$key d;value d)@\: where not `~/:value d:k!{@[-9!;0x010000000a00000067,4h$x;`]} each k:til 255
enum class KdbTernary
{
	EACH_BOTH     = 0x00, // '
	OVER          = 0x01, // /
	SCAN          = 0x02, // \  // don't escape the newline ...
	EACH_PRIOR    = 0x03, // ':
	EACH_RIGHT    = 0x04, // /:
	EACH_LEFT     = 0x05, // \:
};

enum class KdbAttr
{
	NONE    = 0,
	SORTED  = 1,
	UNIQUE  = 2,
	PARTED  = 3,
	GROUPED = 4
};

enum class ReadResult
{
	RD_OK,
	RD_INCOMPLETE,
	RD_ERR_IPC,
	RD_ERR_LOGIC,
	RD_ERR_ALLOC,
};

enum class WriteResult
{
	WR_OK,
	WR_INCOMPLETE,
};

template<size_t> struct BufIter;

template<size_t N>
struct CharBuf
{
	std::array<char,N> m_ary;
	size_t m_len;

	CharBuf() : m_ary{}, m_len{0} {}

	BufIter<N> out() { return BufIter<N>{this}; }

	auto copyTo(auto out)
	{
		std::copy(m_ary.begin(), m_ary.data() + m_len, out);
		return out;
	}

	void push_back(char c)
	{
		if (m_len < N) {
			m_ary[m_len++] = c;
		}
	}

};

// Explanation of c++23 input_iterator and output_iterator concepts: https://stackoverflow.com/a/77064961/322304
template<size_t N>
struct BufIter {
	using difference_type = char;
	CharBuf<N> *m_ptr;

	BufIter(CharBuf<N> *ptr) : m_ptr{ptr} {}
	~BufIter() = default;
	BufIter(const BufIter<N> & rhs) = default;
	BufIter(BufIter<N> && rhs) = default;

	BufIter & operator=(const BufIter & rhs)
	{
		m_ptr = rhs.m_ptr;
		return *this;
	}
	
	BufIter & operator=(BufIter && rhs)
	{
		std::swap(m_ptr, rhs.m_ptr);
		return *this;
	}

	BufIter & operator=(const char & rhs)
	{
		m_ptr->push_back(rhs);
		return *this;
	}
	
	BufIter & operator=(char && rhs)
	{
		m_ptr->push_back(rhs);
		return *this;
	}
	
	BufIter & operator++() { return *this; }
	BufIter   operator++(int) { return *this; }
	BufIter & operator*() { return *this; }

};


namespace time {

	class Timestamp
	{
		Timestamp() = delete;
	public:
		static int64_t now();
		static int64_t getUTC(int32_t y, int32_t m, int32_t d, int32_t h = 0, int32_t u = 0, int32_t v = 0, int64_t n = 0);
		static BufIter<32> toDateNotation(BufIter<32> out, int64_t nanos);
		static BufIter<32> toTimeNotation(BufIter<32> out, int64_t nanos);
		// ...2024.05.31D23:59:59.123456789
		// 01234567890123456789012345678901
		static BufIter<32> format_to(BufIter<32> out, int64_t val, bool suffix = true);
	};

	class Month
	{
		Month() = delete;
	public:
		// .........2024.01m
		// 01234567890123456
		static BufIter<16> format_to(BufIter<16> out, int32_t val, bool suffix = true);
	};

	class Date
	{
		Date() = delete;
	public:
		static int32_t getDays(int32_t y, int32_t m, int32_t d);
		// .......2024.05.31
		// 01234567890123456
		static BufIter<16> format_to(BufIter<16> out, int32_t val, bool suffix = true);
	};

	class Timespan
	{
		Timespan() = delete;
	public:
		static int64_t now(const std::chrono::time_zone * loc);
		// .....0D23:59:59.123456789
		// 0123456789012345678901234
		static BufIter<24> format_to(BufIter<24> out, int64_t val, bool suffix = true);
	};

	class Minute
	{
		Minute() = delete;
	public:
		// ...........00:00
		// 0123456789012345
		static BufIter<16> format_to(BufIter<16> out, int32_t val, bool suffix = true);
	};

	class Second
	{
		Second() = delete;
	public:
		// ........00:00:00
		// 0123456789012345
		static BufIter<16> format_to(BufIter<16> out, int32_t val, bool suffix = true);
	};

	class Time
	{
		Time() = delete;
	public:
		// Interestingly (I need to get our more), if you convert max-int 0x7ffffff6 to a time
		// you get **:31:23:646; perhaps we don't need to allow for the overflow. The max-int
		// value equates to 596 hours, min-int to 597.
		// ....596:31:23.646
		// 01234567890123456
		static BufIter<16> format_to(BufIter<16> out, int32_t time, bool suffix = true);
	};

} // end namespace time

//-------------------------------------------------------------------------------- ReadBuf
class ReadBuf
{
	int8_t    const *m_src;
	uint64_t         m_len;
	uint64_t         m_off{0};
	int64_t          m_csr{0};

	size_t adj(size_t);

	public:
		ReadBuf(const int8_t *src, uint64_t len);
		ReadBuf(const int8_t *src, uint64_t len, int64_t csr);

		uint64_t remaining() const { return m_len - m_off; }
		bool canRead(size_t bytes) const { return remaining() >= bytes; }
		uint64_t length() const { return m_len; }
		uint64_t offset() const { return m_off; }
		int64_t cursorOff() const { return m_csr; }
		void ffwd(size_t val) { m_csr += val; }
		bool cursorActive() const { return m_csr >= 0; }
		void setLength(uint64_t len) { m_len = len; }
		void rewind() { m_csr = -m_off; }


		template<typename T> T peek() const;
		template<typename T> T read();
		template<typename T> uint64_t read(T ary[], uint64_t count);
		template<typename T> uint64_t read(std::vector<T> & ptr, const uint64_t count);
		ReadResult readSym(std::string & str);
		ReadResult readSyms(size_t required, std::vector<struct LocInfo> & locs, std::vector<char> & data);
};

//-------------------------------------------------------------------------------- WriteBuf
class WriteBuf
{
	int8_t  *m_dst;
	uint64_t m_cap;
	int64_t  m_csr;
	uint64_t m_off{0};

	size_t adj(size_t);

	public:
		WriteBuf(void *m_dst, uint64_t cap);
		WriteBuf(void *m_dst, uint64_t cap, int64_t csr);

		bool canWrite(size_t bytes) const { return m_off + bytes <= m_cap; }
		bool cursorActive() const { return m_csr >= 0; }
		void ffwd(size_t bytes) { m_csr += bytes; }
		int64_t cursorOff() const { return m_csr; }
		uint64_t capacity() const { return m_cap; }
		uint64_t offset() const { return m_off; }
		uint64_t remaining() const { return m_cap - m_off; }

		bool writeTyp(KdbType typ);
		bool writeAtt(KdbAttr att);
		bool writeHdr(KdbType typ, KdbAttr att, int32_t m_len);
		template<typename T> bool write(T val);
		bool writeSym(const std::string_view sv);
		template<typename T> size_t writeAry(const T ary[], size_t off, size_t cap);
};

using GuidType = std::array<uint8_t,16>;

//-------------------------------------------------------------------------------- KdbBase
struct KdbBase
{
	const KdbType m_typ;

	KdbBase(KdbType typ) : m_typ(typ) {}

	virtual ~KdbBase() = 0;
	virtual uint64_t count() const = 0;
	virtual uint64_t wireSz() const = 0;
	virtual ReadResult read(ReadBuf & buf) = 0;
	virtual WriteResult write(WriteBuf & buf) const = 0;
};

//-------------------------------------------------------------------------------- KdbException
struct KdbException : public KdbBase
{
	std::string m_msg;

	KdbException() : KdbException("") {}
	KdbException(std::string_view msg): KdbBase(KdbType::EXCEPTION), m_msg(msg) {}

	uint64_t count() const override { return -1; }
	uint64_t wireSz() const override { return SZ_BYTE + m_msg.length() + SZ_BYTE; }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbTimeAtom
struct KdbTimeAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::TIME_ATOM;
	int32_t m_val;

	KdbTimeAtom(int32_t val = 0) : KdbBase(KdbType::TIME_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbSecondAtom
struct KdbSecondAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::SECOND_ATOM;
	int32_t m_val;

	KdbSecondAtom(int32_t val = 0) : KdbBase(KdbType::SECOND_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbMinuteAtom
struct KdbMinuteAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::MINUTE_ATOM;
	int32_t m_val;

	KdbMinuteAtom(int32_t val = 0) : KdbBase(KdbType::MINUTE_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbTimespanAtom
struct KdbTimespanAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::TIMESPAN_ATOM;
	int64_t m_val;

	KdbTimespanAtom(int64_t val = 0) : KdbBase(KdbType::TIMESPAN_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbDateAtom
struct KdbDateAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::DATE_ATOM;
	int32_t m_val;

	KdbDateAtom(int32_t val = 0) : KdbBase(KdbType::DATE_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbMonthAtom
struct KdbMonthAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::MONTH_ATOM;
	int32_t m_val;

	KdbMonthAtom(int32_t val = 0) : KdbBase(KdbType::MONTH_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbTimestampAtom
struct KdbTimestampAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::TIMESTAMP_ATOM;
	int64_t m_val;

	KdbTimestampAtom(int64_t val = 0) : KdbBase(KdbType::TIMESTAMP_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbSymbolAtom
struct KdbSymbolAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::SYMBOL_ATOM;
	std::string m_val;

	KdbSymbolAtom(const std::string_view & val = "") : KdbBase(KdbType::SYMBOL_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	uint64_t wireSz() const override { return SZ_BYTE + m_val.size() + SZ_BYTE; }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbCharAtom
struct KdbCharAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::CHAR_ATOM;
	uint8_t m_val;

	KdbCharAtom(uint8_t val = 0) : KdbBase(KdbType::CHAR_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbFloatAtom
struct KdbFloatAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::FLOAT_ATOM;
	int64_t m_val;

	KdbFloatAtom(double val = 0) : KdbBase(KdbType::FLOAT_ATOM), m_val(std::bit_cast<int64_t>(val)) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
	double asDouble() const { return std::bit_cast<double>(m_val); }
	void setDouble(double f) { m_val = std::bit_cast<int64_t>(f); }
};

//-------------------------------------------------------------------------------- KdbRealAtom
struct KdbRealAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::REAL_ATOM;
	int32_t m_val;

	KdbRealAtom(float val = 0) : KdbBase(KdbType::REAL_ATOM), m_val(std::bit_cast<int32_t>(val)) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
	float asFloat() const { return std::bit_cast<float>(m_val); }
	void setFloat(float e) { m_val = std::bit_cast<int32_t>(e); }
};

//-------------------------------------------------------------------------------- KdbLongAtom
struct KdbLongAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::LONG_ATOM;
	int64_t m_val;

	KdbLongAtom(int64_t val = 0) : KdbBase(KdbType::LONG_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbIntAtom
struct KdbIntAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::INT_ATOM;
	int32_t m_val;

	KdbIntAtom(int32_t val = 0) : KdbBase(KdbType::INT_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbShortAtom
struct KdbShortAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::SHORT_ATOM;
	int16_t m_val;

	KdbShortAtom(int16_t val = 0) : KdbBase(KdbType::SHORT_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbGuidAtom
struct KdbGuidAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::GUID_ATOM;
	GuidType m_val;

	KdbGuidAtom() : KdbBase(KdbType::GUID_ATOM), m_val{} {}
	KdbGuidAtom(GuidType val) : KdbBase(KdbType::GUID_ATOM), m_val{val} {}
	KdbGuidAtom(std::array<uint64_t,2> ary) : KdbGuidAtom{} { std::memcpy(m_val.data(), ary.data(), sizeof(m_val)); }

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbByteAtom
struct KdbByteAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::BYTE_ATOM;
	int8_t m_val;

	KdbByteAtom(int8_t val = 0) : KdbBase(KdbType::BYTE_ATOM), m_val(val) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbBoolAtom
struct KdbBoolAtom : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::BOOL_ATOM;
	int8_t m_val;

	KdbBoolAtom(bool val = false) : KdbBase(KdbType::BOOL_ATOM), m_val(val ? 1 : 0) {}

	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + sizeof(decltype(m_val)); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
	bool asBool() const { return 0 != m_val; }
	void setBool(bool b) { m_val = b ? 1 : 0; }
};

//-------------------------------------------------------------------------------- KdbList/PtrRef
struct PtrRef
{
	KdbBase *m_ptr;
	bool     m_dry;

	PtrRef() = default;
	PtrRef(PtrRef & rhs) : m_ptr(rhs.m_ptr), m_dry(rhs.m_dry) { rhs.m_dry = false; }
	PtrRef(PtrRef && rhs) : m_ptr(rhs.m_ptr), m_dry(rhs.m_dry) { rhs.m_dry = false; }
	PtrRef(const PtrRef &) = delete;

	PtrRef(KdbBase &ptr) : m_ptr(&ptr), m_dry(false) {}
	PtrRef(KdbBase *ptr) : m_ptr(ptr), m_dry(true) {}

	~PtrRef()
	{
		if (m_dry && m_ptr) {
			delete m_ptr;
			m_dry = false;
			m_ptr = nullptr;
		}
	}
};

//-------------------------------------------------------------------------------- KdbList
class KdbList : public KdbBase
{
	KdbAttr             m_attr;
	std::vector<PtrRef> m_vals;

public:
	constexpr static KdbType kdb_type = KdbType::LIST;

	KdbList(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);
	~KdbList();

	uint64_t count() const override { return m_vals.size(); }
	void push(std::unique_ptr<KdbBase> val);
	void push(KdbBase & val);
	const KdbBase* getObj(size_t idx) const;
	KdbType typeAt(size_t idx) const;
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbBoolVector
struct KdbBoolVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::BOOL_VECTOR;

	KdbAttr             m_attr;
	std::vector<int8_t> m_vec;

	KdbBoolVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	bool getBool(uint64_t idx) const { return 1 == m_vec[idx]; }
	void setBool(uint64_t idx, bool val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbGuidVector
struct KdbGuidVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::GUID_VECTOR;

	KdbAttr               m_attr;
	std::vector<GuidType> m_vec;

	KdbGuidVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	const GuidType & getGuid(uint64_t idx) const { return m_vec[idx]; }
	void setGuid(uint64_t idx, const GuidType & val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbByteVector
struct KdbByteVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::BYTE_VECTOR;

	KdbAttr             m_attr;
	std::vector<int8_t> m_vec;

	KdbByteVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int8_t getByte(uint64_t idx) const { return m_vec[idx]; }
	void setByte(uint64_t idx, int8_t val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbShortVector
struct KdbShortVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::SHORT_VECTOR;

	KdbAttr              m_attr;
	std::vector<int16_t> m_vec;

	KdbShortVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int16_t getShort(uint64_t idx) const { return m_vec[idx]; }
	void setShort(uint64_t idx, int16_t val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbIntVector
struct KdbIntVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::INT_VECTOR;

	KdbAttr              m_attr;
	std::vector<int32_t> m_vec;

	KdbIntVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);
	KdbIntVector(const std::vector<int32_t> & vals, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int32_t getInt(uint64_t idx) const { return m_vec[idx]; }
	void setInt(uint64_t idx, int32_t val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbLongVector
struct KdbLongVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::LONG_VECTOR;

	KdbAttr              m_attr;
	std::vector<int64_t> m_vec;

	KdbLongVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int64_t getLong(uint64_t idx) const { return m_vec[idx]; }
	void setLong(uint64_t idx, int64_t val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbRealVector
struct KdbRealVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::REAL_VECTOR;

	KdbAttr              m_attr;
	std::vector<int32_t> m_vec;

	KdbRealVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	float getReal(uint64_t idx) const { return std::bit_cast<float>(m_vec[idx]); }
	void setReal(uint64_t idx, float val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbFloatVector
struct KdbFloatVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::FLOAT_VECTOR;

	KdbAttr              m_attr;
	std::vector<int64_t> m_vec;

	KdbFloatVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	double getFloat(uint64_t idx) const { return std::bit_cast<double>(m_vec[idx]); }
	void setFloat(uint64_t idx, double val);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbCharVector
struct KdbCharVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::CHAR_VECTOR;

	KdbAttr              m_attr;
	std::vector<uint8_t> m_vec;

	KdbCharVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);
	KdbCharVector(const std::string_view & str);

	uint64_t count() const override { return m_vec.size(); }
	char getChar(uint64_t idx) const { return std::bit_cast<char>(m_vec[idx]); }
	void setChar(uint64_t idx, char val);
	void setString(const std::string_view & val);
	std::string_view getString() const;
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbSymbolVector/LocInfo
// Used by the KdbSymbolVector as an index into its char array of null-delimited syms
struct LocInfo {
	uint32_t c_off;
	uint32_t c_len;
	LocInfo() : LocInfo(0, 0) {}
	LocInfo(uint32_t off, uint32_t len)
		: c_off(off)
		, c_len(len) {}
} __attribute__((packed));

//-------------------------------------------------------------------------------- KdbSymbolVector
class KdbSymbolVector : public KdbBase
{

	KdbAttr              m_attr;
	std::vector<LocInfo> m_locs;
	std::vector<char>    m_data;

public:
	constexpr static KdbType kdb_type = KdbType::SYMBOL_VECTOR;

	KdbSymbolVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);
	KdbSymbolVector(const std::vector<std::string_view> & vals);

	uint64_t count() const override { return m_locs.size(); }
	const std::string_view getString(uint64_t idx) const;
	int32_t indexOf(const std::string_view & col) const;
	void push(const std::string_view & sv);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbTimestampVector
struct KdbTimestampVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::TIMESTAMP_VECTOR;

	KdbAttr              m_attr;
	std::vector<int64_t> m_vec;

	KdbTimestampVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int64_t getTimestamp(uint64_t idx) const { return m_vec[idx]; }
	void setTimestamp(uint64_t idx, int64_t zp);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbMonthVector
struct KdbMonthVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::MONTH_VECTOR;

	KdbAttr              m_attr;
	std::vector<int32_t> m_vec;

	KdbMonthVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int32_t getMonth(uint64_t idx) const { return m_vec[idx]; }
	void setMonth(uint64_t idx, int32_t mth);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbDateVector
struct KdbDateVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::DATE_VECTOR;

	KdbAttr              m_attr;
	std::vector<int32_t> m_vec;

	KdbDateVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int32_t getDate(uint64_t idx) const { return m_vec[idx]; }
	void setDate(uint64_t idx, int32_t mth);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbTimespanVector
struct KdbTimespanVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::TIMESPAN_VECTOR;

	KdbAttr              m_attr;
	std::vector<int64_t> m_vec;

	KdbTimespanVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int64_t getTimespan(uint64_t idx) const { return m_vec[idx]; }
	void setTimespan(uint64_t idx, int64_t zp);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbMinuteVector
struct KdbMinuteVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::MINUTE_VECTOR;

	KdbAttr              m_attr;
	std::vector<int32_t> m_vec;

	KdbMinuteVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int32_t getMinute(uint64_t idx) const { return m_vec[idx]; }
	void setMinute(uint64_t idx, int32_t mth);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbSecondVector
struct KdbSecondVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::SECOND_VECTOR;

	KdbAttr              m_attr;
	std::vector<int32_t> m_vec;

	KdbSecondVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int32_t getSecond(uint64_t idx) const { return m_vec[idx]; }
	void setSecond(uint64_t idx, int32_t mth);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbTimeVector
struct KdbTimeVector : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::TIME_VECTOR;

	KdbAttr              m_attr;
	std::vector<int32_t> m_vec;

	KdbTimeVector(uint64_t cap = 0, KdbAttr attr = KdbAttr::NONE);

	uint64_t count() const override { return m_vec.size(); }
	int32_t getMillis(uint64_t idx) const { return m_vec[idx]; }
	void setMillis(uint64_t idx, int32_t millis);
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- ColDef/ColRef & related concepts
template <typename T> 
concept IsKdbType = std::is_base_of<KdbBase, T>::value;

template <typename T>
concept KdbColCapable = 
	IsKdbType<T> && 
	static_cast<int>(T::kdb_type) <= static_cast<int>(KdbType::TIME_VECTOR) &&
	static_cast<int>(T::kdb_type) >= static_cast<int>(KdbType::LIST);

template <typename T>
concept KdbKeySetCapable = 
	IsKdbType<T> && 
	(
		(static_cast<int>(T::kdb_type) <= static_cast<int>(KdbType::TIME_VECTOR)
		  && static_cast<int>(T::kdb_type) >= static_cast<int>(KdbType::LIST)
		) || static_cast<int>(T::kdb_type) == static_cast<int>(KdbType::TABLE)
	);

template <KdbColCapable T>
	struct ColDef
{
	const std::string_view m_name;
	T & m_col;
	ColDef(std::string_view name, T & col) : m_name{name}, m_col{col} {}
};

template <KdbColCapable T>
	struct ColRef
{
	const std::string_view m_name;
	T const * m_col;
	ColRef(std::string_view name) : m_name{name}, m_col{nullptr} {}
};

//-------------------------------------------------------------------------------- KdbTable
class KdbTable : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::TABLE;
	constexpr static bool is_sequence = true;

	std::unique_ptr<KdbSymbolVector> m_cols;
	std::unique_ptr<KdbList>         m_vals;
	KdbTable() : KdbBase(kdb_type), m_cols(), m_vals() {}

public:
	static ReadResult alloc(ReadBuf & buf, KdbBase **ptr);

public:
	KdbTable(const std::string_view & typs, const std::vector<std::string_view> & cols);
	template <KdbColCapable ...T> KdbTable(const std::vector<std::string_view> & names, T & ... cols);
	template <KdbColCapable ...T> KdbTable(const ColDef<T> & ... cols);
	template <KdbColCapable ...T> KdbTable(const ColDef<T> && ... cols);

	uint64_t count() const override;
	const KdbSymbolVector* key() const { return m_cols.get(); }
	const KdbList* value() const { return m_vals.get(); }
	uint64_t wireSz() const override;
	template<KdbColCapable T>
		void lookupCol(ColRef<T> & def) const;
	template<KdbColCapable ... T>
		void lookupCols(ColRef<T> & ...args) const { return (lookupCol(args), ...); }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

template <KdbColCapable ...T>
KdbTable::KdbTable(const std::vector<std::string_view> & names, T & ... cols)
 : KdbBase(KdbType::TABLE)
 , m_cols(std::make_unique<KdbSymbolVector>(names.size()))
 , m_vals(std::make_unique<KdbList>(names.size()))
{
	if (names.size() != sizeof...(cols)) {
		throw "names.length != cols.length";
	}
	if (names.size() == 0) {
		throw "names.size == 0";
	}

	// TODO what if any([0>x for x in typs])
	
	for (size_t i = 0 ; i < names.size() ; i++) {
		m_cols->push(names[i]);
	}
	(m_vals->push(cols), ...);
}

template <KdbColCapable ...T>
	KdbTable::KdbTable(const ColDef<T> & ... cols)
	: KdbBase(KdbTable::kdb_type)
 , m_cols(std::make_unique<KdbSymbolVector>(sizeof...(cols)))
 , m_vals(std::make_unique<KdbList>(sizeof...(cols)))
{
	static_assert(sizeof...(cols) > 0);
	((m_cols->push(cols.m_name), m_vals->push(cols.m_col)), ...);
}

template <KdbColCapable ...T>
	KdbTable::KdbTable(const ColDef<T> && ... cols)
	: KdbBase(KdbTable::kdb_type)
 , m_cols(std::make_unique<KdbSymbolVector>(sizeof...(cols)))
 , m_vals(std::make_unique<KdbList>(sizeof...(cols)))
{
	static_assert(sizeof...(cols) > 0);
	((m_cols->push(cols.m_name), m_vals->push(cols.m_col)), ...);
}

template<KdbColCapable T>
	void KdbTable::lookupCol(ColRef<T> & def) const
{
	int32_t idx = m_cols->indexOf(def.m_name);
	if (idx < 0) 
		throw std::format("No such column '{}'", def.m_name);

	const KdbBase *base = m_vals->getObj(idx);
	if (base->m_typ != T::kdb_type) {
		// throw std::format("Bad cast: requested type {} but have type {} in column '{}'", T::kdb_type, base->m_typ, def.m_name);
		throw std::format("Bad cast: column '{}'", def.m_name);
	}
	def.m_col = static_cast<const T*>(base);
}


//-------------------------------------------------------------------------------- KdbDict

struct KdbDict : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::DICT;

	std::unique_ptr<KdbBase> m_keys;
	std::unique_ptr<KdbBase> m_vals;

public:
	KdbDict() : KdbBase(KdbDict::kdb_type), m_keys{}, m_vals{} {}
	template <typename K, typename V> requires KdbKeySetCapable<K> && KdbColCapable<V>
		KdbDict(std::unique_ptr<K> k, std::unique_ptr<V> v);

	uint64_t count() const override;
	uint64_t wireSz() const override;
	std::optional<KdbType> getKeyType() const;
	std::optional<KdbType> getValueType() const;
	const KdbBase* getKeys() const { return m_keys.get(); }
	const KdbBase* getValues() const { return m_vals.get(); }
	template <typename T> requires KdbKeySetCapable<T> void getKeys(T **const ptr) const;
	template <typename T> requires KdbColCapable<T> void getValues(T **const ptr) const;

	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

template <typename K, typename V>
	requires KdbKeySetCapable<K> && KdbColCapable<V>
KdbDict::KdbDict(std::unique_ptr<K> k, std::unique_ptr<V> v)
 : KdbBase(KdbDict::kdb_type)
 , m_keys{std::move(k)}
 , m_vals{std::move(v)}
{
}

template <typename T> requires KdbKeySetCapable<T>
	void KdbDict::getKeys(T **const ptr) const
{
	if (!m_keys)
		*ptr = nullptr;
	else
		*ptr = reinterpret_cast<T*>(m_keys.get());
}

template <typename T> requires KdbColCapable<T>
	void KdbDict::getValues(T **const ptr) const
{
	if (!m_keys)
		*ptr = nullptr;
	else
		*ptr = reinterpret_cast<T*>(m_vals.get());
}

//-------------------------------------------------------------------------------- KdbFunction
struct KdbFunction : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::FUNCTION;

	std::vector<uint8_t> m_vec;

public:
	static ReadResult alloc(ReadBuf & buf, KdbBase **ptr);

	KdbFunction(uint64_t cap);
	KdbFunction(std::string_view fun);

	constexpr uint64_t count() const override { return -1; }
	uint64_t wireSz() const override;
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbPrimitive
template<typename T>
class KdbPrimitive : public KdbBase
{
	T m_val;

protected:
	KdbPrimitive(KdbType typ, T val) : KdbBase(typ), m_val(val) {}

public:
	constexpr uint64_t count() const override { return -1; }
	constexpr uint64_t wireSz() const override { return SZ_BYTE + SZ_BYTE; }
	T get() const { return m_val; }
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbUnaryPrimitive
struct KdbUnaryPrimitive : public KdbPrimitive<KdbUnary>
{
	constexpr static KdbType kdb_type = KdbType::UNARY_PRIMITIVE;
	KdbUnaryPrimitive() : mg7x::KdbUnaryPrimitive(static_cast<KdbUnary>(0)) {}
	KdbUnaryPrimitive(KdbUnary typ) : KdbPrimitive(KdbUnaryPrimitive::kdb_type, typ) {}
};

//-------------------------------------------------------------------------------- KdbBinaryPrimitive
struct KdbBinaryPrimitive : public KdbPrimitive<KdbBinary>
{
	constexpr static KdbType kdb_type = KdbType::BINARY_PRIMITIVE;
	KdbBinaryPrimitive() : KdbBinaryPrimitive(static_cast<KdbBinary>(0)) {}
	KdbBinaryPrimitive(KdbBinary val) : KdbPrimitive(KdbBinaryPrimitive::kdb_type, val) {}
};
//-------------------------------------------------------------------------------- KdbTernaryPrimitive
struct KdbTernaryPrimitive : public KdbPrimitive<KdbTernary>
{
	constexpr static KdbType kdb_type = KdbType::TERNARY_PRIMITIVE;
	KdbTernaryPrimitive() : KdbTernaryPrimitive(static_cast<KdbTernary>(0)) {}
	KdbTernaryPrimitive(KdbTernary val) : KdbPrimitive(KdbTernaryPrimitive::kdb_type, val) {}
};
//-------------------------------------------------------------------------------- KdbProjection
struct KdbProjection : public KdbBase
{
	constexpr static KdbType kdb_type = KdbType::PROJECTION;
	static ReadResult alloc(ReadBuf & buf, KdbBase **ptr);

	std::vector<std::unique_ptr<KdbBase>> m_vec;

	KdbProjection(uint64_t cap);
	KdbProjection(std::string_view fun, size_t n_args);

	constexpr uint64_t count() const override { return -1; }
	uint64_t wireSz() const override;
	void setArg(uint32_t idx, std::unique_ptr<KdbBase> val);
	ReadResult read(ReadBuf & buf) override;
	WriteResult write(WriteBuf & buf) const override;
};

//-------------------------------------------------------------------------------- KdbQuirks
template <class T> struct KdbQuirks;

template <> struct KdbQuirks<KdbShortVector>
{
	constexpr static int16_t NULL_VALUE    = NULL_SHORT;
	constexpr static int16_t POS_INFINITY  = POS_INF_SHORT;
	constexpr static int16_t NEG_INFINITY  = NEG_INF_SHORT;
	constexpr static KdbType TYPE          = KdbType::SHORT_VECTOR;
	constexpr static char const *TYPE_CHAR = "h";
	constexpr static bool ALWAYS_SUFFIX    = true;
	static auto format_to(auto out, const int16_t val)
	{
		return std::format_to(std::move(out), "{}", val);
	}
};

template <> struct KdbQuirks<KdbIntVector>
{
	constexpr static int32_t NULL_VALUE    = NULL_INT;
	constexpr static int32_t POS_INFINITY  = POS_INF_INT;
	constexpr static int32_t NEG_INFINITY  = NEG_INF_INT;
	constexpr static KdbType TYPE          = KdbType::INT_VECTOR;
	constexpr static char const *TYPE_CHAR = "i";
	constexpr static bool ALWAYS_SUFFIX    = true;
	static auto format_to(auto out, const int32_t val)
	{
		return std::format_to(std::move(out), "{}", val);
	}
};

template <> struct KdbQuirks<KdbLongVector>
{
	constexpr static int64_t NULL_VALUE    = NULL_LONG;
	constexpr static int64_t POS_INFINITY  = POS_INF_LONG;
	constexpr static int64_t NEG_INFINITY  = NEG_INF_LONG;
	constexpr static KdbType TYPE          = KdbType::LONG_VECTOR;
	constexpr static char const *TYPE_CHAR = "j";
	constexpr static bool ALWAYS_SUFFIX    = false;
	static auto format_to(auto out, const int64_t val)
	{
		return std::format_to(std::move(out), "{}", val);
	}
};

template <> struct KdbQuirks<KdbRealVector>
{
	constexpr static int32_t NULL_VALUE    = NULL_REAL;
	constexpr static int32_t POS_INFINITY  = POS_INF_REAL;
	constexpr static int32_t NEG_INFINITY  = NEG_INF_REAL;
	constexpr static KdbType TYPE          = KdbType::REAL_VECTOR;
	constexpr static char const *TYPE_CHAR = "e";
	constexpr static bool ALWAYS_SUFFIX    = true;
	static auto format_to(auto out, const int32_t val)
	{
		return std::format_to(std::move(out), "{}", std::bit_cast<float>(val));
	}
};

template <> struct KdbQuirks<KdbFloatVector>
{
	constexpr static int64_t NULL_VALUE    = NULL_FLOAT;
	constexpr static int64_t POS_INFINITY  = POS_INF_FLOAT;
	constexpr static int64_t NEG_INFINITY  = NEG_INF_FLOAT;
	constexpr static KdbType TYPE          = KdbType::FLOAT_VECTOR;
	constexpr static char const *TYPE_CHAR = "f";
	constexpr static bool ALWAYS_SUFFIX    = true;
	static auto format_to(auto out, const int64_t val)
	{
		return std::format_to(std::move(out), "{}", std::bit_cast<double>(val));
	}
};

template <> struct KdbQuirks<KdbTimestampVector>
{
	constexpr static int64_t NULL_VALUE    = NULL_LONG;
	constexpr static int64_t POS_INFINITY  = POS_INF_LONG;
	constexpr static int64_t NEG_INFINITY  = NEG_INF_LONG;
	constexpr static KdbType TYPE          = KdbType::TIMESTAMP_VECTOR;
	constexpr static char const *TYPE_CHAR = "p";
	constexpr static bool ALWAYS_SUFFIX    = false;
	static auto format_to(auto out, const int64_t val)
	{
		CharBuf<32> buf{};
		time::Timestamp::format_to(buf.out(), val, false);
		return buf.copyTo(out);
	}
};

template <> struct KdbQuirks<KdbMonthVector>
{
	constexpr static int32_t NULL_VALUE    = NULL_INT;
	constexpr static int32_t POS_INFINITY  = POS_INF_INT;
	constexpr static int32_t NEG_INFINITY  = NEG_INF_INT;
	constexpr static KdbType TYPE          = KdbType::MONTH_VECTOR;
	constexpr static char const *TYPE_CHAR = "m";
	constexpr static bool ALWAYS_SUFFIX    = true;
	static auto format_to(auto out, const int32_t val)
	{
		CharBuf<16> buf{};
		time::Month::format_to(buf.out(), val, false);
		return buf.copyTo(out);
	}
};

template <> struct KdbQuirks<KdbDateVector>
{
	constexpr static int32_t NULL_VALUE    = NULL_INT;
	constexpr static int32_t POS_INFINITY  = POS_INF_INT;
	constexpr static int32_t NEG_INFINITY  = NEG_INF_INT;
	constexpr static KdbType TYPE          = KdbType::DATE_VECTOR;
	constexpr static char const *TYPE_CHAR = "d";
	constexpr static bool ALWAYS_SUFFIX    = false;
	static auto format_to(auto out, const int32_t val)
	{
		CharBuf<16> buf{};
		time::Date::format_to(buf.out(), val, false);
		return buf.copyTo(out);
	}
};

template <> struct KdbQuirks<KdbTimespanVector>
{
	constexpr static int64_t NULL_VALUE    = NULL_LONG;
	constexpr static int64_t POS_INFINITY  = POS_INF_LONG;
	constexpr static int64_t NEG_INFINITY  = NEG_INF_LONG;
	constexpr static KdbType TYPE          = KdbType::TIMESPAN_VECTOR;
	constexpr static char const *TYPE_CHAR = "n";
	constexpr static bool ALWAYS_SUFFIX    = false;
	static auto format_to(auto out, const int64_t val)
	{
		CharBuf<24> buf{};
		time::Timespan::format_to(buf.out(), val, false);
		return buf.copyTo(out);
	}
};

template <> struct KdbQuirks<KdbMinuteVector>
{
	constexpr static int32_t NULL_VALUE    = NULL_INT;
	constexpr static int32_t POS_INFINITY  = POS_INF_INT;
	constexpr static int32_t NEG_INFINITY  = NEG_INF_INT;
	constexpr static KdbType TYPE          = KdbType::MINUTE_VECTOR;
	constexpr static char const *TYPE_CHAR = "u";
	constexpr static bool ALWAYS_SUFFIX    = false;
	static auto format_to(auto out, const int32_t val)
	{
		CharBuf<16> buf{};
		time::Minute::format_to(buf.out(), val, false);
		return buf.copyTo(out);
	}
};

template <> struct KdbQuirks<KdbSecondVector>
{
	constexpr static int32_t NULL_VALUE    = NULL_INT;
	constexpr static int32_t POS_INFINITY  = POS_INF_INT;
	constexpr static int32_t NEG_INFINITY  = NEG_INF_INT;
	constexpr static KdbType TYPE          = KdbType::SECOND_VECTOR;
	constexpr static char const *TYPE_CHAR = "v";
	constexpr static bool ALWAYS_SUFFIX    = false;
	static auto format_to(auto out, const int32_t val)
	{
		CharBuf<16> buf{};
		time::Second::format_to(buf.out(), val, false);
		return buf.copyTo(out);
	}
};

template <> struct KdbQuirks<KdbTimeVector>
{
	constexpr static int32_t NULL_VALUE    = NULL_INT;
	constexpr static int32_t POS_INFINITY  = POS_INF_INT;
	constexpr static int32_t NEG_INFINITY  = NEG_INF_INT;
	constexpr static KdbType TYPE          = KdbType::TIME_VECTOR;
	constexpr static char const *TYPE_CHAR = "t";
	constexpr static bool ALWAYS_SUFFIX    = false;
	static auto format_to(auto out, const int32_t val)
	{
		CharBuf<16> buf{};
		time::Time::format_to(buf.out(), val, false);
		return buf.copyTo(out);
	}
};
//-------------------------------------------------------------------------------- VectorPrint
template <class T> struct VectorPrint;

template <class T>
auto vectorPrint(auto & ctx, const T & vec) {
	if (0 == vec.m_vec.size()) {
		return std::format_to(ctx.out(), "({}h$())", abs(static_cast<int>(KdbQuirks<T>::TYPE)));
	}
	else if (1 == vec.count()) {
		if (KdbQuirks<T>::NULL_VALUE == vec.m_vec[0]) {
			return std::format_to(ctx.out(), "(enlist 0N{})", KdbQuirks<T>::TYPE_CHAR);
		}
		if (KdbQuirks<T>::POS_INFINITY == vec.m_vec[0]) {
			return std::format_to(ctx.out(), "(enlist 0W{})", KdbQuirks<T>::TYPE_CHAR);
		}
		if (KdbQuirks<T>::NEG_INFINITY == vec.m_vec[0]) {
			return std::format_to(ctx.out(), "(enlist -0W{})", KdbQuirks<T>::TYPE_CHAR);
		}
		std::string tmp{};
		KdbQuirks<T>::format_to(std::back_inserter(tmp), vec.m_vec[0]);
		return std::format_to(ctx.out(), "(enlist {}{})", tmp, KdbQuirks<T>::ALWAYS_SUFFIX ? KdbQuirks<T>::TYPE_CHAR : "");
	}
	std::string tmp{};
	bool none_value = true;
	for (size_t i = 0 ; i < vec.count() ; i++) {
		if (i > 0) {
			tmp.push_back(' ');
		}
		if (KdbQuirks<T>::NULL_VALUE == vec.m_vec[i]) {
			tmp.append("0N");
		}
		else if (KdbQuirks<T>::POS_INFINITY == vec.m_vec[i]) {
			tmp.append("0W");
		}
		else if (KdbQuirks<T>::NEG_INFINITY == vec.m_vec[i]) {
			tmp.append("-0W");
		}
		else {
			none_value = false;
			KdbQuirks<T>::format_to(std::back_inserter(tmp), vec.m_vec[i]);
		}
	}
	if (KdbQuirks<T>::ALWAYS_SUFFIX || none_value) {
		std::format_to(std::back_inserter(tmp), "{}", KdbQuirks<T>::TYPE_CHAR);
	}
	return std::format_to(ctx.out(), "{}", tmp);
}
//-------------------------------------------------------------------------------- /namespace
}; // end namespace mg7x

//-------------------------------------------------------------------------------- Formatters
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0645r10.html
template<std::derived_from<mg7x::KdbBase> Derived, typename CharT>
struct std::formatter<Derived, CharT> {
	// constexpr typename std::basic_format_parse_context<CharT>::iterator parse(std::format_parse_context & ctx) { return ctx.begin(); }
	constexpr auto parse(std::format_parse_context & ctx) { return ctx.begin(); }
	// std::basic_format_context<std::__format::_Sink_iter<char>, char>
  // template<typename O> typename std::basic_format_context<O, CharT>::iterator format(const Derived & obj, std::basic_format_context<O, CharT> & ctx) const
	template <class FormatContext> FormatContext::iterator format(const Derived & obj, FormatContext& ctx) const
	{
		switch(obj.m_typ) {
			case mg7x::KdbType::EXCEPTION:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbException&>(obj));
			case mg7x::KdbType::TIME_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTimeAtom&>(obj));
			case mg7x::KdbType::SECOND_ATOM:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbSecondAtom&>(obj));
			case mg7x::KdbType::MINUTE_ATOM:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbMinuteAtom&>(obj));
			case mg7x::KdbType::TIMESPAN_ATOM:         return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTimespanAtom&>(obj));
			case mg7x::KdbType::DATE_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbDateAtom&>(obj));
			case mg7x::KdbType::MONTH_ATOM:            return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbMonthAtom&>(obj));
			case mg7x::KdbType::TIMESTAMP_ATOM:        return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTimestampAtom&>(obj));
			case mg7x::KdbType::SYMBOL_ATOM:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbSymbolAtom&>(obj));
			case mg7x::KdbType::CHAR_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbCharAtom&>(obj));
			case mg7x::KdbType::FLOAT_ATOM:            return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbFloatAtom&>(obj));
			case mg7x::KdbType::REAL_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbRealAtom&>(obj));
			case mg7x::KdbType::LONG_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbLongAtom&>(obj));
			case mg7x::KdbType::INT_ATOM:              return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbIntAtom&>(obj));
			case mg7x::KdbType::SHORT_ATOM:            return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbShortAtom&>(obj));
			case mg7x::KdbType::GUID_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbGuidAtom&>(obj));
			case mg7x::KdbType::BYTE_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbByteAtom&>(obj));
			case mg7x::KdbType::BOOL_ATOM:             return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbBoolAtom&>(obj));
			case mg7x::KdbType::LIST:                  return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbList&>(obj));
			case mg7x::KdbType::BOOL_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbBoolVector&>(obj));
			case mg7x::KdbType::GUID_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbGuidVector&>(obj));
			case mg7x::KdbType::BYTE_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbByteVector&>(obj));
			case mg7x::KdbType::SHORT_VECTOR:          return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbShortVector&>(obj));
			case mg7x::KdbType::INT_VECTOR:            return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbIntVector&>(obj));
			case mg7x::KdbType::LONG_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbLongVector&>(obj));
			case mg7x::KdbType::REAL_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbRealVector&>(obj));
			case mg7x::KdbType::FLOAT_VECTOR:          return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbFloatVector&>(obj));
			case mg7x::KdbType::CHAR_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbCharVector&>(obj));
			case mg7x::KdbType::SYMBOL_VECTOR:         return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbSymbolVector&>(obj));
			case mg7x::KdbType::TIMESTAMP_VECTOR:      return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTimestampVector&>(obj));
			case mg7x::KdbType::MONTH_VECTOR:          return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbMonthVector&>(obj));
			case mg7x::KdbType::DATE_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbDateVector&>(obj));
			case mg7x::KdbType::TIMESPAN_VECTOR:       return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTimespanVector&>(obj));
			case mg7x::KdbType::MINUTE_VECTOR:         return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbMinuteVector&>(obj));
			case mg7x::KdbType::SECOND_VECTOR:         return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbSecondVector&>(obj));
			case mg7x::KdbType::TIME_VECTOR:           return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTimeVector&>(obj));
			case mg7x::KdbType::TABLE:                 return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTable&>(obj));
			case mg7x::KdbType::DICT:                  return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbDict&>(obj));
			case mg7x::KdbType::FUNCTION:              return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbFunction&>(obj));
			case mg7x::KdbType::UNARY_PRIMITIVE:       return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbUnaryPrimitive&>(obj));
			case mg7x::KdbType::BINARY_PRIMITIVE:      return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbBinaryPrimitive&>(obj));
			case mg7x::KdbType::TERNARY_PRIMITIVE:     return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbTernaryPrimitive&>(obj));
			case mg7x::KdbType::PROJECTION:            return std::format_to(ctx.out(), "{}", dynamic_cast<const mg7x::KdbProjection&>(obj));
			default:                                   return std::format_to(ctx.out(), "('`MISSING_CONVERSION)");
		}
  }
};

template<>
struct std::formatter<mg7x::KdbException>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbException & err, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "'{}", err.m_msg);
	}
};


template<>
struct std::formatter<mg7x::KdbTimeAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTimeAtom & atm, FormatContext & ctx) const
	{
		mg7x::CharBuf<16> buf{};
		mg7x::time::Time::format_to(buf.out(), atm.m_val);
		return buf.copyTo(ctx.out());
	}
};

template<>
struct std::formatter<mg7x::KdbSecondAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbSecondAtom & atm, FormatContext & ctx) const
	{
		mg7x::CharBuf<16> buf{};
		mg7x::time::Second::format_to(buf.out(), atm.m_val);
		return buf.copyTo(ctx.out());
	}
};

template<>
struct std::formatter<mg7x::KdbMinuteAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbMinuteAtom & atm, FormatContext & ctx) const
	{
		mg7x::CharBuf<16> buf{};
		mg7x::time::Minute::format_to(buf.out(), atm.m_val);
		return buf.copyTo(ctx.out());
	}
};

template<>
struct std::formatter<mg7x::KdbTimespanAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTimespanAtom & atm, FormatContext & ctx) const
	{
		mg7x::CharBuf<24> buf{};
		mg7x::time::Timespan::format_to(buf.out(), atm.m_val);
		return buf.copyTo(ctx.out());
	}
};

template<>
struct std::formatter<mg7x::KdbDateAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbDateAtom & atm, FormatContext & ctx) const
	{
		mg7x::CharBuf<16> buf{};
		mg7x::time::Date::format_to(buf.out(), atm.m_val);
		return buf.copyTo(ctx.out());
	}
};

template<>
struct std::formatter<mg7x::KdbMonthAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbMonthAtom & atm, FormatContext & ctx) const
	{
		mg7x::CharBuf<16> buf{};
		mg7x::time::Month::format_to(buf.out(), atm.m_val, true);
		return buf.copyTo(ctx.out());
	}
};

template<>
struct std::formatter<mg7x::KdbTimestampAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTimestampAtom & atm, FormatContext & ctx) const
	{
		mg7x::CharBuf<32> buf{};
		mg7x::time::Timestamp::format_to(buf.out(), atm.m_val);
		return buf.copyTo(ctx.out());
	}
};

template<>
struct std::formatter<mg7x::KdbCharAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbCharAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "\"{:c}\"", atm.m_val);
	}
};

template<>
struct std::formatter<mg7x::KdbSymbolAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbSymbolAtom & atm, FormatContext & ctx) const
	{
		if (atm.m_val.empty())
			return std::format_to(ctx.out(), "`");
		if (std::string::npos != atm.m_val.find(' '))
			return std::format_to(ctx.out(), "(`$\"{}\")", atm.m_val);
		return std::format_to(ctx.out(), "`{}", atm.m_val);
	}
};

template<>
struct std::formatter<mg7x::KdbFloatAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbFloatAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "{:0.3g}f", atm.asDouble());
	}
};

template<>
struct std::formatter<mg7x::KdbRealAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbRealAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "{:0.3g}e", atm.asFloat());
	}
};

template<>
struct std::formatter<mg7x::KdbLongAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbLongAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "{}", atm.m_val);
	}
};

template<>
struct std::formatter<mg7x::KdbIntAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbIntAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "{}i", atm.m_val);
	}
};

template<>
struct std::formatter<mg7x::KdbShortAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbShortAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "{}h", atm.m_val);
	}
};

template<>
struct std::formatter<mg7x::KdbByteAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbByteAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "{:#2x}", static_cast<uint8_t>(atm.m_val));
	}
};

#define _MG_GUID_FMT_STR "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}"
#define _MG_GUID_FMT_ELM(e) (e)[0], (e)[1], (e)[2], (e)[3], (e)[4], (e)[5], (e)[6], (e)[7] , (e)[8], (e)[9], (e)[10], (e)[11], (e)[12], (e)[13], (e)[14], (e)[15]

template<>
struct std::formatter<mg7x::KdbGuidAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbGuidAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "(\"G\"$\"" _MG_GUID_FMT_STR "\")", _MG_GUID_FMT_ELM(atm.m_val));
	}
};

template<>
struct std::formatter<mg7x::KdbBoolAtom>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbBoolAtom & atm, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "{:d}b", atm.m_val);
	}
};

template<>
struct std::formatter<mg7x::KdbList>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbList & lst, FormatContext & ctx) const
	{
		auto out = ctx.out();
		if (0 == lst.count()) {
			return std::format_to(out, "()");
		}
		else if (1 == lst.count()) {
			const mg7x::KdbBase *ptr = lst.getObj(0);
			std::format_to(out, "(enlist {})", *ptr);
		}
		else {
			std::format_to(out, "(");
			for (size_t i = 0 ; i < lst.count() ; i++) {
				if (i > 0)
					std::format_to(out, ";");
				std::format_to(out, "{}", *(lst.getObj(i)));
			}
			std::format_to(out, ")");
		}
		return out;
	}

};

template<>
struct std::formatter<mg7x::KdbBoolVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbBoolVector & vec, FormatContext & ctx) const
	{
		auto out = ctx.out();

		if (0 == vec.count())
			return std::format_to(out, "(`boolean$())");

		if (1 == vec.count())
			return std::format_to(out, "(enlist {:d}b)", vec.m_vec[0]);
		
		for (size_t i = 0 ; i < vec.m_vec.size() ; i++) {
			std::format_to(out, "{:d}", vec.m_vec[i]);
		}
		return std::format_to(out, "b");
	}
};

template<>
struct std::formatter<mg7x::KdbGuidVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbGuidVector & vec, FormatContext & ctx) const
	{
		auto out = ctx.out();

		if (0 == vec.count())
			return std::format_to(out, "(`guid$())");

		if (1 == vec.count()) {
			return std::format_to(out, "(enlist \"G\"$\"" _MG_GUID_FMT_STR "\")", _MG_GUID_FMT_ELM(vec.m_vec[0]));
		}
		
		std::format_to(out, "(\"G\"$(\"");
		for (size_t i = 0 ; i < vec.m_vec.size() ; i++) {
			
			if (i > 0)
				std::format_to(out, "\";\"");
			std::format_to(out, _MG_GUID_FMT_STR, _MG_GUID_FMT_ELM(vec.m_vec[i]));
		}
		return std::format_to(out, "\"))");
	}
};

template<>
struct std::formatter<mg7x::KdbByteVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbByteVector & vec, FormatContext & ctx) const
	{
		auto out = ctx.out();

		if (0 == vec.count())
			return std::format_to(out, "(`byte$())");

		if (1 == vec.count())
			return std::format_to(out, "(enlist {:#02x})", static_cast<uint8_t>(vec.m_vec[0]));
		
		std::format_to(out, "0x");
		for (size_t i = 0 ; i < vec.m_vec.size() ; i++) {
			std::format_to(out, "{:02x}", static_cast<uint8_t>(vec.m_vec[i]));
		}
		return out;
	}
};

#undef _MG_GUID_FMT_ELM
#undef _MG_GUID_FMT_STR

template<>
struct std::formatter<mg7x::KdbShortVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbShortVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbShortVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbIntVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbIntVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbIntVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbLongVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbLongVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbLongVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbRealVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbRealVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbRealVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbFloatVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbFloatVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbFloatVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbCharVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbCharVector & vec, FormatContext & ctx) const
	{
		if (0 == vec.m_vec.size())
			return std::format_to(ctx.out(), "(`char$())");
		
		if (1 == vec.m_vec.size())
			return std::format_to(ctx.out(), "(enlist\"{}\")", vec.getChar(0));
		
		std::string_view str = vec.getString();
		if (std::string::npos == str.find("\""))
			return std::format_to(ctx.out(), "\"{}\"", str);
		std::string tmp{};
		for (const auto& c : str) {
			if (c == '"')
				tmp.append("\\\"");
			else
				tmp.push_back(c);
		}
		return std::format_to(ctx.out(), "\"{}\"", tmp);
	}
};

template<>
struct std::formatter<mg7x::KdbSymbolVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbSymbolVector & vec, FormatContext & ctx) const
	{
		if (0 == vec.count())
			return std::format_to(ctx.out(), "(`$())");

		if (1 == vec.count()) {
			std::string_view sv = vec.getString(0);
			if (std::string::npos == sv.find(' '))
				return std::format_to(ctx.out(), "(enlist`{})", sv);
			else
				return std::format_to(ctx.out(), "(enlist`$\"{}\")", sv);
		}

		std::string tmp{};
		bool hasSpace = false;
		for (uint64_t i = 0 ; !hasSpace && i < vec.count() ; i++) {
			hasSpace = std::string::npos != vec.getString(i).find(' ');
		}
		if (hasSpace) {
			tmp.append("(`$(");
			for (uint64_t i = 0 ; i < vec.count() ; i++) {
				if (i > 0)
					tmp.push_back(';');
				std::format_to(std::back_inserter(tmp), "\"{}\"", vec.getString(i));
			}
			tmp.append("))");
		}
		else {
			for (uint64_t i = 0 ; i < vec.count() ; i++) {
				std::format_to(std::back_inserter(tmp), "`{}", vec.getString(i));
			}
		}
		return std::format_to(ctx.out(), "{}", tmp);
	}
};

template<>
struct std::formatter<mg7x::KdbTimestampVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTimestampVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbTimestampVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbMonthVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbMonthVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbMonthVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbDateVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbDateVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbDateVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbTimespanVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTimespanVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbTimespanVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbMinuteVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbMinuteVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbMinuteVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbSecondVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbSecondVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbSecondVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbTimeVector>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTimeVector & vec, FormatContext & ctx) const
	{
		return mg7x::vectorPrint<mg7x::KdbTimeVector>(ctx, vec);
	}
};

template<>
struct std::formatter<mg7x::KdbTable>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTable & tbl, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "(flip {}!{})", *tbl.key(), *tbl.value());
	}
};

template<>
struct std::formatter<mg7x::KdbDict>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbDict & dct, FormatContext & ctx) const
	{
		if (dct.getKeyType().has_value() && dct.getValueType().has_value()) {
			return std::format_to(ctx.out(), "({}!{})", *dct.getKeys(), *dct.getValues());
		}
		return std::format_to(ctx.out(), "(null!null)");
	}
};

template<>
struct std::formatter<mg7x::KdbFunction>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbFunction & fnc, FormatContext & ctx) const
	{
		std::string tmp{};
		for (const auto c : fnc.m_vec) {
			tmp.push_back(c);
		}
		return std::format_to(ctx.out(), "{}", tmp);
	}
};

template<>
struct std::formatter<mg7x::KdbUnaryPrimitive>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbUnaryPrimitive & upr, FormatContext & ctx) const
	{
		if (upr.get() == mg7x::KdbUnary::ELIDED_VALUE)
			return ctx.out(); // doesn't show up in argument lists, which is where you might find it

		static const char *ary[] = {
				"::", "+:", "-:", "*:", "%:", "&:", "|:", "^:", "=:", "<:", ">:", "$:", ",:", "#:", "_:", "~:", "!:",
				"?:", "@:", ".:", "0::", "1::", "2::", "avg", "last", "sum", "prd", "min", "max", "exit", "getenv", "abs",
				"sqrt", "log", "exp", "sin", "asin", "cos", "acos", "tan", "atan", "enlist", "var"
		};
		return std::format_to(ctx.out(), "{}", ary[static_cast<size_t>(upr.get())]);
	}
};

template<>
struct std::formatter<mg7x::KdbBinaryPrimitive>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbBinaryPrimitive & bin, FormatContext & ctx) const
	{
		static const char *ary[] = {
			":", "+", "-", "*", "%", "&", "|", "^", "=", "<", ">", "$", ",", "#", "_", "~", "!",
			"?", "@", ".", "0:", "1:", "2:", "in", "within", "like", "bin", "ss", "insert", "wsum",
			"wavg", "div", "xexp", "setenv", "binr", "cov", "cor"
		};
		return std::format_to(ctx.out(), "{}", ary[static_cast<size_t>(bin.get())]);
	}
};

template<>
struct std::formatter<mg7x::KdbTernaryPrimitive>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbTernaryPrimitive & trn, FormatContext & ctx) const
	{
		static const char *ary[] = { "'", "/", "\\", "':", "/:", "\\:" };
		return std::format_to(ctx.out(), "{}", ary[static_cast<size_t>(trn.get())]);
	}
};

template<>
struct std::formatter<mg7x::KdbProjection>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::KdbProjection & prj, FormatContext & ctx) const
	{
		std::string tmp{};
		for (size_t i = 0 ; i < prj.m_vec.size() ; i++) {
			if (i == 1)
				tmp.push_back('[');
			else if (i > 1)
				tmp.push_back(';');
			tmp.append(std::format("{}", *prj.m_vec[i]));
		}
		tmp.push_back(']');
		return std::format_to(ctx.out(), "{}", tmp);
	}
};

//-------------------------------------------------------------------------------- namespace mg7x
namespace mg7x {

class KdbIpcDecompressor
{
	uint64_t m_ipc_len;
	uint64_t m_msg_len;

	uint32_t m_idx{0};   // i
	uint64_t m_off{0};   // s
	uint64_t m_rdx{0};   // tracks offset into src message
	uint64_t m_chx{0};   // p: tracks the trailing cache-pointer 
	uint32_t m_bit{0};   // f
	uint32_t m_lbh[256]; // aa

	std::unique_ptr<int8_t[]> m_dst;

	public:
		KdbIpcDecompressor(uint64_t ipc_len, uint64_t msg_len, std::unique_ptr<int8_t[]> dst);
		uint64_t blockReadSz(ReadBuf & buf) const;
		uint64_t uncompress(ReadBuf & buf);
		bool isComplete() const;
		uint64_t getUsedInputCount() const;
		ReadBuf getReadBuf(int64_t csr) const;
};

struct ReadMsgResult
{
	ReadResult               result;
	uint64_t                 bytes_consumed;
	std::unique_ptr<KdbBase> message;
};

class KdbIpcMessageReader
{
	KdbMsgType m_msg_typ;
	uint64_t   m_ipc_len{0};
	uint64_t   m_msg_len{0};
	uint64_t   m_byt_usd{0};
	uint64_t   m_byt_dez{0};
	bool       m_compressed;
	std::unique_ptr<KdbBase> m_msg;
	std::unique_ptr<KdbIpcDecompressor> m_inflater;

	bool readMsgHdr(ReadBuf & buf, ReadMsgResult & result);
	bool readMsgData(ReadBuf & buf, ReadMsgResult & result);
	bool readMsgData1(ReadBuf & buf, ReadMsgResult & result);

	public:
		bool readMsg(const void *src, uint64_t len, ReadMsgResult & result);
		uint64_t getIpcLength() const;
		uint64_t getInputBytesConsumed() const;
		uint64_t getMsgBytesDeserialized() const;


};

struct KdbUtil
{
	static
	size_t writeLoginMsg(std::string_view usr, std::string_view pwd, std::unique_ptr<char> & ptr);
};

class KdbIpcMessageWriter
{
	KdbMsgType               m_msg_typ;
	size_t                   m_ipc_len;
	std::shared_ptr<KdbBase> m_root;
	size_t                   m_byt_rem;

	public:
		KdbIpcMessageWriter(KdbMsgType msg_typ, std::shared_ptr<KdbBase> msg);

		size_t bytesRemaining() const;
		WriteResult write(void *dst, size_t cap);
};
//-------------------------------------------------------------------------------- /namespace
}; // end namespace mg7x (again)

#endif // defined __mg7x_KdbType__H__
