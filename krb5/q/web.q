
.web.init:{
  system"l q/krb5.q"
 ;.krb.init[]
 ;.h.HOME:getenv[`PWD],"/html"
 ;.z.ac:.web.ac
 ;.web.reqs:()
 ;.web.http401:(0;"")
 ;1b
 }

.web.userOk:{[U] (1;U) }

.web.customRps:{[M] (2;M) }

// H: header-dict
.web.chkCookie:{[H]
  $[$[10h~type cuk:H`Cookie;0<count cuk;0b]
   ;"MICHAEL!"
   ;0b
   ]
 }

// C.f. https://www.rfc-editor.org/rfc/rfc9110.html#http.date
// E.g. Sun, 06 Nov 1994 08:49:37 GMT
.web.fmtHttpDate:{
  first system "TZ=GMT date '+%a, %d %b %Y %H:%M:%S %Z'"
 }

// T is the two-element tuple passed to .web.ac (.z.ac) of request-text and header-dict.
// https://moi.vonos.net/programming/kerberos/
// https://docs.pingidentity.com/solution-guides/standards_and_protocols_use_cases/htg_config_browsers_for_kerberos_and_ntlm.html#configuring-mozilla-firefox-for-kerberos-and-ntlm
.web.genKrb5Challenge:{[B]
  txt:("HTTP/1.1 401 Unauthorized"
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
// C.f. https://code.kx.com/q/ref/dotz/#zac-http-auth
.web.ac:{[T]
  -1 "DEBUG: connection from ",string .utl.zw[]
 ;txt:T 0
 ;hdr:T 1
 ;.web.reqs,:enlist T
 ;0N!$[10h~type usr:.web.chkCookie hdr
      ;.web.userOk usr
      ;$[10h~type usr:.krb.getUserForConn .utl.zw[];count usr;0b]
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
 }

.web.init[];
