#include <stdio.h>  // printf, stderr
#include <string.h> // strlen
#include <stdlib.h> // EXIT_FAILURE
#include <stdint.h> // int64_t, uintptr_t

#include <gssapi.h>
#include <gssapi/gssapi_krb5.h> // GSS_KRB5_NT_PRINCIPAL_NAME

#define KXVER 3
#include "k.h"

//#include <sys/types.h>
//#include <ctype.h>
//#include <unistd.h>

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <sys/uio.h>
//#include <sys/time.h>

// From https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

unsigned char * base64_encode(const unsigned char *src, size_t len, size_t *out_len);
unsigned char * base64_decode(const unsigned char *src, size_t len, size_t *out_len);

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
unsigned char * base64_encode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	size_t olen;
	int line_len;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len)
		return NULL; /* integer overflow */
	out = malloc(olen);
	if (out == NULL)
		return NULL;

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) |
					      (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
	if (out_len)
		*out_len = pos - out;
	return out;
}


/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char* base64_decode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}



// See https://docs.oracle.com/cd/E19455-01/806-3814/806-3814.pdf, although 2000 they're better than most


static gss_cred_id_t _cred_handle_g;

static
void mg_print_gss_error(OM_uint32 major_code, OM_uint32 minor_code, const gss_OID mech_typ)
{
	const char *err_name = "No mapping";
	if (GSS_SUPPLEMENTARY_INFO(major_code)) {
		switch(major_code) {
			case GSS_S_CONTINUE_NEEDED: err_name = "GSS_S_CONTINUE_NEEDED"; break;
			case GSS_S_DUPLICATE_TOKEN: err_name = "GSS_S_DUPLICATE_TOKEN"; break;
			case GSS_S_OLD_TOKEN: err_name = "GSS_S_OLD_TOKEN"; break;
			case GSS_S_UNSEQ_TOKEN: err_name = "GSS_S_UNSEQ_TOKEN"; break;
			case GSS_S_GAP_TOKEN: err_name = "GSS_S_GAP_TOKEN"; break;
		}
		fprintf(stderr, "ERROR: routine error: %s\n", err_name);
	}
	else if (GSS_ERROR(major_code)) {
		if (GSS_CALLING_ERROR(major_code)) {
			switch(major_code) {
				case GSS_S_CALL_INACCESSIBLE_READ: err_name = "GSS_S_CALL_INACCESSIBLE_READ"; break;
				case GSS_S_CALL_INACCESSIBLE_WRITE: err_name = "GSS_S_CALL_INACCESSIBLE_WRITE"; break;
				case GSS_S_CALL_BAD_STRUCTURE: err_name = "GSS_S_CALL_BAD_STRUCTURE"; break;
			}
			fprintf(stderr, "ERROR: calling error: %s\n", err_name);
		}
		if (GSS_ROUTINE_ERROR(major_code)) {
			switch(major_code) {
				case GSS_S_BAD_MECH: err_name = "GSS_S_BAD_MECH"; break;
				case GSS_S_BAD_NAME: err_name = "GSS_S_BAD_NAME"; break;
				case GSS_S_BAD_NAMETYPE: err_name = "GSS_S_BAD_NAMETYPE"; break;
				case GSS_S_BAD_BINDINGS: err_name = "GSS_S_BAD_BINDINGS"; break;
				case GSS_S_BAD_STATUS: err_name = "GSS_S_BAD_STATUS"; break;
				case GSS_S_BAD_MIC: err_name = "GSS_S_BAD_MIC"; break;
				case GSS_S_NO_CRED: err_name = "GSS_S_NO_CRED"; break;
				case GSS_S_NO_CONTEXT: err_name = "GSS_S_NO_CONTEXT"; break;
				case GSS_S_DEFECTIVE_TOKEN: err_name = "GSS_S_DEFECTIVE_TOKEN"; break;
				case GSS_S_DEFECTIVE_CREDENTIAL: err_name = "GSS_S_DEFECTIVE_CREDENTIAL"; break;
				case GSS_S_CREDENTIALS_EXPIRED: err_name = "GSS_S_CREDENTIALS_EXPIRED"; break;
				case GSS_S_CONTEXT_EXPIRED: err_name = "GSS_S_CONTEXT_EXPIRED"; break;
				case GSS_S_FAILURE: err_name = "GSS_S_FAILURE"; break;
				case GSS_S_BAD_QOP: err_name = "GSS_S_BAD_QOP"; break;
				case GSS_S_UNAUTHORIZED: err_name = "GSS_S_UNAUTHORIZED"; break;
				case GSS_S_UNAVAILABLE: err_name = "GSS_S_UNAVAILABLE"; break;
				case GSS_S_DUPLICATE_ELEMENT: err_name = "GSS_S_DUPLICATE_ELEMENT"; break;
				case GSS_S_NAME_NOT_MN: err_name = "GSS_S_NAME_NOT_MN"; break;
			}
			fprintf(stderr, "ERROR: routine error: %s\n", err_name);
		}
		// C.f. https://man.freebsd.org/cgi/man.cgi?query=gss_display_status&sektion=3&manpath=FreeBSD+7.1-RELEASE
		OM_uint32 err_status;
		OM_uint32 message_context = 0;
		gss_buffer_desc major_string;
		gss_buffer_desc minor_string;
		OM_uint32 err;

		do {
			err = gss_display_status(
									&err_status
								, minor_code
								, GSS_C_GSS_CODE
								, GSS_C_NO_OID
								, &message_context
								, &major_string
								);
			if (GSS_S_COMPLETE != err) {
				fprintf(stderr, "ERROR: while calling gss_display_status(,,GSS_C_GSS_CODE,,,): could not resolve error code\n");
				return;
			}
			err = gss_display_status(
									&err_status
								, minor_code
								, GSS_C_MECH_CODE
								, mech_typ
								, &message_context
								, &minor_string
								);
			if (GSS_S_BAD_MECH == err) {
				fprintf(stderr, "ERROR: while calling gss_display_status(,,GSS_C_MECH_CODE,,,): bad mech-type\n");
			}
			else if (GSS_S_BAD_STATUS == err) {
				fprintf(stderr, "ERROR: while calling gss_display_status(,,GSS_C_MECH_CODE,,,): bad minor-status\n");
			}
			fprintf(stderr, "ERROR: %.*s, %.*s\n", (int)major_string.length, (char*)major_string.value, (int)minor_string.length, (char*)minor_string.value);
			
			gss_release_buffer(&err_status, &major_string);
			gss_release_buffer(&err_status, &minor_string);
		}
		while (0 != message_context);
	}
}

// Helper function that calls the gss_indicate_mechs function, and prints out
// the results' textual representation. You'll want to ensure you see the 
// Kerberos OID { 1 2 840 113554 1 2 2 }.
__attribute__((unused))
static
void mg_print_gss_mechs(void)
{
	OM_uint32 res;
	OM_uint32 minor_status;

	gss_OID_set mech_set = GSS_C_NO_OID_SET;

	res = gss_indicate_mechs(&minor_status, &mech_set);
	if (GSS_S_COMPLETE != res) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
	}
	else {
		gss_buffer_desc buf = {0};
		for (size_t i = 0 ; i < mech_set->count ; i++) {
			// https://github.com/krb5/krb5/blob/master/src/lib/gssapi/generic/oid_ops.c#L237
			res = gss_oid_to_str(&minor_status, &mech_set->elements[i], &buf);
			if (GSS_S_COMPLETE != res) {
				mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
				fprintf(stderr, "ERROR: while converting OID to string\n");
			}
			else {
				fprintf(stdout, " INFO: have mech %.*s\n", (int)buf.length, (char*)buf.value);
			}
		}
		res = gss_release_buffer(&minor_status, &buf);
		if (GSS_S_COMPLETE != res) {
			fprintf(stderr, "ERROR: in gss_release_buffer at %i\n", __LINE__);
		}
	}
	res = gss_release_oid_set(&minor_status, &mech_set);
	if (GSS_S_COMPLETE != res) {
		fprintf(stderr, "ERROR: failed in gss_release_oid_set\n");
	}

}


__attribute__((unused))
static
OM_uint32 mg_acquire_cred(const gss_name_t name, gss_cred_id_t output_cred_handle, OM_uint32 *time_rec)
{
	OM_uint32 res;
	OM_uint32 minor_status;

	res = gss_acquire_cred(
	           &minor_status
	         , name
	         , GSS_C_INDEFINITE      // time_req; how long we want this to last for
	         , gss_mech_set_krb5     // desired_mechs; can supply GSS_C_NO_OID_SET but it seems to access the keytab twice
	         , GSS_C_ACCEPT          // gss_cred_usage_t (C.f. GSS_C_INITIATE and GSS_C_BOTH)
	         , &output_cred_handle
	         , NULL                  // actual mechs; we don't need to know so set this to NULL
	         , time_rec              // number of seconds for which this credential is valid
	         );
	
	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, gss_mech_krb5);
	}
	else {
		fprintf(stdout, "DEBUG: gss_acquire_creds complete\n");
	}

	return res;
}

static
OM_uint32 mg_accept_ctx(gss_buffer_t input_token, gss_ctx_id_t *context_handle)
{
	OM_uint32 minor_status;
	OM_uint32 res;

	OM_uint32       ret_flags      = 0;
	OM_uint32       time_rec       = 0;
	gss_buffer_desc output_token   = {0};
	gss_name_t      src_name       = NULL;
	gss_OID         mech_typ       = {0};

	*context_handle = GSS_C_NO_CONTEXT;

	fprintf(stdout, "DEBUG: token byte-vec has length %zd and has bookendz ", input_token->length);
	for (size_t i = 0 ; i < 5 ; i++) {
		fprintf(stdout, "%02x", 0xFF & ((char*)input_token->value)[i]);
	}
	fprintf(stdout, "..");
	for (size_t i = input_token->length - 10 ; i < input_token->length ; i++) {
		fprintf(stdout, "%02x", 0xFF & ((char*)input_token->value)[i]);
	}
	fprintf(stdout, "\n");

	// The simplest choice is to pass GSS_C_NO_CREDENTIAL as the acceptor
	// credential. In this case, clients may authenticate to any service
	// principal in the default keytab (typically FILE:/etc/krb5.keytab, or the
	// value of the KRB5_KTNAME environment variable). This is the recommended
	// approach if the server application has no specific requirements to the
	// contrary.

	// NB if res is GSS_S_CONTINUE_NEEDED we need to invoke again
	res = gss_accept_sec_context(
	         &minor_status
	       , context_handle            // set above to GSS_C_NO_CONTEXT
	       , GSS_C_NO_CREDENTIAL       // _cred_handle_g, initialised by gss_acquire_cred?
	       , input_token
	       , GSS_C_NO_CHANNEL_BINDINGS // the null value for gss_channel_bindings_t
	       , &src_name                 // name of the initiating principal
	       , &mech_typ                 // security-mechanism to use; set NULL if we don't care
	       , &output_token             //
	       , &ret_flags                // bitwise collection indicating e.g. GSS_C_INTEG_FLAG
	       , &time_rec                 // number of seconds this will remain vaild
	       , NULL                      // gss_cred_id_t delegated credentials handle
	       );

	if (GSS_SUPPLEMENTARY_INFO(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		if (GSS_S_CONTINUE_NEEDED == res) {
			// TODO, output_token.length will be > 0
		}
	}

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		mg_print_gss_error(res, minor_status, gss_mech_krb5);
	}
	else {
		fprintf(stdout, " INFO: gss_accept_sec_context success; context is valid for %u seconds\n", time_rec);

		if (NULL != src_name) {
			OM_uint32 tmp_status;
			gss_buffer_desc name_buf = {0};
			gss_OID name_typ = NULL;
			OM_uint32 err = gss_display_name(&tmp_status, src_name, &name_buf, &name_typ);
			if (GSS_ERROR(err)) {
				fprintf(stderr, "ERROR: failed in gss_display_name while retrieving Client name\n");
				mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
			}
			else {
				fprintf(stdout, " INFO: Client is %.*s\n", (int)name_buf.length, (char*)name_buf.value);
			}
		}
		else {
			fprintf(stdout, " WARN: no source name available from Client token\n");
		}
		if (0 != ret_flags) {
#define CHK_RET_FLAGS(_VAL, _FLAG) if ((_VAL) & (_FLAG)) fprintf(stdout, "DEBUG: token includes the " #_FLAG " flag\n");
			CHK_RET_FLAGS(ret_flags, GSS_C_DELEG_FLAG);         // the initiator's credentials may be delegated
			CHK_RET_FLAGS(ret_flags, GSS_C_MUTUAL_FLAG);        // mutual authentication is available
			CHK_RET_FLAGS(ret_flags, GSS_C_REPLAY_FLAG);        // detection of repeated messages is available
			CHK_RET_FLAGS(ret_flags, GSS_C_SEQUENCE_FLAG);      // detection of out-of-sequence messages is available
			CHK_RET_FLAGS(ret_flags, GSS_C_CONF_FLAG);          // messages can be encrypted (confidentiality)
			CHK_RET_FLAGS(ret_flags, GSS_C_INTEG_FLAG);         // messages can be stamped with a MIC to ensure their validity
			CHK_RET_FLAGS(ret_flags, GSS_C_ANON_FLAG);          // the context initiator will be anonymous
			CHK_RET_FLAGS(ret_flags, GSS_C_PROT_READY_FLAG);    // indicates that validation indicated by the CONF and INTEG flags is now possible. We can now call gss_unwrap() or gss_verify_mic(); if false, data-reception may be incomplete
			CHK_RET_FLAGS(ret_flags, GSS_C_TRANS_FLAG);         // the context can be exported
			CHK_RET_FLAGS(ret_flags, GSS_C_DELEG_POLICY_FLAG);  // delegation is permissible only if permitted by KDC policy
#undef CHK_RET_FLAGS

			if (res & GSS_C_PROT_READY_FLAG) {
				if (res & (GSS_C_CONF_FLAG)) {
					OM_uint32 unwrap_status;
					int conf_state;
					gss_qop_t qop_state;
					OM_uint32 unwrap_err = gss_unwrap(&unwrap_status, *context_handle, input_token, &output_token, &conf_state, &qop_state);
					if (GSS_ERROR(unwrap_err)) {
						mg_print_gss_error(unwrap_err, unwrap_status, GSS_C_NULL_OID);
					}
					else {
						fprintf(stdout, "DEBUG: gss_unwrap was successful\n");
					}
				}
				else if (res & GSS_C_INTEG_FLAG) {
					// TODO gss_verify_mic
				}
			}
			else {
				fprintf(stdout, "DEBUG: PROT_READY not set, not validating\n");
			}
		}
	}
	return res;
}

K mg_print_mechs(K dummy)
{
	mg_print_gss_mechs();
	return kb(1);
}

//K mg_krb5_auth(K b64_tkn)
K mg_krb5_auth(K b64_tkn)
{
	// if (KG != b64_tkn->t) {
	if (KC != b64_tkn->t) {
		//return krr("expect token as byte-vector");
		return krr("expect token as char-vector");
	}

	OM_uint32 res;
	gss_ctx_id_t context_handle;

	size_t out_len = 0;
	unsigned char *b64 = base64_decode(b64_tkn->G0, b64_tkn->n, &out_len);
	if (NULL == b64) {
		return krr("ERROR: failed in base64_decode\n");
	}

	gss_buffer_desc input_token = {
		.length = out_len,
		.value = b64,
	};

	res = mg_accept_ctx(&input_token, &context_handle);

	free(b64);

	if (GSS_ERROR(res)) {
		return krr("failed to authenticate token");
	}

	return kb(1);
}


__attribute__((unused))
static
K mg_krb5_init(K principal)
{
	if (KC != principal->t) {
		return krr("expected type 10h for principal");
	}

	OM_uint32 res, minor_status;
	gss_name_t output_name;
	gss_buffer_desc input_name = {
		.length = (size_t)principal->n,
		.value = (void*)principal->G0,
	};

	fprintf(stdout, "DEBUG: resolving principal %.*s\n", (int)input_name.length, (char*)input_name.value);
	
	res = gss_import_name(
	           &minor_status
	         , &input_name
	         , GSS_KRB5_NT_PRINCIPAL_NAME
	         , &output_name
	         );

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		return krr("failed to import principal OID name");
	}

	// OM_uint32 time_rec = 0;

	res = gss_acquire_cred(
	           &minor_status
	         , output_name
	         , GSS_C_INDEFINITE      // time_req; how long we want this to last for
	         , GSS_C_NULL_OID_SET    // gss_mech_set_krb5     // desired_mechs; can supply GSS_C_NO_OID_SET but it seems to access the keytab twice
	         , GSS_C_ACCEPT          // gss_cred_usage_t (C.f. GSS_C_INITIATE and GSS_C_BOTH)
	         , &_cred_handle_g
	         , NULL                  // actual mechs; we don't need to know so set this to NULL
	         , NULL                  // number of seconds for which this credential is valid
	         );
	if (GSS_SUPPLEMENTARY_INFO(res)) {
		fprintf(stdout, " INFO: extra info for gss_acquire_cred: \n");
		mg_print_gss_error(res, minor_status, 0);
	}
	
	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, gss_mech_krb5);
		return krr("failed to acquire credentials");
	}

	fprintf(stdout, "DEBUG: gss_acquire_creds complete\n");

	return kb(1);
	
}

K mg_accept_sec_context(K b64_tkn, K ctx_ptr)
{
	if (KC != b64_tkn->t) {
		return krr("expect token as char-vector");
	}
	if (-KJ != ctx_ptr->t) {
		fprintf(stderr, "ERROR: arg ctx_ptr has type %hhi\n", ctx_ptr->t);
		return krr("expect context pointer as long-atom");
	}

	size_t out_len = 0;
	unsigned char *b64 = base64_decode(b64_tkn->G0, b64_tkn->n, &out_len);
	if (NULL == b64 || 0 == out_len) {
		return krr("ERROR: failed in base64_decode\n");
	}

	gss_buffer_desc input_token = {
		.length = out_len,
		.value = b64,
	};

	OM_uint32 res, minor_status;

	if ((J)nj == ctx_ptr->j) {
		ctx_ptr->j = (J)(uintptr_t)GSS_C_NO_CONTEXT;
	}
	else {
		fprintf(stdout, "DEBUG: Using ctx_ptr->j %llx\n", ctx_ptr->j);
	}

	gss_ctx_id_t    context_handle = (void*)(uintptr_t)ctx_ptr->j;
	OM_uint32       ret_flags      = 0;
	OM_uint32       time_rec       = 0;
	gss_buffer_desc output_token   = {0};
	gss_name_t      src_name       = NULL;
	gss_OID         mech_typ       = {0};

	// The recommended choice is to pass GSS_C_NO_CREDENTIAL as the acceptor credential. In this case,
	// clients may authenticate to _any_ service principal in the default keytab or the value of the
	// KRB5_KTNAME environment variable.

	// NB if res is GSS_S_CONTINUE_NEEDED we need to invoke again
	res = gss_accept_sec_context(
	         &minor_status
	       , &context_handle           // set above to GSS_C_NO_CONTEXT
	       , GSS_C_NO_CREDENTIAL       // _cred_handle_g, initialised by gss_acquire_cred?
	       , &input_token
	       , GSS_C_NO_CHANNEL_BINDINGS // the null value for gss_channel_bindings_t
	       , &src_name                 // name of the initiating principal
	       , &mech_typ                 // security-mechanism to use; set NULL if we don't care
	       , &output_token             //
	       , &ret_flags                // bitwise collection indicating e.g. GSS_C_INTEG_FLAG
	       , &time_rec                 // number of seconds the context will remain vaild
	       , NULL                      // gss_cred_id_t delegated credentials handle
	       );

	if (GSS_SUPPLEMENTARY_INFO(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		if (GSS_S_CONTINUE_NEEDED == res) {
			if (0 == output_token.length) {
				return krr("GSSAPI signals CONTINUE_NEEDED but output_token.length is zero");
			}
			size_t b64_len = 0;
			unsigned char *b64 = base64_encode((unsigned char*)output_token.value, output_token.length, &b64_len);
			if (NULL == b64) {
				fprintf(stderr, "ERROR: failed in base64_encode\n");
				return krr("GSSAPI signals CONTINUE_NEEDED but base64_encode failed");
			}
			K out_tkn = kpn((S)b64, b64_len);
			r0(ctx_ptr);
			ctx_ptr = kj((J)(uintptr_t)context_handle);
			K vals = knk(3, ki(2), ctx_ptr, out_tkn);
			return vals;
		}
	}

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, gss_mech_krb5);
		return krr("failed to decode Client token");
	}

	K k_src_name = (K)NULL;

	fprintf(stdout, "TRACE: gss_accept_sec_context success; context is valid for %u seconds\n", time_rec);

	{
		gss_buffer_desc buf = {0};
		// https://github.com/krb5/krb5/blob/master/src/lib/gssapi/generic/oid_ops.c#L237
		OM_uint32 oid_res = gss_oid_to_str(&minor_status, mech_typ, &buf);
		if (GSS_ERROR(oid_res)) {
			mg_print_gss_error(oid_res, minor_status, GSS_C_NULL_OID);
		}
		else {
			// Will be kerberos "{ 1 2 840 113554 1 2 2 }"
			// fprintf(stdout, "DEBUG: mech-type is %.*s\n", (int)buf.length, (char*)buf.value);
		}
	}

	if (NULL != src_name) {
		OM_uint32 tmp_status;
		gss_buffer_desc name_buf = {0};
		gss_OID name_typ = NULL;
		OM_uint32 err = gss_display_name(&tmp_status, src_name, &name_buf, &name_typ);
		if (GSS_ERROR(err)) {
			fprintf(stderr, "ERROR: failed in gss_display_name while retrieving Client name\n");
			mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		}
		else {
			fprintf(stdout, "DEBUG: Client is %.*s\n", (int)name_buf.length, (char*)name_buf.value);
			k_src_name = kpn((S)name_buf.value, (J)name_buf.length);
		}
	}
	else {
		fprintf(stdout, " WARN: no source name available from Client token\n");
	}

	if (0 != ret_flags) {
#define CHK_RET_FLAGS(_VAL, _FLAG) if ((_VAL) & (_FLAG)) fprintf(stdout, "DEBUG: token includes the " #_FLAG " flag\n");
		CHK_RET_FLAGS(ret_flags, GSS_C_DELEG_FLAG);         // the initiator's credentials may be delegated
		CHK_RET_FLAGS(ret_flags, GSS_C_MUTUAL_FLAG);        // mutual authentication is available
		CHK_RET_FLAGS(ret_flags, GSS_C_REPLAY_FLAG);        // detection of repeated messages is available
		CHK_RET_FLAGS(ret_flags, GSS_C_SEQUENCE_FLAG);      // detection of out-of-sequence messages is available
		CHK_RET_FLAGS(ret_flags, GSS_C_CONF_FLAG);          // messages can be encrypted (confidentiality)
		CHK_RET_FLAGS(ret_flags, GSS_C_INTEG_FLAG);         // messages can be stamped with a MIC to ensure their validity
		CHK_RET_FLAGS(ret_flags, GSS_C_ANON_FLAG);          // the context initiator will be anonymous
		CHK_RET_FLAGS(ret_flags, GSS_C_PROT_READY_FLAG);    // indicates that validation indicated by the CONF and INTEG flags is now possible. We can now call gss_unwrap() or gss_verify_mic(); if false, data-reception may be incomplete
		CHK_RET_FLAGS(ret_flags, GSS_C_TRANS_FLAG);         // the context can be exported
		CHK_RET_FLAGS(ret_flags, GSS_C_DELEG_POLICY_FLAG);  // delegation is permissible only if permitted by KDC policy
#undef CHK_RET_FLAGS

		if (res & GSS_C_PROT_READY_FLAG) {
			if (res & (GSS_C_CONF_FLAG)) {
				OM_uint32 unwrap_status;
				int conf_state;
				gss_qop_t qop_state;
				OM_uint32 unwrap_err = gss_unwrap(&unwrap_status, context_handle, &input_token, &output_token, &conf_state, &qop_state);
				if (GSS_ERROR(unwrap_err)) {
					mg_print_gss_error(unwrap_err, unwrap_status, GSS_C_NULL_OID);
				}
				else {
					fprintf(stdout, "DEBUG: gss_unwrap was successful\n");
				}
			}
			else if (res & GSS_C_INTEG_FLAG) {
				// TODO gss_verify_mic
			}
		}
		else {
			fprintf(stdout, "DEBUG: PROT_READY not set, not validating\n");
		}
	}

	K k_tgt_name = ktn(KC, 0);
	{
		gss_name_t tgt_name;
		OM_uint32 lifetime_rec;

		res = gss_inquire_context(
		            &minor_status
		          , context_handle
		          , &src_name
		          , &tgt_name
		          , &lifetime_rec
		          , NULL
		          , NULL
		          , NULL
		          , NULL);
		if (GSS_ERROR(res)) {
			mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		}
		else {
			OM_uint32 tmp_status;
			gss_buffer_desc name_buf = {0};
			gss_OID name_typ = NULL;
			OM_uint32 err = gss_display_name(&tmp_status, tgt_name, &name_buf, &name_typ);
			if (GSS_ERROR(err)) {
				fprintf(stderr, " WARN: failed while decoding target-name from gss_inquire_context\n");
				mg_print_gss_error(err, tmp_status, GSS_C_NULL_OID);
			}
			else {
				fprintf(stdout, "DEBUG: Target principal is %.*s\n", (int)name_buf.length, (char*)name_buf.value);
				r0(k_tgt_name);
				k_tgt_name = kpn((S)name_buf.value, (J)name_buf.length);
			}
		}
	}
	// Release the incoming context value
	r0(ctx_ptr);

	ctx_ptr = kj((J)(uintptr_t)context_handle);
	fprintf(stdout, "DEBUG: comprare ctx_ptr->j %llx, context_handle %p\n", ctx_ptr->j, (void*)context_handle);
	// NB the validity of the context will be bounded by the validity of the applicant's ticket
	K validity = ki((I)time_rec);

	K vals = knk(5, ki(1), k_src_name, k_tgt_name, ctx_ptr, validity);
	return vals;
}
