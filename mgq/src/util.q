/ This file is part of the Mg kdb+/mgq Library (hereinafter "The Library").

/ The Library is free software: you can redistribute it and/or modify it under
/ the terms of the GNU Affero Public License as published by the Free Software
/ Foundation, either version 3 of the License, or (at your option) any later
/ version.

/ The Library is distributed in the hope that it will be useful, but WITHOUT ANY
/ WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
/ PARTICULAR PURPOSE. See the GNU Affero Public License for more details.

/ You should have received a copy of the GNU Affero Public License along with The
/ Library. If not, see https://www.gnu.org/licenses/agpl.txt.

.utl.init:{
  .utl.tid:0
 ;.utl.conns:1!flip`fd`url`usr`rgs!enlist each (0Ni;`;`;::)
 ;.utl.timers:1!flip`id`millis`rpt`fn`nxttp!"JIB*P"$\:()
 ;.utl.cbks:flip`fd`typ`cbk!enlist each (0Ni;`;::)
 ;.utl.zpcs:()
 ;.utl.zpos:()
 ;.z.pc:.utl.zpc
 ;.z.po:.utl.zpo
 ;.z.ts:.utl.zts
  // TODO: implement command line switch to disable .chkArity in prod
 ;if[0b
    ;.utl.chkArity:{[N;F]}
    ]
 }

.utl.zw:{.z.w}
.utl.zt:{.z.t}
.utl.zT:{.z.T}
.utl.zd:{.z.d}
.utl.zD:{.z.D}
.utl.zp:{.z.p}
.utl.zP:{.z.P}
.utl.zn:{.z.n}
.utl.zN:{.z.N}
.utl.zu:{.z.u}
.utl.za:{.z.a}
.utl.zi:{.z.i}

.utl.arity:{[F]
   $[-11h~typ:type F                                                           //   if| F is a symbol atom
    ;.z.s value F                                                              // then| resolve the symbol and recurse
    ;100h~typ                                                                  // elif| it's a function
    ;count (value F)1                                                          // then| count 2nd element (the sym-vec of args)
    ;not 104h~typ                                                              // elif| it's not a projection
    ;'"expected type 104"                                                      // then| protest
    ;enlist~first lst:value F                                                  // elif| we are dealing with an elided list
    ;(.z.s lst 1) - count where not ![-2;]~/:attr each 2_ lst                  // then| count elided args where they produce an identity transform
    ;(.z.s lst 0) - count where not (::)~/:1_ lst                              // else| count where a standard projection has non-nil args
    ]
 };

.utl.chkArity:{[N;F]
  if[N<>num:.utl.arity F
    ;'"Expected ",(string N)," arguments but found ",string num
    ]
 }

//--------------------------------------------------------------------------- IPC functions
.utl.onZpoCbkErr:{[H;E;B]
  .log.error("Failed in on-open callback for FD ";H;": ";E;"\n";.Q.sbt B)
 }

.utl.onZpcCbkErr:{[H;E;B]
  .log.error("Failed in on-close callback for FD ";H;": ";E;"\n";.Q.sbt B)
 }

.utl.zpc:{[H]
  .log.debug("Have socket-close event for FD ";H)
 ;exec .Q.trp[;H;.utl.onZpcCbkErr H] each cbk from .utl.cbks where fd = H, typ=`zpc
 ;.Q.trp[;H;.utl.onZpcCbkErr H] each .utl.zpcs
 ;delete from `.utl.cbks where fd = H
 ;delete from `.utl.conns where fd = H
 }

.utl.toIpv4:{[A]
  $[2130706433i~A
   ;`localhost
   ;0i~A
   ;`unix
   ;`$"."sv string 6h$0x0 vs A
   ]
 }

.utl.zpo:{[H]
  .log.info("Have socket-open event with FD ";H)
 ;`.utl.conns upsert (H;.utl.toIpv4 .utl.za[];.utl.zu[];::)
 ;.Q.trp[;H;.utl.onZpoCbkErr H] each .utl.zpos
 }

.utl.addFdPcCbk:{[H;F]
  `.utl.cbks insert (H;`zpc;F)
 }

.utl.addPcCbk:{[F]
  .utl.zpcs,:enlist F
 }

// U: url hsym; T: timeout; R: retry -16 -17 -18 -19h; O: on-open required monadic 100 104h
// C: on-close optional monadic 100 104h
.utl.hopen:{[U;T;R;O;C]
  // `.utl.conns insert (-1;U;`;`timeout`retry`open`close!(T;R;O;C))
 }

//--------------------------------------------------------------------------- env helpers
// Look for file F in a path-like environment variable E
// E: env-var -11h; F: full file name 10h
.utl.pathFind:{[E;F]
  system"find -H ${",(string E),"//:/ } -mindepth 1 -maxdepth 1 -name ",F
 }

//--------------------------------------------------------------------------- timers
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

.boot.register[`util;`.utl;()]
