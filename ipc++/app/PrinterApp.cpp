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

#include <cstdint>
#include <iostream>
#include <format>

//-------------------------------------------------------------------------------- main
using namespace mg7x;

static void print(const KdbBase & o)
{
	std::cout << std::format("Have object with type {} and value {}\n", static_cast<int>(o.m_typ), o);
}

template<typename T> void setIntValues(T & obj)
{
	obj.m_vec.push_back(KdbQuirks<T>::POS_INFINITY);
	obj.m_vec.push_back(KdbQuirks<T>::NEG_INFINITY);
	// obj.m_vec.push_back(static_cast<decltype(obj.m_vec)::value_type>(42));
	obj.m_vec.push_back(42);
	obj.m_vec.push_back(KdbQuirks<T>::NULL_VALUE);
}

int main(int argc, char **argv)
{
	KdbException r{"type"};
	KdbTimeAtom t{83938163};
	KdbSecondAtom v{84140};
	KdbMinuteAtom u{1400};
	KdbTimespanAtom n{84422764399000L};
	KdbDateAtom d{8892};
	KdbMonthAtom m{292};
	KdbTimestampAtom p{768344697469171000L};
	KdbSymbolAtom s{"VOD.L"};
	KdbCharAtom c{'C'};
	KdbFloatAtom f{42};
	KdbRealAtom e{42};
	KdbLongAtom j{42};
	KdbIntAtom i{42};
	KdbShortAtom h{42};
	KdbByteAtom x{42};
	KdbGuidAtom g{std::array<uint64_t,2>{1, 2}};
	KdbBoolAtom b{true};

	KdbBoolVector B{3}; for(int i = 0 ; i < 3 ; i++) B.setBool(i, (i % 2) == 0);
	KdbGuidVector G{2}; for(int i = 0 ; i < 2 ; i++) G.setGuid(i, g.m_val);
	KdbByteVector X{3}; for(int8_t i = 0 ; i < 3 ; i++) X.setByte(i, i);
	KdbShortVector H{3}; setIntValues(H);
	KdbIntVector I1{1}; I1.setInt(0, 42);
	KdbIntVector I{4}; setIntValues(I);
	KdbLongVector J{4}; setIntValues(J);
	KdbRealVector E{4}; setIntValues(E); E.setReal(2, 42);
	KdbFloatVector F{4}; setIntValues(F); F.setFloat(2, 42);
	KdbFloatVector F2{2}; F2.setFloat(0, 42.); F2.setFloat(1, 43.);
	KdbCharVector C{"Hello, \"World\"!"}; 
	KdbSymbolVector S1{{"Hello", "World"}};
	KdbSymbolVector S2{{"AZN LN", "VOD LN"}};
	KdbSymbolVector S3{{"VOD.L"}};
	KdbTimestampVector P{2}; P.setTimestamp(0, 769730095044082000); P.setTimestamp(1, 769816495044000000);
	KdbMonthVector M{2}; M.setMonth(0, 290); M.setMonth(1, 291);
	KdbDateVector D{2}; D.setDate(0, 8908); D.setDate(1, 8909);
	KdbTimespanVector N{2}; N.setTimespan(0, 44055000000000); N.setTimespan(1, 58638000000000);
	KdbMinuteVector U{2}; U.setMinute(0, 733); U.setMinute(1, 855);
	KdbSecondVector V{2}; V.setSecond(0, 43994); V.setSecond(1, 54977);
	KdbTimeVector T{2}; T.setMillis(0, 43994567); T.setMillis(1, 54977890);

	ColDef c0{"time", T};
	ColDef c1{"sym", S2};
	ColDef c2{"price", F2};
	KdbTable tbl{c0, c1, c2};
	std::cout << std::format("tbl is {}", tbl) << std::endl;

	KdbTable tbl1{{"time", "sym", "price"}, T, S2, F2};
	std::cout << std::format("tbl1 is {}", tbl1) << std::endl;

	KdbTable tbl2{"tsf", {"time", "sym", "price"}};
	std::cout << std::format("tbl2 is {}", tbl2) << std::endl;

	ColRef<KdbTimeVector> time{"time"};
	ColRef<KdbSymbolVector> sym{"sym"};
	ColRef<KdbFloatVector> price{"price"};

	tbl.lookupCols(time, sym, price);

	std::cout << std::format("time.length is {}, time.data() are: {}", time.m_col->count(), *time.m_col) << std::endl;

	print(r);
	print(t);
	print(v);
	print(u);
	print(n);
	print(d);
	print(m);
	print(p);
	print(s);
	print(c);
	print(f);
	print(e);
	print(j);
	print(i);
	print(h);
	print(x);
	print(g);
	print(b);
	print(B);
	print(G);
	print(X);
	print(H);
	print(I1);
	print(I);
	print(J);
	print(E);
	print(F);
	print(C);
	print(S1);
	print(S2);
	print(S3);
	print(P);
	print(M);
	print(D);
	print(N);
	print(U);
	print(V);
	print(T);
	print(tbl);

	auto dk = new KdbCharVector("abc");
	auto dv = new KdbIntVector({1, 2, 3});
	KdbDict dct{std::unique_ptr<KdbCharVector>{dk}, std::unique_ptr<KdbIntVector>{dv}};
	print(dct);

	KdbFunction fnc{"{`yomomma}"};
	print(fnc);

	KdbUnaryPrimitive avg{KdbUnary::AVG};
	print(avg);

	KdbBinaryPrimitive cast{KdbBinary::CAST};
	print(cast);

	KdbTernaryPrimitive elft{KdbTernary::EACH_LEFT};
	print(elft);

	KdbProjection prj{"{[x;y;z] x+y+z}", 3};
	prj.setArg(1, std::make_unique<KdbIntAtom>(2));
	prj.setArg(2, std::make_unique<KdbIntAtom>(3));
	print(prj);

	KdbList L{};

	L.push(t);
	L.push(v);
	L.push(u);
	L.push(n);
	L.push(d);
	L.push(m);
	L.push(p);
	L.push(s);
	L.push(c);
	L.push(f);
	L.push(e);
	L.push(j);
	L.push(i);
	L.push(h);
	L.push(x);
	L.push(g);
	L.push(b);
	L.push(B);
	L.push(G);
	L.push(X);
	L.push(H);
	L.push(I1);
	L.push(I);
	L.push(J);
	L.push(E);
	L.push(F);
	L.push(C);
	L.push(S1);
	L.push(S2);
	L.push(S3);
	L.push(P);
	L.push(M);
	L.push(D);
	L.push(N);
	L.push(U);
	L.push(V);
	L.push(T);

	std::cout << std::format("Have list of type {} and size {} with values {}\n", static_cast<int>(L.m_typ), L.count(), L);
	return 0;
}
