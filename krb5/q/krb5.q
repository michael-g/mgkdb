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

// TODO include validity of connection in the client .krb.conns table
//
// Not implemented (ha, left as an exercise to the .. you know):
// . Removal of stale connection data, in particular, unused serialised context data
// . Expiry of connections in .krb.conns

.krb.init:{
  dso:hsym`$getenv[`PWD],"/lib/libmgkrb5"
 ;.krb.acceptSecCtx:    dso 2: (`mg_accept_sec_context;1)
 ;.krb.acceptSecCtxCont:dso 2: (`mg_accept_sec_context_cont;2)
 ;.krb.initSecCtx:      dso 2: (`mg_init_sec_context;2)
 ;.krb.initSecCtxCont:  dso 2: (`mg_init_sec_context_cont;2)
 ;.krb.conns:1!flip`fd`src`tgt`xpy`ctx`dir!enlist each (0Ni;"";"";0Np;::;" ")
  // Given the process arguments -krb.server 1, require _incoming_ connections to be
  // Kerberized. When running as a client of a Kerberos server we may not want this.
  // NB unsecured clients could easily be used as a springboard/proxy. This is not
  // a hardening guide ;)
 ;if[$[10h~type arg:first(.Q.opt .z.x)`krb.server;"B"$arg;0b]
    ;.z.pw:.krb.zpw
    ]
 }

// T: target 10h; K: token 10h may be ""
.krb.onMutualAuth:{[T;K]
  cfd:.utl.zw[]
 ;.log.debug("Have mutual-auth reply from ";T)
  // We update the tgt value in .krb.conns and delete the sec-context pointers, which are
  // now invalid. Anyone considering implementing this code would probably set a flag on
  // the table indicating that T matched expectations and the IPC connection is considered
  // Good.
 ;update ctx:(::), tgt:enlist T from `.krb.conns where fd = cfd
  // deal with K being of non-zero length: it would imply yet more data to be sent back
  // to the client ... which given we're "complete" would be an unexpected configuration
 }

// Called asynchronously by the remote after it has replied (1;"username") in .z.ac; this is
// the return leg of the mutual-authentication we (the client) requested during the
// handshake, and confirms the remote service to which we're connected. We have to use this
// "out of band" method because the .z.pw checks don't allow for multi-part authentication.
// See https://mindfruit.co.uk/posts/2025/09/kdb_and_kerberos_authentication/ for a brief
// discussion of the uncomfortable alternatives.
// K: token 10h
.krb.onRemoteId:{[K]
  .log.debug("received reply-token on FD ";.utl.zw[];" from remote ";$[50<count K;(50#K),"..";K])
 ;if[1=count tbl:select ctx from .krb.conns where fd = .utl.zw[], dir="O"
    ;res:.krb.initSecCtxCont[;K] first first value flip tbl
     // validate 'tgt' here:
     // . check whether the actual target matches our expectation
     // . note whether it contains the Krb5 Realm or not
    ;$[$[3=count res;$[2i~res 0;10h~type res 1;0b];0b] // res is: knk(3, ki(2), k_peer_name, k_token)
      ;.krb.onMutualAuth . 1_ res
      ;.log.warn("Have unexpected mutual-auth response:\n";.Q.s1 res)
      ]
    ]
 }

// Open a kdb+ IPC connection to the remote peer sending a Kerberos token (TGS-REQ) for the
// given local principal and remote service. On success, returns an IPC file descriptor.
// e.g. for localhost:
// q)svc:.krb.kopen . (`;30097;"michaelg@MINDFRUIT.KRB";"HTTP/fedora@MINDFRUIT.KRB")
//
// This is a blocking connection request (.i.e. without timeout) which may hang indefinitely.
// You should probably modify this to connect asynchronously and deliver the file-descriptor
// on a callback upon success or N failures via the timer system to release the main thread.
//
// H: hostname -11h; P: port one of -5 -6 -7 -11h; S: Client src_name (optionally with realm) 10h;
// T: target Service name (optionally with realm) 10h
.krb.kopen:{[H;P;S;T]
  if[not 1i~first res:.krb.initSecCtx[S;T] // returns knk(5, ki(1i), k_tgt_name, k_ctx, k_token, k_ttl);
    ;'"failed to initialise krb5 context"
    ]
 ;rfd:hopen `$":",":"sv(string H;string P;string .z.u;res 3)
 ;`.krb.conns upsert (rfd;S;T;0Np;res 2;"O")
 ;rfd
 }

// Sends token T to IPC peer H upon timer Id I firing
.krb.sendToken:{[H;T;I]
  (neg H)(`.krb.onRemoteId;T)
 ;(neg H)[]
 }

// S: src-name 10h; T: auth'd tgt-name 10h; V: seconds TTL -6h; K: base-64 output-token 10h, may be ""
.krb.onAuthSuccess:{[S;T;V;K]
  cfd:.utl.zw[]
 ;.log.info("authenticated user ";S;" for service ";T;" on FD ";cfd)
 ;`.krb.conns upsert (cfd;S;T;.web.ttlFromSecs V;::;"I")
 ;if[count K
    ;.utl.addTimer[.krb.sendToken[cfd;K;];0i;0b]
    ]
 ;1b
 }

// Where the accept-context GSSAPI function signals further data are required from the peer
// (via GSS_S_CONTINUE_NEEDED), we have to fail the .z.pw password check, since there's no
// facility to pass multiple messages back and forth before "blessing" the connection. We
// have no choice -- in this implementation -- but to deny the connection.
// X: context-handle 4h; K b64 output-token 10h
.krb.denyContinuation:{
  cfd:.utl.zw[]
 ;.log.debug("Denying auth-request for FD ";cfd;" as GSSAPI supplementary info was requested")
 ;delete from `.krb.conns where fd = cfd
 ;0b
 }

// Default handler for the authentication "switch block" in .krb.zpw: prints the unexpected
// response to the console and denies the connection request.
// R: C-library response, probably 0h
.krb.denyUnreachable:{[R]
  .log.warn("Unexpected response from krb5 shared library on FD ";.utl.zw[];":\n",.Q.s1 R)
 ;0b
 }

// PE error-handler for the .krb.acceptSecCtx native code delegate.
.krb.onAcceptSecCtxFail:{[E]
  .log.error("krb5 auth failed for FD ";.utl.zw[];": ";E)
 ;0b
 }

// .z.pw username/password handler. Installed if the kdb+ instance is started with command-line
// arguments `-krb.server 1`. Requires all incoming IPC connections to present a valid Kerberos
// token that can be decrypted by this instance. Note KRB5_KTNAME and KRB5CCNAME environment
// variables: https://web.mit.edu/kerberos/krb5-1.13/doc/admin/env_variables.html
//
// U: username -11h; K: Kerberos token/password 10h
.krb.zpw:{[U;K]
  .log.debug("authenticating IPC connetion for user ";U;" with password of length ";count K)
 ;$[count ctx:exec first ctx from .krb.conns where fd = .utl.zw[], dir="O"
   ;.log.debug "calling .krb.acceptSecCtx with context arg ",.Q.s1 ctx
   ;.log.debug "calling .krb.acceptSecCtx, no existing context found"
   ]
 ;res:@[.krb.acceptSecCtx;trim K;.krb.onAcceptSecCtxFail]
 ;.mg.server.res:res
 ;$[$[not 0h~type res;1b;6<>count res]
   ;0b
   ;1i~res 0      // auth complete, knk(6, [0] ki(workflow), [1] k_src_name, [2] k_tgt_name, [3] ki((I)time_rec), [4] k_ctx, [5] k_out_tkn);
   ;.krb.onAuthSuccess[res 1;res 2;res 3;res 5]
   ;2i~res 0      // auth incomplete, res is a 3-element list (2i;context_ptr;b64_encoded_msg)
   ;.krb.denyContinuation[]
   ;.krb.denyUnreachable res
   ]
 }

.krb.init[];
