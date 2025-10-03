// This file is part of the Mg kdb+/Krb5 Library (hereinafter "The Library").
//
// The Library is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// The Library is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
//
// You should have received a copy of the GNU Affero Public License along with The
// Library. If not, see https://www.gnu.org/licenses/agpl.txt.

.log.s1:{[M]
  $[10h~typ:type M
   ;M
   ;-10h~typ
   ;enlist M
   ;0h~typ
   ;raze .log.s1 each M
   ;.Q.s1 M
   ]
 }
.log.log:{[H;L;M]
  H L,.log.s1 M
 }
.log.debug:{[M]
  .log.log[-1;"DEBUG: ";M]
 }
.log.info:{[M]
  .log.log[-1;" INFO: ";M]
 }
.log.warn:{[M]
  .log.log[-1;" WARN: ";M]
 }
.log.error:{[M]
  .log.log[-2;"ERROR: ";M]
 }

.boot.ld:{[F]
  system"l ",1_ string F
 ;.log.info ("Loaded ";F)
 ;1b
 }

.boot.init:{
  src:`$":",first system"dirname $(readlink -f '",(string .z.f),"')"
 ;.boot.ld each ` sv/: src,/:`util.q`krb5.q`web.q
 ;1b
 }

.boot.init[];
