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

// Apply any custom formatting from .log.cvt for `type M`, otherwise default to .Q.s1
// M: message
.log.s1:{[M]
  raze $[type[M] in key .log.cvt
        ;.log.cvt[type M] M
        ;.Q.s1 M
        ]
 };

// Root log function, which current writes text to `stdout`
// V: integer logging level; L: text label for logging level; M: message
.log.log:{[V;L;M]
   if[V >= .log.lvl
     ;-1(L," ",(string .z.D)," ",(string .z.T)," ",((.log.spc .z.w)#" "),(string .z.w),"| ",.log.s1 M)
     ]
 }

// Installs a logging function (e.g. .log.debug) as a projection over .log.log. See .log.init. 
// L: upper text logging level; V: integer logging level
.log.mkfn:{[L;V]
  .log[`$lower string L]:.log.log[V;#[5-count[c];" "],c:string L]
 ;.log[L]:V
 ;
 }

// Bootstraps the logging system, autogenerating the different .log.debug, .log.info etc handlers
.log.init:{
   rgs:.boot.getargs flip `name`default`reqd!enlist each (`level;`DEBUG;0b)
 ;.log.lvl:(lvl:`SPAM`TRACE`DEBUG`INFO`WARN`ERROR`OFF)?`$upper string rgs`level
 ;.log.spc:`s#0 10 100 1000!3 2 1 0
 ;.log.mkfn ./: flip (-1_ lvl;til -1+count lvl)
 ;.log.cvt:10 -10 0h!({x};{enlist x};{.log.s1 each x})
 ; 
 }

.boot.args:{
  1 _ .z.x
 }

// Formerly at
// http://www.listbox.com/member/archive/1080/2012/12/search/aylkZWYy/sort/time_rev/page/1/entry/0:1/20121228140350:39D5D876-5121-11E2-B042-A5A5D9FB2443/
// by Aaron Davies
// .Q.def2[`a`b`c`d!("--";`bb;"quux";`:)] .Q.opt ("-a";"aa";"-d";"file")
\d .Q
k)defAD:{x,((!y)#(1#.q),x){$[10h=@x;*y;$[$[11h=@,/x;1b~&/":"=*:'$,/x;0];-1!';::]$[0h>@x;*:;::]$[(::)~x;x;(@*x)$]y]}'y}
\d .

.boot.getargs:{[T]
  dct:.Q.opt .boot.args[]
 ;if[count opt:exec name from T where reqd, not name in key dct
    ;{.log.error("-";x;" is a required option");} each opt
    ;exit 1
    ]
 ;.Q.defAD[(!/)T`name`default] dct
 }

.boot.load:{[F]
  pth:.boot.srcdir,"/",string F
 ;.boot.register:.boot.register1 `$pth
 ;.log.trace("Loading script ";pth)
 ;system "l ",pth
 ;delete register from `.boot
 ;
 }

.boot.plot:{[T;S]
  dps:exec distinct raze deps from T where nspace in S
 ;tbl:delete from T where nspace in S
 ;distinct raze dps,$[count dps;.boot.plot[tbl;dps];()]
 }

.boot.onInitFail:{[N;E;B]
  .log.error ("Failure in init function ";N;": '";E;"\n",.Q.sbt B)
 ;`fail.42
 }

.boot.start:{[N]
  ini:` sv N,`init
 ;.log.info ("Calling ";ini)
 ;if[`fail.42~.Q.trp[ini;::;.boot.onInitFail N]
    ;'"init.fail"
    ]
 }

.boot.startLeaves:{[T]
  nsp:exec nspace from T where ((all null@);deps) fby nspace
 ;.boot.start each nsp
 ;delete from T where (nspace in nsp)|deps in nsp
 }

.boot.uniqChk:{[P]
  if[(count P)<>count distinct P
    ;.log.error("Cycle detected in dependency chain ";"->"sv string P)
    ;'"dependency.cycle"
    ]
 }

// E: edges; P: paths
.boot.cycleChk:{[E]
 ;rhs:first each E
 ;lhs:last each E
 ;lhs:E idc:where any each whr:lhs~/:\:rhs
 ;rhs:(1_/:E) where each whr idc
 ;pth:raze lhs,/:'rhs
 ;.boot.uniqChk each pth
 ;pth
 }

.boot.init:{
  .log.init[]
 ;.boot.srcdir:1_ string (` vs hsym`$first system"readlink -f ",string .z.f) 0
 ;.boot.svcs:1!flip `name`file`nspace`deps!"SSS*"$\:()
 ;scr:scr where (scr:key `$":",.boot.srcdir) like "*.q"
 ;.boot.load each scr except `boot.q
 ;root:`$first .z.x
 ;$[not count .z.x
   ;'"No root-script name provided"
   ;not exec count i from .boot.svcs where name = root
   ;'"Cannot find service ",string root
   ;1b
   ]
 ;rqd:nsp,.boot.plot[.boot.svcs] nsp:exec first nspace from .boot.svcs where name = root

 ;if[count rmv:distinct raze exec nspace from .boot.svcs where not nspace in rqd
    ;.log.info ("Deleting unused namespaces ";rmv)
    ;{if[y in x;delete from y]}[` sv/:`,/:key`] each rmv
    ]
 ;delete from `.boot.svcs where not nspace in rqd
 ;if[not exec count nspace from .boot.svcs
    ;'"No services to start"
    ]
 ;.log.info("Starting namespaces ";rqd)
 ;edg:select nspace, deps from .boot.svcs
 ;edg:ungroup update deps:(deps,\:`) from edg where 0=count each deps
 ;edg:exec (nspace,'deps) from edg
 ;.boot.cycleChk/[edg]
 ;tbl:ungroup update deps:(deps,\:`) from select nspace, deps from .boot.svcs
 ;.boot.startLeaves/[tbl]
 ;1b
 }

.boot.register1:{[F;N;S;D]
  if[count tbl:select from .boot.svcs where (name=N)|nspace=S
    ;'.log.s1("Duplicate name/namespace registration for ";N;"/";S;"\n",.Q.s tbl)
    ]
 ;`.boot.svcs upsert (N;F;S;(),D)
 ; 
 }

.boot.init[]

