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
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
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

static
K mg_void_ptr_to_byte_vec(void *ptr)
{
	union {
		void *pointer;
		int8_t ary[sizeof(void*)];
	} convert;

	convert.pointer = ptr;
	K vec = ktn(KG, 8);
	memcpy(vec->G0, convert.ary, sizeof(void*));
	return vec;
}

static
void* mg_byte_vec_to_void_ptr(K vec)
{
	union {
		void *pointer;
		int8_t ary[sizeof(void*)];
	} convert;
	memcpy(convert.ary, vec->G0, sizeof(void*));
	return convert.pointer;
}

// See https://docs.oracle.com/cd/E19455-01/806-3814/806-3814.pdf, although 2000 they're better than most

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

#define CHK_RET_FLAGS(_VAL, _FLAG) if ((_VAL) & (_FLAG)) fprintf(stdout, "DEBUG: token includes the " #_FLAG " flag\n");
static
void mg_print_accept_context_flags(int flags)
{
	if (0 != flags) {
		CHK_RET_FLAGS(flags, GSS_C_DELEG_FLAG);         // the initiator's credentials may be delegated
		CHK_RET_FLAGS(flags, GSS_C_MUTUAL_FLAG);        // mutual authentication is available
		CHK_RET_FLAGS(flags, GSS_C_REPLAY_FLAG);        // detection of repeated messages is available
		CHK_RET_FLAGS(flags, GSS_C_SEQUENCE_FLAG);      // detection of out-of-sequence messages is available
		CHK_RET_FLAGS(flags, GSS_C_CONF_FLAG);          // messages can be encrypted (confidentiality)
		CHK_RET_FLAGS(flags, GSS_C_INTEG_FLAG);         // messages can be stamped with a MIC to ensure their validity
		CHK_RET_FLAGS(flags, GSS_C_ANON_FLAG);          // the context initiator will be anonymous
		CHK_RET_FLAGS(flags, GSS_C_PROT_READY_FLAG);    // indicates that validation indicated by the CONF and INTEG flags is now possible. We can now call gss_unwrap() or gss_verify_mic(); if false, data-reception may be incomplete
		CHK_RET_FLAGS(flags, GSS_C_TRANS_FLAG);         // the context can be exported
		CHK_RET_FLAGS(flags, GSS_C_DELEG_POLICY_FLAG);  // delegation is permissible only if permitted by KDC policy
	}
}
#undef CHK_RET_FLAGS

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

K mg_print_mechs(K dummy)
{
	mg_print_gss_mechs();
	return kb(1);
}

void mg_print_mech(gss_OID mech_typ)
{
	OM_uint32 minor_status, err;
	gss_buffer_desc buf = {0};
	// https://github.com/krb5/krb5/blob/master/src/lib/gssapi/generic/oid_ops.c#L237
	err = gss_oid_to_str(&minor_status, mech_typ, &buf);
	if (GSS_ERROR(err)) {
		mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
	}
	else {
		// Will be kerberos "{ 1 2 840 113554 1 2 2 }"
		// fprintf(stdout, "DEBUG: mech-type is %.*s\n", (int)buf.length, (char*)buf.value);
	}
}

void mg_release_name(gss_name_t *name)
{
	OM_uint32 minor_status, err;
	err = gss_release_name(&minor_status, name);
	if (GSS_ERROR(err)) {
		mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
	}
}

K mg_get_display_name(gss_name_t name)
{
	K k_name = (K)NULL;
	if (NULL != name) {
		OM_uint32 minor_status, err;
		gss_buffer_desc name_buf = {0};
		gss_OID name_typ = NULL;

		err = gss_display_name(&minor_status, name, &name_buf, &name_typ);
		if (GSS_ERROR(err)) {
			fprintf(stderr, "ERROR: failed in gss_display_name\n");
			mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
			k_name = kpn("_UNRESOLVED_", sizeof("_UNRESOLVED_"));
		}
		else {
			k_name = kpn((S)name_buf.value, (J)name_buf.length);
		}
	}
	return k_name;
}

K mg_inquire_service_target(gss_ctx_id_t context_handle)
{
	OM_uint32 minor_status, res;
	gss_name_t tgt_name;

	res = gss_inquire_context(
							&minor_status
						, context_handle
						, NULL            // source_name
						, &tgt_name       // target_name
						, NULL            // lifetime_rec
						, NULL            // mech_type
						, NULL            // ctx_flags
						, NULL            // locally_initiated
						, NULL);          // open

	K k_tgt_name = (K)NULL;
	
	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
	}
	else {
		k_tgt_name = mg_get_display_name(tgt_name);
		mg_release_name(&tgt_name);
	}
	return k_tgt_name;
}


K mg_accept_sec_context(K b64_tkn, K ctx_ptr)
{
	if (KC != b64_tkn->t) {
		return krr("expect token as char-vector");
	}

	size_t out_len = 0;
	unsigned char *b64 = base64_decode(b64_tkn->G0, b64_tkn->n, &out_len);
	if (NULL == b64 || 0 == out_len) {
		return krr("ERROR: failed in base64_decode\n");
	}

	gss_buffer_desc input_token = { .length = out_len, .value = b64 };

	OM_uint32 res, minor_status;

	gss_ctx_id_t context_handle = GSS_C_NO_CONTEXT;

	// ctx_ptr can be 0h or 4h; if 4h then its length must == sizeof(void*)
	if (KG == ctx_ptr->t && sizeof(void*) == ctx_ptr->n) {
		context_handle = mg_byte_vec_to_void_ptr(ctx_ptr);
		fprintf(stdout, "DEBUG: Using continuation context_handle %p\n", (void*)context_handle);
	}

	OM_uint32       ret_flags      = 0;
	OM_uint32       time_rec       = 0;
	gss_buffer_desc output_token   = {0};
	gss_name_t      src_name       = NULL;
	//gss_OID         mech_typ       = {0};

	// The recommended choice is to pass GSS_C_NO_CREDENTIAL as the acceptor credential. In this case,
	// clients may authenticate to _any_ service principal in the default keytab or the value of the
	// KRB5_KTNAME environment variable.

	// NB if res is GSS_S_CONTINUE_NEEDED we need to invoke again
	res = gss_accept_sec_context(
	         &minor_status
	       , &context_handle           // set above to GSS_C_NO_CONTEXT
	       , GSS_C_NO_CREDENTIAL       // use any in the keytab
	       , &input_token
	       , GSS_C_NO_CHANNEL_BINDINGS // the null value for gss_channel_bindings_t
	       , &src_name                 // name of the initiating principal
	       , NULL                      // &mech_typ // security-mechanism to use; set NULL if we don't care
	       , &output_token             //
	       , &ret_flags                // bitwise collection indicating e.g. GSS_C_INTEG_FLAG
	       , &time_rec                 // number of seconds the context will remain vaild
	       , NULL                      // gss_cred_id_t delegated credentials handle
	       );

	if (GSS_SUPPLEMENTARY_INFO(res)) {
		fprintf(stdout, "DEBUG: GSS_SUPPLEMENTARY_INFO requested\n");
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
			//K out_tkn = kpn((S)b64, b64_len);
			K out_tkn = ktn(KC, b64_len);
			memcpy(out_tkn->G0, (S)b64, b64_len);
			r0(ctx_ptr);

			ctx_ptr = mg_void_ptr_to_byte_vec(context_handle);

			fprintf(stdout, "DEBUG: gss_accept_sec_context requested CONTINUE_NEEDED; context_handle is %p\n", (void*)context_handle);
			K vals = knk(3, ki(2), ctx_ptr, out_tkn);
			return vals;
		}
	}

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, gss_mech_krb5);
		return krr("failed to decode Client token");
	}

	fprintf(stdout, "TRACE: gss_accept_sec_context success; context is valid for %u seconds\n", time_rec);

	// mg_print_mech_typ(mech_typ);

	K k_src_name = mg_get_display_name(src_name);

	mg_release_name(&src_name);

	fprintf(stdout, "DEBUG: Client is %.*s\n", (int)k_src_name->n, (char*)k_src_name->G0);

	if (0 != ret_flags) {

		mg_print_accept_context_flags(ret_flags);

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

	// Retrieve the Target name using gss_inquire_context
	K k_tgt_name = mg_inquire_service_target(context_handle);

	fprintf(stdout, "DEBUG: Service target is %.*s\n", (int)k_tgt_name->n, (char*)k_tgt_name->G0);

	// Release the gss_ctx_id_t if non-null
	if (GSS_C_NO_CONTEXT != context_handle) {
		gss_buffer_desc output_token = {0};
		res = gss_delete_sec_context(&minor_status, &context_handle, &output_token);
		if (GSS_ERROR(res)) {
			mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		}
		else {
			fprintf(stdout, "DEBUG: released context_handle\n");
		}
	}

	K vals = knk(4, ki(1), k_src_name, k_tgt_name, ki((I)time_rec));
	return vals;
} // end mg_accept_sec_context

K mg_init_sec_context(K k_src_name, K k_tgt_name)
{
	if (KC != k_src_name->t) {
		return krr("Bad k_src_name.type");
	}
	if (KC != k_tgt_name->t) {
		return krr("Bad k_tgt_name.type");
	}


	OM_uint32 res, minor_status;

	gss_cred_id_t cred_handle = GSS_C_NO_CREDENTIAL;

	//-----------------------------------
	{
		gss_name_t src_name;
		gss_buffer_desc buf = {
			.length = (size_t)k_src_name->n,
			.value = (void*)k_src_name->G0,
		};

		res = gss_import_name(&minor_status, &buf, GSS_KRB5_NT_PRINCIPAL_NAME, &src_name);

		if (GSS_ERROR(res)) {
			mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
			return krr("failed to import tgt_name");
		}

		res = gss_acquire_cred(
		           &minor_status
		         , src_name
		         , GSS_C_INDEFINITE      // time_req; how long we want this to last for
		         , gss_mech_set_krb5     // GSS_C_NULL_OID_SET    // gss_mech_set_krb5     // desired_mechs; can supply GSS_C_NO_OID_SET but it seems to access the keytab twice
		         , GSS_C_INITIATE        // gss_cred_usage_t (C.f. GSS_C_INITIATE and GSS_C_BOTH)
		         , &cred_handle
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
	}

	if (GSS_C_NO_CREDENTIAL == cred_handle) {
		return krr("failed to acquire credentials for source principal");
	}
	fprintf(stdout, "DEBUG: acquired credentials for %.*s\n", (int)k_src_name->n, (char*)k_src_name->G0);

	//-----------------------------------

	gss_name_t tgt_name = GSS_C_NO_NAME;
	{
		gss_buffer_desc buf = {
			.length = (size_t)k_tgt_name->n,
			.value = (void*)k_tgt_name->G0,
		};

		fprintf(stdout, "DEBUG: resolving target name %.*s to internal format\n", (int)buf.length, (char*)buf.value);

		res = gss_import_name(&minor_status, &buf, GSS_KRB5_NT_PRINCIPAL_NAME, &tgt_name);

		if (GSS_ERROR(res)) {
			mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
			return krr("failed to import tgt_name");
		}
	}

	if (GSS_C_NO_NAME == tgt_name) {
		return krr("failed to resolve target service\n");
	}
	fprintf(stdout, "DEBUG: resolved target service %.*s\n", (int)k_tgt_name->n, (char*)k_tgt_name->G0);

	//-----------------------------------

	gss_ctx_id_t    context_handle = GSS_C_NO_CONTEXT;
	OM_uint32       req_flags = GSS_C_CONF_FLAG | GSS_C_INTEG_FLAG | GSS_C_REPLAY_FLAG | GSS_C_SEQUENCE_FLAG;
	gss_buffer_desc output_token = {0};
	OM_uint32       ret_flags;
	OM_uint32       time_rec;

	fprintf(stdout, "DEBUG: beginning init_sec_context\n");
	res = gss_init_sec_context(
	            &minor_status
	          , cred_handle                         // use the entirety of the keytab with GSS_C_NO_CREDENTIAL
	          , &context_handle                     // context_handle
	          , tgt_name                            // target_name
	          , GSS_C_NULL_OID                      // mech-type or GSS_C_NULL_OID
	          , req_flags                           // request flags
	          , GSS_C_INDEFINITE                    // time request
	          , GSS_C_NO_CHANNEL_BINDINGS           // input_chan_bindings
	          , GSS_C_NO_BUFFER                     // input_token
	          , NULL                                // mech-type used (obvs krb5)
	          , &output_token                       // output_token structure
	          , &ret_flags                          // flags applied to the request
	          , &time_rec                           // validity time
	          );

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		return krr("failed in gss_init_sec_context");
	}
	fprintf(stdout, "DEBUG: context initialised successfully\n");

	size_t b64_len = 0;
	unsigned char *b64 = base64_encode((unsigned char*)output_token.value, output_token.length, &b64_len);
	if (NULL == b64 || 0 == b64_len) {
		free(b64);
		return krr("failed in base64_encode");
	}
	K k_token = kpn((S)b64, b64_len);
	free(b64);

	K vals = knk(2, ki(1), k_token);
	return vals;
} // end mg_init_sec_context
