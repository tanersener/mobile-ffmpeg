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
#include "handshake.h"
#include "auth/cert.h"
#include "ext/signature.h"
#include "algorithms.h"
#include "tls13-sig.h"
#include "mbuffers.h"
#include "tls13/certificate_verify.h"

#define SRV_CTX "TLS 1.3, server CertificateVerify"
static const gnutls_datum_t srv_ctx = {
	(void*)SRV_CTX, sizeof(SRV_CTX)-1
};

#define CLI_CTX "TLS 1.3, client CertificateVerify"
static const gnutls_datum_t cli_ctx = {
	(void*)CLI_CTX, sizeof(CLI_CTX)-1
};

int _gnutls13_recv_certificate_verify(gnutls_session_t session)
{
	int ret;
	gnutls_buffer_st buf;
	const gnutls_sign_entry_st *se;
	gnutls_datum_t sig_data;
	gnutls_certificate_credentials_t cred;
	unsigned vflags;
	gnutls_pcert_st peer_cert;
	cert_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	bool server = 0;
	gnutls_certificate_type_t cert_type;

	memset(&peer_cert, 0, sizeof(peer_cert));

	/* this message is only expected if we have received
	 * a certificate message */
	if (!(session->internals.hsk_flags & HSK_CRT_VRFY_EXPECTED))
		return 0;

	if (session->security_parameters.entity == GNUTLS_SERVER)
		server = 1;

	cred = (gnutls_certificate_credentials_t)
		_gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (unlikely(cred == NULL))
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	if (unlikely(info == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	ret = _gnutls_recv_handshake(session, GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY, 0, &buf);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_handshake_log("HSK[%p]: Parsing certificate verify\n", session);

	if (buf.length < 2) {
		gnutls_assert();
		ret = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
		goto cleanup;
	}

	se = _gnutls_tls_aid_to_sign_entry(buf.data[0], buf.data[1], get_version(session));
	if (se == NULL) {
		_gnutls_handshake_log("Found unsupported signature (%d.%d)\n", (int)buf.data[0], (int)buf.data[1]);
		ret = gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
		goto cleanup;
	}

	if (server)
		gnutls_sign_algorithm_set_client(session, se->id);
	else
		gnutls_sign_algorithm_set_server(session, se->id);

	buf.data+=2;
	buf.length-=2;

	/* we check during verification whether the algorithm is enabled */

	ret = _gnutls_buffer_pop_datum_prefix16(&buf, &sig_data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (sig_data.size == 0) {
		gnutls_assert();
		ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
		goto cleanup;
	}

	/* We verify the certificate of the peer. Therefore we need to
	 * retrieve the negotiated certificate type for the peer. */
	cert_type = get_certificate_type(session, GNUTLS_CTYPE_PEERS);

	/* Verify the signature */
	ret = _gnutls_get_auth_info_pcert(&peer_cert, cert_type, info);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	vflags = cred->verify_flags | session->internals.additional_verify_flags;

	ret = _gnutls13_handshake_verify_data(session, vflags, &peer_cert,
					      server?(&cli_ctx):(&srv_ctx),
					      &sig_data, se);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (buf.length > 0) {
		gnutls_assert();
		ret = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_pcert_deinit(&peer_cert);
	_gnutls_buffer_clear(&buf);
	return ret;
}

int _gnutls13_send_certificate_verify(gnutls_session_t session, unsigned again)
{
	int ret;
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;
	mbuffer_st *bufel = NULL;
	gnutls_buffer_st buf;
	gnutls_datum_t sig = {NULL, 0};
	gnutls_sign_algorithm_t algo;
	const gnutls_sign_entry_st *se;
	bool server = 0;

	if (again == 0) {
		if (!session->internals.initial_negotiation_completed &&
		    session->internals.hsk_flags & HSK_PSK_SELECTED)
			return 0;

		if (session->security_parameters.entity == GNUTLS_SERVER &&
		    session->internals.resumed)
			return 0;

		if (session->security_parameters.entity == GNUTLS_SERVER)
			server = 1;

		ret = _gnutls_get_selected_cert(session, &apr_cert_list,
						&apr_cert_list_length, &apr_pkey);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (apr_cert_list_length == 0) {
			if (server) {
				return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
			} else {
				/* for client, this means either we
				 * didn't get a cert request or we are
				 * declining authentication; in either
				 * case we don't send a cert verify */
				return 0;
			}
		}

		if (server) {
			algo = _gnutls_session_get_sign_algo(session, &apr_cert_list[0], apr_pkey, 0, GNUTLS_KX_UNKNOWN);
			if (algo == GNUTLS_SIGN_UNKNOWN)
				return gnutls_assert_val(GNUTLS_E_INCOMPATIBLE_SIG_WITH_KEY);

			gnutls_sign_algorithm_set_server(session, algo);
		} else {
			/* for client, signature algorithm is already
			 * determined from Certificate Request */
			algo = gnutls_sign_algorithm_get_client(session);
			if (unlikely(algo == GNUTLS_SIGN_UNKNOWN))
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		}

		se = _gnutls_sign_to_entry(algo);

		ret = _gnutls13_handshake_sign_data(session, &apr_cert_list[0], apr_pkey,
						    server?(&srv_ctx):(&cli_ctx),
						    &sig, se);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_buffer_append_data(&buf, se->aid.id, 2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_buffer_append_data_prefix(&buf, 16, sig.data, sig.size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);

		gnutls_free(sig.data);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY);

 cleanup:
	gnutls_free(sig.data);
	_gnutls_buffer_clear(&buf);
	return ret;
}
