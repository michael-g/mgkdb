
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

.mg.init[];

