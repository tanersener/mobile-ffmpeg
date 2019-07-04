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
#include "tls13/certificate.h"
#include "auth/cert.h"
#include "mbuffers.h"
#include "ext/status_request.h"

static int parse_cert_extension(void *ctx, unsigned tls_id, const uint8_t *data, unsigned data_size);
static int parse_cert_list(gnutls_session_t session, uint8_t * data, size_t data_size);

int _gnutls13_recv_certificate(gnutls_session_t session)
{
	int ret;
	gnutls_buffer_st buf;
	unsigned optional = 0;

	if (!session->internals.initial_negotiation_completed &&
	    session->internals.hsk_flags & HSK_PSK_SELECTED)
		return 0;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		/* if we didn't request a certificate, there will not be any */
		if (session->internals.send_cert_req == 0)
			return 0;

		if (session->internals.send_cert_req != GNUTLS_CERT_REQUIRE)
			optional = 1;
	}

	ret = _gnutls_recv_handshake(session, GNUTLS_HANDSHAKE_CERTIFICATE_PKT, 0, &buf);
	if (ret < 0) {
		if (ret == GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET && session->internals.send_cert_req)
			return gnutls_assert_val(GNUTLS_E_NO_CERTIFICATE_FOUND);

		return gnutls_assert_val(ret);
	}

	if (buf.length == 0) {
		gnutls_assert();
		ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
		goto cleanup;
	}

	if (session->internals.initial_negotiation_completed &&
	    session->internals.post_handshake_cr_context.size > 0) {
		gnutls_datum_t context;

		/* verify whether the context matches */
		ret = _gnutls_buffer_pop_datum_prefix8(&buf, &context);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (context.size != session->internals.post_handshake_cr_context.size ||
		    memcmp(context.data, session->internals.post_handshake_cr_context.data,
		           context.size) != 0) {
			ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			gnutls_assert();
			goto cleanup;
		}
	} else {
		if (buf.data[0] != 0) {
			/* The context field must be empty during handshake */
			gnutls_assert();
			ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			goto cleanup;
		}

		/* buf.length is positive */
		buf.data++;
		buf.length--;
	}

	_gnutls_handshake_log("HSK[%p]: parsing certificate message\n", session);

	ret = parse_cert_list(session, buf.data, buf.length);
	if (ret < 0) {
		if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND) {
			if (optional)
				ret = 0;
			else if (session->security_parameters.entity ==
				 GNUTLS_SERVER)
				ret = GNUTLS_E_CERTIFICATE_REQUIRED;
		}
		gnutls_assert();
		goto cleanup;
	}

	session->internals.hsk_flags |= HSK_CRT_VRFY_EXPECTED;

	ret = 0;
cleanup:

	_gnutls_buffer_clear(&buf);
	return ret;
}

struct ocsp_req_ctx_st {
	gnutls_pcert_st *pcert;
	unsigned cert_index;
	gnutls_session_t session;
	gnutls_certificate_credentials_t cred;
};

static
int append_status_request(void *_ctx, gnutls_buffer_st *buf)
{
	struct ocsp_req_ctx_st *ctx = _ctx;
	gnutls_session_t session = ctx->session;
	int ret;
	gnutls_datum_t resp;
	unsigned free_resp = 0;

	assert(session->internals.selected_ocsp_func != NULL ||
	       session->internals.selected_ocsp_length != 0);

	/* The global ocsp callback function can only be used to return
	 * a single certificate request */
	if (session->internals.selected_ocsp_length == 1 && ctx->cert_index != 0)
		return 0;

	if (session->internals.selected_ocsp_length > 0) {
		if (ctx->cert_index < session->internals.selected_ocsp_length) {
			if ((session->internals.selected_ocsp[ctx->cert_index].exptime != 0 &&
			    gnutls_time(0) >= session->internals.selected_ocsp[ctx->cert_index].exptime) ||
			    session->internals.selected_ocsp[ctx->cert_index].response.data == NULL) {
				return 0;
			}

			resp.data = session->internals.selected_ocsp[ctx->cert_index].response.data;
			resp.size = session->internals.selected_ocsp[ctx->cert_index].response.size;
			ret = 0;
		} else {
			return 0;
		}
	} else if (session->internals.selected_ocsp_func) {
		if (ctx->cert_index == 0) {
			ret = session->internals.selected_ocsp_func(session, session->internals.selected_ocsp_func_ptr, &resp);
			free_resp = 1;
		} else {
			return 0;
		}
	} else
		return 0;

	if (ret == GNUTLS_E_NO_CERTIFICATE_STATUS || resp.data == 0) {
		return 0;
	} else if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	ret = _gnutls_buffer_append_data(buf, "\x01", 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_buffer_append_data_prefix(buf, 24, resp.data, resp.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	if (free_resp)
		gnutls_free(resp.data);
	return ret;
}

int _gnutls13_send_certificate(gnutls_session_t session, unsigned again)
{
	int ret;
	gnutls_pcert_st *apr_cert_list = NULL;
	gnutls_privkey_t apr_pkey = NULL;
	int apr_cert_list_length = 0;
	mbuffer_st *bufel = NULL;
	gnutls_buffer_st buf;
	unsigned pos_mark, ext_pos_mark;
	unsigned i;
	struct ocsp_req_ctx_st ctx;
	gnutls_certificate_credentials_t cred;

	if (again == 0) {
		if (!session->internals.initial_negotiation_completed &&
		    session->internals.hsk_flags & HSK_PSK_SELECTED)
			return 0;

		if (session->security_parameters.entity == GNUTLS_SERVER &&
		    session->internals.resumed)
			return 0;

		cred = (gnutls_certificate_credentials_t)
		    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
		if (cred == NULL) {
			gnutls_assert();
			return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
		}

		if (session->security_parameters.entity == GNUTLS_CLIENT &&
		    !(session->internals.hsk_flags & HSK_CRT_ASKED)) {
			return 0;
		}

		ret = _gnutls_get_selected_cert(session, &apr_cert_list,
						&apr_cert_list_length, &apr_pkey);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (session->security_parameters.entity == GNUTLS_CLIENT) {
			ret = _gnutls_buffer_append_data_prefix(&buf, 8,
								session->internals.post_handshake_cr_context.data,
								session->internals.post_handshake_cr_context.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

		} else {
			ret = _gnutls_buffer_append_prefix(&buf, 8, 0);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}

		/* mark total size */
		pos_mark = buf.length;
		ret = _gnutls_buffer_append_prefix(&buf, 24, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		for (i=0;i<(unsigned)apr_cert_list_length;i++) {
			ret = _gnutls_buffer_append_data_prefix(&buf, 24,
								apr_cert_list[i].cert.data,
								apr_cert_list[i].cert.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

#ifdef ENABLE_OCSP
			if ((session->internals.selected_ocsp_length > 0 ||
			     session->internals.selected_ocsp_func) &&
			    _gnutls_hello_ext_is_present(session, GNUTLS_EXTENSION_STATUS_REQUEST)) {
				/* append status response if available */
				ret = _gnutls_extv_append_init(&buf);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}
				ext_pos_mark = ret;

				ctx.pcert = &apr_cert_list[i];
				ctx.cert_index = i;
				ctx.session = session;
				ctx.cred = cred;
				ret = _gnutls_extv_append(&buf, STATUS_REQUEST_TLS_ID,
							  &ctx, append_status_request);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}

				ret = _gnutls_extv_append_final(&buf, ext_pos_mark, 0);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}
			} else
#endif
			{
				ret = _gnutls_buffer_append_prefix(&buf, 16, 0);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}
			}
		}

		_gnutls_write_uint24(buf.length-pos_mark-3, &buf.data[pos_mark]);

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CERTIFICATE_PKT);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}

typedef struct crt_cert_ctx_st {
	gnutls_session_t session;
	gnutls_datum_t *ocsp;
	unsigned idx;
} crt_cert_ctx_st;

static int parse_cert_extension(void *_ctx, unsigned tls_id, const uint8_t *data, unsigned data_size)
{
	crt_cert_ctx_st *ctx = _ctx;
	gnutls_session_t session = ctx->session;
	int ret;

	if (tls_id == STATUS_REQUEST_TLS_ID) {
#ifdef ENABLE_OCSP
		if (!_gnutls_hello_ext_is_present(session, ext_mod_status_request.gid)) {
			gnutls_assert();
			goto unexpected;
		}

		_gnutls_handshake_log("Found OCSP response on cert %d\n", ctx->idx);

		ret = _gnutls_parse_ocsp_response(session, data, data_size, ctx->ocsp);
		if (ret < 0)
			return gnutls_assert_val(ret);
#endif
	} else {
		goto unexpected;
	}

	return 0;

 unexpected:
	_gnutls_debug_log("received unexpected certificate extension (%d)\n", (int)tls_id);
	return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
}

static int
parse_cert_list(gnutls_session_t session, uint8_t * data, size_t data_size)
{
	int len, ret;
	uint8_t *p = data;
	cert_auth_info_t info;
	gnutls_certificate_credentials_t cred;
	ssize_t dsize = data_size, size;
	int i;
	unsigned npeer_certs, npeer_ocsp, j;
	crt_cert_ctx_st ctx;
	gnutls_datum_t *peer_certs = NULL;
	gnutls_datum_t *peer_ocsp = NULL;
	unsigned nentries = 0;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret =
	     _gnutls_auth_info_init(session, GNUTLS_CRD_CERTIFICATE,
				   sizeof(cert_auth_info_st), 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	if (data == NULL || data_size == 0) {
		/* no certificate was sent */
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL)
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);

	DECR_LEN(dsize, 3);
	size = _gnutls_read_uint24(p);
	p += 3;

	if (size != dsize)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (size == 0)
		return gnutls_assert_val(GNUTLS_E_NO_CERTIFICATE_FOUND);

	i = dsize;

	while (i > 0) {
		DECR_LEN(dsize, 3);
		len = _gnutls_read_uint24(p);
		if (len == 0)
			return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		DECR_LEN(dsize, len);
		p += len + 3;
		i -= len + 3;

		DECR_LEN(dsize, 2);
		len = _gnutls_read_uint16(p);
		DECR_LEN(dsize, len);

		i -= len + 2;
		p += len + 2;

		nentries++;
	}

	if (dsize != 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	/* this is unnecessary - keeping to avoid a regression due to a re-org
	 * of the loop above */
	if (nentries == 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	npeer_ocsp = 0;
	npeer_certs = 0;

	/* Ok we now allocate the memory to hold the
	 * certificate list
	 */
	peer_certs = gnutls_calloc(nentries, sizeof(gnutls_datum_t));
	if (peer_certs == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	peer_ocsp = gnutls_calloc(nentries, sizeof(gnutls_datum_t));
	if (peer_ocsp == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		goto cleanup;
	}

	p = data+3;

	/* Now we start parsing the list (again).
	 * We don't use DECR_LEN since the list has
	 * been parsed before.
	 */

	ctx.session = session;

	for (j = 0; j < nentries; j++) {
		len = _gnutls_read_uint24(p);
		p += 3;

		ret = _gnutls_set_datum(&peer_certs[j], p, len);
		if (ret < 0) {
			gnutls_assert();
			ret = GNUTLS_E_CERTIFICATE_ERROR;
			goto cleanup;
		}
		npeer_certs++;

		p += len;

		len = _gnutls_read_uint16(p);

		ctx.ocsp = &peer_ocsp[j];
		ctx.idx = j;

		ret = _gnutls_extv_parse(&ctx, parse_cert_extension, p, len+2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		p += len+2;
		npeer_ocsp++;
	}

	/* The OCSP entries match the certificate entries, although
	 * the contents of each OCSP entry may be NULL.
	 */
	for(j=0;j<info->ncerts;j++)
		gnutls_free(info->raw_certificate_list[j].data);
	gnutls_free(info->raw_certificate_list);

	for(j=0;j<info->nocsp;j++)
		gnutls_free(info->raw_ocsp_list[j].data);
	gnutls_free(info->raw_ocsp_list);


	info->raw_certificate_list = peer_certs;
	info->ncerts = npeer_certs;

	info->raw_ocsp_list = peer_ocsp;
	info->nocsp = npeer_ocsp;

	return 0;

 cleanup:
	for(j=0;j<npeer_certs;j++)
		gnutls_free(peer_certs[j].data);

	for(j=0;j<npeer_ocsp;j++)
		gnutls_free(peer_ocsp[j].data);
	gnutls_free(peer_certs);
	gnutls_free(peer_ocsp);
	return ret;

}

