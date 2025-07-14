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
\

// Run using:
//  qq test/test_tp.q -p 30098 -sym test_pos -dst $PWD/logs
.mg.sub:{[T;S]
  sch:$[T~`
       ;.mg.u.sub[;S] each tables`
       ;.mg.u.sub[;S] each T
       ]
 ;-1(string .z.Z)," DEBUG: have subscription request for ",(.Q.s1 (T;S))," on FD ",(string .z.w)," for user ",string .z.u
 ;0N!(.u.i;.u.L;sch)
 }

.mg.init:{
  if[not system"p"
    ;'"You must provide a port (-p); for testing this should be 30098 or 30099"
    ]
 ;dir:1_ string first` vs hsym .z.f
 ;dir:first system "readlink -f ",dir,"/../contrib/kdb-tick"
 ;system"l ",dir,"/tick/u.q"
 ;system"l ",dir,"/tick.q"
 ;.mg.u.sub:.u.sub
 ;.u.sub:.mg.sub
 ;.z.pg:{[X] value 0N!X}
 ;1b
 }

.mg.genMsg:{[H;N;I;T]
  cnt:1+rand$[T~`trade;3;9]
 ;tkr:upper cnt?`3
 ;tkr:` sv/:tkr,\:`L
 ;tms:(til cnt)+.z.D + 08:00:00 + 19h$I * (6h$19h$16:30 - 08:00) div N
 ;lst:$[T~`trade
       ;(tms;tkr;100.0 + cnt?100.0;100 + cnt?100)
       ;T~`quote
       ;(tms;tkr;ask-1;ask:50.0 + cnt?5;cnt#42;cnt#43)
       ;T~`position
       ;(tms;tkr;cnt;100.0 + cnt?100.0;100 + cnt?100;cnt?"BS")
       ;T~`execs
       ;(tms;tkr;cnt;100.0 + cnt?100.0;100 + cnt?100;cnt?"BS")
       ;'T
       ]
 ;enlist lst
 }

/H:hopen .[jnl:`$":/home/michaelg/dev/projects/github.com/mgkdb/tpmux/logs/test_sym",string .z.D;();:;()]
/jnl:get jnl
.mg.genMktData:{[H]
  H `upd,/:tbl,'.mg.genMsg[H;idx]'[til idx;tbl:(idx:100)?`trade`quote]
 }

.mg.genPosData:{[H]
  H `upd,/:tbl,'.mg.genMsg[H;idx]'[til idx;tbl:(idx:100)?`execs`position]
 }

.mg.init[];
