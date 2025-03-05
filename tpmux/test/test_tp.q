
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
    ;system"p 30099"
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
 ;tms:(til cnt)+.z.D + 08:00:00 + I * (6h$19h$16:30 - 08:00) div N
 ;lst:$[T~`trade
       ;(tms;tkr;100.0 + cnt?100.0;100 + cnt?100)
       ;(tms;tkr;ask-1;ask:50.0 + cnt?5;cnt#42;cnt#43)
       ]
 ;enlist lst
 }

/H:hopen .[jnl:`$":/home/michaelg/dev/projects/github.com/mgkdb/tpmux/test/test_sym",string .z.D;();:;()]
/jnl:get jnl
.mg.genData:{[H]
  H `upd,/:tbl,'.mg.genMsg[H;idx]'[til idx;tbl:(idx:100)?`trade`quote]
 }

.mg.init[];

