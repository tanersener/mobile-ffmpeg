/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */
#include "gnutls_int.h"
#include "errors.h"
#include "debug.h"
#include <session_pack.h>
#include <datum.h>

/**
 * gnutls_session_get_data:
 * @session: is a #gnutls_session_t type.
 * @session_data: is a pointer to space to hold the session.
 * @session_data_size: is the session_data's size, or it will be set by the function.
 *
 * Returns all session parameters needed to be stored to support resumption.
 * The client should call this, and store the returned session data. A session
 * may be resumed later by calling gnutls_session_set_data().  
 *
 * This function will fail if called prior to handshake completion. In
 * case of false start TLS, the handshake completes only after data have
 * been successfully received from the peer.
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

/**
 * gnutls_session_get_data2:
 * @session: is a #gnutls_session_t type.
 * @data: is a pointer to a datum that will hold the session.
 *
 * Returns all session parameters needed to be stored to support resumption.
 * The client should call this, and store the returned session data. A session
 * may be resumed later by calling gnutls_session_set_data().  
 *
 * The returned @data are allocated and must be released using gnutls_free().
 *
 * This function will fail if called prior to handshake completion. In
 * case of false start TLS, the handshake completes only after data have
 * been successfully received from the peer.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_session_get_data2(gnutls_session_t session, gnutls_datum_t *data)
{

	int ret;

	if (data == NULL) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	if (gnutls_session_is_resumed(session) && session->internals.resumption_data.data) {
		ret = _gnutls_set_datum(data, session->internals.resumption_data.data, session->internals.resumption_data.size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return 0;
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
 * Returns the current session ID. This can be used if you want to
 * check if the next session you tried to resume was actually
 * resumed.  That is because resumed sessions share the same session ID
 * with the original session.
 *
 * The session ID is selected by the server, that identify the
 * current session.  In all supported TLS protocols, the session id
 * is less than %GNUTLS_MAX_SESSION_ID_SIZE.
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
 * Returns the current session ID. The returned data should be
 * treated as constant.
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

#define DESC_SIZE 64

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
	const char *kx_str;
	unsigned type;
	char kx_name[32];
	char proto_name[32];
	const char *curve_name = NULL;
	unsigned dh_bits = 0;
	unsigned mac_id;
	char *desc;

	if (session->internals.initial_negotiation_completed == 0)
		return NULL;

	kx = session->security_parameters.kx_algorithm;

	if (kx == GNUTLS_KX_ANON_ECDH || kx == GNUTLS_KX_ECDHE_PSK ||
	    kx == GNUTLS_KX_ECDHE_RSA || kx == GNUTLS_KX_ECDHE_ECDSA) {
		curve_name =
		    gnutls_ecc_curve_get_name(gnutls_ecc_curve_get
					      (session));
#if defined(ENABLE_DHE) || defined(ENABLE_ANON)
	} else if (kx == GNUTLS_KX_ANON_DH || kx == GNUTLS_KX_DHE_PSK
		   || kx == GNUTLS_KX_DHE_RSA || kx == GNUTLS_KX_DHE_DSS) {
		dh_bits = gnutls_dh_get_prime_bits(session);
#endif
	}

	kx_str = gnutls_kx_get_name(kx);
	if (kx_str) {
		if (curve_name != NULL)
			snprintf(kx_name, sizeof(kx_name), "%s-%s",
				 kx_str, curve_name);
		else if (dh_bits != 0)
			snprintf(kx_name, sizeof(kx_name), "%s-%u",
				 kx_str, dh_bits);
		else
			snprintf(kx_name, sizeof(kx_name), "%s",
				 kx_str);
	} else {
		strcpy(kx_name, "NULL");
	}

	type = gnutls_certificate_type_get(session);
	if (type == GNUTLS_CRT_X509)
		snprintf(proto_name, sizeof(proto_name), "%s",
			 gnutls_protocol_get_name(get_num_version
						  (session)));
	else
		snprintf(proto_name, sizeof(proto_name), "%s-%s",
			 gnutls_protocol_get_name(get_num_version
						  (session)),
			 gnutls_certificate_type_get_name(type));

	desc = gnutls_malloc(DESC_SIZE);
	if (desc == NULL)
		return NULL;

	mac_id = gnutls_mac_get(session);
	if (mac_id == GNUTLS_MAC_AEAD) { /* no need to print */
		snprintf(desc, DESC_SIZE,
			 "(%s)-(%s)-(%s)",
			 proto_name,
			 kx_name,
			 gnutls_cipher_get_name(gnutls_cipher_get(session)));
	} else {
		snprintf(desc, DESC_SIZE,
			 "(%s)-(%s)-(%s)-(%s)",
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
