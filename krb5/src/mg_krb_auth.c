#include <gssapi/gssapi.h>
#include <stdio.h>      // printf, stderr
#include <string.h>     // strlen
#include <stdlib.h>     // EXIT_FAILURE
#include <stdint.h>     // int64_t, uintptr_t

#include <gssapi.h>
#include <gssapi/gssapi_krb5.h> // GSS_KRB5_NT_PRINCIPAL_NAME

#define KXVER 3
#include "k.h"

#define KUNARY 0x65


/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
// From https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
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

			(void)gss_release_buffer(&err_status, &major_string);
			(void)gss_release_buffer(&err_status, &minor_string);
		}
		while (0 != message_context);
	}
}

#define CHK_RET_FLAGS(_VAL, _FLAG) if ((_VAL) & (_FLAG)) fprintf(stdout, "DEBUG: token includes the " #_FLAG " flag\n");
static
void mg_print_context_flags(int flags)
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

static
void mg_release_buffer(gss_buffer_t buffer)
{
	if (NULL != buffer->value) {
		OM_uint32 err, minor_status = 0;
		err = gss_release_buffer(&minor_status, buffer);
		if (GSS_ERROR(err)) {
			mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
		}
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
		mg_release_buffer(&buf);
	}
	res = gss_release_oid_set(&minor_status, &mech_set);
	if (GSS_S_COMPLETE != res) {
		fprintf(stderr, "ERROR: failed in gss_release_oid_set\n");
	}
}

__attribute__((unused))
static
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

static
void mg_release_name(gss_name_t *name)
{
	OM_uint32 minor_status, err;
	err = gss_release_name(&minor_status, name);
	if (GSS_ERROR(err)) {
		mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
	}
}

static
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

static
K mg_inquire_service_target(gss_ctx_id_t context_handle)
{
	if (GSS_C_NO_CONTEXT == context_handle) {
		return kpn("", 0);
	}
	OM_uint32 minor_status, res;
	gss_name_t tgt_name;
	// gss_name_t src_name, tgt_name;
	// OM_uint32 ttl, flags;
	// gss_OID mech;
	// int local, open;

	fprintf(stdout, "TEST: 1\n");
	// res = gss_inquire_context(
	// 						&minor_status
	// 					, context_handle
	// 					, &src_name       // source_name
	// 					, &tgt_name       // target_name
	// 					, &ttl            // lifetime_rec
	// 					, &mech           // mech_type
	// 					, &flags          // ctx_flags
	// 					, &local          // locally_initiated
	// 					, &open);         // open
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

	fprintf(stdout, "TEST: 2\n");
	K k_tgt_name = (K)0;

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
	}
	else {
		k_tgt_name = mg_get_display_name(tgt_name);
		mg_release_name(&tgt_name);
	}
	return k_tgt_name;
}

static
void mg_delete_sec_context(gss_ctx_id_t *context_handle)
{
	if (GSS_C_NO_CONTEXT != *context_handle) {
		OM_uint32 err, minor_status = 0;
		err = gss_delete_sec_context(&minor_status, context_handle, GSS_C_NO_BUFFER);
		if (GSS_ERROR(err)) {
			mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
		}
	}
}

__attribute__((unused))
static
void mg_print_buffer_data_hex(const char *lede, gss_buffer_t buf)
{
	fprintf(stdout, "DEBUG: %s; bytes are:\n", lede);
	unsigned char *bin = buf->value;

	for (size_t i = 0 ; i < buf->length ; i++) {
	 	fprintf(stdout, "%02hhx", bin[i]);
	}
	fprintf(stdout, "\n");
}

static
int mg_export_sec_context(gss_ctx_id_t *context_handle, K *result)
{
	if (GSS_C_NO_CONTEXT == *context_handle) {
		*result = kpn("", 0);
		return 1;
	}

	OM_uint32 err, minor_status = 0;
	gss_buffer_desc buf = {0};
	err = gss_export_sec_context(&minor_status, context_handle, &buf);
	if (GSS_ERROR(err)) {
		mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
		mg_delete_sec_context(context_handle);
		*result = krr("context export");
		return 0;
	}

	size_t b64_len = 0;

	unsigned char *b64 = base64_encode((unsigned char*)buf.value, buf.length, &b64_len);

	if (NULL == b64) {
		// context has been disassociated and closed already, no need to/can't delete again
		*result = krr("b64.encode failed, export failed");
		return 0;
	}

	*result = kpn((S)b64, (J)b64_len);

	free(b64);
	return 1;
}

static
gss_ctx_id_t mg_import_sec_context(K k_ctx)
{
	OM_uint32 err, minor_status = 0;
	gss_ctx_id_t context_handle = GSS_C_NO_CONTEXT;
	size_t vec_len = 0;
	unsigned char *vec = base64_decode((unsigned char*)k_ctx->G0, (size_t)k_ctx->n, &vec_len);
	if (NULL == vec) {
		fprintf(stderr, "ERROR: failed in b64-decode of import-sec-context\n");
		return GSS_C_NO_CONTEXT;
	}
	gss_buffer_desc buf = { .length = vec_len, .value = vec };

	err = gss_import_sec_context(&minor_status, &buf, &context_handle);

	free(vec);

	if (GSS_ERROR(err)) {
		mg_print_gss_error(err, minor_status, GSS_C_NULL_OID);
		return GSS_C_NO_CONTEXT;
	}
	fprintf(stdout, "DEBUG: imported sec-context\n");
	return context_handle;
}

static
K mg_delete_ctx_return_err_xk(gss_ctx_id_t *context_handle, K err)
{
	mg_delete_sec_context(context_handle);
	return err;
}

static
K mg_delete_ctx_return_err_xs(gss_ctx_id_t *context_handle, char *msg)
{
	return mg_delete_ctx_return_err_xk(context_handle, krr(msg));
}

K mg_accept_sec_context(K k_b64_tkn, K k_ctx)
{
	if (KC != k_b64_tkn->t) {
		return krr("expect token as char-vector");
	}

	size_t out_len = 0;
	unsigned char *b64 = base64_decode(k_b64_tkn->G0, k_b64_tkn->n, &out_len);
	if (NULL == b64 || 0 == out_len) {
		return krr("ERROR: failed in base64_decode\n");
	}

	gss_buffer_desc input_token = { .length = out_len, .value = b64 };

	OM_uint32 res, minor_status;

	gss_ctx_id_t context_handle = GSS_C_NO_CONTEXT;

	if (KC == k_ctx->t && 0 < k_ctx->n) {
		context_handle = mg_import_sec_context(k_ctx);
		if (GSS_C_NO_CONTEXT == context_handle) {
			return krr("failed to import context");
		}
	}

	OM_uint32       ret_flags      = 0;
	OM_uint32       time_rec       = 0;
	gss_buffer_desc output_token   = {0};
	gss_name_t      src_name       = NULL;

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
	       , NULL                      // security-mechanism to use; using NULL as we know
	       , &output_token             //
	       , &ret_flags                // bitwise collection indicating e.g. GSS_C_INTEG_FLAG
	       , &time_rec                 // number of seconds the context will remain vaild
	       , NULL                      // gss_cred_id_t delegated credentials handle
	       );

	free(b64);

	if (GSS_SUPPLEMENTARY_INFO(res)) {
		fprintf(stdout, "DEBUG: GSS_SUPPLEMENTARY_INFO requested\n");
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		if (GSS_S_CONTINUE_NEEDED == res) {
			if (0 == output_token.length) {
				mg_delete_sec_context(&context_handle);
				return krr("GSSAPI signals CONTINUE_NEEDED but output_token.length is zero");
			}
			size_t b64_len = 0;
			unsigned char *b64 = base64_encode((unsigned char*)output_token.value, output_token.length, &b64_len);
			if (NULL == b64) {
				fprintf(stderr, "ERROR: failed in base64_encode\n");
				mg_delete_sec_context(&context_handle);
				return krr("GSSAPI signals CONTINUE_NEEDED but base64_encode failed");
			}
			K k_out_tkn = kpn((S)b64, b64_len);
			free(b64);

			mg_release_buffer(&output_token);

			K k_ctx = NULL;
			int err = mg_export_sec_context(&context_handle, &k_ctx);
			if (0 == err) {
				mg_delete_sec_context(&context_handle);
				return k_ctx;
			}

			fprintf(stdout, "DEBUG: gss_accept_sec_context requested CONTINUE_NEEDED\n");

			return knk(3, ki(2), k_ctx, k_out_tkn);
		}
	}

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, gss_mech_krb5);
		return mg_delete_ctx_return_err_xs(&context_handle, "failed to decode Client token");
	}

	fprintf(stdout, "DEBUG: gss_accept_sec_context success; context is valid for %u seconds\n", time_rec);

	// We can print the mech-type, using mg_print_mech_typ, but it's pretty boring

	K k_src_name = mg_get_display_name(src_name);

	mg_release_name(&src_name);

	fprintf(stdout, "DEBUG: Client is %.*s\n", (int)k_src_name->n, (char*)k_src_name->G0);

	if (0 != ret_flags) {

		mg_print_context_flags(ret_flags);

		if (res & GSS_C_PROT_READY_FLAG) {
			if (res & (GSS_C_CONF_FLAG)) {
				int conf_state;
				gss_qop_t qop_state;
				res = gss_unwrap(&minor_status, context_handle, &input_token, &output_token, &conf_state, &qop_state);
				if (GSS_ERROR(res)) {
					mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
				}
				else {
					fprintf(stdout, "DEBUG: gss_unwrap was successful\n");
				}
			}
			else if (res & GSS_C_INTEG_FLAG) {
				gss_qop_t qop_state;
				res = gss_verify_mic(&minor_status, context_handle, &input_token, &output_token, &qop_state);
				if (GSS_ERROR(res)) {
					mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
				}
				else {
					fprintf(stdout, "DEBUG: gss_verify_mic was successful\n");
				}
			}
		}
		else {
			fprintf(stdout, "DEBUG: PROT_READY not set, not validating\n");
		}
	}

	K k_out_tkn = (K)0;

	if (NULL == output_token.value) {
		k_out_tkn = kpn("", 0);
	}
	else {
		fprintf(stderr, "DEBUG: accept-context published reply of length %zd for the caller\n", output_token.length);
		size_t b64_len = 0;
		unsigned char *b64 = base64_encode((unsigned char*)output_token.value, output_token.length, &b64_len);

		mg_release_buffer(&output_token);

		if (NULL == b64) {
			fprintf(stderr, "ERROR: while encoding accept_ctx' output data\n");
			return mg_delete_ctx_return_err_xs(&context_handle,"GSSAPI provided additional data for the Client but base-64 encoding failed");
		}
		k_out_tkn = kpn((S)b64, (J)b64_len);
		free(b64);
	}

	// Retrieve the Target name using gss_inquire_context
	K k_tgt_name = mg_inquire_service_target(context_handle);

	fprintf(stdout, "DEBUG: Service target is %.*s\n", (int)k_tgt_name->n, (char*)k_tgt_name->G0);

	if (0 == (ret_flags & GSS_C_TRANS_FLAG)) {
		return krr("context-export prohibited");
	}

	k_ctx = NULL;
	int err = mg_export_sec_context(&context_handle, &k_ctx);

	if (0 == err) {
		// context was disposed by mg_export_sec_context
		return k_ctx;
	}

	return knk(6, ki(1), k_src_name, k_tgt_name, ki((I)time_rec), k_ctx, k_out_tkn);
}
// end mg_accept_sec_context

static
K mg_import_name(K k_name, gss_name_t *name)
{
	OM_uint32 res, minor_status = 0;
	gss_buffer_desc buf = {
		.length = (size_t)k_name->n,
		.value = (void*)k_name->G0,
	};

	fprintf(stdout, "DEBUG: resolving %.*s to internal format\n", (int)buf.length, (char*)buf.value);

	res = gss_import_name(&minor_status, &buf, GSS_KRB5_NT_PRINCIPAL_NAME, name);

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		return krr("failed to import tgt_name");
	}

	return (K)0;
}

static
K mg_acquire_cred(K k_name, gss_cred_id_t *cred_handle)
{
	OM_uint32 res, minor_status = 0;
	gss_name_t name = GSS_C_NO_NAME;

	K k_err = mg_import_name(k_name, &name);
	if ((K)0 != k_err || GSS_C_NO_NAME == name) {
		return k_err;
	}

	res = gss_acquire_cred(
	           &minor_status
	         , name
	         , GSS_C_INDEFINITE      // time_req; how long we want this to last for
	         , gss_mech_set_krb5     // desired_mechs; can supply GSS_C_NO_OID_SET but it seems to access the keytab twice
	         , GSS_C_INITIATE        // gss_cred_usage_t (C.f. GSS_C_ACCEPT and GSS_C_BOTH)
	         , cred_handle
	         , NULL                  // actual mechs; we don't need to know so set this to NULL
	         , NULL                  // number of seconds for which this credential is valid
	         );

	mg_release_name(&name);

	if (GSS_SUPPLEMENTARY_INFO(res)) {
		fprintf(stdout, " INFO: suppliementary-info provided for gss_acquire_cred ...\n");
		mg_print_gss_error(res, minor_status, 0);
	}

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, gss_mech_krb5);
		return krr("failed to acquire credentials");
	}

	return (K)0;
}

K mg_cont_sec_context(K k_src_name, K k_tgt_name, K k_ctx, K k_token)
{
	if (KC != k_src_name->t) {
		return krr("Bad k_src_name.type");
	}
	if (KC != k_tgt_name->t) {
		return krr("Bad k_tgt_name.type");
	}
	if (!(KUNARY == k_token->t || KC == k_token->t)) {
		return krr("Bad k_token.type");
	}

	gss_ctx_id_t context_handle;
	if (KUNARY == k_ctx->t) {
		context_handle = GSS_C_NO_CONTEXT;
	}
	else if (KC == k_ctx->t) {
		context_handle = mg_import_sec_context(k_ctx);
		if (GSS_C_NO_CONTEXT == context_handle) {
			return krr("failed to import context");
		}
	}
	else {
		fprintf(stderr, "ERROR: have ctx_ptr->t %hhi, ctx_ptr->n %lli\n", k_ctx->t, k_ctx->n);
		return krr("Bad k_ctx.type");
	}

	// Define the workflow as 1, initial or 2, continuation
	int workflow = 1;

	if (GSS_C_NO_CONTEXT != context_handle && KC == k_token->t) {
		workflow = 2;
	}

	OM_uint32 res, minor_status;

	//----------------------------------- acquire credential

	gss_cred_id_t cred_handle = GSS_C_NO_CREDENTIAL;

	K k_err = mg_acquire_cred(k_src_name, &cred_handle);
	if ((K)0 != k_err) {
		return k_err;
	}

	if (GSS_C_NO_CREDENTIAL == cred_handle) {
		return krr("failed to acquire credentials for source principal");
	}

	fprintf(stdout, "DEBUG: acquired credentials for %.*s\n", (int)k_src_name->n, (char*)k_src_name->G0);

	//----------------------------------- convert target service-name

	gss_name_t tgt_name = GSS_C_NO_NAME;

	k_err = mg_import_name(k_tgt_name, &tgt_name);
	if ((K)0 != k_err) {
		return k_err;
	}

	if (GSS_C_NO_NAME == tgt_name) {
		return krr("failed to resolve target service");
	}

	fprintf(stdout, "DEBUG: resolved target service %.*s\n", (int)k_tgt_name->n, (char*)k_tgt_name->G0);

	//----------------------------------- initialise security context

	gss_buffer_t input_token = GSS_C_NO_BUFFER;
	gss_buffer_desc token_data = {0};
	unsigned char *b64 = NULL;

	if (KC == k_token->t && k_token->n > 0) {
		size_t b64_len = 0;
		b64 = base64_decode((unsigned char*)k_token->G0, k_token->n, &b64_len);
		if (NULL == b64) {
			return krr("failed to decode continuation input-token");
		}
		token_data.length = b64_len;
		token_data.value = b64;
		input_token = &token_data;
	}

	OM_uint32       req_flags = GSS_C_CONF_FLAG | GSS_C_INTEG_FLAG | GSS_C_REPLAY_FLAG | GSS_C_SEQUENCE_FLAG | GSS_C_MUTUAL_FLAG | GSS_C_TRANS_FLAG;
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
	          , req_flags                           // flags requested
	          , GSS_C_INDEFINITE                    // time request
	          , GSS_C_NO_CHANNEL_BINDINGS           // input_chan_bindings
	          , input_token                         // input_token
	          , NULL                                // mech-type used (obvs krb5)
	          , &output_token                       // output_token structure
	          , &ret_flags                          // flags applied
	          , &time_rec                           // validity time
	          );

	free(b64);

	if (KC == k_token->t) {
		mg_release_buffer(&token_data);
	}

	if (0 != ret_flags) {
		mg_print_context_flags(ret_flags);
	}

	if (GSS_ERROR(res)) {
		mg_print_gss_error(res, minor_status, GSS_C_NULL_OID);
		return krr("failed in gss_init_sec_context");
	}
	fprintf(stdout, "DEBUG: context initialised successfully\n");

	if (output_token.length > 0 && NULL != output_token.value) {
		size_t b64_len = 0;
		unsigned char *b64 = base64_encode((unsigned char*)output_token.value, output_token.length, &b64_len);
		if (NULL == b64) {
			return krr("failed in base64_encode");
		}
		k_token = kpn((S)b64, b64_len);

		free(b64);
	}
	else {
		k_token = kpn("", 0);
	}

	if (1 == workflow) {
		k_tgt_name = kpn("", 0);
	}
	else if (2 != workflow) {
		return krr("logic error: should be workflow 2");
	}
	else {
		fprintf(stdout, " TEST: about to inquire-service-target\n");
		k_tgt_name = mg_inquire_service_target(context_handle);
		fprintf(stdout, " TEST: service-target-inquiry complete\n");
		if ((K)0 == k_tgt_name) {
			return krr("failed to resolve a service name");
		}
	}

	if (0 == (ret_flags & GSS_C_TRANS_FLAG)) {
		return krr("context-export prohibited");
	}

	k_ctx = NULL;
	int err = mg_export_sec_context(&context_handle, &k_ctx);
	if (0 == err) {
		return k_ctx;
	}
	return knk(4, ki(workflow), k_tgt_name, k_ctx, k_token);
}
// end mg_cont_sec_context

K mg_init_sec_context(K k_src_name, K k_tgt_name)
{
	K nil = kg(0);
	nil->t = KUNARY;
	K ret = mg_cont_sec_context(k_src_name, k_tgt_name, nil, nil);
	r0(nil);
	return ret;
}
