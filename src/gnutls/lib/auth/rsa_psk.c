/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2009, 2010
 * Free Software Foundation, Inc.
 *
 * Copyright (C) 2011
 * Bardenheuer GmbH, Munich and Bundesdruckerei GmbH, Berlin
 *
 * Copyright (C) 2013 Frank Morgner
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

#include "gnutls_int.h"

#ifdef ENABLE_PSK

#include "auth.h"
#include "dh.h"
#include "errors.h"
#include "mpi.h"
#include "num.h"
#include "gnutls_int.h"
#include "pk.h"
#include "random.h"
#include <abstract_int.h>
#include <algorithms.h>
#include <auth/dh_common.h>
#include <auth/psk.h>
#include <auth/psk_passwd.h>
#include <auth/rsa_common.h>
#include <cert.h>
#include <datum.h>
#include <state.h>

static int _gnutls_gen_rsa_psk_client_kx(gnutls_session_t session,
					 gnutls_buffer_st * data);
static int _gnutls_proc_rsa_psk_client_kx(gnutls_session_t, uint8_t *,
					  size_t);
static int
_gnutls_proc_rsa_psk_server_kx(gnutls_session_t session, uint8_t * data,
			   size_t _data_size);

const mod_auth_st rsa_psk_auth_struct = {
	"RSA PSK",
	_gnutls_gen_cert_server_crt,
	NULL,			/* generate_client_certificate */
	_gnutls_gen_psk_server_kx,
	_gnutls_gen_rsa_psk_client_kx,
	NULL,			/* generate_client_cert_vrfy */
	NULL,			/* generate_server_certificate_request */
	_gnutls_proc_crt,
	NULL,			/* process_client_certificate */
	_gnutls_proc_rsa_psk_server_kx,
	_gnutls_proc_rsa_psk_client_kx,
	NULL,			/* process_client_cert_vrfy */
	NULL			/* process_server_certificate_reuqest */
};

/* Set the PSK premaster secret.
 */
static int
set_rsa_psk_session_key(gnutls_session_t session,
			gnutls_datum_t * ppsk, gnutls_datum_t * rsa_secret)
{
	unsigned char *p;
	size_t rsa_secret_size;
	int ret;


	rsa_secret_size = rsa_secret->size;

	/* set the session key
	 */
	session->key.key.size = 2 + rsa_secret_size + 2 + ppsk->size;
	session->key.key.data = gnutls_malloc(session->key.key.size);
	if (session->key.key.data == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto error;
	}

	/* format of the premaster secret:
	 * (uint16_t) other_secret size (48)
	 * other_secret: 2 byte version + 46 byte random
	 * (uint16_t) psk_size
	 * the psk
	 */
	_gnutls_write_uint16(rsa_secret_size, session->key.key.data);
	memcpy(&session->key.key.data[2], rsa_secret->data,
	       rsa_secret->size);
	p = &session->key.key.data[rsa_secret_size + 2];
	_gnutls_write_uint16(ppsk->size, p);
	if (ppsk->data != NULL)
		memcpy(p + 2, ppsk->data, ppsk->size);

	ret = 0;

      error:
	return ret;
}

/* Generate client key exchange message
 *
 *
 * struct {
 *    select (KeyExchangeAlgorithm) {
 *       uint8_t psk_identity<0..2^16-1>;
 *       EncryptedPreMasterSecret;
 *    } exchange_keys;
 * } ClientKeyExchange;
 */
static int
_gnutls_gen_rsa_psk_client_kx(gnutls_session_t session,
			      gnutls_buffer_st * data)
{
	cert_auth_info_t auth = session->key.auth_info;
	gnutls_datum_t sdata;	/* data to send */
	gnutls_pk_params_st params;
	gnutls_psk_client_credentials_t cred;
	gnutls_datum_t username, key;
	int ret, free;
	unsigned init_pos;

	if (auth == NULL) {
		/* this shouldn't have happened. The proc_certificate
		 * function should have detected that.
		 */
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	gnutls_datum_t premaster_secret;
	premaster_secret.size = GNUTLS_MASTER_SIZE;
	premaster_secret.data =
	    gnutls_malloc(premaster_secret.size);

	if (premaster_secret.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	/* Generate random */
	ret = gnutls_rnd(GNUTLS_RND_RANDOM, premaster_secret.data,
			  premaster_secret.size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* Set version */
	if (session->internals.rsa_pms_version[0] == 0) {
		premaster_secret.data[0] =
		    _gnutls_get_adv_version_major(session);
		premaster_secret.data[1] =
		    _gnutls_get_adv_version_minor(session);
	} else {		/* use the version provided */
		premaster_secret.data[0] =
		    session->internals.rsa_pms_version[0];
		premaster_secret.data[1] =
		    session->internals.rsa_pms_version[1];
	}

	/* move RSA parameters to key (session).
	 */
	if ((ret = _gnutls_get_public_rsa_params(session, &params)) < 0) {
		gnutls_assert();
		return ret;
	}

	/* Encrypt premaster secret */
	if ((ret =
	     _gnutls_pk_encrypt(GNUTLS_PK_RSA, &sdata, &premaster_secret,
				&params)) < 0) {
		gnutls_assert();
		return ret;
	}

	gnutls_pk_params_release(&params);

	cred = (gnutls_psk_client_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_PSK);

	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	ret = _gnutls_find_psk_key(session, cred, &username, &key, &free);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Here we set the PSK key */
	ret = set_rsa_psk_session_key(session, &key, &premaster_secret);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Create message for client key exchange
	 *
	 * struct {
	 *   uint8_t psk_identity<0..2^16-1>;
	 *   EncryptedPreMasterSecret;
	 * }
	 */

	init_pos = data->length;

	/* Write psk_identity and EncryptedPreMasterSecret into data stream
	 */
	ret =
	    _gnutls_buffer_append_data_prefix(data, 16,
					      username.data,
					      username.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_buffer_append_data_prefix(data, 16, sdata.data,
					      sdata.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = data->length - init_pos;

      cleanup:
	_gnutls_free_datum(&sdata);
	_gnutls_free_temp_key_datum(&premaster_secret);
	if (free) {
		_gnutls_free_temp_key_datum(&key);
		gnutls_free(username.data);
	}

	return ret;
}

/*
  Process the client key exchange message
*/
static int
_gnutls_proc_rsa_psk_client_kx(gnutls_session_t session, uint8_t * data,
			       size_t _data_size)
{
	gnutls_datum_t username;
	psk_auth_info_t info;
	gnutls_datum_t plaintext;
	gnutls_datum_t ciphertext;
	gnutls_datum_t pwd_psk = { NULL, 0 };
	int ret, dsize;
	int randomize_key = 0;
	ssize_t data_size = _data_size;
	gnutls_psk_server_credentials_t cred;
	gnutls_datum_t premaster_secret = { NULL, 0 };

	cred = (gnutls_psk_server_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_PSK);

	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	ret = _gnutls_auth_info_init(session, GNUTLS_CRD_PSK,
				    sizeof(psk_auth_info_st), 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

  /*** 1. Extract user psk_identity ***/

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

	_gnutls_copy_psk_username(info, &username);

	/* Adjust data so it points to EncryptedPreMasterSecret */
	data += username.size + 2;

  /*** 2. Decrypt and extract EncryptedPreMasterSecret ***/

	DECR_LEN(data_size, 2);
	ciphertext.data = &data[2];
	dsize = _gnutls_read_uint16(data);

	if (dsize != data_size) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}
	ciphertext.size = dsize;

	ret =
	    gnutls_privkey_decrypt_data(session->internals.selected_key, 0,
					&ciphertext, &plaintext);
	if (ret < 0 || plaintext.size != GNUTLS_MASTER_SIZE) {
		/* In case decryption fails then don't inform
		 * the peer. Just use a random key. (in order to avoid
		 * attack against pkcs-1 formatting).
		 */
		gnutls_assert();
		_gnutls_debug_log
		    ("auth_rsa_psk: Possible PKCS #1 format attack\n");
		if (ret >= 0) {
			gnutls_free(plaintext.data);
		}
		randomize_key = 1;
	} else {
		/* If the secret was properly formatted, then
		 * check the version number.
		 */
		if (_gnutls_get_adv_version_major(session) !=
		    plaintext.data[0]
		    || (session->internals.allow_wrong_pms == 0
			&& _gnutls_get_adv_version_minor(session) !=
			plaintext.data[1])) {
			/* No error is returned here, if the version number check
			 * fails. We proceed normally.
			 * That is to defend against the attack described in the paper
			 * "Attacking RSA-based sessions in SSL/TLS" by Vlastimil Klima,
			 * Ondej Pokorny and Tomas Rosa.
			 */
			gnutls_assert();
			_gnutls_debug_log
			    ("auth_rsa: Possible PKCS #1 version check format attack\n");
		}
	}


	if (randomize_key != 0) {
		premaster_secret.size = GNUTLS_MASTER_SIZE;
		premaster_secret.data =
		    gnutls_malloc(premaster_secret.size);
		if (premaster_secret.data == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		/* we do not need strong random numbers here.
		 */
		ret = gnutls_rnd(GNUTLS_RND_NONCE, premaster_secret.data,
				  premaster_secret.size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else {
		premaster_secret.data = plaintext.data;
		premaster_secret.size = plaintext.size;
	}

	/* This is here to avoid the version check attack
	 * discussed above.
	 */

	premaster_secret.data[0] = _gnutls_get_adv_version_major(session);
	premaster_secret.data[1] = _gnutls_get_adv_version_minor(session);

	/* find the key of this username
	 */
	ret =
	    _gnutls_psk_pwd_find_entry(session, info->username, strlen(info->username), &pwd_psk);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    set_rsa_psk_session_key(session, &pwd_psk, &premaster_secret);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
      cleanup:
	_gnutls_free_key_datum(&pwd_psk);
	_gnutls_free_temp_key_datum(&premaster_secret);

	return ret;
}

static int
_gnutls_proc_rsa_psk_server_kx(gnutls_session_t session, uint8_t * data,
			   size_t _data_size)
{
	/* In RSA-PSK the key is calculated elsewhere.
	 * Moreover, since we only keep a single auth info structure, we cannot
	 * store the hint (as we store certificate auth info).
	 * Ideally we need to handle that by multiple auth info
	 * structures or something similar.
	 */

	return 0;
}

#endif				/* ENABLE_PSK */
