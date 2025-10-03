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


.srv.zwo:{
  0N!"Websocket connected on ",string .z.w
 }
.srv.zws:{[M]
  msg:$[4h~type M
       ;-9!M
       ;10h~type M
       ;.j.k M
       ;'0N!"Can't do anything with WS message ",.Q.s1 M
       ]
 ;0N!"Have WS message ",.Q.s1 msg
 ;
 }

.srv.init:{
 ;opt:.Q.opt .z.x
 ;system"p ",$[`port in key opt;first opt`port;"30097"]
 ;.h.HOME:first system"readlink -f ",$[`html in key opt;first opt`html;"html"]
 ;system"test -d ",.h.HOME
 ;if[not `ipc.js in key hsym`$.h.HOME
    ;system"ln -s ",.h.HOME,"/../../ipcjs/src/ipc.js ",.h.HOME,"/ipc.js"
    ]
 ;.z.wo:.srv.zwo
 ;.z.ws:.srv.zws
 ;
 }

.srv.init[];
