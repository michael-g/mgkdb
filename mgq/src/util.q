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
 ;.utl.timers:1!flip`id`millis`rpt`fn`nxttp!"JTT*P"$\:()
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
.utl.t:{[T]
  value"\\t ",$[10h~type T;T;string T]
 }

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

.utl.onHopenCbkErr:{[H;E;B]
  .log.error("While invoking callback for FD ";H;": ";E;"\n ",.Q.sbt B)
 }

// U: url hsym; T: timeout -19h; R: retry -19h; O: on-open monadic func, required;
// C: on-close monadic, optional; I: timer-ID -7h
.utl.hopen1:{[U;T;R;O;C;I]
  cfd:@[hopen;(U;7h$T);{x}]
 ;if[10h~type cfd
    ;if[null R
       ;delete from `.utl.timers where id=I
       ]
    ;.log.debug("Failed to open connection to ";U;": '";cfd)
    ;:0b
    ]
 ;delete from `.utl.timers where id=I
 ;`.utl.conns upsert (cfd;U;.utl.zu[];`T`R`O`C!(T;R;O;C))
 ;.Q.trp[O;cfd;.utl.onHopenCbkErr cfd]
 }

// U: url hsym; T: timeout -19h; R: retry -19h or 0Nt; O: on-open monadic func, required;
// C: on-close monadic, optional (use :: to ignore)
.utl.hopen:{[U;T;R;O;C]
  .utl.chkArity[1;O]
 ;if[not(::)~C
    ;.utl.chkArity[1;C]
    ]
 ;if[-19h<>type T;'"T.type"]
 ;if[-19h<>type R;'"R.type"]
 ;.utl.addTimer[(`.utl.hopen1;U;T;R;O;C;);19h$0;R]
 }

//--------------------------------------------------------------------------- env helpers
// Look for file F in a path-like environment variable E
// E: env-var -11h; F: full file name 10h
.utl.pathFind:{[E;F]
  system"find -H ${",(string E),"//:/ } -mindepth 1 -maxdepth 1 -name ",F
 }

//--------------------------------------------------------------------------- eval helpers
.utl.eval1:{[F;A;K]
  $[$[104h~type F;enlist~first value F;0b]
   ;.Q.trp[value;F @ A;K]
   ;.Q.trp[F;A;K]
   ]
 }

//--------------------------------------------------------------------------- timers
.utl.onTimerFail:{[E;B]
  .log.error("While executing timer: '";E;"\n:";.Q.sbt B)
 }

// Invoked with the values from a timer-row in .utl.timers
// I: timer ID; M: millis; R: rpt; F: func; X nxttp
.utl.execTimer:{[I;M;R;F;X]
  .utl.eval1[F;I;.utl.onTimerFail]
 ;$[R
   ;update nxttp:(.z.p + 1000000 * M) from `.utl.timers where id=I
   ;not count tp:exec nxttp from .utl.timers where id=I
   ;0
   ;X <> first tp
   ;.log.debug("Leaving timer with id ";I)
   ;[delete from `.utl.timers where id = I;.log.debug("Cleared timer with id ";I)]
   ]
 ;
 }

.utl.zts:{
  .utl.execTimer ./: flip value flip 0!select from .utl.timers where nxttp <= .z.p
 ;.utl.setTimeout[]
 ;
 }

.utl.setTimeout:{
  $[not count .utl.timers                                                 //   if .timers is empty
   ;.utl.t 0                                                              // then clear the timeout
   ;(now:.utl.zp[]) >= nxt:exec min nxttp from .utl.timers                // elif a timer should have fired or be firing now
   ;.utl.t 1                                                              // then set the smallest timeout possible
   ;.utl.t $[0=ivl:6h$19h$nxt-now;1;ivl]                                  // else set a timeout to fire in N millseconds
   ]
 ;
 }

// Simple timer system offering time-til-initial-fire and repeat-at-interval controls. "Invokable" F
// is ideally an elided list, e.g. (`.eg.test;1;2;3;), where the missing argument is provided as the
// timer-ID, so it can be cancelled easily.
// F: monadic function/projection; M: millis -19h; R: repeat -19h
.utl.addTimer:{[F;M;R]
  `.utl.timers upsert (.utl.tid+:1;M;R;F;.utl.zp[] + M)
 ;.utl.setTimeout[]
 ;
 }

.boot.register[`util;`.utl;()]
