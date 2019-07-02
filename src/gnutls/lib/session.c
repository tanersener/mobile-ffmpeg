/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
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
#include "debug.h"
#include <session_pack.h>
#include <datum.h>
#include "buffers.h"
#include "state.h"
#include "ext/cert_types.h"
#include <minmax.h>

/**
 * gnutls_session_get_data:
 * @session: is a #gnutls_session_t type.
 * @session_data: is a pointer to space to hold the session.
 * @session_data_size: is the session_data's size, or it will be set by the function.
 *
 * Returns all session parameters needed to be stored to support resumption,
 * in a pre-allocated buffer.
 *
 * See gnutls_session_get_data2() for more information.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_session_get_data(gnutls_session_t session,
			void *session_data, size_t * session_data_size)
{

	gnutls_datum_t psession;
	int ret;

	ret = gnutls_session_get_data2(session, &psession);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (psession.size > *session_data_size) {
		*session_data_size = psession.size;
		ret = GNUTLS_E_SHORT_MEMORY_BUFFER;
		goto error;
	}
	*session_data_size = psession.size;

	if (session_data != NULL)
		memcpy(session_data, psession.data, psession.size);

	ret = 0;

      error:
	_gnutls_free_datum(&psession);
	return ret;
}

#define EMPTY_DATA "\x00\x00\x00\x00"
#define EMPTY_DATA_SIZE 4

/**
 * gnutls_session_get_data2:
 * @session: is a #gnutls_session_t type.
 * @data: is a pointer to a datum that will hold the session.
 *
 * Returns necessary parameters to support resumption. The client
 * should call this function and store the returned session data. A session
 * can be resumed later by calling gnutls_session_set_data() with the returned
 * data. Note that under TLS 1.3, it is recommended for clients to use
 * session parameters only once, to prevent passive-observers from correlating
 * the different connections.
 *
 * The returned @data are allocated and must be released using gnutls_free().
 *
 * This function will fail if called prior to handshake completion. In
 * case of false start TLS, the handshake completes only after data have
 * been successfully received from the peer.
 *
 * Under TLS1.3 session resumption is possible only after a session ticket
 * is received by the client. To ensure that such a ticket has been received use
 * gnutls_session_get_flags() and check for flag %GNUTLS_SFLAGS_SESSION_TICKET;
 * if this flag is not set, this function will wait for a new ticket within
 * an estimated rountrip, and if not received will return dummy data which
 * cannot lead to resumption.
 *
 * To get notified when new tickets are received by the server
 * use gnutls_handshake_set_hook_function() to wait for %GNUTLS_HANDSHAKE_NEW_SESSION_TICKET
 * messages. Each call of gnutls_session_get_data2() after a ticket is
 * received, will return session resumption data corresponding to the last
 * received ticket.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_session_get_data2(gnutls_session_t session, gnutls_datum_t *data)
{
	const version_entry_st *vers = get_version(session);
	int ret;

	if (data == NULL || vers == NULL) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	if (vers->tls13_sem && !(session->internals.hsk_flags & HSK_TICKET_RECEIVED)) {
		unsigned ertt = session->internals.ertt;
		/* use our estimation of round-trip + some time for the server to calculate
		 * the value(s). */
		ertt += 60;

		/* wait for a message with timeout */
		ret = _gnutls_recv_in_buffers(session, GNUTLS_APPLICATION_DATA, -1, ertt);
		if (ret < 0 && (gnutls_error_is_fatal(ret) && ret != GNUTLS_E_TIMEDOUT)) {
			return gnutls_assert_val(ret);
		}

		if (!(session->internals.hsk_flags & HSK_TICKET_RECEIVED)) {
			ret = _gnutls_set_datum(data, EMPTY_DATA, EMPTY_DATA_SIZE);
			if (ret < 0)
				return gnutls_assert_val(ret);

			return 0;
		}
	} else if (!vers->tls13_sem) {
		/* under TLS1.3 we want to pack the latest ticket, while that's
		 * not the case in TLS1.2 or earlier. */
		if (gnutls_session_is_resumed(session) && session->internals.resumption_data.data) {
			ret = _gnutls_set_datum(data, session->internals.resumption_data.data, session->internals.resumption_data.size);
			if (ret < 0)
				return gnutls_assert_val(ret);

			return 0;
		}
	}

	if (session->internals.resumable == RESUME_FALSE)
		return GNUTLS_E_INVALID_SESSION;

	ret = _gnutls_session_pack(session, data);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}


/**
 * gnutls_session_get_id:
 * @session: is a #gnutls_session_t type.
 * @session_id: is a pointer to space to hold the session id.
 * @session_id_size: initially should contain the maximum @session_id size and will be updated.
 *
 * Returns the TLS session identifier. The session ID is selected by the
 * server, and in older versions of TLS was a unique identifier shared
 * between client and server which was persistent across resumption.
 * In the latest version of TLS (1.3) or TLS with session tickets, the
 * notion of session identifiers is undefined and cannot be relied for uniquely
 * identifying sessions across client and server.
 *
 * In client side this function returns the identifier returned by the
 * server, and cannot be assumed to have any relation to session resumption.
 * In server side this function is guaranteed to return a persistent
 * identifier of the session since GnuTLS 3.6.4, which may not necessarily
 * map into the TLS session ID value. Prior to that version the value
 * could only be considered a persistent identifier, under TLS1.2 or earlier
 * and when no session tickets were in use.
 *
 * The session identifier value returned is always less than
 * %GNUTLS_MAX_SESSION_ID_SIZE.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_session_get_id(gnutls_session_t session,
		      void *session_id, size_t * session_id_size)
{
	size_t given_session_id_size = *session_id_size;

	*session_id_size = session->security_parameters.session_id_size;

	/* just return the session size */
	if (session_id == NULL) {
		return 0;
	}

	if (given_session_id_size <
	    session->security_parameters.session_id_size) {
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	memcpy(session_id, &session->security_parameters.session_id,
	       *session_id_size);

	return 0;
}

/**
 * gnutls_session_get_id2:
 * @session: is a #gnutls_session_t type.
 * @session_id: will point to the session ID.
 *
 * Returns the TLS session identifier. The session ID is selected by the
 * server, and in older versions of TLS was a unique identifier shared
 * between client and server which was persistent across resumption.
 * In the latest version of TLS (1.3) or TLS 1.2 with session tickets, the
 * notion of session identifiers is undefined and cannot be relied for uniquely
 * identifying sessions across client and server.
 *
 * In client side this function returns the identifier returned by the
 * server, and cannot be assumed to have any relation to session resumption.
 * In server side this function is guaranteed to return a persistent
 * identifier of the session since GnuTLS 3.6.4, which may not necessarily
 * map into the TLS session ID value. Prior to that version the value
 * could only be considered a persistent identifier, under TLS1.2 or earlier
 * and when no session tickets were in use.
 *
 * The session identifier value returned is always less than
 * %GNUTLS_MAX_SESSION_ID_SIZE and should be treated as constant.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.1.4
 **/
int
gnutls_session_get_id2(gnutls_session_t session,
		       gnutls_datum_t * session_id)
{
	session_id->size = session->security_parameters.session_id_size;
	session_id->data = session->security_parameters.session_id;

	return 0;
}

/**
 * gnutls_session_set_data:
 * @session: is a #gnutls_session_t type.
 * @session_data: is a pointer to space to hold the session.
 * @session_data_size: is the session's size
 *
 * Sets all session parameters, in order to resume a previously
 * established session.  The session data given must be the one
 * returned by gnutls_session_get_data().  This function should be
 * called before gnutls_handshake().
 *
 * Keep in mind that session resuming is advisory. The server may
 * choose not to resume the session, thus a full handshake will be
 * performed.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_session_set_data(gnutls_session_t session,
			const void *session_data, size_t session_data_size)
{
	int ret;
	gnutls_datum_t psession;

	psession.data = (uint8_t *) session_data;
	psession.size = session_data_size;

	if (session_data == NULL || session_data_size == 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* under TLS1.3 we always return some data on resumption when there
	 * is no ticket in order to keep compatibility with existing apps */
	if (session_data_size == EMPTY_DATA_SIZE &&
	    memcmp(session_data, EMPTY_DATA, EMPTY_DATA_SIZE) == 0) {
		return 0;
	}

	ret = _gnutls_session_unpack(session, &psession);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	session->internals.resumption_requested = 1;

	if (session->internals.resumption_data.data != NULL)
		gnutls_free(session->internals.resumption_data.data);
	_gnutls_set_datum(&session->internals.resumption_data, session_data, session_data_size);

	return 0;
}

/**
 * gnutls_session_force_valid:
 * @session: is a #gnutls_session_t type.
 *
 * Clears the invalid flag in a session. That means
 * that sessions were corrupt or invalid data were received 
 * can be re-used. Use only when debugging or experimenting
 * with the TLS protocol. Should not be used in typical
 * applications.
 *
 **/
void gnutls_session_force_valid(gnutls_session_t session)
{
	session->internals.invalid_connection = 0;
}

#define DESC_SIZE 96

/**
 * gnutls_session_get_desc:
 * @session: is a gnutls session
 *
 * This function returns a string describing the current session.
 * The string is null terminated and allocated using gnutls_malloc().
 *
 * If initial negotiation is not complete when this function is called,
 * %NULL will be returned.
 *
 * Returns: a description of the protocols and algorithms in the current session.
 *
 * Since: 3.1.10
 **/
char *gnutls_session_get_desc(gnutls_session_t session)
{
	gnutls_kx_algorithm_t kx;
	const char *kx_str, *sign_str;
	gnutls_certificate_type_t ctype_client, ctype_server;
	char kx_name[64] = "";
	char proto_name[32];
	char _group_name[24];
	const char *group_name = NULL;
	int dh_bits = 0;
	unsigned mac_id;
	unsigned sign_algo;
	char *desc;
	const struct gnutls_group_entry_st *group = get_group(session);
	const version_entry_st *ver = get_version(session);

	if (session->internals.initial_negotiation_completed == 0)
		return NULL;

	kx = session->security_parameters.cs->kx_algorithm;
	if (group)
		group_name = group->name;
#if defined(ENABLE_DHE) || defined(ENABLE_ANON)
	if (group_name == NULL && _gnutls_kx_is_dhe(kx)) {
		dh_bits = gnutls_dh_get_prime_bits(session);
		if (dh_bits > 0)
			snprintf(_group_name, sizeof(_group_name), "CUSTOM%u", dh_bits);
		else
			snprintf(_group_name, sizeof(_group_name), "CUSTOM");
		group_name = _group_name;
	}
#endif

	/* Key exchange    - Signature algorithm */
	/* DHE-3072        - RSA-PSS-2048        */
	/* ECDHE-SECP256R1 - ECDSA-SECP256R1     */

	sign_algo = gnutls_sign_algorithm_get(session);
	sign_str = gnutls_sign_get_name(sign_algo);

	if (kx == 0 && ver->tls13_sem) { /* TLS 1.3 */
		if (session->internals.hsk_flags & HSK_PSK_SELECTED) {
			if (group) {
				if (group->pk == GNUTLS_PK_DH)
					snprintf(kx_name, sizeof(kx_name), "(DHE-PSK-%s)",
						 group_name);
				else
					snprintf(kx_name, sizeof(kx_name), "(ECDHE-PSK-%s)",
						 group_name);
			} else {
					snprintf(kx_name, sizeof(kx_name), "(PSK)");
			}
		} else if (group && sign_str) {
			if (group->curve)
				snprintf(kx_name, sizeof(kx_name), "(ECDHE-%s)-(%s)",
					 group_name, sign_str);
			else
				snprintf(kx_name, sizeof(kx_name), "(DHE-%s)-(%s)",
					 group_name, sign_str);
		}
	} else {
		kx_str = gnutls_kx_get_name(kx);
		if (kx_str == NULL) {
			gnutls_assert();
			return NULL;
		}

		if (kx == GNUTLS_KX_ECDHE_ECDSA || kx == GNUTLS_KX_ECDHE_RSA || 
		    kx == GNUTLS_KX_ECDHE_PSK) {
			if (sign_str)
				snprintf(kx_name, sizeof(kx_name), "(ECDHE-%s)-(%s)",
					 group_name, sign_str);
			else
				snprintf(kx_name, sizeof(kx_name), "(ECDHE-%s)",
					 group_name);
		} else if (kx == GNUTLS_KX_DHE_DSS || kx == GNUTLS_KX_DHE_RSA || 
		    kx == GNUTLS_KX_DHE_PSK) {
			if (sign_str)
				snprintf(kx_name, sizeof(kx_name), "(DHE-%s)-(%s)", group_name, sign_str);
			else
				snprintf(kx_name, sizeof(kx_name), "(DHE-%s)", group_name);
		} else if (kx == GNUTLS_KX_RSA) {
			/* Possible enhancement: include the certificate bits */
			snprintf(kx_name, sizeof(kx_name), "(RSA)");
		} else {
			snprintf(kx_name, sizeof(kx_name), "(%s)",
				 kx_str);
		}
	}

	if (are_alternative_cert_types_allowed(session)) {
		// Get certificate types
		ctype_client = get_certificate_type(session, GNUTLS_CTYPE_CLIENT);
		ctype_server = get_certificate_type(session, GNUTLS_CTYPE_SERVER);

		if (ctype_client == ctype_server) {
			// print proto version, client/server cert type
			snprintf(proto_name, sizeof(proto_name), "%s-%s",
				 gnutls_protocol_get_name(get_num_version(session)),
				 gnutls_certificate_type_get_name(ctype_client));
		} else {
			// print proto version, client cert type, server cert type
			snprintf(proto_name, sizeof(proto_name), "%s-%s-%s",
				 gnutls_protocol_get_name(get_num_version(session)),
				 gnutls_certificate_type_get_name(ctype_client),
				 gnutls_certificate_type_get_name(ctype_server));
		}
	} else { // Assumed default certificate type (X.509)
		snprintf(proto_name, sizeof(proto_name), "%s",
				 gnutls_protocol_get_name(get_num_version(session)));
	}

	desc = gnutls_malloc(DESC_SIZE);
	if (desc == NULL)
		return NULL;

	mac_id = gnutls_mac_get(session);
	if (mac_id == GNUTLS_MAC_AEAD) { /* no need to print */
		snprintf(desc, DESC_SIZE,
			 "(%s)-%s-(%s)",
			 proto_name,
			 kx_name,
			 gnutls_cipher_get_name(gnutls_cipher_get(session)));
	} else {
		snprintf(desc, DESC_SIZE,
			 "(%s)-%s-(%s)-(%s)",
			 proto_name,
			 kx_name,
			 gnutls_cipher_get_name(gnutls_cipher_get(session)),
			 gnutls_mac_get_name(mac_id));
	}

	return desc;
}

/**
 * gnutls_session_set_id:
 * @session: is a #gnutls_session_t type.
 * @sid: the session identifier
 *
 * This function sets the session ID to be used in a client hello.
 * This is a function intended for exceptional uses. Do not use this
 * function unless you are implementing a custom protocol.
 *
 * To set session resumption parameters use gnutls_session_set_data() instead.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.2.1
 **/
int
gnutls_session_set_id(gnutls_session_t session, const gnutls_datum_t * sid)
{
	if (session->security_parameters.entity == GNUTLS_SERVER ||
	    sid->size > GNUTLS_MAX_SESSION_ID_SIZE)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memset(&session->internals.resumed_security_parameters, 0,
	       sizeof(session->internals.resumed_security_parameters));

	session->internals.resumed_security_parameters.session_id_size =
	    sid->size;
	memcpy(session->internals.resumed_security_parameters.session_id,
	       sid->data, sid->size);

	return 0;
}
