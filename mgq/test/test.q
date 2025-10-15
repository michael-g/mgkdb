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


.mok.init:{
 ;.mok.dir:first tmp:` vs hsym`$first system "readlink -f ",string .z.f
 ;.mok.tst:tmp where(tmp:(key .mok.dir) except last tmp) like "*.q"
 ;.mok.src:` sv (first` vs .mok.dir;`src)
 ;$[`tst.names in key rgs:.Q.opt .z.x
   ;.mok.run each `$","vs first rgs`tst.names
   ;.mok.run each .mok.src
   ]
 }

.mok.log:{[M]
  -1 (string .z.Z)," ",M
 ;
 }

.mok.ldSrc:{[F]
  .mok.src,$[-11h~type F;F;`$F]
 }

.mok.register:{[S;F;M;N;D]
  .mok.log "Loaded source ",string S
 ;.mok.runTest[S] each F
 ;
 }

.mok.testFailed:{[F;E;B]
  btr:$[5<count B;5#B;B]
 ;.mok.log "Test FAILURE: ",(.Q.s1 F),": error is '",E,"\n",.Q.sbt btr
 }

.mok.ilog:{[N;M]
  `.mok.logged insert (N;M)
 }

.mok.mockLogger:{[N]
  (` sv (`.log;N)) set .mok.ilog upper N
 }

.mok.setUp:{
  .mok.logged:flip`name`arg!enlist each (`;::)
 ;.mok.mockLogger each `trace`debug`info`warn`error
 }

.mok.runTest:{[S;F]
  .mok.setUp[]
 ;.mok.log "Running ",string F
 ;.Q.trp[F;::;.mok.testFailed F]
 ;
 }

// F: target script -11h; N that script's namespace -11h
.mok.test:{[F;N]
 ;src:` sv .mok.src,F
 ;fns:key value ` sv dir:N,`tst
 ;fns:` sv/: dir,/:fns except `
 ;.boot.register:.mok.register[src;fns]
 ;system"l ",1_ string src
 }

.mok.run:{[F]
  tst:string` sv .mok.dir,F
 ;if[not F in .mok.tst
    ;'"No such test script: ",tst
    ]
 ;.mok.log "Running script ",tst
 ;system"l ",1_ tst
 ;
 }

.mok.ast.fail:{[M]
  'M
 }
.mok.ast.eq:{[L;R]
  if[not L = R;.mok.ast.fail "not equal: ",(.Q.s1 L)," != ",.Q.s1 R]
 }
.mok.ast.is:{[L;R]
  if[not L ~ R;.mok.ast.fail "not identical: ",(.Q.s1 L)," !~ ",.Q.s1 R]
 }

.mok.init[];
