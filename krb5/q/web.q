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
 ;.z.pc:.utl.zpc
 ;.z.po:.utl.zpo
 ;.utl.conns:1!flip`fd`usr`since!"ISP"$\:()
 ;.utl.cbks:flip`fd`typ`cbk!enlist each (0Ni;`;::)
 ;.utl.zpcCbks:()
 ;.utl.zpoCbks:()
 }

.utl.zw:{.z.w}
.utl.zP:{.z.P}

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
 ;.krb.initSecCtx:  dso 2: (`mg_init_sec_context;2)
 ;.krb.conns:1!flip`fd`ctx`src`tgt`xpy!"I***P"$\:()
 ;.z.pw:.krb.zpw
 }

// S: src-name 10h; T: tgt-name 10h; V: seconds TTL -6h
.krb.addUserForConn:{[S;T;V]
  cfd:.utl.zw[]
 ;.log.info("authenticated user ";S;" for service ";T;" on FD ";cfd)
 ;xpy:.z.P + 18h$V
 ;`.krb.conns upsert (cfd;4h$();S;T;xpy)
 ;(1b;S;T;V)
 }

.krb.stashCtx:{[X;M]
  cfd:.utl.zw[]
 ;.log.debug("storing intermediate context pending further Client data on FD ";cfd)
 ;`.krb.conns upsert (cfd;X;"";"";0Np)
 ;(0b;M)
 }

.krb.onAcceptSecCtxFail:{[E]
  .log.error("krb5 auth failed: ";E)
 ;0b
 }

.krb.doKrb5Auth:{[T]
  ctx:exec first ctx from .krb.conns where fd = .utl.zw[]
 ;.log.debug("calling .krb.acceptSecCtx with ";$[not count ctx;"null context arg";"context arg ",.Q.s1 ctx])
 ;res:.[.krb.acceptSecCtx;(trim T;ctx);.krb.onAcceptSecCtxFail]
 ;.log.debug("krb5 authentication response is ";.Q.s1 res)
 ;$[not 0h~type res
   ;0b
   ;not count res
   ;0b
   ;1i~res 0      // auth complete, res is a 4-element list (1i;src_name;tgt_name;validity_secs_i)
   ;.krb.addUserForConn . 1_ res
   ;2i~res 0      // auth incomplte, res is a 3-element list (2i;context_ptr;b64_encoded_msg)
   ;.krb.stashCtx . 1_ res
   ;0b
   ]
 }

.krb.doKrbPwAuth:{[U;P]
  .log.debug("authenticating IPC connetion for user ";U;" with .rkb.doKrb5Auth over password of length ";count P)
 ;first .krb.doKrb5Auth P
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
 ;if[not 1i~first res:.krb.initSecCtx[src;tgt]
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
 ;.z.ph:.web.zph
 ;.z.wo:.web.zwo
 ;.z.wc:.web.zwc
 ;.z.ws:.web.zws
 ;.web.reqs:()
 ;.web.cookies:1!flip`user`validity`data!enlist each ("";0Np;"")
 ;.web.http401:(0;"")
 ;.web.http404:.h.hn["404 Not Found";`txt;""]
 ;.web.keksNom:"some_cookie_name"
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

// F: -11h file hsym; Y: 10h Content-Type
.web.sendContent:{[F]
  .log.debug("Serving file ";F)
 ;res:"c"$@[1::;F;""]
 ;res:("HTTP/1.1 200 OK"
      ;"Content-Type: ",.h.ty`$last"."vs string F
      ;"Connection: keep-alive"
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;""
      ;res
      )
 ;"\r\n"sv res
 }

.web.txFile:{[F]
  $[""~F
   ;"index.html"
   ;"favicon.ico"~F
   ;"favicon.png"
   ;F
   ]
 }

.web.zph:{[T]
  .log.debug("Have GET request for ";T 0)
 ;$[-11h~type key fle:`$":",.h.HOME,"/",.web.txFile T 0
   ;.web.sendContent fle
   ;.web.http404
   ]
 }

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Set-Cookie
// Don't set Expires, so we get a session cookie; we could set when the Kereros token expires,
// of course. Don't set Domain, so we get string domain binding. 
// We could do so much better, e.g. different cookie names for different stages of the 
// authentication pipeline, deleting cookies as we go, rotating values on each new 401 
// response, eventually, once we hit .z.ph, removing the previous session cookie and 
// replacing it with one which expires when the Kerberos token expires ... but that is
// left for another day.
.web.getCookie:{
  .web.keksNom,"=",raze system"od -vAn -N64 -tx1 /dev/urandom | sed 's/ //g'"
 }

// B: SPNEGO base64 data as char-vec (or the empty vec "")
.web.genKrb5Challenge:{[H;B]
  cuk:.web.getCookie[]
 ;$[count B
   ;.log.debug("Issuing WWW-Authenticate reply on FD ";.utl.zw[];"; SNPEGO data provided, length ";count B)
   ;.log.debug("Issuing initial WWW-Authenticate challenge on FD ";.utl.zw[];"; cookie begins ";50#cuk)
   ]
 ;txt:("HTTP/1.1 401 Unauthorized"
      ;"WWW-Authenticate: Negotiate",$[count B;" ",B;""]
      ;"Content-Type: text/html"
      ;"Content-Length: 0"
      ;"Connection: ",.h.ka 601000i
      //;"Set-Cookie: ",cuk
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;"";"")
 ;"\r\n"sv txt
 }

// K: cookie 10h; C: client 10h; S: service 10h; T: seconds TTL -6h
.web.blessConn:{[K;C;S;T]
//   .log.debug("blessConn: ";T;" ";$[50<count K;50#K;K])
//  ;if[K like .web.keksNom,"*"
//     ;`.web.cookies upsert (C;.utl.zP[] + 18h$T;K)
//     ]
 ;.web.userOk C
 }

// .web.cookieAuthOk:{[K]
//   $[count tbl:select user from .web.cookies where data~\:K
//    ;[.log.debug("Have valid cookie-auth for user ";first tbl`user;" with cookie ";50#K)
//     ;(1b;first tbl`user)
//     ]
//    ;0b
//    ]
//  }
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
  .log.debug("HTTP auth required for FD ";.utl.zw[])
 ;txt:T 0
 ;hdr:T 1
 ;.web.reqs,:enlist T
 //;tup:$[1b~first tag:.web.cookieAuthOk hdr`Cookie
 //      ;.web.userOk tag 1
 ;tup:$[""~tkn:hdr`Authorization
       ;.web.customRps .web.genKrb5Challenge[hdr;""]
       ;not tkn like "Negotiate YII*"
       ;.web.http401
       ;0b~res:.krb.doKrb5Auth (count "Negotiate ")_ tkn
       ;.web.http401
       ;1b~res 0
       ;.web.blessConn . @[res;0;:;hdr`Cookie]
       ;0b~res 0
       ;.web.customRps .web.genKrb5Challenge[hdr;res 1]
       ;.web.http401
       ]
 ;.log.debug(".z.ac result is (";tup 0;";";$[2=tup 0;first"\r\n"vs tup 1;tup 1];")")
 ;tup
 }
//.web.zac:{[F;T] .Q.trp[F;T;{.log.error(x;":\n";.Q.sbt y)}]}.web.zac

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

