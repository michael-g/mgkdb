
.web.log:{[M]
  -1 (string .z.Z)," ",(string .z.w)," ",M
 ;
 }

.web.zws:{[X]
  msg:-9!X
 ;.web.log .Q.s1 msg
 ;.mg.msg:msg
 ;if[`.web.request ~ first msg
    ;qid:msg 1
    ;rps:value msg 2
    ;msg:(`.web.response;qid;0b;rps)
    ;.web.log "Replying with ",.Q.s1 msg
    ;(neg .z.w) -8! msg
    ]
 }

.web.init:{
  .h.HOME:(getenv`HOME),"/dev/projects/github.com/mgkdb/ipcjs/static"
 ;.z.ws:.web.zws
 ;
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
 }

.web.run[]
