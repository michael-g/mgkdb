#!/bin/bash

# So ... as of my Fedora 42 upgrade, and GCC-15, the library no longer compiles with complaints
# about the formatters. In file /usr/include/c++/15/format at line 3007 there is:
#
# // _GLIBCXX_RESOLVE_LIB_DEFECTS
# // 3944. Formatters converting sequences of char to sequences of wchar_t
# If you do some searching you get to this resource, which discusses wchar_t use in formatters
# and ranges in c++23; however, I'm not sure how to proceed or even what the issue here is:
#
# https://www.open-std.org/jtc1/sc22/wg21/docs/lwg-active.html#3944
#
# The build error emitted looks like this:
#
# myuser@fedora:build$ make                                                                                                                                                                                                                                                                                                   [  8%] Building CXX object src/CMakeFiles/mgkdbipc.dir/KdbType.cpp.o                                                                                          
# In file included from /usr/include/c++/15/bit:38,
#                  from /home/myuser/dev/projects/github.com/mgkdb/ipc++/include/KdbType.h:20,
#                  from /home/myuser/dev/projects/github.com/mgkdb/ipc++/src/KdbType.cpp:17:
# /usr/include/c++/15/concepts: In substitution of ‘template<class Derived, class CharT>  requires  derived_from<Derived, mg7x::KdbBase> struct std::formatter<_Tp, _CharT> [with Derived = std::__format::__disabled; CharT = wchar_t]’:
# /home/myuser/dev/projects/github.com/mgkdb/ipc++/include/KdbType.h:1846:13:   required from here
#  1846 | struct std::formatter<Derived, CharT> {
#       |             ^~~~~~~~~~~~~~~~~~~~~~~~~
# /usr/include/c++/15/concepts:76:28: error: invalid use of incomplete type ‘struct std::__format::__disabled’
#    76 |     concept derived_from = __is_base_of(_Base, _Derived)
#       |                            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# In file included from /home/myuser/dev/projects/github.com/mgkdb/ipc++/include/KdbType.h:22:
# /usr/include/c++/15/format:3007:31: note: forward declaration of ‘struct std::__format::__disabled’
#  3007 |   namespace __format { struct __disabled; }
#       |                               ^~~~~~~~~~
# make[2]: *** [src/CMakeFiles/mgkdbipc.dir/build.make:79: src/CMakeFiles/mgkdbipc.dir/KdbType.cpp.o] Error 1
# make[1]: *** [CMakeFiles/Makefile2:161: src/CMakeFiles/mgkdbipc.dir/all] Error 2
# make: *** [Makefile:101: all] Error 2
#
# I therefore wrote this to insist on clang:

if [ build != "$(basename "$PWD")" ] ; then
	if [ ! -f build ] ; then
		mkdir build
		cd build
	fi
fi

cmake -DCMAKE_CXX_COMPILER=$(which clang++) ..

