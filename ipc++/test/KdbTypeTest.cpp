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

#include <cstring>   // strlen
#include <memory>    // make_shared
#include <ostream>
#include <stdexcept> // runtime_error
#include <string>    // string
#include <memory>    // make_shared
#include <algorithm> // std::equal
#include <cmath>     // std::isnan

#include "KdbType.h"

#include <gtest/gtest.h>

#include "../src/KdbType.cpp"

using namespace mg7x;
using namespace std;

namespace mg7x::test {

std::unique_ptr<int8_t[]> fromHex(const char *hex, int64_t *len) {
	std::string hex_str{hex};
	if (0 != (hex_str.length() % 2))
		throw std::runtime_error("Non-even string length");

	const auto pos = hex_str.find("0x", 0, 2);
	if (string::npos != pos)
		hex_str = std::string(hex_str.substr(2));

	*len = hex_str.length() / 2;
	auto dst = make_unique<int8_t[]>(*len);
	for (size_t i = 0 ; i < hex_str.length() ; i += 2) {
		dst[i/2] = (int8_t)std::stoi(hex_str.substr(i, 2), nullptr, 16);
	}
	return dst;
}

template<typename T>
static std::unique_ptr<T> byteByByte(const char *hex)
{
	static_assert(std::is_base_of<KdbBase,T>::value, "T must derive from KdbBase");
	int64_t len;
	std::unique_ptr<int8_t[]> ary = fromHex(hex, &len);
	if (len < 2)
		throw std::runtime_error("Unpossible");

	const int8_t *src = ary.get();

	KdbBase *ptr = nullptr;

	int64_t off = 1;

	ReadBuf buf{src, 0};
	for (int64_t i = 1 ; i <= len ; i++) {
		buf.rewind();
		buf.setLength(i);
		if (nullptr == ptr) {
			ReadResult rr = newInstance(buf, &ptr);
			EXPECT_EQ(nullptr == ptr ? ReadResult::RD_INCOMPLETE : ReadResult::RD_OK, rr);
		}
		else {
			// buf.ffwd(sizeof(int8_t));
			ReadResult rr = ptr->read(buf);
			//std::cout << "Offered buf.remaining " << buf.remaining() << " bytes at offset " << (SZ_BYTE - off) << ", got " << red << " bytes in reply\n";
			EXPECT_EQ(i == len ? ReadResult::RD_OK : ReadResult::RD_INCOMPLETE, rr);
		}

	}
	// TODO evaluate content of buf
	EXPECT_EQ(len, ptr->wireSz());

	return std::unique_ptr<T>(reinterpret_cast<T*>(ptr));
}

static bool testWrite(const KdbBase *obj, const char *exp) {
	const size_t wsz = obj->wireSz();
	auto ptr = std::make_unique<int8_t[]>(wsz);
	WriteBuf buf{ptr.get(), wsz};
	bool isOk = false;
	if (WriteResult::WR_OK != obj->write(buf)) {
		std::cerr << "FAILURE in 'testWrite': to write data for type " << static_cast<int>(obj->m_typ) << std::endl;
	}
	else if (wsz != buf.cursorOff()) {
		std::cerr << "FAILURE in 'testWrite': unexpected number of bytes written (type " << static_cast<int>(obj->m_typ) << ')' << std::endl;
	}
	else {
		int64_t len = 0;
		std::unique_ptr<int8_t[]> hxp = fromHex(exp, &len);
		if (len != wsz) {
			std::cerr << "FAILURE in 'testWrite': mismatch between expected hex.length and number of bytes written (type " << static_cast<int>(obj->m_typ) << ')' << std::endl;
		}
		else {
			isOk = true;
			for (size_t i = 0 ; i < wsz ; i++) {
				if (hxp[i] != ptr[i]) {
					std::cerr << "FAILURE in 'testWrite': invalid output byte at index [" << i << "] for type " << static_cast<int>(obj->m_typ) << std::endl;
					isOk = false;
					break;
				}
			}
		}
	}
	return isOk;
}



template <typename T>
static std::unique_ptr<T> testReadAndStr(const char *hex, std::string expected)
{
	std::unique_ptr<T> ptr = byteByByte<T>(hex);
	EXPECT_EQ(expected, std::format("{}", *ptr.get()));
	return ptr;
}

TEST(KdbTypeTest, TestKdbBoolVectorRead)
{
	// q)8_-8!0N! 10101010b
	// 10101010b
	const auto vec = testReadAndStr<KdbBoolVector>("0x0100080000000100010001000100", "10101010b");
	EXPECT_EQ(8, vec->count());
	for (size_t i = 0 ; i < vec->count() ; i++) {
		EXPECT_EQ((1 + i) % 2, vec->getBool(i));
	}
}

TEST(KdbTypeTest, TestKdbByteVectorRead)
{
	// 8_-8!4h1+til 6
	const auto vec = testReadAndStr<KdbByteVector>("0x040006000000010203040506", "0x010203040506"s);
	EXPECT_EQ(6, vec->count());
	for (int i = 0 ; i < 6 ; i++) {
		EXPECT_EQ(i+1, vec->getByte(i));
	}
}

TEST(KdbTypeTest, TestKdbByteVectorRead1)
{
	// q)8_-8! enlist 0xff
	// 0x040001000000ff
	const auto vec = testReadAndStr<KdbByteVector>("0x040001000000ff", "(enlist 0xff)"s);
	EXPECT_EQ(1, vec->count());
	EXPECT_EQ(0xFF, 0xFF & vec->getByte(0));
}	

TEST(KdbTypeTest, TestKdbByteVectorRead0)
{
	// q)8_-8! 4h$()
	// 0x040000000000
	const auto vec = testReadAndStr<KdbByteVector>("0x040000000000", "(`byte$())"s);
	EXPECT_EQ(0, vec->count());
}

TEST(KdbTypeTest, TestKdbCharVectorRead)
{
	// q)8_-8!"Neil Gaiman"
	const auto vec = testReadAndStr<KdbCharVector>("0x0a000b0000004e65696c204761696d616e", "\"Neil Gaiman\""s);
	const auto str = "Neil Gaiman"s;
	EXPECT_EQ(str.length(), vec->count());
	EXPECT_EQ(std::string_view{str}, vec->getString());
	EXPECT_EQ('G', vec->getChar(5));
}

TEST(KdbTypeTest, TestKdbShortVector)
{
	// q)8_-8!0N! 1 2 3 4 0N 6h
	// 1 2 3 4 0N 6h
	constexpr uint64_t LEN = 6;
	const auto vec = testReadAndStr<KdbShortVector>("0x050006000000010002000300040000800600", "1 2 3 4 0N 6h"s);
	EXPECT_EQ(LEN, vec->count());
	uint64_t i = 0;
	for (auto const i16 : std::array<int16_t, LEN>{1, 2, 3, 4, NULL_SHORT, 6}) {
		EXPECT_EQ(i16, vec->getShort(i));
		i += 1;
	}
}

TEST(KdbTypeTest, TestKdbIntVector)
{
	// q)8_-8!0N! 1 2 3 4 0N 6i
	// 1 2 3 4 0N 6i
	constexpr uint64_t LEN = 6;
	const auto vec = testReadAndStr<KdbIntVector>("0x060006000000010000000200000003000000040000000000008006000000", "1 2 3 4 0N 6i"s);
	EXPECT_EQ(LEN, vec->count());
	uint64_t i = 0;
	for (auto const i32 : std::array<int32_t, LEN>{1, 2, 3, 4, NULL_INT, 6}) {
		EXPECT_EQ(i32, vec->getInt(i));
		i += 1;
	}
}

TEST(KdbTypeTest, TestKdbLongVector)
{
	// q)8_-8!0N! 1 0N 2 0N 3j
	// 1 0N 2 0N 3
	constexpr uint64_t LEN = 5;
	const auto vec = testReadAndStr<KdbLongVector>("0x07000500000001000000000000000000000000000080020000000000000000000000000000800300000000000000", "1 0N 2 0N 3"s);
	EXPECT_EQ(LEN, vec->count());
	uint64_t i = 0;
	for (auto const i64 : std::array<int64_t, LEN>{1, NULL_LONG, 2, NULL_LONG, 3}) {
		EXPECT_EQ(i64, vec->getLong(i));
		i += 1;
	}
}

TEST(KdbTypeTest, TestKdbRealVector)
{
	// q)8_-8!0N! 1.1 1.2 0N 1.4e
	// 1.1 1.2 0N 1.4e
	constexpr uint64_t LEN = 4;
	const auto vec = testReadAndStr<KdbRealVector>("0x080004000000cdcc8c3f9a99993f0000c0ff3333b33f", "1.1 1.2 0N 1.4e"s);
	EXPECT_EQ(LEN, vec->count());
	uint64_t i = 0;
	for (auto const f32 : std::array<float, LEN>{1.1, 1.2, std::bit_cast<float>(NULL_REAL), 1.4}) {
		if (NULL_REAL != std::bit_cast<int32_t>(f32)) {
			EXPECT_FLOAT_EQ(f32, vec->getReal(i));
		}
		else {
			EXPECT_TRUE(std::isnan(vec->getReal(i)));
		}
		i += 1;
	}
}

TEST(KdbTypeTest, TestKdbFloatVector)
{
	// q)8_-8!0N!1.1 1.2 0N 1.4f
	// 1.1 1.2 0n 1.4
	const auto vec = testReadAndStr<KdbFloatVector>("0x0900040000009a9999999999f13f333333333333f33f000000000000f8ff666666666666f63f", "1.1 1.2 0N 1.4f"s);
	EXPECT_FLOAT_EQ(4, vec->count());
	EXPECT_FLOAT_EQ(1.1, vec->getFloat(0));
	EXPECT_FLOAT_EQ(1.2, vec->getFloat(1));
	EXPECT_TRUE(std::isnan(vec->getFloat(2)));
	EXPECT_FLOAT_EQ(1.4, vec->getFloat(3));
}

TEST(KdbTypeTest, TestKdbTimestampVectorRead)
{
	// q)zps:2023.08.23D21:48:35.300769000 2023.08.23D21:50:47.166448000 0N
	// q)8_-8!zps
	const auto vec = testReadAndStr<KdbTimestampVector>("0x0c0003000000e89c5118aad45a0a803120ccc8d45a0a0000000000000080", "2023.08.23D21:48:35.300769000 2023.08.23D21:50:47.166448000 0N"s);
	std::array<int64_t,3> exp{
		time::Timestamp::getUTC(2023, 8, 23, 21, 48, 35, 300769000),
		time::Timestamp::getUTC(2023, 8, 23, 21, 50, 47, 166448000),
		NULL_LONG
	};
	for (size_t i = 0 ; i < exp.size() ; i++) {
		EXPECT_EQ(exp[i], vec->getTimestamp(i));
	}
}

TEST(KdbTypeTest, TestKdbMonthVector)
{
	// q)8_-8! 2023.01 2023.02 0N 2023.04m
	const auto vec = testReadAndStr<KdbMonthVector>("0x0d000400000014010000150100000000008017010000", "2023.01 2023.02 0N 2023.04m"s);
	// q)6h$ 2023.01 2023.02 0N 2023.04m
	std::array<int32_t, 4> exp{276, 277, NULL_INT, 279};
	for (size_t i = 0 ; i < exp.size() ; i++) {
		EXPECT_EQ(exp[i], vec->getMonth(i));
	}
}

TEST(KdbTypeTest, TestKdbGuidVector) {
	// q)show g:-2?0Ng
	// 23c06889-fda0-7f3d-d5b1-9af6c7fcdae7 07998543-9d01-5704-1899-c6ddf167a858
	// q)8_-8!g
	// 0x02000200000023c06889fda07f3dd5b19af6c7fcdae7079985439d0157041899c6ddf167a858
	const auto vec = testReadAndStr<KdbGuidVector>("0x02000200000023c06889fda07f3dd5b19af6c7fcdae7079985439d0157041899c6ddf167a858", "(\"G\"$(\"23c06889-fda0-7f3d-d5b1-9af6c7fcdae7\";\"07998543-9d01-5704-1899-c6ddf167a858\"))"s);
	EXPECT_EQ(2, vec->count());

	std::array<uint8_t,16> exp0 = {0x23 ,0xc0 ,0x68 ,0x89 ,0xfd ,0xa0 ,0x7f ,0x3d ,0xd5 ,0xb1 ,0x9a ,0xf6 ,0xc7 ,0xfc ,0xda ,0xe7};
	EXPECT_TRUE(std::equal(exp0.cbegin(), exp0.cend(), vec->getGuid(0).cbegin()));
	
	std::array<uint8_t,16> exp1 = {0x07 ,0x99 ,0x85 ,0x43 ,0x9d ,0x01 ,0x57 ,0x04 ,0x18 ,0x99 ,0xc6 ,0xdd ,0xf1 ,0x67 ,0xa8 ,0x58 };
	EXPECT_TRUE(std::equal(exp1.cbegin(), exp1.cend(), vec->getGuid(1).cbegin()));
}

TEST(KdbTypeTest, TestKdbGuidVectorHasNull) {
	// q)8_-8!1#0Ng
  // 0x02000100000000000000000000000000000000000000
	const auto vec = testReadAndStr<KdbGuidVector>("0x02000100000000000000000000000000000000000000", "(enlist \"G\"$\"00000000-0000-0000-0000-000000000000\")"s);
	EXPECT_EQ(1, vec->count());

	std::array<uint8_t,16> exp0 = {0, 0, 0, 0, 0, 0, 0, 0};
	EXPECT_TRUE(std::equal(exp0.cbegin(), exp0.cend(), vec->getGuid(0).cbegin()));
}

TEST(KdbTypeTest, TestKdbGuidAtom)
{
	// q)0x0 sv/: reverse each 8 cut 9_ -8! "G"$"6d5314c6-c013-9cab-eac2-b8928cea460e"
	// -6080963678179142803 1028767454378640106
	// 8_ -8! "G"$"6d5314c6-c013-9cab-eac2-b8928cea460e"
	// 0xfe6d5314c6c0139cabeac2b8928cea460e
	const auto atom = testReadAndStr<KdbGuidAtom>("0xfe6d5314c6c0139cabeac2b8928cea460e", "(\"G\"$\"6d5314c6-c013-9cab-eac2-b8928cea460e\")");
	auto ary = atom->m_val;
	const std::array<uint8_t,16> exp = {0x6d ,0x53 ,0x14 ,0xc6 ,0xc0 ,0x13 ,0x9c ,0xab ,0xea ,0xc2 ,0xb8 ,0x92 ,0x8c ,0xea ,0x46 ,0x0e};
	for (size_t i = 0 ; i < SZ_GUID ; i++) {
		EXPECT_EQ(exp[i], ary[i]);
	}

	// Create an array to replace the existing value
	std::array<uint8_t,16> val = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
	atom->m_val = val;
	// q)8_-8!"G"$"01020304-0506-0708-090a-0b0c0d0e0f10"
	// 0xfe0102030405060708090a0b0c0d0e0f10
	if (!testWrite(atom.get(), "0xfe0102030405060708090a0b0c0d0e0f10")) {
		FAIL();
	}

}

TEST(KdbTypeTest, TestKdbTimestampAtom)
{
	// q)8_-8! 0N! zp:.z.p
	// 2023.08.03D15:49:31.244854000
	// 0xf4f0da58a0769d540a
	// q)7h$zp
	// 744392971244854000
	const auto atom = testReadAndStr<KdbTimestampAtom>("0xf4f0da58a0769d540a", "2023.08.03D15:49:31.244854000"s);
	EXPECT_EQ(744392971244854000L, atom->m_val);
	// q)8_-8! 0N!.z.P
	// 2024.02.29D21:30:32.034054000
	// 0xf4704f39b9f125950a
	atom->m_val = time::Timestamp::getUTC(2024, 2, 29, 21, 30, 32, 34054000);
	if (!testWrite(atom.get(), "0xf4704f39b9f125950a")) {
		FAIL();
	}
}

TEST(KdbTypeTest, TestKdbDateAtom)
{
	// q)8_-8! 0N!.z.d
	// 2023.08.03
	// q)6h$.z.d
	// 8615i
	const auto atom = testReadAndStr<KdbDateAtom>("0xf2a7210000", "2023.08.03"s);
	EXPECT_EQ(8615, atom->m_val);
	const int32_t dys = time::Date::getDays(2024, 2, 29);
	// q)6h$ 2024.02.29
	// 8825i
	EXPECT_EQ(8825, dys); // leap year ;)
	atom->m_val = dys;
	// q)8_-8! 2024.02.29
	// 0xf279220000
	if (!testWrite(atom.get(), "0xf279220000")) {
		FAIL();
	}
}

TEST(KdbTypeTest, TestKdbTimespanAtom)
{
	// q)8_-8! 0N! zn:.z.n
	// 0D15:02:18.191637000
	// 0xf0086aae073d310000
	// q)7h$zn
	// 54138191637000
	const auto atom = testReadAndStr<KdbTimespanAtom>("0xf0086aae073d310000", "0D15:02:18.191637000"s);
	EXPECT_EQ(54138191637000L, atom->m_val);
	if (!testWrite(atom.get(), "0xf0086aae073d310000")) {
		FAIL();
	}
}

TEST(KdbTypeTest, TestKdbMinuteAtom)
{
	// q)8_-8!0N! m:`minute$.z.t
	// 17:55
	// 0xef33040000
	// q)6h$m
	// 1075i
	const auto atom = testReadAndStr<KdbMinuteAtom>("0xef33040000", "17:55"s);
	EXPECT_EQ(1075, atom->m_val);
	if (!testWrite(atom.get(), "0xef33040000")) {
		FAIL();
	}
}

TEST(KdbTypeTest, TestKdbSecondAtom)
{
	// 8_-8!0N! s:09:00:04
	// 09:00:04
	// 0xee947e0000
	// q)6h$s
	// 32404i
	const auto atom = testReadAndStr<KdbSecondAtom>("0xee947e0000", "09:00:04"s);
	EXPECT_EQ(32404, atom->m_val);
	if (!testWrite(atom.get(), "0xee947e0000")) {
		FAIL();
	}
}

TEST(KdbTypeTest, TestKdbTimeAtom)
{
	// q)8_-8! `time$ 0N! 6h$ 0N! 10:03:01.009
	// 10:03:01.009
	// 36181009i
	// 0xed11142802
	const auto atom = testReadAndStr<KdbTimeAtom>("0xed11142802", "10:03:01.009"s);
	EXPECT_EQ(36181009, atom->m_val);
	if (!testWrite(atom.get(), "0xed11142802")) {
		FAIL();
	}
}

TEST(KdbTypeTest, TestKdbSymbolAtom)
{
	// q)8_-8! `Charles.Stross
	// 0xf5436861726c65732e5374726f737300
	const auto atom = testReadAndStr<KdbSymbolAtom>("0xf5436861726c65732e5374726f737300", "`Charles.Stross"s);
	EXPECT_EQ("Charles.Stross"sv, atom->m_val);
	atom->m_val = "CODE NIGHTMARE GREEN";
	// q)8_-8!`$"CODE NIGHTMARE GREEN"
	// 0xf5434f4445204e494748544d41524520475245454e00
	if (!testWrite(atom.get(), "0xf5434f4445204e494748544d41524520475245454e00")) {
		FAIL();
	}
}

TEST(KdbTypeTest, TestKdbSymbolVector)
{
	// q)8_-8!`Arthur`Simon`Charlie
	auto vec = testReadAndStr<KdbSymbolVector>("0x0b00030000004172746875720053696d6f6e00436861726c696500", "`Arthur`Simon`Charlie");

	std::array<std::string,3> names{{"Arthur", "Simon", "Charlie"}};
	EXPECT_EQ(names.size(), vec->count());
	for (size_t i = 0 ; i < names.size() ; i++) {
		EXPECT_EQ(names[i], vec->getString(i));
	}
}

TEST(KdbTypeTest, TestKdbSymbolVector1)
{
	// q)8_-8!`$("Athos";"Porthos";"Aramis";"Dr. Who")
	// 0x0b00040000004174686f7300506f7274686f73004172616d69730044722e2057686f00
	const auto str = "(`$(\"Athos\";\"Porthos\";\"Aramis\";\"Dr. Who\"))"s;
	auto vec = testReadAndStr<KdbSymbolVector>("0x0b00040000004174686f7300506f7274686f73004172616d69730044722e2057686f00", str);
	std::array<std::string,4> names{{"Athos","Porthos","Aramis","Dr. Who"}};
	EXPECT_EQ(names.size(), vec->count());
	for (size_t i = 0 ; i < names.size() ; i++) {
		EXPECT_EQ(names[i], vec->getString(i));
	}
}

TEST(KdbTypeTest, TestKdbList)
{
	// q)8_-8!asc ("a";`b)
	// 0x000102000000f661f56200
	auto vec = testReadAndStr<KdbList>("0x000102000000f661f56200", "(\"a\";`b)"s);
	EXPECT_EQ(2, vec->count());
	// KdbBase *e0 = vals->at(0).get();

	auto ptr = vec->getObj(0);

	if (nullptr == ptr)
		FAIL() << "Expected KdbCharAtom, but none was found";
	EXPECT_EQ(KdbType::CHAR_ATOM, ptr->m_typ);
	const KdbCharAtom *c0 = reinterpret_cast<const KdbCharAtom*>(ptr);
	EXPECT_EQ('a', c0->m_val);

	ptr = vec->getObj(1);
	if (nullptr == ptr)
		FAIL() << "Expected KdbSymbolAtom, but none was found";
	EXPECT_EQ(KdbType::SYMBOL_ATOM, ptr->m_typ);
	const KdbSymbolAtom *c1 = reinterpret_cast<const KdbSymbolAtom*>(ptr);
	EXPECT_EQ("b"s, c1->m_val);
}

TEST(KdbTypeTest, TestKdbDict)
{
	// q)8_-8!dct:"abc"!1 2 3
	// 0x630a0003000000616263070003000000010000000000000002000000000000000300000000000000
	const char *hex = "0x630a0003000000616263070003000000010000000000000002000000000000000300000000000000";
	const char *exp = "(\"abc\"!1 2 3)";
	auto dct = testReadAndStr<KdbDict>(hex, exp);

	EXPECT_EQ(KdbType::CHAR_VECTOR, dct->getKeyType());
	EXPECT_EQ(KdbType::LONG_VECTOR, dct->getValueType());
	EXPECT_EQ(3, dct->count());

	KdbCharVector *keys = nullptr;
	dct->getKeys(&keys);
	EXPECT_NE(nullptr, keys);

	KdbLongVector *vals = nullptr;
	dct->getValues(&vals);
	EXPECT_NE(nullptr, vals);

	EXPECT_EQ(dct->count(), keys->count());
	EXPECT_EQ(dct->count(), vals->count());

	const auto kry = std::array<char,3>{'a', 'b', 'c'};
	const auto vry = std::array<int64_t,3>{1, 2, 3};

	for (int i = 0 ; i < dct->count() ; i++) {
		EXPECT_EQ(kry[i], keys->getChar(i));
		EXPECT_EQ(vry[i], vals->getLong(i));
	}
	
}

TEST(KdbTypeTest, TestKdbTable)
{
	// 8_-8!tbl:flip`sym`price`size!(`VOD.L`AZN.L;100 101.;200 400)
	// 0x6200630b000300000073796d0070726963650073697a65000000030000000b0002000000564f442e4c00415a4e2e4c0009000200000000000000000059400000000000405940070002000000c8000000000000009001000000000000
	// "+`sym`price`size!(`VOD.L`AZN.L;100 101f;200 400)"

	const char *hex = "0x6200630b000300000073796d0070726963650073697a65000000030000000b0002000000564f442e4c00415a4e2e4c0009000200000000000000000059400000000000405940070002000000c8000000000000009001000000000000";
	const char *exp = "(flip `sym`price`size!(`VOD.L`AZN.L;100 101f;200 400))";
	auto tbl = testReadAndStr<KdbTable>(hex, exp);
}

TEST(KdbTypeTest, TestKdbTableEmptyWrite)
{
	auto tbl = new KdbTable("tsfj", {"time", "sym", "price", "size"});
	if (!tbl)
		FAIL() << "Failed to instantiate KdbTable";
	uint64_t cap = tbl->wireSz();
	char mem[cap];
	WriteBuf buf{mem, cap};
	WriteResult wr = tbl->write(buf);
	EXPECT_EQ(WriteResult::WR_OK, wr);
	bool matches = cap == buf.offset();
	EXPECT_TRUE(matches);
	// q)-8!flip`time`sym`price`size!"TSFJ"$\:()
	int64_t len = 0;
	auto ptr = fromHex("6200630b000400000074696d650073796d0070726963650073697a65000000040000001300000000000b0000000000090000000000070000000000", &len);
	EXPECT_EQ(len, cap) << "Canconical encoding.length does not match this library's encoding.length";

	std::string kdb{}, lib{};
	auto i_kdb = std::back_inserter(kdb), i_lib = std::back_inserter(lib);
	for (uint64_t i = 0 ; i < cap ; i++) {
		std::format_to(i_lib, "{:02x}", static_cast<uint8_t>(mem[i]));
		matches &= mem[i] == ptr[i];
	}
	for (uint64_t i = 0 ; i < len ; i++) {
		std::format_to(i_kdb, "{:02x}", static_cast<uint8_t>(ptr[i]));
	}

	if (!matches)
		FAIL() << "IPC output does not match KDB:" << std::endl << "LIB: 0x" << lib << "\nKDB: 0x" << kdb << std::endl;

}

TEST(KdbTypeTest, TestKdbFunction)
{
	// q)8_-8!{x+y}
	// 0x64000a00050000007b782b797d
	auto fun = testReadAndStr<KdbFunction>("0x64000a00050000007b782b797d", "{x+y}");
}

TEST(KdbTypeTest, TestKdbUnaryFunction)
{
	// q)8_-8!{-9!0x010000000a00000065,x} each (4h$til 0x2a),0x2aff
	// NB the round-trip makes q convert 0x642a (var) into 0x64{var x}, so I've updated the hex to suit this test
	// 0x00002c0000006500650165026503650465056506650765086509650a650b650c650d650e650f6510651165126513651465156516651765186519651a651b651c651d651e651f6520652165226523652465256526652765286529652a65ff
	const char *hex = "0x00002c0000006500650165026503650465056506650765086509650a650b650c650d650e650f6510651165126513651465156516651765186519651a651b651c651d651e651f6520652165226523652465256526652765286529652a65ff";
	// q).Q.s1 -9! -8!{-9!0x010000000a00000065,x} each (4h$til 0x2a),0x2aff
	const char *exp = "(::;+:;-:;*:;%:;&:;|:;^:;=:;<:;>:;$:;,:;#:;_:;~:;!:;?:;@:;.:;0::;1::;2::;avg;last;sum;prd;min;max;exit;getenv;abs;sqrt;log;exp;sin;asin;cos;acos;tan;atan;enlist;var;)";
	auto lst = testReadAndStr<KdbList>(hex, exp);
}

TEST(KdbTypeTest, TestKdbBinaryFunction)
{
	// q)0x0000,0x25000000,raze {0x66,x} each 4h$til 0x25
	// 0x00004a0000006600660166026603660466056606660766086609660a660b660c660d660e660f6610661166126613661466156616661766186619661a661b661c661d661e661f66206621662266236624
	const char *hex = "0x0000250000006600660166026603660466056606660766086609660a660b660c660d660e660f6610661166126613661466156616661766186619661a661b661c661d661e661f66206621662266236624";
	// q).Q.s1 {-9!0x010000000a00000066,x} each 4h$til 0x25
	const char *exp = "(:;+;-;*;%;&;|;^;=;<;>;$;,;#;_;~;!;?;@;.;0:;1:;2:;in;within;like;bin;ss;insert;wsum;wavg;div;xexp;setenv;binr;cov;cor)";
	auto lst = testReadAndStr<KdbList>(hex, exp);
}

TEST(KdbTypeTest, TestKdbTernaryFunction)
{
	// q)0x0000,0x06000000,raze {0x67,x} each 4h$til 6
	// 0x000006000000670067016702670367046705 
	const char *hex = "0x000006000000670067016702670367046705";
	// q).Q.s1 {-9!0x010000000a00000067,x} each 4h$til 6
	// "(';/;\\;':;/:;\\:)"
	const char *exp = "(';/;\\;':;/:;\\:)";
	auto lst = testReadAndStr<KdbList>(hex, exp);
}

TEST(KdbTypeTest, TestKdbProjection)
{
	// q)8_-8!{x+y+z}[1;;2]
	// 0x680400000064000a00070000007b782b792b7a7df9010000000000000065fff90200000000000000
	const char *hex = "0x680400000064000a00070000007b782b792b7a7df9010000000000000065fff90200000000000000";
	const char *exp = "{x+y+z}[1;;2]";
	auto prj = testReadAndStr<KdbProjection>(hex, exp);
}

TEST(KdbTypeTest, TestKdbException)
{
	// C.f. KdbType.cpp: KdbException notes
	const char *hex = "0x806661696c656400";
	const char *exp = "'failed";
	auto err = testReadAndStr<KdbException>(hex, exp);
}

} // end namespace mg7x::test

int main(int argc, char *argv[]) {
	using namespace mg7x::test;
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
