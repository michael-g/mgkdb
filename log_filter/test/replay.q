
upd:{[T;X]
  -1 (string .z.Z)," DEBUG: ",.Q.s1 (`upd;T;X)
 ;
 }

.rpl.trdMsg:{
  (.z.p;`VOD.L;100;100f;"||||h r|";`XLON)
 }

.rpl.qteMsg:{
  (.z.p;`AZN.L;42;100.;101.;19;`XLON)
 }

.rpl.wr:{[H;T]
  H enlist (`upd;T;$[`trade~T;.rpl.trdMsg;.rpl.qteMsg]`)
 }

.rpl.wrjnl:{[F;N]
  wrf:.rpl.wr jfd:hopen .[F;();:;()]
 ;wrf each ?[1h$N?2;`trade;`quote]
 ;hclose jfd
 }

.rpl.init:{
  lib:first system"(cd \"$(dirname $(readlink -f ",(string .z.f),"))/..\" && pwd || exit 1)"
 ;lib:`$":",lib,"/lib/libmglogfilter"
 ;.rpl.replay: lib 2: (`mg_replay;2)
 ;-1 "Now maybe run\n-11!`:test/journal\nor ...\n.rpl.replay[`:test/journal] enlist`trade"
 }

.rpl.init[];

