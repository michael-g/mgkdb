/ This file is part of the "Mg kdb+ Library" (hereinafter "The Library").
/.
/ The Library is free software: you can redistribute it and/or modify it under
/ the terms of the GNU Affero Public License as published by the Free Software
/ Foundation, either version 3 of the License, or (at your option) any later
/ version.
/.
/ The Library is distributed in the hope that it will be useful, but WITHOUT ANY
/ WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
/ PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
/.
/ You should have received a copy of the GNU Affero Public License along with The
/ Library. If not, see https://www.gnu.org/licenses/agpl.txt.


upd:{[T;X]
  -1 (string .z.Z)," DEBUG: ",.Q.s1 (`upd;T;X)
 ;
 }

.rpl.trdMsg:{
 	enlist each (.z.p;`VOD.L;100;100f;"||||h r|";`XLON)
 }

.rpl.trdTbl:{
  flip`time`sym`prc`sz`mmt`xch!.rpl.trdMsg[]
 }

.rpl.qteMsg:{
  enlist each (.z.p;`AZN.L;42;100.;101.;19;`XLON)
 }

.rpl.qteTbl:{
  flip`time`sym`bsz`bid`ask`asz`xch!.rpl.qteMsg[]
 }

.rpl.wr:{[H;T]
  H enlist (`upd;T;$[`trade~T;.rpl.trdMsg;`trade_tbl~T;.rpl.trdTbl;`quote~T;.rpl.qteMsg;.rpl.qteTbl]`)
 }

.rpl.wrjnl:{[F;N]
  wrf:.rpl.wr jfd:hopen .[F;();:;()]
 ;wrf each ?[1h$N?2;`trade;`quote]
 ;wrf each ?[1h$N?2;`trade_tbl;`quote_tbl]
 ;hclose jfd
 }

.rpl.init:{
  lib:first system"(cd \"$(dirname $(readlink -f ",(string .z.f),"))/..\" && pwd || exit 1)"
 ;lib:`$":",lib,"/lib/libmglogfilter"
 ;.rpl.replay: lib 2: (`mg_replay;2)
 ;-1 "Now maybe run\n-11!`:test/journal\nor ...\n.rpl.replay[`:test/journal] enlist`trade"
 }

.rpl.init[];

