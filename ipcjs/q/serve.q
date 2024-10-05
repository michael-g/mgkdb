
.web.log:{[M]
  -1 (string .z.Z)," ",(string .z.w)," ",M
 ;
 }

.web.zws:{[X]
  .web.log .Q.s1 -9!X
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
