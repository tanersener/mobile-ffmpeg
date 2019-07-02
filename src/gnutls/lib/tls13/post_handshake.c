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

/* Functions that relate to the TLS handshake procedure.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "dh.h"
#include "debug.h"
#include "algorithms.h"
#include "cipher.h"
#include "buffers.h"
#include "mbuffers.h"
#include "kx.h"
#include "handshake.h"
#include "num.h"
#include "hash_int.h"
#include "db.h"
#include "hello_ext.h"
#include "supplemental.h"
#include "auth.h"
#include "sslv2_compat.h"
#include <auth/cert.h>
#include "constate.h"
#include <record.h>
#include <state.h>
#include <random.h>
#include <dtls.h>
#include "tls13/certificate_request.h"
#include "tls13/certificate_verify.h"
#include "tls13/certificate.h"
#include "tls13/finished.h"

#undef AGAIN
#define AGAIN(x) ((x)==(REAUTH_STATE))

/*
 * _gnutls13_reauth_client
 * This function performs the client side of the post-handshake authentication
 */
static
int _gnutls13_reauth_client(gnutls_session_t session)
{
	int ret = 0;
	size_t tmp;

	if (!session->internals.initial_negotiation_completed)
		return gnutls_assert_val(GNUTLS_E_UNAVAILABLE_DURING_HANDSHAKE);

	if (!(session->internals.flags & GNUTLS_POST_HANDSHAKE_AUTH))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (session->internals.reauth_buffer.length == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	switch (REAUTH_STATE) {
	case REAUTH_STATE0:

		/* restore handshake transcript */
		_gnutls_buffer_reset(&session->internals.handshake_hash_buffer);
		ret = gnutls_buffer_append_data(&session->internals.handshake_hash_buffer,
						session->internals.post_handshake_hash_buffer.data,
						session->internals.post_handshake_hash_buffer.length);
		if (ret < 0)
			return gnutls_assert_val(ret);

		/* append the previously received certificate request message, to the
		 * transcript. */
		ret = gnutls_buffer_append_data(&session->internals.handshake_hash_buffer,
						session->internals.reauth_buffer.data,
						session->internals.reauth_buffer.length);
		if (ret < 0)
			return gnutls_assert_val(ret);

		session->internals.handshake_hash_buffer_prev_len = session->internals.handshake_hash_buffer.length;

		/* skip the reauth buffer handshake message headers */
		ret = _gnutls_buffer_pop_prefix32(&session->internals.reauth_buffer, &tmp, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);

		FALLTHROUGH;
	case REAUTH_STATE1:
		ret = _gnutls13_recv_certificate_request_int(session,
							     &session->internals.reauth_buffer);
		REAUTH_STATE = REAUTH_STATE1;
		IMED_RET("recv certificate request", ret, 0);
		FALLTHROUGH;
	case REAUTH_STATE2:
		ret = _gnutls13_send_certificate(session, AGAIN(REAUTH_STATE2));
		REAUTH_STATE = REAUTH_STATE2;
		IMED_RET("send certificate", ret, 0);
		FALLTHROUGH;
	case REAUTH_STATE3:
		ret = _gnutls13_send_certificate_verify(session, AGAIN(REAUTH_STATE3));
		REAUTH_STATE = REAUTH_STATE3;
		IMED_RET("send certificate verify", ret, 0);
		FALLTHROUGH;
	case REAUTH_STATE4:
		ret = _gnutls13_send_finished(session, AGAIN(REAUTH_STATE4));
		REAUTH_STATE = REAUTH_STATE4;
		IMED_RET("send finished", ret, 0);
		break;
	default:
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	_gnutls_handshake_hash_buffers_clear(session);
	_gnutls_buffer_reset(&session->internals.reauth_buffer);
	REAUTH_STATE = REAUTH_STATE0;

	return 0;
}

/*
 * _gnutls13_reauth_server
 * This function does the server stuff of the post-handshake authentication.
 */
static
int _gnutls13_reauth_server(gnutls_session_t session)
{
	int ret = 0;

	if (session->security_parameters.post_handshake_auth == 0 ||
	    (!(session->internals.flags & GNUTLS_POST_HANDSHAKE_AUTH)))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (session->internals.send_cert_req == 0) {
		_gnutls_debug_log("You need to call gnutls_certificate_server_set_request to enable post handshake auth\n");
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	switch (REAUTH_STATE) {
	case REAUTH_STATE0:
		/* restore handshake transcript */
		_gnutls_buffer_reset(&session->internals.handshake_hash_buffer);
		ret = gnutls_buffer_append_data(&session->internals.handshake_hash_buffer,
						session->internals.post_handshake_hash_buffer.data,
						session->internals.post_handshake_hash_buffer.length);
		if (ret < 0)
			return gnutls_assert_val(ret);

		session->internals.handshake_hash_buffer_prev_len = session->internals.handshake_hash_buffer.length;

		FALLTHROUGH;
	case REAUTH_STATE1:
		ret = _gnutls13_send_certificate_request(session, AGAIN(REAUTH_STATE1));
		REAUTH_STATE = REAUTH_STATE1;
		IMED_RET("send certificate request", ret, 0);
		FALLTHROUGH;
	case REAUTH_STATE2:
		/* here we should tolerate application data */
		ret = _gnutls13_recv_certificate(session);
		REAUTH_STATE = REAUTH_STATE2;
		IMED_RET("recv certificate", ret, 0);
		FALLTHROUGH;
	case REAUTH_STATE3:
		ret = _gnutls13_recv_certificate_verify(session);
		REAUTH_STATE = REAUTH_STATE3;
		IMED_RET("recv certificate verify", ret, 0);
		FALLTHROUGH;
	case REAUTH_STATE4:
		ret = _gnutls_run_verify_callback(session, GNUTLS_CLIENT);
		REAUTH_STATE = REAUTH_STATE4;
		if (ret < 0)
			return gnutls_assert_val(ret);
		FALLTHROUGH;
	case REAUTH_STATE5:
		ret = _gnutls13_recv_finished(session);
		REAUTH_STATE = REAUTH_STATE5;
		IMED_RET("recv finished", ret, 0);
		break;
	default:
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	_gnutls_handshake_hash_buffers_clear(session);
	REAUTH_STATE = REAUTH_STATE0;

	return 0;
}

/**
 * gnutls_reauth:
 * @session: is a #gnutls_session_t type.
 * @flags: must be zero
 *
 * This function performs the post-handshake authentication
 * for TLS 1.3. The post-handshake authentication is initiated by the server
 * by calling this function. Clients respond when %GNUTLS_E_REAUTH_REQUEST
 * has been seen while receiving data.
 *
 * The non-fatal errors expected by this function are:
 * %GNUTLS_E_INTERRUPTED, %GNUTLS_E_AGAIN, as well as
 * %GNUTLS_E_GOT_APPLICATION_DATA when called on server side.
 *
 * The former two interrupt the authentication procedure due to the transport
 * layer being interrupted, and the latter because there were pending data prior
 * to peer initiating the re-authentication. The server should read/process that
 * data as unauthenticated and retry calling gnutls_reauth().
 *
 * When this function is called under TLS1.2 or earlier or the peer didn't
 * advertise post-handshake auth, it always fails with
 * %GNUTLS_E_INVALID_REQUEST. The verification of the received peers certificate
 * is delegated to the session or credentials verification callbacks. A
 * server can check whether post handshake authentication is supported
 * by the client by checking the session flags with gnutls_session_get_flags().
 *
 * Prior to calling this function in server side, the function
 * gnutls_certificate_server_set_request() must be called setting expectations
 * for the received certificate (request or require). If none are set
 * this function will return with %GNUTLS_E_INVALID_REQUEST.
 *
 * Note that post handshake authentication is available irrespective
 * of the initial negotiation type (PSK or certificate). In all cases
 * however, certificate credentials must be set to the session prior
 * to calling this function.
 *
 * Returns: %GNUTLS_E_SUCCESS on a successful authentication, otherwise a negative error code.
 **/
int gnutls_reauth(gnutls_session_t session, unsigned int flags)
{
	const version_entry_st *vers = get_version(session);

	if (unlikely(!vers->tls13_sem))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (session->security_parameters.entity == GNUTLS_SERVER)
		return _gnutls13_reauth_server(session);
	else
		return _gnutls13_reauth_client(session);
}
