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
 ;.z.ts:.utl.zts
 ;.utl.conns:1!flip`fd`usr`since!"ISP"$\:()
 ;.utl.cbks:flip`fd`typ`cbk!enlist each (0Ni;`;::)
 ;.utl.tid:0
 ;.utl.timers:1!flip`id`millis`rpt`fn`nxttp!"JIB*P"$\:()
 ;.utl.zpcCbks:()
 ;.utl.zpoCbks:()
 }

.utl.zw:{.z.w}
.utl.zP:{.z.P}
.utl.zp:{.z.p}

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

//--------------------------------------------------------------------------- .utl.timers
.utl.onTimerFail:{[E;B]
  .log.error("While executing timer: '";E;"\n:";.Q.sbt B)
 }

.utl.execTimer:{[K;M;R;F;X]
  .Q.trp[F;K;.utl.onTimerFail]
 ;$[R
   ;update nxttp:(.z.p + 1000000 * M) from `.utl.timers where id = K
   ;not count tp:exec nxttp from .utl.timers where id=K
   ;0
   ;X <> first tp
   ;.log.debug("Leaving timer with id ";K)
   ;[delete from `.utl.timers where id = K;.log.debug("Cleared timer with id ";K)]
   ]
 ;
 }

.utl.zts:{
  .utl.execTimer ./: flip value flip 0!select from .utl.timers where nxttp <= .z.p
 ;.utl.setTimeout[]
 ;
 }

.utl.setTimeout:{
  $[not count .utl.timers
   ;system "t 0"
   ;(19h$zp:.z.p) >= 19h$tp:(exec from .utl.timers where nxttp = min nxttp)`nxttp
   ;system "t 1"
   ;system "t ",string $[0=tp:6h$19h$tp-zp;1;tp]
   ]
 ;
 }

// F: monadic function/projection; M: millis -6h; R: repeat 1h
.utl.addTimer:{[F;M;R]
  `.utl.timers upsert (.utl.tid+:1;M;R;F;.utl.zp[] + 1000000 * M)
 ;.utl.setTimeout[]
 ; 
 }

.utl.init[];
//--------------------------------------------------------------------------- .krb

.krb.init:{
  dso:hsym`$getenv[`PWD],"/lib/libmgkrb5"
 ;.krb.acceptSecCtx:dso 2: (`mg_accept_sec_context;2)
 ;.krb.initSecCtx:  dso 2: (`mg_init_sec_context;2)
 ;.krb.contSecCtx:  dso 2: (`mg_cont_sec_context;4)
 ;.krb.conns:1!flip`fd`src`tgt`xpy`ctx`dir!enlist each (0Ni;"";"";0Np;"";" ")
 ;.z.pw:.krb.zpw
 }

// Called asynchronously by the remote after it has replied (1;"username") in .z.ac; this
// is the return leg of the mutual-authentication we've requested during the handshake, and
// confirms the remote service to which we're connected. We have to use the "out of band" 
// method because the .z.pw checks don't allow for multi-leg authentication. We could run
// our own server-socket, listen for connections, do multi-leg authentication, but from the
// server's point-of-view that's a load of hassle for no upside: the server is done as soon
// as it's resolved who _we_ are. Sending this application-level reply doesn't seem much
// worse, since it's in here that we, the client, can elect to close the connection if the
// remote doesn't match who we think it should be.
.krb.onRemoteId:{[T]
  .log.debug("Received reply-token from remote ";$[50<count T;T;50#T])
 ;.krb.reply:T
 ;if[1=count tbl:select src, tgt, ctx from .krb.conns where fd = .utl.zw[], dir="O"
    ;res:.krb.contSecCtx[;;;T] . value first tbl // returns  K vals = knk(4, ki(workflow), k_tgt_name, k_ctx, k_token);
    ;if[$[4=count res;$[2i~res 0;10h~type res 1;0b];0b]
       ;.log.debug("Have mutual-auth reply from ";res 1;"; result is ";52#.Q.s1 res;"...\")")
       ;tgt:res 1 
       // check if tbl`tgt has realm in it; if so, compare both, if not, strip from 'tgt'
       // TODO store context?
       ]
    ]
 }

// H: hostname -11h; P: port any neg 5 6 7h; S: Client src_name optionally with realm 10h;
// T: target Service name optionally with realm 10h
// e.g. for localhost:
// rfd:.krb.kopen . (`;30097;"michaelg@MINDFRUIT.KRB";"HTTP/fedora@MINDFRUIT.KRB")
// On success, returns a normal IPC file descriptor.
.krb.kopen:{[H;P;S;T]
  if[not 1i~first res:.krb.initSecCtx[S;T] // returns knk(4, ki(workflow), k_tgt_name, k_ctx, k_token);
    ;'"failed to initialise krb5 context"
    ]
 ;rfd:hopen `$":",":"sv(string H;string P;string .z.u;res 3)
 ;`.krb.conns upsert (rfd;S;T;0Np;res 2;"O")
 ;rfd
 }

.krb.sendToken:{[H;T;I]
  (neg H)(`.krb.onRemoteId;T)
 ;(neg H)[]
 }

// W: workflow -11h; S: src-name 10h; T: auth'd tgt-name 10h; V: seconds TTL -6h; X: context 10h; K: base-64 output-token 10h maybe count 0
.krb.onAuthSuccess:{[W;S;T;V;X;K]
  cfd:.utl.zw[]
 ;.log.info("authenticated user ";S;" for service ";T;" on FD ";cfd)
 ;`.krb.conns upsert (cfd;S;T;.web.ttlFromSecs V;X;"I")
 ;if[$[`ipc~W;count K;0b]
    ;.utl.addTimer[.krb.sendToken[cfd;K;];0i;0b]
    ]
 ;(1b;S;T;V;X;K)
 }

// W: workflow -11h; X: context-handle 4h; K b64 output-token 10h
.krb.stashCtx:{[W;X;T]
  if[`ipc~W
    ;cfd:.utl.zw[]
    ;.log.debug("storing intermediate context pending further Client data on FD ";cfd)
    ;`.krb.conns upsert (cfd;"";"";0Np;X;"I")  // Do not store the context if we're doing HTTP auth
    ]
 ;(0b;M)
 }

.krb.onAcceptSecCtxFail:{[E]
  .log.error("krb5 auth failed: ";E)
 ;0b
 }

// W: workflow -11h; K: token 10h
.krb.doKrb5Auth:{[W;K]
  ctx:exec first ctx from .krb.conns where fd = .utl.zw[], dir="O"
 ;$[not count ctx
   ;.log.debug "calling .krb.acceptSecCtx with null context arg"
   ;.log.debug("calling .krb.acceptSecCtx with context arg ",.Q.s1 ctx)
   ]
 ;res:.[.krb.acceptSecCtx;(trim K;ctx);.krb.onAcceptSecCtxFail]
 ;.log.debug("krb5 authentication response is ";80#.Q.s1 res;"...\")")
 ;$[not 0h~type res
   ;0b
   ;$[1i~res 0;6=count res;0b]      // auth complete, res is a 6-element list (1i;src_name;tgt_name;validity_secs_i;context;out_token)
   ;.krb.onAuthSuccess . @[res;0;:;W]
   ;$[2i~res 0;3=count res;0b]      // auth incomplete, res is a 3-element list (2i;context_ptr;b64_encoded_msg)
   ;.krb.stashCtx . @[res;0;:;W]
   ;0b
   ]
 }

.krb.doKrbPwAuth:{[U;P]
  .log.debug("authenticating IPC connetion for user ";U;" with .rkb.doKrb5Auth over password of length ";count P)
 ;first .krb.doKrb5Auth[`ipc;P]
 }

// username/password handler
// U: username -11h; P: password 10h
.krb.zpw:{[U;P]
  $[$[count tbl:select src from .krb.conns where fd = .utl.zw[], dir="I";count first tbl`src;0b]
   ;1b
   ;not P like "YII*"
   ;0b
   ;.krb.doKrbPwAuth[U;P]
   ]
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
 ;.web.authReply:()
 ;.web.cukId:1
 ;.web.cookies:1!flip`data`usr`tgt`validity!enlist each ("";"";"";0Np)
 ;.web.webctx:1!flip`sid`ctx!enlist each ("";"")
 ;.web.http401:(0;"")
 ;.web.http404:.h.hn["404 Not Found";`txt;""]
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

// F: -11h file hsym; K: Set-Cookie header 10h, may be ""
.web.sendContent:{[F;K]
  .log.debug("Serving file ";F)
 ;cnt:"c"$@[1::;F;""]
 ;res:("HTTP/1.1 200 OK"
      ;"Content-Type: ",.h.ty`$last"."vs string F
      ;"Connection: keep-alive" // TODO .h.ka
      ;"Connection-Length: ",string count cnt
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      )
 ;if[count K
    ;res,:enlist "Set-Cookie: ",K
    ]
 ;res,:(""
       ;cnt
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

.web.fileExists:{[F]
  $[-11h~type key fle:`$":",.h.HOME,"/",.web.txFile F
   ;(1b;fle)
   ;0b
   ]
 }

.web.zph:{[T]
  .log.debug("Have GET request for ";T 0)
 ;$[1b~first fle:.web.fileExists T 0
   ;.web.sendContent[fle 1;""]
   ;.web.http404
   ]
 }

.web.rndtok:{
  // please do something better here if you're in anything other than the most trusted environment
  (raze system"od -vAn -N32 -tx8 /dev/urandom") except " "
 }

// C: Client name 10h; T: Target name 10h; V: validity -12h
.web.regClientCookie:{[C;T;V]
  kid:.web.rndtok[]
 ;`.web.cookies upsert (kid;C;T;V)
 ;kid
 }

// P: match pattern 10h; K: cookie text 10h
.web.getCookieValue:{[P;K]
  v:$[not 10h~type tkn:first tks where (tks:";"vs K) like P
   ;tkn
   ;("="vs trim tkn) 1
   ]
 ;.log.debug(-1_1_ P;" cookie-value is ";v)
 ;v
 }

// K: cookie text 10h
.web.cliCookieVal:{[K]
  .web.getCookieValue["*client_id*";K]
 }

// K: cookie text 10h
.web.ctxCookieVal:{[K]
  .web.getCookieValue["*context*";K]
 }

.web.prmAdd:{[R;P]
  qry:$[not"?"in R
       ;"?",P
       ;"="in R
       ;"&",P
       ;P
       ]
 ;R,qry
 }

.web.prmDel:{[R;P]
  $[1=count frg:"?"vs R
   ;R
   ;not count qry:"&"sv tkn where not (tkn:"&"vs"?"sv 1_frg) like\:P,"=*"
   ;frg[0]
   ;frg[0],"?",qry
   ]
 }

// H: header dict; K: token to return 10h (may have count = 0)
.web.sendKrb5Challenge:{[H;K]
  $[count K
   ;.log.debug"Sending WWW-Authenticate continuation"
   ;.log.debug"Sending initial WWW-Authenticate response"
   ]
 ;txt:("HTTP/1.1 401 Unauthorized"
      ;"WWW-Authenticate: Negotiate",$[count K;" ",K;""]
      ;"Content-Length: 0"
      ;"Connection: ",.h.ka 601000i
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;"";"")
 ;.log.debug("Replying 401\n  ";"\n  "sv -2_ txt)
 ;.web.customRep "\r\n"sv txt
 }

.web.sendAuthIdRedirect:{[T]
  `.web.webctx upsert (tkn:.web.rndtok[];"")
 ;qry:.web.prmDel[T] "auth_id"
 ;qry:.web.prmAdd[qry] "auth_id=",tkn
 ;txt:("HTTP/1.1 302 Found"
      ;"Content-Length: 0"
      ;"Location: ",qry
      ;"Connection: ",.h.ka 601000i
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;"";"")
 ;.log.debug("Replying 302\n  ";"\n  "sv -2_ txt)
 ;.web.customRep "\r\n"sv txt
 }

// H: header dict
.web.tryCookieAuth:{[H]
  $[""~cuk:H`Cookie
   ;0b
   ;not 10h~type cuk:.web.cliCookieVal cuk
   ;0b
   ;1<>count tbl:select usr from .web.cookies where data~\:cuk, .utl.zP[] < validity
   ;0b
   ;1b,exec usr from tbl
   ]
 }

.web.ttlFromSecs:{[V]
  .utl.zP[] + 18h$V
 }

// G: GET path 10h; H: header dict; C: client 10h; T: target 10h; V: seconds TTL -6h; X: b64 context 10h; O: b64 output_token? 10h; I: auth_id URL param-value 10h
.web.authComplete:{[G;H;C;T;V;X;O;I]
  // Given the header `WWW-Authenticate: Negotiate oRQwEqADCgEAoQsGCSqGSIb3EgECAg==`, Wireshark
  // decodes it as follows:
  //   GSS-API Generic Security Service Application Program Interface
  //     Simple Protected Negotiation
  //       negTokenTarg
  //         negResult: accept-completed (0)
  //         supportedMech: 1.2.840.113554.1.2.2 (KRB5 - Kerberos 5)
  //
  // In other words: a bit of a no-op. Chromium handles this in a reply properly, but Firefox
  // doesn't like it. This is a stable encoding and we can check for it and avoid sending the
  // response if that's all our context wants to say.
 ;tkn:O except "\r\n"
 ;if[$[not count tkn;1b;"oRQwEqADCgEAoQsGCSqGSIb3EgECAg=="~tkn]
    ;.log.debug("Client authenticated, sending 200 OK for ";G)
    ;$[1b~first fle:.web.fileExists .web.prmDel[G] "auth_id"
      ;:.web.customRep .web.sendContent[fle 1] "client_id=",.web.regClientCookie[C;T] .web.ttlFromSecs V
      ;:.web.customRep .web.http404
      ]
    ]
  // We have a token to send back to the HTTP client. We reply 401 Unauthd with a
  // WWW-Authenticate header which supplies the continuation value, and store the context
  // against the auth_id URL parameter, private to this message exchange.
 ;`.web.webctx upsert (I;X)
 ;.web.sendKrb5Challenge[H;tkn]
 }

.web.authIncomplete:{[H;S;X;K]
  `.web.webctx upsert (S;X)
 ;.web.sendKrb5Challenge[H;K]  
 }

// G: GET path 10h; H: header dict; K: token 10h; S: auth_id URL param value 10h; X: context 10h
.web.sendCtxReply:{[G;H;K;S;X]
  .log.debug("calling .krb.acceptSecCtx on FD ";.utl.zw[])
 ;res:.[.krb.acceptSecCtx;(K;X);.krb.onAcceptSecCtxFail]
 ;.mg.a.GHKSX:(G;H;K;S;X) // `G`H`K`S`X set' .mg.a.GHKSX
 ;.mg.a.res:res           // `res set .mg.a.res
 ;$[1i~first res // auth complete, res is a 6-element list (1i;src;tgt;ttl;ctx;tkn)
   ;.web.authComplete[G;H;res 1;res 2;res 3;res 4;res 5;S]
   ;2i~first res // auth incomplete, res is a 3-element list (2i;ctx;tkn)
   ;.web.authIncomplete[H;S;res 1;res 2]
   ;.web.http401
   ]
 }

// H: header dict; X: auth_id URL param value 10h;
.web.sendInitialChallenge:{[H;X]
  `.web.webctx upsert (X;"")
 ;.web.sendKrb5Challenge[H;""]
 }

.web.hasToken:{[H]
  $[""~tkn:H`Authorization
   ;0b
   ;not tkn like "Negotiate Y*"
   ;0b
   ;1b,enlist trim (count "Negotiate ")_ tkn
   ]
 }

// G: GET path 10h
.web.hasAuthParam:{[G]
  $[2<>count prm:"?"vs G
   ;0b
   ;not count ss[prm:last prm;"auth_id"]
   ;0b
   ;not count prm:last "="vs first prm where (prm:"&"vs prm)like\:"auth_id*"
   ;0b
   ;not 10 10h~type each tup:exec last each (sid;ctx) from .web.webctx where sid~\:prm
   ;0b
   ;1b,tup
   ]
 }

// T is a two-element tuple, of request text and headers; the latter is a dict. The
// function must return a two-element list, whose first element is in the set {0, 1, 2, 4}.
// 0j generates a 401 not-authorized response.
// 1j authorises the user, whose username should be the second element (as char-vec).
// 2j sends the second element as the response text, while
// 4j (whose second element is the empty char-vec), falls back to Basic authentication.
//
// C.f. https://code.kx.com/q/ref/dotz/#zac-http-auth
.web.zac:{[T]
  .log.debug("HTTP auth required for FD ";.utl.zw[])
 ;.web.reqs,:enlist T // TODO remove me
 ;qry:T 0
 ;hdr:T 1
 ;rtn:$[1b~first usr:.web.tryCookieAuth hdr
       ;.web.userOk usr 1
       ;0b~first ctx:.web.hasAuthParam qry
       ;.web.sendAuthIdRedirect qry
       ;1b~first tkn:.web.hasToken hdr
       ;.web.sendCtxReply[qry;hdr;tkn 1;ctx 1;ctx 2]
       ;.web.sendInitialChallenge[hdr;ctx 1]
       ]
 ;.log.debug(".z.ac result is:\n";.Q.s rtn)
 ;rtn
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

