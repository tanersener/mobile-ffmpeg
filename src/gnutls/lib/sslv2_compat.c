/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

/* Functions to parse the SSLv2.0 hello message.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "dh.h"
#include "debug.h"
#include "algorithms.h"
#include "cipher.h"
#include "buffers.h"
#include "kx.h"
#include "handshake.h"
#include "num.h"
#include "hash_int.h"
#include "db.h"
#include "hello_ext.h"
#include "auth.h"
#include "sslv2_compat.h"
#include "constate.h"

#ifdef ENABLE_SSL2
/* This selects the best supported ciphersuite from the ones provided */
static int
_gnutls_handshake_select_v2_suite(gnutls_session_t session,
				  uint8_t * data, unsigned int datalen)
{
	unsigned int i, j;
	int ret;
	uint8_t *_data;
	int _datalen;

	_gnutls_handshake_log
	    ("HSK[%p]: Parsing a version 2.0 client hello.\n", session);

	if (datalen % 3 != 0) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	_data = gnutls_malloc(datalen);
	if (_data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	i = _datalen = 0;
	for (j = 0; j < datalen; j += 3) {
		if (data[j] == 0) {
			memcpy(&_data[i], &data[j + 1], 2);
			i += 2;
			_datalen += 2;
		}
	}

	ret = _gnutls_server_select_suite(session, _data, _datalen, 0);
	gnutls_free(_data);

	return ret;

}


/* Read a v2 client hello. Some browsers still use that beast!
 * However they set their version to 3.0 or 3.1.
 */
int
_gnutls_read_client_hello_v2(gnutls_session_t session, uint8_t * data,
			     unsigned int len)
{
	uint16_t session_id_len = 0;
	int pos = 0;
	int ret = 0, sret = 0;
	uint16_t sizeOfSuites;
	uint8_t rnd[GNUTLS_RANDOM_SIZE], major, minor;
	int neg_version;
	const version_entry_st *vers;
	uint16_t challenge;
	uint8_t session_id[GNUTLS_MAX_SESSION_ID_SIZE];

	DECR_LEN(len, 2);

	_gnutls_handshake_log
	    ("HSK[%p]: SSL 2.0 Hello: Client's version: %d.%d\n", session,
	     data[pos], data[pos + 1]);

	major = data[pos];
	minor = data[pos + 1];
	set_adv_version(session, major, minor);

	ret = _gnutls_negotiate_version(session, major, minor, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	vers = get_version(session);
	if (vers == NULL)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	neg_version = vers->id;

	pos += 2;

	/* Read uint16_t cipher_spec_length */
	DECR_LEN(len, 2);
	sizeOfSuites = _gnutls_read_uint16(&data[pos]);
	pos += 2;

	/* read session id length */
	DECR_LEN(len, 2);
	session_id_len = _gnutls_read_uint16(&data[pos]);
	pos += 2;

	if (session_id_len > GNUTLS_MAX_SESSION_ID_SIZE) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	/* read challenge length */
	DECR_LEN(len, 2);
	challenge = _gnutls_read_uint16(&data[pos]);
	pos += 2;

	if (challenge < 16 || challenge > GNUTLS_RANDOM_SIZE) {
		gnutls_assert();
		return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
	}

	/* call the user hello callback
	 */
	ret = _gnutls_user_hello_func(session, major, minor);
	if (ret < 0) {
		if (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED) {
			sret = GNUTLS_E_INT_RET_0;
		} else {
			gnutls_assert();
			return ret;
		}
	}

	/* find an appropriate cipher suite */

	DECR_LEN(len, sizeOfSuites);
	ret =
	    _gnutls_handshake_select_v2_suite(session, &data[pos],
					      sizeOfSuites);

	pos += sizeOfSuites;
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* check if the credentials (username, public key etc.) are ok
	 */
	if (_gnutls_get_kx_cred
	    (session,
	     session->security_parameters.cs->kx_algorithm) == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	/* set the mod_auth_st to the appropriate struct
	 * according to the KX algorithm. This is needed since all the
	 * handshake functions are read from there;
	 */
	session->internals.auth_struct =
	    _gnutls_kx_auth_struct(session->security_parameters.
				    cs->kx_algorithm);
	if (session->internals.auth_struct == NULL) {

		_gnutls_handshake_log
		    ("HSK[%p]: SSL 2.0 Hello: Cannot find the appropriate handler for the KX algorithm\n",
		     session);

		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	/* read random new values -skip session id for now */
	DECR_LEN(len, session_id_len);	/* skip session id for now */
	memcpy(session_id, &data[pos], session_id_len);
	pos += session_id_len;

	DECR_LEN(len, challenge);
	memset(rnd, 0, GNUTLS_RANDOM_SIZE);

	memcpy(&rnd[GNUTLS_RANDOM_SIZE - challenge], &data[pos],
	       challenge);

	_gnutls_set_client_random(session, rnd);

	/* generate server random value */
	ret = _gnutls_gen_server_random(session, neg_version);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->security_parameters.timestamp = gnutls_time(NULL);


	/* RESUME SESSION */

	DECR_LEN(len, session_id_len);
	ret =
	    _gnutls_server_restore_session(session, session_id,
					   session_id_len);

	if (ret == 0) {		/* resumed! */
		/* get the new random values */
		memcpy(session->internals.resumed_security_parameters.
		       server_random,
		       session->security_parameters.server_random,
		       GNUTLS_RANDOM_SIZE);
		memcpy(session->internals.resumed_security_parameters.
		       client_random,
		       session->security_parameters.client_random,
		       GNUTLS_RANDOM_SIZE);

		session->internals.resumed = RESUME_TRUE;
		return 0;
	} else {
		ret = _gnutls_generate_session_id(
			session->security_parameters.session_id,
			&session->security_parameters.session_id_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		session->internals.resumed = RESUME_FALSE;
	}

	return sret;
}
#endif
