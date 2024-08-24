/
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
/

.unz.write10kQa:{[D]
  (` sv D,`tenKQa.zipc) 1: -18!10000#.Q.a
 }
.unz.debug10kQa:{
   ipc:-18!10000#.Q.a
  ;(sums 0 4 4 4 1 8 1 8 1 8 1 8 1 8 1 9 1 11 1 11 1 12 1 10 1 11 1 11 1 11 1 10 1 11 1 11 1 11 1 11 1)cut ipc
 }

.unz.writeZipcFiles:{[D]
  if[not 11h~type key D
    ;'"Expected D to be a directory"
    ]
 ;.unz.write10kQa D
 }
