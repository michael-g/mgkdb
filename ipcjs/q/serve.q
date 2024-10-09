
.web.log:{[M]
  -1 (string .z.Z)," ",(string .z.w)," ",M
 ;
 }

.web.getTable:{[C]
  .web.log"In function .web.getTable"
 ;n:1000;
 ;sym:` sv' flip (upper n?`3;`L)
 ;tme:(.z.d+08:00) + n?7 * 60 * 60 * 1000 * 1000 *1000
 ;tbl:`sym`time xasc flip`time`sym`price`size!(tme;sym;n?1000f;768 + n ? 250000)
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
