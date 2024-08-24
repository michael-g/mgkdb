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

// optional file you can load into the q process to see what's going on during the test

.tst.nfo:{[M]
  -1 (string .z.Z),"  INFO: ",M
 }

.tst.err:{[M]
  -2 (string .z.Z)," ERROR: ",M
 }

.tst.zpw:{[U;P]
  .tst.nfo "Beginning '",(string U),"'"
 ;`.tst.fds upsert (.z.w;U)
 ;1b
 }

.tst.zpc:{[H]
  dct:exec from .tst.fds where fd = H
 ;$[not null dct`tst
   ;.tst.nfo "Ended ",string dct`tst
   ;.tst.nfo "Ended test"
   ]
 ;
 }

.u.upd:{[T;X]
  .tst.args,:enlist `T`X!(T;X)
 ;.tst.nfo "Received .u.upd message for table ",.Q.s1 T
 ;
 }

.tst.zps:{[M]
  value M
 ;.tst.args,:enlist (!/)enlist each (`.z.ps;M)
 ;(neg .z.w) M
 ;
 }

.tst.init:{
  .tst.fds:1!flip`fd`tst!"IS"$\:()
 ;.tst.args:enlist(::)
 ;.z.pw:.tst.zpw
 ;.z.pc:.tst.zpc
 ;.z.ps:.tst.zps
 ;system"p 30098"
 ;1b
 }

.tst.init[];
