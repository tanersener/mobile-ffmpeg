/*
 * Copyright (C) 2001-2015 Free Software Foundation, Inc.
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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

/* This file contains certificate authentication functions to be exported in the
 * API which did not fit elsewhere.
 */

#include "gnutls_int.h"
#include <auth/srp_kx.h>
#include <auth/anon.h>
#include <auth/cert.h>
#include <auth/psk.h>
#include "errors.h"
#include <auth.h>
#include <state.h>
#include <datum.h>
#include <algorithms.h>

/* CERTIFICATE STUFF */

/**
 * gnutls_certificate_get_ours:
 * @session: is a gnutls session
 *
 * Gets the certificate as sent to the peer in the last handshake.
 * The certificate is in raw (DER) format.  No certificate
 * list is being returned. Only the first certificate.
 *
 * Returns: a pointer to a #gnutls_datum_t containing our
 *   certificate, or %NULL in case of an error or if no certificate
 *   was used.
 **/
const gnutls_datum_t *gnutls_certificate_get_ours(gnutls_session_t session)
{
	gnutls_certificate_credentials_t cred;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, NULL);

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return NULL;
	}

	if (session->internals.selected_cert_list == NULL)
		return NULL;

	return &session->internals.selected_cert_list[0].cert;
}

/**
 * gnutls_certificate_get_peers:
 * @session: is a gnutls session
 * @list_size: is the length of the certificate list (may be %NULL)
 *
 * Get the peer's raw certificate (chain) as sent by the peer.  These
 * certificates are in raw format (DER encoded for X.509).  In case of
 * a X.509 then a certificate list may be present.  The list
 * is provided as sent by the server; the server must send as first
 * certificate in the list its own certificate, following the
 * issuer's certificate, then the issuer's issuer etc. However, there
 * are servers which violate this principle and thus on certain
 * occasions this may be an unsorted list.
 *
 * In case of OpenPGP keys a single key will be returned in raw
 * format.
 *
 * Returns: a pointer to a #gnutls_datum_t containing the peer's
 *   certificates, or %NULL in case of an error or if no certificate
 *   was used.
 **/
const gnutls_datum_t *gnutls_certificate_get_peers(gnutls_session_t
						   session,
						   unsigned int *list_size)
{
	cert_auth_info_t info;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, NULL);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL)
		return NULL;

	if (list_size)
		*list_size = info->ncerts;
	return info->raw_certificate_list;
}

#ifdef ENABLE_OPENPGP
/**
 * gnutls_certificate_get_peers_subkey_id:
 * @session: is a gnutls session
 * @id: will contain the ID
 *
 * Get the peer's subkey ID when OpenPGP certificates are
 * used. The returned @id should be treated as constant.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.1.3
 **/
int gnutls_certificate_get_peers_subkey_id(gnutls_session_t session,
					   gnutls_datum_t * id)
{
	cert_auth_info_t info;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL)
		return GNUTLS_E_INVALID_REQUEST;

	id->data = info->subkey_id;
	id->size = GNUTLS_OPENPGP_KEYID_SIZE;

	return 0;
}
#endif

/**
 * gnutls_certificate_client_get_request_status:
 * @session: is a gnutls session
 *
 * Get whether client certificate was requested on the last
 * handshake or not.
 *
 * Returns: 0 if the peer (server) did not request client
 *   authentication or 1 otherwise.
 **/
int gnutls_certificate_client_get_request_status(gnutls_session_t session)
{
	return session->internals.crt_requested;
}

/**
 * gnutls_certificate_set_params_function:
 * @res: is a gnutls_certificate_credentials_t type
 * @func: is the function to be called
 *
 * This function will set a callback in order for the server to get
 * the Diffie-Hellman or RSA parameters for certificate
 * authentication.  The callback should return %GNUTLS_E_SUCCESS (0) on success.
 **/
void
gnutls_certificate_set_params_function(gnutls_certificate_credentials_t
				       res, gnutls_params_function * func)
{
	res->params_func = func;
}

/**
 * gnutls_certificate_set_flags:
 * @res: is a gnutls_certificate_credentials_t type
 * @flags: are the flags of #gnutls_certificate_flags type
 *
 * This function will set flags to tweak the operation of
 * the credentials structure. See the #gnutls_certificate_flags enumerations
 * for more information on the available flags. 
 *
 * Since: 3.4.7
 **/
void
gnutls_certificate_set_flags(gnutls_certificate_credentials_t res,
			     unsigned int flags)
{
	res->flags = flags;
}

/**
 * gnutls_certificate_set_verify_flags:
 * @res: is a gnutls_certificate_credentials_t type
 * @flags: are the flags
 *
 * This function will set the flags to be used for verification 
 * of certificates and override any defaults.  The provided flags must be an OR of the
 * #gnutls_certificate_verify_flags enumerations. 
 *
 **/
void
gnutls_certificate_set_verify_flags(gnutls_certificate_credentials_t
				    res, unsigned int flags)
{
	res->verify_flags = flags;
}

/**
 * gnutls_certificate_get_verify_flags:
 * @res: is a gnutls_certificate_credentials_t type
 *
 * Returns the verification flags set with
 * gnutls_certificate_set_verify_flags().
 *
 * Returns: The certificate verification flags used by @res.
 *
 * Since: 3.4.0
 */
unsigned int
gnutls_certificate_get_verify_flags(gnutls_certificate_credentials_t res)
{
	return res->verify_flags;
}

/**
 * gnutls_certificate_set_verify_limits:
 * @res: is a gnutls_certificate_credentials type
 * @max_bits: is the number of bits of an acceptable certificate (default 8200)
 * @max_depth: is maximum depth of the verification of a certificate chain (default 5)
 *
 * This function will set some upper limits for the default
 * verification function, gnutls_certificate_verify_peers2(), to avoid
 * denial of service attacks.  You can set them to zero to disable
 * limits.
 **/
void
gnutls_certificate_set_verify_limits(gnutls_certificate_credentials_t res,
				     unsigned int max_bits,
				     unsigned int max_depth)
{
	res->verify_depth = max_depth;
	res->verify_bits = max_bits;
}

