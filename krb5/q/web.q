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

// H: hostname -11h; P: port any neg 5 6 7h; S: Client src_name with realm 10h;
// T: target Service name with realm 10h
// e.g. for localhost:
// rfd:.krb.kopen . (`;30097;"michaelg@MINDFRUIT.KRB";"HTTP/fedora@MINDFRUIT.KRB")
// On success, returns a normal IPC file descriptor.
.krb.kopen:{[H;P;S;T]
  if[not 1i~first res:.krb.initSecCtx[S;T]
    ;'"failed to initialise krb5 context"
    ]
 ;hopen `$":",":"sv(string H;string P;string .z.u;res 1)
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

.web.customRep:{[M]
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
      ;"Connection-Length: ",string count res
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;""
      ;res
      )
 ;"\r\n"sv res
 }

// Translate requests for file F (10h) on-the-fly
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

// B: SPNEGO base64 data as char-vec (or the empty vec "")
.web.genKrb5Challenge:{[H;B]
  $[count B
   ;.log.debug("Issuing WWW-Authenticate reply on FD ";.utl.zw[];"; SNPEGO data provided, length ";count B)
   ;.log.debug("Issuing initial WWW-Authenticate challenge on FD ";.utl.zw[])
   ]
 ;txt:("HTTP/1.1 401 Unauthorized"
      ;"WWW-Authenticate: Negotiate",$[count B;" ",B;""]
      ;"Content-Type: text/html"
      ;"Content-Length: 0"
      ;"Connection: ",.h.ka 601000i
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;"";"")
 ;"\r\n"sv txt
 }

// Given Client, target Service, TTL and any cookie value submitted by the client, return
// (1;C) -- or some derivation of C, e.g. after removing the Krb5 Realm. Were you to set a
// new cookie in .web.genKrb5Challenge, it would be present here, and you would be able to
// identify future connections by the client from the cookie and not need to issue the 
// SPNEGO challenge. I did play with this until I got the websocket working! However, the
// problem there was the websocket was opening ws://localhost instead of ws://hostname, and
// the 401 Not Authorised response was terminal. Since making that change the SPNEGO 
// (pre-upgrade) challenge worked as expected and the upgrade successful (i.e. no cookie
// needed).
// K: cookie 10h; C: client 10h; S: service 10h; T: seconds TTL -6h
.web.blessConn:{[K;C;S;T]
  .web.userOk C
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
// Perhaps after we have validated the SPNEGO token, extracted and approved the client, we
// return (1;"user"). kdb will then call .z.ph to return the resource. Prior to .h.ka in kdb
// 4.1 (despite setting `Connection: keep-alive` in the headers) kdb closes the connection
// after each request/response pair. This is evident from the RST packets seen in
// Wireshark.
//
// However, this doesn't upset the SPNEGO sequence, which then runs as follows:
//   GET /something
//   401 Not authd, WWW-Authenticate: Negotiate
//   [close]
//   GET /something, Authorization: Negotiate YII..
//   200 OK
//   [close]
// 
// This may look different if we're using .h.ka, as kdb will not immediately close the
// connection.
// 
// However ... in RFC 4559 (https://www.rfc-editor.org/rfc/rfc4559.html), re the SPNEGO
// auth flow, s.5 provides: 
// "If the context is not complete, the server will respond with a 401 status code with
// a WWW-Authenticate header containing the gssapi-data."
// The _next_ 401 Not Authd WWW-Authenticate response then becomes `Negotiate 749efa7b..`. 
//
// The intractable problem is then connection tracking, since we need to associate the
// current GSSAPI `gss_ctx_id_t` context-handle with _this_ message sequence. HTTP is meant
// to be a stateless protocol. Even with .h.ka, we can't lean on the file-descriptor, since
// the browser might close the connection and there is no HTTP close-handler like .z.pc we
// can use.
//
// Perhaps we could set a cookie to encode a lookup key for a context-handle we can stash
// somewhere? I'm only half joking. This might work some of the time, but it's not
// trustworthy as a browser can use mutiple concurrent connections and "the other one" could
// return the cookie out-of-band and corrupt our data flow.
//
// Thus, _mutual_ Krb5 authentication seems to be off the cards (not that I know how to
// get the browser to ask for this).
//
// To be fair, I haven't seen the GSSAPI ask to send data back to the browser.
.web.zac:{[T]
  .log.debug("HTTP auth required for FD ";.utl.zw[])
 ;txt:T 0
 ;hdr:T 1
 ;.web.reqs,:enlist T
 // Were you to use a cookie to identify clients that had authenticated, you would
 // probably want to add a (new) .web.chkCookieAuth[hdr`Cookie] here as the first
 // test-branch.
 ;tup:$[""~tkn:hdr`Authorization
       ;.web.customRep .web.genKrb5Challenge[hdr;""]
       ;not tkn like "Negotiate YII*"
       ;.web.http401
       ;0b~res:.krb.doKrb5Auth (count "Negotiate ")_ tkn
       ;.web.http401
       ;1b~res 0
       ;.web.blessConn . @[res;0;:;hdr`Cookie]
       ;0b~res 0
       ;.web.customRep .web.genKrb5Challenge[hdr;res 1]
       ;.web.http401
       ]
 ;.log.debug(".z.ac result is (";tup 0;";";$[2=tup 0;first"\r\n"vs tup 1;tup 1];")")
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

