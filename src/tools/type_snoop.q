/ This file is part of the "Mg kdb+ Library" (hereinafter "The Library").
/.
/ The Library is free software: you can redistribute it and/or modify it under
/ the terms of the GNU Affero Public License as published by the Free Software
/ Foundation, either version 3 of the License, or (at your option) any later
/ version.
/.
/ The Library is distributed in the hope that it will be useful, but WITHOUT ANY
/ WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
/ PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
/.
/ You should have received a copy of the GNU Affero Public License along with The
/ Library. If not, see https://www.gnu.org/licenses/agpl.txt.

so:hsym`$(first system"pwd"),"/libsnoop"
snp_i:so 2: (`hex_snoop;1)
op:(/:\:)
snp:{-1 "Op is ",.Q.s1 x; -1 "-8! is ",.Q.s1 8_-8!x; snp_i x}
snp op
