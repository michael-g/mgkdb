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
//--------------------------------------------------------------------------- .utl
.utl.init:{
  .utl.zw:{.z.w}
 ;.z.pc:.utl.zpc
 ;.z.po:.utl.zpo
 ;.utl.conns:1!flip`fd`usr`since!"ISP"$\:()
 ;.utl.cbks:flip`fd`typ`cbk!enlist each (0Ni;`;::)
 ;.utl.zpcCbks:()
 ;.utl.zpoCbks:()
 }

.utl.zpo:{[H]
  .log.debug("Have socket-open event for FD ";H)
 ;`.utl.conns upsert (H;.z.u;.z.P)
 }
.utl.onZpcCbkErr:{[H;E;B]
  .log.error("Failed in on-close callback for FD ";H;": ";E;"\n";.Q.sbt B)
 }
.utl.zpc:{[H]
  .log.debug("Have socket-close event for FD ";H)
 ;exec .Q.trp[;H;.utl.onZpcCbkErr H] each cbk from .utl.cbks where fd = H, typ=`zpc
 ;delete from `.utl.cbks where fd = H
 ;delete from `.utl.conns where fd = H
 }

.utl.init[];
//--------------------------------------------------------------------------- .krb

.krb.init:{
  dso:hsym`$getenv[`PWD],"/lib/libmgkrb5"
 ;.krb.acceptSecCtx:dso 2: (`mg_accept_sec_context;2)
 ;.krb.initSecCtx:  dso 2: (`mg_init_sec_context;3) // src_name, tgt_name, context
 ;.krb.conns:1!flip`fd`ctx`src`tgt`xpy!"IJ**P"$\:()
 ;.z.pw:.krb.zpw
 }

.krb.getUserForConn:{[H]
  exec first src from .krb.conns where fd = H
 }

// X: GSS context pointer -7h; S: src-name 10h; T: tgt-name 10h; V: seconds TTL -6h
.krb.addUserForConn:{[X;S;T;V]
  cfd:.utl.zw[]
 ;.log.info("authenticated user ";S;" for service ";T;" on FD ";cfd)
 ;xpy:.z.P + 18h$V
 ;`.krb.conns upsert (cfd;X;S;T;xpy)
 ;(1b;S)
 }

.krb.stashCtx:{[X;M]
  cfd:.utl.zw[]
 ;.log.debug("Storing intermediate context pending further Client data on FD ";cfd)
 ;`.krb.conns upsert (cfd;X;"";"";0Np)
 ;(0b;M)
 }

.krb.onAcceptSecCtxFail:{[E]
  .log.error("krb5 auth failed: ";E)
 ;0b
 }

.krb.doKrb5Auth:{[T]
  // TODO lookoup any existing context-pointer and pass to gss_accept_sec_context
  ctx:$[exec count ctx from .krb.conns where fd = .utl.zw[]
       ;exec first ctx from .krb.conns where fd = .utl.zw[]
       ;0Nj
       ]
 ;.log.debug("Calling acceptSecCtx with context arg ";.Q.s1 ctx)
 ;res:.[.krb.acceptSecCtx;(trim T;ctx);.krb.onAcceptSecCtxFail]
 ;.log.debug("libmgkrb5 authentication response is ";.Q.s1 res)
 ;$[not 0h~type res
   ;0b
   ;not count res
   ;0b
   ;1i~res 0      // auth complete, should be a 4-element list (1i;src_name;tgt_name;context_ptr;validity_secs_i)
   ;.krb.addUserForConn . res 3 1 2 4
   ;2i~res 0      // auth incomplte, should be a 3-element list (2i;context_ptr;b64_encoded_msg)
   ;.krb.stashCtx . res 1 2
   ;0b
   ]
 }

.krb.doKrbPwAuth:{[U;P]
  .log.debug("authenticating user ";U;"; password.length is ";count P)
 ;first res:.krb.doKrb5Auth P
 }

.krb.zpw:{[U;P]
 ;$[$[count tbl:select src from .krb.conns where fd = .utl.zw[];count first tbl`src;0b]
   ;1b
   ;not P like "YII*"
   ;0b
   ;.krb.doKrbPwAuth[U;P]
   ]
 }

.krb.kopen:{[H;P;S;T]
  src:"michaelg@MINDFRUIT.KRB"
 ;tgt:"HTTP/fedora@MINDFRUIT.KRB"
 ;H:""
 ;P:30097
 ;ctx:0Nj
 ;if[not 1i~first res:.krb.initSecCtx[src;tgt;ctx]
    ;'"failed to initialise krb5 context"
    ]
 ;hopen `$":",":"sv(H;string P;string .z.u;res 1)
 }

.krb.init[];
//--------------------------------------------------------------------------- .web

.web.init:{
 ;.krb.init[]
 ;.h.HOME:getenv[`PWD],"/html"
 ;.z.ac:.web.zac
 ;.z.wo:.web.zwo
 ;.z.wc:.web.zwc
 ;.z.ws:.web.zws
 ;.web.reqs:()
 ;.web.http401:(0;"")
 ;1b
 }

.web.userOk:{[U]
  (1;U)
 }

.web.customRps:{[M]
  (2;M)
 }

// C.f. https://www.rfc-editor.org/rfc/rfc9110.html#http.date
// E.g. Sun, 06 Nov 1994 08:49:37 GMT
.web.fmtHttpDate:{
  first system "TZ=GMT date '+%a, %d %b %Y %H:%M:%S %Z'"
 }

// B: SPNEGO base64 data as char-vec (or the empty vec "")
.web.genKrb5Challenge:{[B]
  $[count B
   ;.log.debug("Issuing WWW-Authenticate reply on FD ";.utl.zw[];"; SNPEGO data provided, length ";count B)
   ;.log.debug("Issuing initial WWW-Authenticate challenge on FD ";.utl.zw[])
   ]
 ;txt:("HTTP/1.1 401 Unauthorized"
      ;"WWW-Authenticate: Negotiate",$[count B;" ",B;""]
      //;"Content-Type: text/html"
      ;"Content-Length: 0"
      ;"Connection: keep-alive"
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;"";"")
 ;"\r\n"sv txt
 }

// T is a two-element tuple, of request text and headers; the latter is a dict. The
// function must return a two-element list, whose first element is in the set {0, 1, 2, 4}.
// 0j generates a 401 not-authorized response.
// 1j authorises the user, whose username should be the second element (as char-vec).
// 2j sends the second element as the response text, while
// 4j (whose second element is the empty char-vec), falls back to Basic authentication.
//
// C.f. https://code.kx.com/q/ref/dotz/#zac-http-auth
//
// RFC 4559 s.5, https://www.rfc-editor.org/rfc/rfc4559.html:
// If the context is not complete, the server will respond with a 401 status code with
// a WWW-Authenticate header containing the gssapi-data.
//   S: HTTP/1.1 401 Unauthorized
//   S: WWW-Authenticate: Negotiate 749efa7b23409c20b92356
// The context will need to be preserved between client messages.
.web.zac:{[T]
  .log.debug(".z.ac authentication requested for FD ";.utl.zw[])
 ;txt:T 0
 ;hdr:T 1
 ;.web.reqs,:enlist T
 ;tup:$[$[10h~type usr:.krb.getUserForConn .utl.zw[];count usr;0b]
       ;.web.userOk usr
       ;""~tkn:hdr`Authorization
       ;.web.customRps .web.genKrb5Challenge ""
       ;not tkn like "Negotiate YII*"
       ;.web.http401
       ;0b~res:.krb.doKrb5Auth (count "Negotiate ")_ tkn
       ;.web.http401
       ;1b~res 0
       ;.web.userOk res 1
       ;0b~res 0
       ;.web.customRps .web.genKrb5Challenge res 1
       ;.web.http401
       ]
 ;.log.debug(".z.ac result is ";.Q.s1 tup)
 ;tup
 }

.web.zwo:{[H]
  .log.debug("Have websocket-open on FD ";H)
 }

.web.zwc:{[H]
  .log.debug("Have websocket-close on FD ";H)
 }

// websocket request handler; X has type in 10 4h.
// C.f. https://code.kx.com/q/ref/dotz/#zwo-websocket-open
.web.zws:{[X]
   $[10h~type X
    ;.log.debug("Have websocket text request:\n";X)
    ;4h~type X
    ;.log.debug("Have websocket binary request:\n";-9!X)
    ;.log.error("Have bad websocket type: ";type X;"; expected type in 10 4h")
    ]
  }
.web.init[];
