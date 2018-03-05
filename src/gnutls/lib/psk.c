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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* Functions for manipulating the PSK credentials. */

#include "gnutls_int.h"
#include "errors.h"
#include <auth/psk.h>
#include <state.h>

#ifdef ENABLE_PSK

#include <auth/psk_passwd.h>
#include <num.h>
#include <file.h>
#include <datum.h>
#include "debug.h"
#include "dh.h"

/**
 * gnutls_psk_free_client_credentials:
 * @sc: is a #gnutls_psk_client_credentials_t type.
 *
 * Free a gnutls_psk_client_credentials_t structure.
 **/
void gnutls_psk_free_client_credentials(gnutls_psk_client_credentials_t sc)
{
	_gnutls_free_datum(&sc->username);
	_gnutls_free_datum(&sc->key);
	gnutls_free(sc);
}

/**
 * gnutls_psk_allocate_client_credentials:
 * @sc: is a pointer to a #gnutls_psk_server_credentials_t type.
 *
 * Allocate a gnutls_psk_client_credentials_t structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_psk_allocate_client_credentials(gnutls_psk_client_credentials_t *
				       sc)
{
	*sc = gnutls_calloc(1, sizeof(psk_client_credentials_st));

	if (*sc == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	return 0;
}

/**
 * gnutls_psk_set_client_credentials:
 * @res: is a #gnutls_psk_client_credentials_t type.
 * @username: is the user's zero-terminated userid
 * @key: is the user's key
 * @flags: indicate the format of the key, either
 *   %GNUTLS_PSK_KEY_RAW or %GNUTLS_PSK_KEY_HEX.
 *
 * This function sets the username and password, in a
 * gnutls_psk_client_credentials_t type.  Those will be used in
 * PSK authentication.  @username should be an ASCII string or UTF-8
 * strings prepared using the "SASLprep" profile of "stringprep".  The
 * key can be either in raw byte format or in Hex format (without the
 * 0x prefix).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_psk_set_client_credentials(gnutls_psk_client_credentials_t res,
				  const char *username,
				  const gnutls_datum_t * key,
				  gnutls_psk_key_flags flags)
{
	int ret;

	if (username == NULL || key == NULL || key->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _gnutls_set_datum(&res->username, username, strlen(username));
	if (ret < 0)
		return ret;

	if (flags == GNUTLS_PSK_KEY_RAW) {
		if (_gnutls_set_datum(&res->key, key->data, key->size) < 0) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto error;
		}
	} else {		/* HEX key */
		size_t size;
		size = res->key.size = key->size / 2;
		res->key.data = gnutls_malloc(size);
		if (res->key.data == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto error;
		}

		ret =
		    gnutls_hex_decode(key, (char *) res->key.data, &size);
		res->key.size = (unsigned int) size;
		if (ret < 0) {
			
			gnutls_assert();
			goto error;
		}

		if (size < 4) {
			gnutls_assert();
			ret = GNUTLS_E_INVALID_REQUEST;
			goto error;
		}
	}

	return 0;

      error:
	_gnutls_free_datum(&res->username);
	_gnutls_free_datum(&res->key);

	return ret;
}

/**
 * gnutls_psk_free_server_credentials:
 * @sc: is a #gnutls_psk_server_credentials_t type.
 *
 * Free a gnutls_psk_server_credentials_t structure.
 **/
void gnutls_psk_free_server_credentials(gnutls_psk_server_credentials_t sc)
{
	if (sc->deinit_dh_params) {
		gnutls_dh_params_deinit(sc->dh_params);
	}

	gnutls_free(sc->password_file);
	gnutls_free(sc->hint);
	gnutls_free(sc);
}

/**
 * gnutls_psk_allocate_server_credentials:
 * @sc: is a pointer to a #gnutls_psk_server_credentials_t type.
 *
 * Allocate a gnutls_psk_server_credentials_t structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_psk_allocate_server_credentials(gnutls_psk_server_credentials_t *
				       sc)
{
	*sc = gnutls_calloc(1, sizeof(psk_server_cred_st));

	if (*sc == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	return 0;
}


/**
 * gnutls_psk_set_server_credentials_file:
 * @res: is a #gnutls_psk_server_credentials_t type.
 * @password_file: is the PSK password file (passwd.psk)
 *
 * This function sets the password file, in a
 * #gnutls_psk_server_credentials_t type.  This password file
 * holds usernames and keys and will be used for PSK authentication.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_psk_set_server_credentials_file(gnutls_psk_server_credentials_t
				       res, const char *password_file)
{

	if (password_file == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the files can be opened */
	if (_gnutls_file_exists(password_file) != 0) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	res->password_file = gnutls_strdup(password_file);
	if (res->password_file == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

/**
 * gnutls_psk_set_server_credentials_hint:
 * @res: is a #gnutls_psk_server_credentials_t type.
 * @hint: is the PSK identity hint string
 *
 * This function sets the identity hint, in a
 * #gnutls_psk_server_credentials_t type.  This hint is sent to
 * the client to help it chose a good PSK credential (i.e., username
 * and password).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 2.4.0
 **/
int
gnutls_psk_set_server_credentials_hint(gnutls_psk_server_credentials_t res,
				       const char *hint)
{
	res->hint = gnutls_strdup(hint);
	if (res->hint == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

/**
 * gnutls_psk_set_server_credentials_function:
 * @cred: is a #gnutls_psk_server_credentials_t type.
 * @func: is the callback function
 *
 * This function can be used to set a callback to retrieve the user's PSK credentials.
 * The callback's function form is:
 * int (*callback)(gnutls_session_t, const char* username,
 *  gnutls_datum_t* key);
 *
 * @username contains the actual username.
 * The @key must be filled in using the gnutls_malloc().
 *
 * In case the callback returned a negative number then gnutls will
 * assume that the username does not exist.
 *
 * The callback function will only be called once per handshake.  The
 * callback function should return 0 on success, while -1 indicates
 * an error.
 **/
void
gnutls_psk_set_server_credentials_function(gnutls_psk_server_credentials_t
					   cred,
					   gnutls_psk_server_credentials_function
					   * func)
{
	cred->pwd_callback = func;
}

/**
 * gnutls_psk_set_client_credentials_function:
 * @cred: is a #gnutls_psk_server_credentials_t type.
 * @func: is the callback function
 *
 * This function can be used to set a callback to retrieve the username and
 * password for client PSK authentication.
 * The callback's function form is:
 * int (*callback)(gnutls_session_t, char** username,
 *  gnutls_datum_t* key);
 *
 * The @username and @key->data must be allocated using gnutls_malloc().
 * @username should be ASCII strings or UTF-8 strings prepared using
 * the "SASLprep" profile of "stringprep".
 *
 * The callback function will be called once per handshake.
 *
 * The callback function should return 0 on success.
 * -1 indicates an error.
 **/
void
gnutls_psk_set_client_credentials_function(gnutls_psk_client_credentials_t
					   cred,
					   gnutls_psk_client_credentials_function
					   * func)
{
	cred->get_function = func;
}


/**
 * gnutls_psk_server_get_username:
 * @session: is a gnutls session
 *
 * This should only be called in case of PSK authentication and in
 * case of a server.
 *
 * Returns: the username of the peer, or %NULL in case of an error.
 **/
const char *gnutls_psk_server_get_username(gnutls_session_t session)
{
	psk_auth_info_t info;

	CHECK_AUTH(GNUTLS_CRD_PSK, NULL);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
	if (info == NULL)
		return NULL;

	if (info->username[0] != 0)
		return info->username;

	return NULL;
}

/**
 * gnutls_psk_client_get_hint:
 * @session: is a gnutls session
 *
 * The PSK identity hint may give the client help in deciding which
 * username to use.  This should only be called in case of PSK
 * authentication and in case of a client.
 *
 * Returns: the identity hint of the peer, or %NULL in case of an error.
 *
 * Since: 2.4.0
 **/
const char *gnutls_psk_client_get_hint(gnutls_session_t session)
{
	psk_auth_info_t info;

	CHECK_AUTH(GNUTLS_CRD_PSK, NULL);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
	if (info == NULL)
		return NULL;

	if (info->hint[0] != 0)
		return info->hint;

	return NULL;
}

/**
 * gnutls_psk_set_server_dh_params:
 * @res: is a gnutls_psk_server_credentials_t type
 * @dh_params: is a structure that holds Diffie-Hellman parameters.
 *
 * This function will set the Diffie-Hellman parameters for an
 * anonymous server to use. These parameters will be used in
 * Diffie-Hellman exchange with PSK cipher suites.
 **/
void
gnutls_psk_set_server_dh_params(gnutls_psk_server_credentials_t res,
				gnutls_dh_params_t dh_params)
{
	res->dh_params = dh_params;
}

/**
 * gnutls_psk_set_server_known_dh_params:
 * @res: is a gnutls_psk_server_credentials_t type
 * @sec_param: is an option of the %gnutls_sec_param_t enumeration
 *
 * This function will set the Diffie-Hellman parameters for a
 * PSK server to use. These parameters will be used in
 * Ephemeral Diffie-Hellman cipher suites and will be selected from
 * the FFDHE set of RFC7919 according to the security level provided.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.6
 **/
int
gnutls_psk_set_server_known_dh_params(gnutls_psk_server_credentials_t res,
				       gnutls_sec_param_t sec_param)
{
	int ret;

	if (res->deinit_dh_params) {
		res->deinit_dh_params = 0;
		gnutls_dh_params_deinit(res->dh_params);
		res->dh_params = NULL;
	}

	ret = _gnutls_set_cred_dh_params(&res->dh_params, sec_param);
	if (ret < 0)
		return gnutls_assert_val(ret);

	res->deinit_dh_params = 1;

	return 0;
}

/**
 * gnutls_psk_set_server_params_function:
 * @res: is a #gnutls_certificate_credentials_t type
 * @func: is the function to be called
 *
 * This function will set a callback in order for the server to get
 * the Diffie-Hellman parameters for PSK authentication.  The callback
 * should return %GNUTLS_E_SUCCESS (0) on success.
 **/
void
gnutls_psk_set_server_params_function(gnutls_psk_server_credentials_t res,
				      gnutls_params_function * func)
{
	res->params_func = func;
}

/**
 * gnutls_psk_set_params_function:
 * @res: is a gnutls_psk_server_credentials_t type
 * @func: is the function to be called
 *
 * This function will set a callback in order for the server to get
 * the Diffie-Hellman or RSA parameters for PSK authentication.  The
 * callback should return %GNUTLS_E_SUCCESS (0) on success.
 **/
void
gnutls_psk_set_params_function(gnutls_psk_server_credentials_t res,
			       gnutls_params_function * func)
{
	res->params_func = func;
}

#endif				/* ENABLE_PSK */
