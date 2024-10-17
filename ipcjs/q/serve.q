/
This file is part of the Mg KDB-IPC Javascript Library (hereinafter "The Library").

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

.web.log:{[M]
  -1 (string .z.Z)," ",(string .z.w)," ",M
 ;
 }

.web.getTable:{[C;L]
  .web.log"In function .web.getTable1[",(.Q.s1 C),";",(.Q.s1 L),"]"
 ;`.web.msgs insert (.z.P;`.web.getTable;`C`L!(C;L))
 ;n:100
 ;cls:`bool`byte`short`int`long`real`float`char`sym`timestamp`month`date`timespan`minute`second`time`string
 ;vls:{x?y}[n] each (0b;0x0;0h;0i;0j;99e;99f;" ";`3;.z.P;13h$.z.d;.z.d;.z.n),(17 18 19h$\: .z.t),enlist 3 cut (n * 3) ?  .Q.a,.Q.A,.Q.n
 ;tbl:flip cls!vls
 ;.web.replyOk[C;.z.w] tbl
 }

.web.reply:{[C;H;E;M]
  msg:(`.web.response;C;E;M)
 ;.web.log "Replying with ",.Q.s1 msg
 ;(neg .z.w) -8! msg
 }

.web.replyOk:{[C;H;M]
  .web.reply[C;H;0b;M]
 }

.web.zws:{[X]
  msg:-9!X
 ;.web.log .Q.s1 msg
 ;.mg.msg:msg
 ;if[`.web.request ~ first msg
    ;value last msg
    ]
 }

.web.init:{
  .h.HOME:(getenv`HOME),"/dev/projects/github.com/mgkdb/ipcjs/static"
 ;.z.ws:.web.zws
 ;.web.msgs:flip`time`fun`args!enlist each (0Np;`;::)
 }

.web.send:{[V]
  $[not count fhs:key .z.W
   ;'"None connected"
   ;1<count fhs
   ;'"Many connected"
   ;(neg first fhs) -8! V
   ]
 }

.web.run:{
  .web.init[]
 ;system"p 30098"
 ;`send set .web.send
 ;
 }

.web.run[]
