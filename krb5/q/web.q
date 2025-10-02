.web.init:{
  .h.HOME:getenv[`PWD],"/html"
 ;.z.ph:.web.zph
 ;.z.wo:.web.zwo
 ;.z.wc:.web.zwc
 ;.z.ws:.web.zws
 ;if[$[10h~type arg:first(.Q.opt .z.x)`web.server;"B"$arg;0b]
    ;.z.ac:.web.zac
    ]
 ;.web.reqs:()
 ;.web.authReply:()
 ;.web.cukId:1
 ;.web.cookies:1!flip`data`usr`tgt`validity!enlist each ("";"";"";0Np)
 ;.web.webctx:1!flip`sid`ctx!enlist each ("";::)
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
      ;"Connection: keep-alive"
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

// H: header dict; K: token to return 10h or ""; C: cookie 10h or ""
.web.sendKrb5Challenge:{[H;K;C]
  $[count K
   ;.log.debug"sending WWW-Authenticate continuation"
   ;.log.debug"sending initial WWW-Authenticate response"
   ]
 ;txt:$[count C
       ;("Set-Cookie: ",C;"";"")
       ;("";"")
       ]
 ;txt:("HTTP/1.1 401 Unauthorized"
      ;"WWW-Authenticate: Negotiate",$[count K;" ",K;""]
      ;"Content-Length: 0"
      ;"Connection: close"
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ),txt
 ;.web.customRep "\r\n"sv txt
 }

.web.sendAuthIdRedirect:{[T]
  .log.debug"sending auth_id redirect"
 ;`.web.webctx upsert (tkn:.web.rndtok[];::)
 ;qry:.web.prmDel[T] "auth_id"
 ;qry:.web.prmAdd[qry] "auth_id=",tkn
 ;txt:("HTTP/1.1 302 Found"
      ;"Content-Length: 0"
      ;"Location: ",qry
      ;"Connection: close"
      ;"Date: ",.web.fmtHttpDate[]
      ;"Server: ",first system"hostname"
      ;"";"")
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

// G: GET path 10h; H: header dict; S: auth_id URL param-value 10h; C: client 10h; T: target 10h; V: seconds TTL -6h; X: context (::); O: b64 output_token? 10h
.web.authComplete:{[G;H;S;C;T;V;X;O]
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
 ;.log.debug("client authenticated with URL param ";$[20>count S;(20#S),"..";S])
 ;tkn:O except "\r\n"
 ;ook:"client_id=",.web.regClientCookie[C;T] .web.ttlFromSecs V
 ;delete from `.web.webctx where sid~\:S
 ;$[$[count tkn;not"oRQwEqADCgEAoQsGCSqGSIb3EgECAg=="~tkn;1b]  //   if (token.length > 0 && token is not "accept-complete")
   ;0b                                                         // then skip
   ;1b~first fle:.web.fileExists .web.prmDel[G] "auth_id"      // elif the file requested exists
   ;:.web.customRep .web.sendContent[fle 1;ook]                // then servie the file, passing the cookie value
   ;:.web.customRep .web.http404                               // else send 404 Not Found
   ]
  // At this point, we have a material token to send back to the HTTP client. We reply 401 Unauthd
  // with a WWW-Authenticate header which supplies the continuation value, and store the context
  // against the auth_id URL parameter. 
 ;.web.sendKrb5Challenge[H;tkn;ook]
 }

// G: GET path 10h; H: header dict; S: auth_id URL param-value 10h; C: client 10h; T: target 10h; V: seconds TTL -6h; X: context 4h; O: b64 output_token? 10h
.web.authIncomplete:{[G;H;S;C;T;V;X;O]
  `.web.webctx upsert (S;X)
 ;tkn:O except "\r\n"
 ;.web.customRep .web.sendKrb5Challenge[H;tkn;""]
 }

// G: GET path 10h; H: header dict; K: token 10h; I: auth_id URL param value 10h; R: accept-response 0h
.web.handleAcceptResponse:{[G;H;K;I;R]
  // knk(6, [0] ki(workflow), [1] k_src_name, [2] k_tgt_name, [3] ki((I)time_rec), [4] k_ctx, [5] k_out_tkn);
  $[1i~R 0
   ;.web.authComplete[G;H;I;R 1;R 2;R 3;R 4; R 5]
   ;2i~R 0
   ;.web.authIncomplete[G;H;I;R 1;R 2;R 3;R 4; R 5]
   ;.web.http401
   ]
 }

// G: GET path 10h; H: header dict; K: token 10h; I: auth_id URL param value 10h
.web.doInitialAccept:{[G;H;K;I]
  .log.debug("calling .krb.acceptSecCtx on FD ";.utl.zw[])
 ;res:@[.krb.acceptSecCtx;K;.krb.onAcceptSecCtxFail]
 ;.web.handleAcceptResponse[G;H;K;I;res]
 }

// G: GET path 10h; H: header dict; K: token 10h; I: auth_id URL param value 10h; X: context 4h
.web.continueAccept:{[G;H;K;I;X]
  .log.debug("calling .krb.acceptSecCtxCont on FD ";.utl.zw[];"; token begins ";$[50<count K;(50#K),"..";K])
 ;res:.[.krb.acceptSecCtxCont;(X;K);.krb.onAcceptSecCtxFail]
 ;.web.handleAcceptResponse[G;H;K;I;res]
 }

// G: GET path 10h; H: header dict; K: token 10h; I: auth_id URL param value 10h
.web.sendSubsqChallenge:{[G;H;K;I]
 .log.debug"sending further WWW-Authenticate challenge"
 ;$[0=count tbl:select ctx from .web.webctx where 0<count each sid, sid~\:I
   ;.web.doInitialAccept[G;H;K;I]
   ;1=count tbl
   ;.web.continueAccept[G;H;K;I;first first tbl`ctx]
   ;.web.http401
   ]
 }

// H: header dict; I: auth_id URL param value 10h;
.web.sendInitialChallenge:{[H;I]
  `.web.webctx upsert (I;::)
 ;.web.sendKrb5Challenge[H;"";""]
 }

// H: header dict; I: auth_id URL param value 10h;
.web.sendInitialChallenge:{[H;I]
  `.web.webctx upsert (I;::)
 ;.web.sendKrb5Challenge[H;"";""]
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
   ;1<>exec count sid from .web.webctx where 0<count each sid, sid~\:prm
   ;0b
   ;(1b;prm)
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
       ;0b~first rid:.web.hasAuthParam qry
       ;.web.sendAuthIdRedirect qry
       ;1b~first tkn:.web.hasToken hdr
       ;.web.sendSubsqChallenge[qry;hdr;tkn 1;rid 1]
       ;.web.sendInitialChallenge[hdr;rid 1]
       ]
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

