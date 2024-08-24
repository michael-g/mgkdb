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
#include <cstdlib>
#include <string_view>


namespace mg7x {

template<typename...Args>
WriteResult write(WriteBuf & buf, Args ... args)
{
  WriteResult rtn = WriteResult::WR_OK;
  // From https://stackoverflow.com/a/60136761/322304
  ([&]{
    if (WriteResult::WR_OK == rtn) {
      rtn = args.write(buf);
    }
  }(),...);
  return rtn;
}

};

using namespace mg7x;

int main(int argc, char *argv[])
{
  uint8_t mem[1024];
  WriteBuf buf{mem, sizeof mem};
  write<KdbSymbolAtom, KdbSymbolAtom, KdbSymbolAtom, KdbIntAtom, KdbFloatAtom>(buf, std::string_view{".u.upd"}, std::string_view{"trade"}, std::string_view{"VOD.L"}, {42}, {42.});
  return EXIT_SUCCESS;
}
