/*
 * Copyright (C) 2005-2012 Free Software Foundation, Inc.
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

#ifdef ENABLE_PSK

#include "errors.h"
#include "auth.h"
#include "debug.h"
#include "num.h"
#include <auth/psk.h>
#include <auth/psk_passwd.h>
#include <str.h>
#include <datum.h>


static int _gnutls_proc_psk_client_kx(gnutls_session_t, uint8_t *, size_t);
static int
_gnutls_proc_psk_server_kx(gnutls_session_t session, uint8_t * data,
			   size_t _data_size);


const mod_auth_st psk_auth_struct = {
	"PSK",
	NULL,
	NULL,
	_gnutls_gen_psk_server_kx,
	_gnutls_gen_psk_client_kx,
	NULL,
	NULL,

	NULL,
	NULL,			/* certificate */
	_gnutls_proc_psk_server_kx,
	_gnutls_proc_psk_client_kx,
	NULL,
	NULL
};

/* Set the PSK premaster secret.
 */
int
_gnutls_set_psk_session_key(gnutls_session_t session,
			    gnutls_datum_t * ppsk /* key */ ,
			    gnutls_datum_t * dh_secret)
{
	gnutls_datum_t pwd_psk = { NULL, 0 };
	size_t dh_secret_size;
	uint8_t *p;
	int ret;

	if (dh_secret == NULL)
		dh_secret_size = ppsk->size;
	else
		dh_secret_size = dh_secret->size;

	/* set the session key
	 */
	session->key.key.size = 4 + dh_secret_size + ppsk->size;
	session->key.key.data = gnutls_malloc(session->key.key.size);
	if (session->key.key.data == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto error;
	}

	/* format of the premaster secret:
	 * (uint16_t) psk_size
	 * psk_size bytes of (0)s
	 * (uint16_t) psk_size
	 * the psk
	 */
	p = session->key.key.data;
	_gnutls_write_uint16(dh_secret_size, p);
	p += 2;
	if (dh_secret == NULL)
		memset(p, 0, dh_secret_size);
	else
		memcpy(p, dh_secret->data, dh_secret->size);

	p += dh_secret_size;
	_gnutls_write_uint16(ppsk->size, p);
	if (ppsk->data != NULL)
		memcpy(p + 2, ppsk->data, ppsk->size);

	ret = 0;

      error:
	_gnutls_free_temp_key_datum(&pwd_psk);
	return ret;
}


/* Generates the PSK client key exchange
 *
 * 
 * struct {
 *    select (KeyExchangeAlgorithm) {
 *       uint8_t psk_identity<0..2^16-1>;
 *    } exchange_keys;
 * } ClientKeyExchange;
 *
 */
int
_gnutls_gen_psk_client_kx(gnutls_session_t session,
			  gnutls_buffer_st * data)
{
	int ret, free;
	gnutls_datum_t username = {NULL, 0};
	gnutls_datum_t key;
	gnutls_psk_client_credentials_t cred;
	psk_auth_info_t info;

	cred = (gnutls_psk_client_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_PSK);

	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
	if (info == NULL) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	ret = _gnutls_find_psk_key(session, cred, &username, &key, &free);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_set_psk_session_key(session, &key, NULL);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_buffer_append_data_prefix(data, 16, username.data,
					      username.size);
	if (ret < 0) {
		gnutls_assert();
	}

	if (username.size > sizeof(info->username)-1) {
		gnutls_assert();
		ret = GNUTLS_E_ILLEGAL_SRP_USERNAME;
		goto cleanup;
	}

	assert(username.data != NULL);
	memcpy(info->username, username.data, username.size);
	info->username[username.size] = 0;


      cleanup:
	if (free) {
		gnutls_free(username.data);
		_gnutls_free_temp_key_datum(&key);
	}

	return ret;
}


/* just read the username from the client key exchange.
 */
static int
_gnutls_proc_psk_client_kx(gnutls_session_t session, uint8_t * data,
			   size_t _data_size)
{
	ssize_t data_size = _data_size;
	int ret;
	gnutls_datum_t username, psk_key;
	gnutls_psk_server_credentials_t cred;
	psk_auth_info_t info;

	cred = (gnutls_psk_server_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_PSK);

	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret =
	     _gnutls_auth_info_init(session, GNUTLS_CRD_PSK,
				   sizeof(psk_auth_info_st), 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	DECR_LEN(data_size, 2);
	username.size = _gnutls_read_uint16(&data[0]);

	DECR_LEN(data_size, username.size);

	username.data = &data[2];


	/* copy the username to the auth info structures
	 */
	info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
	if (info == NULL) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if (username.size > MAX_USERNAME_SIZE) {
		gnutls_assert();
		return GNUTLS_E_ILLEGAL_SRP_USERNAME;
	}

	memcpy(info->username, username.data, username.size);
	info->username[username.size] = 0;

	ret =
	    _gnutls_psk_pwd_find_entry(session, info->username, &psk_key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_set_psk_session_key(session, &psk_key, NULL);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = 0;

      error:
	_gnutls_free_key_datum(&psk_key);

	return ret;
}


/* Generates the PSK server key exchange
 *
 * struct {
 *     select (KeyExchangeAlgorithm) {
 *	 // other cases for rsa, diffie_hellman, etc.
 *	 case psk:  // NEW
 *	     uint8_t psk_identity_hint<0..2^16-1>;
 *     };
 * } ServerKeyExchange;
 *
 */
int
_gnutls_gen_psk_server_kx(gnutls_session_t session,
			  gnutls_buffer_st * data)
{
	gnutls_psk_server_credentials_t cred;
	gnutls_datum_t hint;

	cred = (gnutls_psk_server_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_PSK);

	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	/* Abort sending this message if there is no PSK identity hint. */
	if (cred->hint == NULL) {
		gnutls_assert();
		return GNUTLS_E_INT_RET_0;
	}

	hint.data = (uint8_t *) cred->hint;
	hint.size = strlen(cred->hint);

	return _gnutls_buffer_append_data_prefix(data, 16, hint.data,
						 hint.size);
}


/* just read the hint from the server key exchange.
 */
static int
_gnutls_proc_psk_server_kx(gnutls_session_t session, uint8_t * data,
			   size_t _data_size)
{
	ssize_t data_size = _data_size;
	int ret;
	gnutls_datum_t hint;
	gnutls_psk_client_credentials_t cred;
	psk_auth_info_t info;

	cred = (gnutls_psk_client_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_PSK);

	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret =
	     _gnutls_auth_info_init(session, GNUTLS_CRD_PSK,
				   sizeof(psk_auth_info_st), 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	DECR_LENGTH_RET(data_size, 2, 0);
	hint.size = _gnutls_read_uint16(&data[0]);

	DECR_LEN(data_size, hint.size);

	hint.data = &data[2];

	/* copy the hint to the auth info structures
	 */
	info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
	if (info == NULL) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if (hint.size > sizeof(info->hint)-1) {
		gnutls_assert();
		return GNUTLS_E_ILLEGAL_SRP_USERNAME;
	}

	memcpy(info->hint, hint.data, hint.size);
	info->hint[hint.size] = 0;

	ret = 0;

	return ret;
}

#endif				/* ENABLE_PSK */
