// This file is part of the Mg kdb+/Krb5 Library (hereinafter "The Library").
//
// The Library is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// The Library is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
//
// You should have received a copy of the GNU Affero Public License along with The
// Library. If not, see https://www.gnu.org/licenses/agpl.txt.

.utl.init:{
 ;.z.pc:.utl.zpc
 ;.z.po:.utl.zpo
 ;.z.ts:.utl.zts
 ;.utl.conns:1!flip`fd`usr`since!"ISP"$\:()
 ;.utl.cbks:flip`fd`typ`cbk!enlist each (0Ni;`;::)
 ;.utl.tid:0
 ;.utl.timers:1!flip`id`millis`rpt`fn`nxttp!"JIB*P"$\:()
 ;.utl.zpcCbks:()
 ;.utl.zpoCbks:()
 }

.utl.zw:{.z.w}
.utl.zP:{.z.P}
.utl.zp:{.z.p}

.utl.zpo:{[H]
  .log.debug("Have socket-open event for FD ";H)
 ;`.utl.conns upsert (H;.z.u;.z.P)
 }

.utl.onZpcCbkErr:{[H;E;B]
  .log.error("Failed in on-close callback for FD ";H;": ";E;"\n";.Q.sbt B)
 }

.utl.zpc:{[H]
  .log.debug("Have socket-close event for FD ";H)
 ;exec .Q.trp[;H;.utl.onZpcCbkErr H] each cbk from .utl.cbks where fd = H, typ=`zpc
 ;delete from `.utl.cbks where fd = H
 ;delete from `.utl.conns where fd = H
 }

//--------------------------------------------------------------------------- .timers
.utl.onTimerFail:{[E;B]
  .log.error("While executing timer: '";E;"\n:";.Q.sbt B)
 }

.utl.execTimer:{[K;M;R;F;X]
  .Q.trp[F;K;.utl.onTimerFail]
 ;$[R
   ;update nxttp:(.z.p + 1000000 * M) from `.utl.timers where id = K
   ;not count tp:exec nxttp from .utl.timers where id=K
   ;0
   ;X <> first tp
   ;.log.debug("Leaving timer with id ";K)
   ;[delete from `.utl.timers where id = K;.log.debug("Cleared timer with id ";K)]
   ]
 ;
 }

.utl.zts:{
  .utl.execTimer ./: flip value flip 0!select from .utl.timers where nxttp <= .z.p
 ;.utl.setTimeout[]
 ;
 }

.utl.setTimeout:{
  $[not count .utl.timers
   ;system "t 0"
   ;(19h$zp:.z.p) >= 19h$tp:(exec from .utl.timers where nxttp = min nxttp)`nxttp
   ;system "t 1"
   ;system "t ",string $[0=tp:6h$19h$tp-zp;1;tp]
   ]
 ;
 }

// F: monadic function/projection; M: millis -6h; R: repeat 1h
.utl.addTimer:{[F;M;R]
  `.utl.timers upsert (.utl.tid+:1;M;R;F;.utl.zp[] + 1000000 * M)
 ;.utl.setTimeout[]
 ;
 }

.utl.init[];
