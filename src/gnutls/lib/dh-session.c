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

/* ANON & DHE */

#if defined(ENABLE_DHE) || defined(ENABLE_ANON)
/**
 * gnutls_dh_set_prime_bits:
 * @session: is a #gnutls_session_t type.
 * @bits: is the number of bits
 *
 * This function sets the number of bits, for use in a Diffie-Hellman
 * key exchange.  This is used both in DH ephemeral and DH anonymous
 * cipher suites.  This will set the minimum size of the prime that
 * will be used for the handshake.
 *
 * In the client side it sets the minimum accepted number of bits.  If
 * a server sends a prime with less bits than that
 * %GNUTLS_E_DH_PRIME_UNACCEPTABLE will be returned by the handshake.
 *
 * Note that this function will warn via the audit log for value that
 * are believed to be weak.
 *
 * The function has no effect in server side.
 * 
 * Note that since 3.1.7 this function is deprecated. The minimum
 * number of bits is set by the priority string level.
 * Also this function must be called after gnutls_priority_set_direct()
 * or the set value may be overridden by the selected priority options.
 *
 *
 **/
void gnutls_dh_set_prime_bits(gnutls_session_t session, unsigned int bits)
{
	if (bits < gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_WEAK)
		&& bits != 0)
		_gnutls_audit_log(session,
				  "Note that the security level of the Diffie-Hellman key exchange has been lowered to %u bits and this may allow decryption of the session data\n",
				  bits);
	session->internals.priorities.dh_prime_bits = bits;
}


/**
 * gnutls_dh_get_group:
 * @session: is a gnutls session
 * @raw_gen: will hold the generator.
 * @raw_prime: will hold the prime.
 *
 * This function will return the group parameters used in the last
 * Diffie-Hellman key exchange with the peer.  These are the prime and
 * the generator used.  This function should be used for both
 * anonymous and ephemeral Diffie-Hellman.  The output parameters must
 * be freed with gnutls_free().
 *
 * Note, that the prime and generator are exported as non-negative
 * integers and may include a leading zero byte.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_dh_get_group(gnutls_session_t session,
		    gnutls_datum_t * raw_gen, gnutls_datum_t * raw_prime)
{
	dh_info_st *dh;
	int ret;
	anon_auth_info_t anon_info;
	cert_auth_info_t cert_info;
	psk_auth_info_t psk_info;

	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		anon_info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
		if (anon_info == NULL)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		dh = &anon_info->dh;
		break;
	case GNUTLS_CRD_PSK:
		psk_info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
		if (psk_info == NULL)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		dh = &psk_info->dh;
		break;
	case GNUTLS_CRD_CERTIFICATE:
		cert_info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
		if (cert_info == NULL)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		dh = &cert_info->dh;
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_set_datum(raw_prime, dh->prime.data, dh->prime.size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_set_datum(raw_gen, dh->generator.data,
			      dh->generator.size);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(raw_prime);
		return ret;
	}

	return 0;
}

/**
 * gnutls_dh_get_pubkey:
 * @session: is a gnutls session
 * @raw_key: will hold the public key.
 *
 * This function will return the peer's public key used in the last
 * Diffie-Hellman key exchange.  This function should be used for both
 * anonymous and ephemeral Diffie-Hellman.  The output parameters must
 * be freed with gnutls_free().
 *
 * Note, that public key is exported as non-negative
 * integer and may include a leading zero byte.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_dh_get_pubkey(gnutls_session_t session, gnutls_datum_t * raw_key)
{
	dh_info_st *dh;
	anon_auth_info_t anon_info;
	cert_auth_info_t cert_info;
	psk_auth_info_t psk_info;

	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		{
			anon_info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
			if (anon_info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			dh = &anon_info->dh;
			break;
		}
	case GNUTLS_CRD_PSK:
		{
			psk_info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (psk_info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			dh = &psk_info->dh;
			break;
		}
	case GNUTLS_CRD_CERTIFICATE:
		{

			cert_info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
			if (cert_info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			dh = &cert_info->dh;
			break;
		}
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_set_datum(raw_key, dh->public_key.data,
				 dh->public_key.size);
}

/**
 * gnutls_dh_get_secret_bits:
 * @session: is a gnutls session
 *
 * This function will return the bits used in the last Diffie-Hellman
 * key exchange with the peer.  Should be used for both anonymous and
 * ephemeral Diffie-Hellman.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int gnutls_dh_get_secret_bits(gnutls_session_t session)
{
	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		{
			anon_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			return info->dh.secret_bits;
		}
	case GNUTLS_CRD_PSK:
		{
			psk_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			return info->dh.secret_bits;
		}
	case GNUTLS_CRD_CERTIFICATE:
		{
			cert_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			return info->dh.secret_bits;
		}
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}
}


static int mpi_buf2bits(gnutls_datum_t * mpi_buf)
{
	bigint_t mpi;
	int rc;

	rc = _gnutls_mpi_init_scan_nz(&mpi, mpi_buf->data, mpi_buf->size);
	if (rc) {
		gnutls_assert();
		return rc;
	}

	rc = _gnutls_mpi_get_nbits(mpi);
	_gnutls_mpi_release(&mpi);

	return rc;
}

/**
 * gnutls_dh_get_prime_bits:
 * @session: is a gnutls session
 *
 * This function will return the bits of the prime used in the last
 * Diffie-Hellman key exchange with the peer.  Should be used for both
 * anonymous and ephemeral Diffie-Hellman.  Note that some ciphers,
 * like RSA and DSA without DHE, do not use a Diffie-Hellman key
 * exchange, and then this function will return 0.
 *
 * Returns: The Diffie-Hellman bit strength is returned, or 0 if no
 *   Diffie-Hellman key exchange was done, or a negative error code on
 *   failure.
 **/
int gnutls_dh_get_prime_bits(gnutls_session_t session)
{
	dh_info_st *dh;

	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		{
			anon_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_PSK:
		{
			psk_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_CERTIFICATE:
		{
			cert_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if(dh->prime.size == 0)
		return 0;

	return mpi_buf2bits(&dh->prime);
}


/**
 * gnutls_dh_get_peers_public_bits:
 * @session: is a gnutls session
 *
 * Get the Diffie-Hellman public key bit size.  Can be used for both
 * anonymous and ephemeral Diffie-Hellman.
 *
 * Returns: The public key bit size used in the last Diffie-Hellman
 *   key exchange with the peer, or a negative error code in case of error.
 **/
int gnutls_dh_get_peers_public_bits(gnutls_session_t session)
{
	dh_info_st *dh;

	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		{
			anon_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_PSK:
		{
			psk_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_CERTIFICATE:
		{
			cert_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return mpi_buf2bits(&dh->public_key);
}

#endif				/* DH */

