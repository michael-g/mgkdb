
.krb.init:{
	 dso:`:/home/michaelg/dev/projects/github.com/mgkdb/krb5/lib/libmgkrb5
 ;.krb.acceptSecCtx:dso 2: (`mg_accept_sec_context;2)
 ;.krb.conns:1!flip`fd`ctx`src`tgt`xpy!"IJ**P"$\:()
 ;.utl.zw:{.z.w}
 }

.krb.getUserForConn:{[H]
	exec first src from .krb.conns where fd = H
 }

// X: GSS context pointer -7h; S: src-name 10h; T: tgt-name 10h; V: seconds TTL -6h
.krb.addUserForConn:{[X;S;T;V]
	cfd:.utl.zw[]
 ;xpy:.z.P + 18h$V
 ;`.krb.conns upsert (cfd;X;S;T;xpy)
 ;(1b;S)
 }

.krb.stashCtx:{[X;M]
	`.krb.conns upsert (cfd;X;"";"";0Np)
 ;(0b;M)
 }
			// RFC 4559 s.5, https://www.rfc-editor.org/rfc/rfc4559.html:
			// If the context is not complete, the server will respond with a 401 status code with
			// a WWW-Authenticate header containing the gssapi-data.
			//   S: HTTP/1.1 401 Unauthorized
			//   S: WWW-Authenticate: Negotiate 749efa7b23409c20b92356
			// The context will need to be preserved between client messages.
.krb.onAcceptSecCtxFail:{[E]
  -1 "ERROR: krb5 auth failed: ",E
 ;0b
 }

.krb.doKrb5Auth:{[T]
	// TODO lookoup any existing context-pointer and pass to gss_accept_sec_context
 ;ctx:exec first ctx from .krb.conns where fd = .utl.zw[]
 ;res:.[.krb.acceptSecCtx;(trim T;ctx);.krb.onAcceptSecCtxFail]
 ;-1 "Authentication response is ",.Q.s1 res
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
