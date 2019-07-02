/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include "extv.h"
#include "handshake.h"
#include "tls13/certificate_request.h"
#include "ext/signature.h"
#include "mbuffers.h"
#include "algorithms.h"
#include "auth/cert.h"

/* for tlist dereference */
#include "x509/verify-high.h"

#define EXTID_CERTIFICATE_AUTHORITIES 47

typedef struct crt_req_ctx_st {
	gnutls_session_t session;
	unsigned got_sig_algo;
	gnutls_pk_algorithm_t pk_algos[MAX_ALGOS];
	unsigned pk_algos_length;
	const uint8_t *rdn; /* pointer inside the message buffer */
	unsigned rdn_size;
} crt_req_ctx_st;

static unsigned is_algo_in_list(gnutls_pk_algorithm_t algo, gnutls_pk_algorithm_t *list, unsigned list_size)
{
	unsigned j;

	for (j=0;j<list_size;j++) {
		if (list[j] == algo)
			return 1;
	}
	return 0;
}

static
int parse_cert_extension(void *_ctx, unsigned tls_id, const uint8_t *data, unsigned data_size)
{
	crt_req_ctx_st *ctx = _ctx;
	gnutls_session_t session = ctx->session;
	unsigned v;
	int ret;

	/* Decide which certificate to use if the signature algorithms extension
	 * is present.
	 */
	if (tls_id == ext_mod_sig.tls_id) {
		const version_entry_st *ver = get_version(session);
		const gnutls_sign_entry_st *se;
		/* signature algorithms; let's use it to decide the certificate to use */
		unsigned i;

		if (ctx->got_sig_algo)
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

		ctx->got_sig_algo = 1;

		if (data_size < 2)
			return gnutls_assert_val(GNUTLS_E_TLS_PACKET_DECODING_ERROR);

		v = _gnutls_read_uint16(data);
		if (v != data_size-2)
			return gnutls_assert_val(GNUTLS_E_TLS_PACKET_DECODING_ERROR);

		data += 2;
		data_size -= 2;

		ret = _gnutls_sign_algorithm_parse_data(session, data, data_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		/* The APIs to retrieve a client certificate accept the public
		 * key algorithms instead of signatures. Get the public key algorithms
		 * from the signatures.
		 */
		for (i=0;i<(unsigned)data_size;i+=2) {
			se = _gnutls_tls_aid_to_sign_entry(data[i], data[i+1], ver);
			if (se == NULL)
				continue;

			if (ctx->pk_algos_length >= sizeof(ctx->pk_algos)/sizeof(ctx->pk_algos[0]))
				break;

			if (is_algo_in_list(se->pk, ctx->pk_algos, ctx->pk_algos_length))
				continue;

			ctx->pk_algos[ctx->pk_algos_length++] = se->pk;
		}
	} else if (tls_id == EXTID_CERTIFICATE_AUTHORITIES) {
		if (data_size < 3) {
			return gnutls_assert_val(GNUTLS_E_TLS_PACKET_DECODING_ERROR);
		}

		v = _gnutls_read_uint16(data);
		if (v != data_size-2)
			return gnutls_assert_val(GNUTLS_E_TLS_PACKET_DECODING_ERROR);

		ctx->rdn = data+2;
		ctx->rdn_size = v;
	}

	return 0;
}

int _gnutls13_recv_certificate_request_int(gnutls_session_t session, gnutls_buffer_st *buf)
{
	int ret;
	crt_req_ctx_st ctx;
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;

	_gnutls_handshake_log("HSK[%p]: parsing certificate request\n", session);

	if (unlikely(session->security_parameters.entity == GNUTLS_SERVER))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	/* if initial negotiation is complete, this is a post-handshake auth */
	if (!session->internals.initial_negotiation_completed) {
		if (buf->data[0] != 0) {
			/* The context field must be empty during handshake */
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
		}

		/* buf->length is positive */
		buf->data++;
		buf->length--;
	} else {
		gnutls_datum_t context;

		ret = _gnutls_buffer_pop_datum_prefix8(buf, &context);
		if (ret < 0)
			return gnutls_assert_val(ret);

		gnutls_free(session->internals.post_handshake_cr_context.data);
		ret = _gnutls_set_datum(&session->internals.post_handshake_cr_context,
					context.data, context.size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	memset(&ctx, 0, sizeof(ctx));
	ctx.session = session;

	ret = _gnutls_extv_parse(&ctx, parse_cert_extension, buf->data, buf->length);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* The "signature_algorithms" extension MUST be specified */
	if (!ctx.got_sig_algo)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

	session->internals.hsk_flags |= HSK_CRT_ASKED;

	ret = _gnutls_select_client_cert(session, ctx.rdn, ctx.rdn_size,
					 ctx.pk_algos, ctx.pk_algos_length);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_get_selected_cert(session, &apr_cert_list,
					&apr_cert_list_length, &apr_pkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (apr_cert_list_length > 0) {
		gnutls_sign_algorithm_t algo;

		algo = _gnutls_session_get_sign_algo(session, &apr_cert_list[0], apr_pkey, 0);
		if (algo == GNUTLS_SIGN_UNKNOWN) {
			_gnutls_handshake_log("HSK[%p]: rejecting client auth because of no suitable signature algorithm\n", session);
			_gnutls_selected_certs_deinit(session);
			return gnutls_assert_val(0);
		}

		gnutls_sign_algorithm_set_client(session, algo);
	}

	return 0;
}

int _gnutls13_recv_certificate_request(gnutls_session_t session)
{
	int ret;
	gnutls_buffer_st buf;

	if (!session->internals.initial_negotiation_completed &&
	    session->internals.hsk_flags & HSK_PSK_SELECTED)
		return 0;

	if (unlikely(session->security_parameters.entity != GNUTLS_CLIENT))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	ret = _gnutls_recv_handshake(session, GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST, 1, &buf);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* if not received */
	if (buf.length == 0) {
		_gnutls_buffer_clear(&buf);
		return 0;
	}

	ret = _gnutls13_recv_certificate_request_int(session, &buf);

	_gnutls_buffer_clear(&buf);
	return ret;
}

static
int write_certificate_authorities(void *ctx, gnutls_buffer_st *buf)
{
	gnutls_session_t session = ctx;
	gnutls_certificate_credentials_t cred;

	if (session->internals.ignore_rdn_sequence != 0)
		return 0;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if (cred->tlist->x509_rdn_sequence.size == 0)
		return 0;

	return
	    _gnutls_buffer_append_data_prefix(buf, 16,
					      cred->
					      tlist->x509_rdn_sequence.
					      data,
					      cred->
					      tlist->x509_rdn_sequence.
					      size);
}

int _gnutls13_send_certificate_request(gnutls_session_t session, unsigned again)
{
	gnutls_certificate_credentials_t cred;
	int ret;
	mbuffer_st *bufel = NULL;
	gnutls_buffer_st buf;
	unsigned init_pos;

	if (again == 0) {
		unsigned char rnd[12];

		if (!session->internals.initial_negotiation_completed &&
		    session->internals.hsk_flags & HSK_PSK_SELECTED)
			return 0;

		if (session->internals.send_cert_req == 0)
			return 0;

		cred = (gnutls_certificate_credentials_t)
		    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
		if (cred == NULL)
			return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);

		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (session->internals.initial_negotiation_completed) { /* reauth */
			ret = gnutls_rnd(GNUTLS_RND_NONCE, rnd, sizeof(rnd));
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			gnutls_free(session->internals.post_handshake_cr_context.data);
			ret = _gnutls_set_datum(&session->internals.post_handshake_cr_context,
						rnd, sizeof(rnd));
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret = _gnutls_buffer_append_data_prefix(&buf, 8,
							        session->internals.post_handshake_cr_context.data,
							        session->internals.post_handshake_cr_context.size);
		} else {
			ret = _gnutls_buffer_append_prefix(&buf, 8, 0);
		}

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_extv_append_init(&buf);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		init_pos = ret;

		ret = _gnutls_extv_append(&buf, ext_mod_sig.tls_id, session,
					  (extv_append_func)_gnutls_sign_algorithm_write_params);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_extv_append(&buf, EXTID_CERTIFICATE_AUTHORITIES, session,
					  write_certificate_authorities);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_extv_append_final(&buf, init_pos, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);

		session->internals.hsk_flags |= HSK_CRT_REQ_SENT;
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;

}

