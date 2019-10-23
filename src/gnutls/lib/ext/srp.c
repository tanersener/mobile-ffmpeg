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

#include "gnutls_int.h"
#include <ext/srp.h>

#ifdef ENABLE_SRP

#include "auth.h"
#include <auth/srp_kx.h>
#include "errors.h"
#include "algorithms.h"
#include <num.h>
#include <hello_ext.h>

static int _gnutls_srp_unpack(gnutls_buffer_st * ps,
			      gnutls_ext_priv_data_t * _priv);
static int _gnutls_srp_pack(gnutls_ext_priv_data_t epriv,
			    gnutls_buffer_st * ps);
static void _gnutls_srp_deinit_data(gnutls_ext_priv_data_t epriv);
static int _gnutls_srp_recv_params(gnutls_session_t state,
				   const uint8_t * data, size_t data_size);
static int _gnutls_srp_send_params(gnutls_session_t state,
				   gnutls_buffer_st * extdata);

const hello_ext_entry_st ext_mod_srp = {
	.name = "SRP",
	.tls_id = 12,
	.gid = GNUTLS_EXTENSION_SRP,
	.parse_type = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO,
	.recv_func = _gnutls_srp_recv_params,
	.send_func = _gnutls_srp_send_params,
	.pack_func = _gnutls_srp_pack,
	.unpack_func = _gnutls_srp_unpack,
	.deinit_func = _gnutls_srp_deinit_data,
	.cannot_be_overriden = 1
};


static int
_gnutls_srp_recv_params(gnutls_session_t session, const uint8_t * data,
			size_t data_size)
{
	uint8_t len;
	gnutls_ext_priv_data_t epriv;
	srp_ext_st *priv;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		if (data_size > 0) {
			DECR_LEN(data_size, 1);

			len = data[0];
			DECR_LEN(data_size, len);

			if (MAX_USERNAME_SIZE < len) {
				gnutls_assert();
				return GNUTLS_E_ILLEGAL_SRP_USERNAME;
			}

			priv = gnutls_calloc(1, sizeof(*priv));
			if (priv == NULL) {
				gnutls_assert();
				return GNUTLS_E_MEMORY_ERROR;
			}

			priv->username = gnutls_malloc(len + 1);
			if (priv->username) {
				memcpy(priv->username, &data[1], len);
				/* null terminated */
				priv->username[len] = 0;
			}

			epriv = priv;
			_gnutls_hello_ext_set_priv(session,
						     GNUTLS_EXTENSION_SRP,
						     epriv);
		}
	}
	return 0;
}

static unsigned have_srp_ciphersuites(gnutls_session_t session)
{
	unsigned j;
	unsigned kx;

	for (j = 0; j < session->internals.priorities->cs.size; j++) {
		kx = session->internals.priorities->cs.entry[j]->kx_algorithm;
		if (kx == GNUTLS_KX_SRP || kx == GNUTLS_KX_SRP_RSA || kx == GNUTLS_KX_SRP_DSS)
			return 1;
	}

	return 0;
}

/* returns data_size or a negative number on failure
 * data is allocated locally
 */
static int
_gnutls_srp_send_params(gnutls_session_t session,
			gnutls_buffer_st * extdata)
{
	unsigned len;
	int ret;
	gnutls_ext_priv_data_t epriv;
	srp_ext_st *priv = NULL;
	char *username = NULL, *password = NULL;
	gnutls_srp_client_credentials_t cred =
		    (gnutls_srp_client_credentials_t)
		    _gnutls_get_cred(session, GNUTLS_CRD_SRP);

	if (session->security_parameters.entity != GNUTLS_CLIENT)
		return 0;

	if (cred == NULL)
		return 0;

	if (!have_srp_ciphersuites(session)) {
		return 0;
	}

	/* this function sends the client extension data (username) */
	priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	if (cred->username != NULL) {	/* send username */
		len = MIN(strlen(cred->username), 255);

		ret =
		    _gnutls_buffer_append_data_prefix(extdata, 8,
						      cred->
						      username,
						      len);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		priv->username = gnutls_strdup(cred->username);
		if (priv->username == NULL) {
			gnutls_assert();
			goto cleanup;
		}

		priv->password = gnutls_strdup(cred->password);
		if (priv->password == NULL) {
			gnutls_assert();
			goto cleanup;
		}

		epriv = priv;
		_gnutls_hello_ext_set_priv(session,
					     GNUTLS_EXTENSION_SRP,
					     epriv);

		return len + 1;
	} else if (cred->get_function != NULL) {
		/* Try the callback
		 */

		if (cred->
		    get_function(session, &username, &password) < 0
		    || username == NULL || password == NULL) {
			gnutls_assert();
			return GNUTLS_E_ILLEGAL_SRP_USERNAME;
		}

		len = MIN(strlen(username), 255);

		priv->username = username;
		priv->password = password;

		ret =
		    _gnutls_buffer_append_data_prefix(extdata, 8,
						      username,
						      len);
		if (ret < 0) {
			ret = gnutls_assert_val(ret);
			goto cleanup;
		}

		epriv = priv;
		_gnutls_hello_ext_set_priv(session,
					     GNUTLS_EXTENSION_SRP,
					     epriv);

		return len + 1;
	}
	return 0;

      cleanup:
	gnutls_free(username);
	gnutls_free(password);
	gnutls_free(priv);

	return ret;
}

static void _gnutls_srp_deinit_data(gnutls_ext_priv_data_t epriv)
{
	srp_ext_st *priv = epriv;

	gnutls_free(priv->username);
	gnutls_free(priv->password);
	gnutls_free(priv);
}

static int
_gnutls_srp_pack(gnutls_ext_priv_data_t epriv, gnutls_buffer_st * ps)
{
	srp_ext_st *priv = epriv;
	int ret;
	int password_len = 0, username_len = 0;

	if (priv->username)
		username_len = strlen(priv->username);

	if (priv->password)
		password_len = strlen(priv->password);

	BUFFER_APPEND_PFX4(ps, priv->username, username_len);
	BUFFER_APPEND_PFX4(ps, priv->password, password_len);

	return 0;
}

static int
_gnutls_srp_unpack(gnutls_buffer_st * ps, gnutls_ext_priv_data_t * _priv)
{
	srp_ext_st *priv;
	int ret;
	gnutls_ext_priv_data_t epriv;
	gnutls_datum_t username = { NULL, 0 };
	gnutls_datum_t password = { NULL, 0 };

	priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	BUFFER_POP_DATUM(ps, &username);
	BUFFER_POP_DATUM(ps, &password);

	priv->username = (char *) username.data;
	priv->password = (char *) password.data;

	epriv = priv;
	*_priv = epriv;

	return 0;

      error:
	_gnutls_free_datum(&username);
	_gnutls_free_datum(&password);
	return ret;
}


#endif				/* ENABLE_SRP */
