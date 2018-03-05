/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz, Nikos Mavrogiannopoulos
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

/* Compatibility functions on OpenPGP key parsing.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <openpgp.h>
#include <openpgp_int.h>

/*-
 * gnutls_openpgp_verify_key:
 * @hostname: the name of the certificate holder
 * @cert_list: the structure that holds the certificates.
 * @cert_list_lenght: the items in the cert_list.
 * @status: the output of the verification function
 *
 * Verify all signatures in the certificate list. When the key
 * is not available, the signature is skipped.
 *
 * Returns: a negative error code on error and %GNUTLS_E_SUCCESS (0) on success.
 *
 * NOTE: this function does not verify using any "web of trust". You
 * may use GnuPG for that purpose, or any other external PGP application.
 -*/
int
_gnutls_openpgp_verify_key(const gnutls_certificate_credentials_t cred,
			   gnutls_x509_subject_alt_name_t type,
			   const char *hostname,
			   const gnutls_datum_t * cert_list,
			   int cert_list_length,
			   unsigned int verify_flags,
			   unsigned int *status)
{
	int ret = 0;
	gnutls_openpgp_crt_t key = NULL;
	unsigned int verify = 0, verify_self = 0;

	if (!cert_list || cert_list_length != 1) {
		gnutls_assert();
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	ret = gnutls_openpgp_crt_init(&key);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    gnutls_openpgp_crt_import(key, &cert_list[0],
				      GNUTLS_OPENPGP_FMT_RAW);
	if (ret < 0) {
		gnutls_assert();
		goto leave;
	}

	if (cred->keyring != NULL) {
		ret =
		    gnutls_openpgp_crt_verify_ring(key, cred->keyring, 0,
						   &verify);
		if (ret < 0) {
			gnutls_assert();
			goto leave;
		}
	}

	/* Now try the self signature. */
	ret = gnutls_openpgp_crt_verify_self(key, 0, &verify_self);
	if (ret < 0) {
		gnutls_assert();
		goto leave;
	}

	*status = verify_self | verify;

	/* If we only checked the self signature. */
	if (!cred->keyring)
		*status |= GNUTLS_CERT_SIGNER_NOT_FOUND;

	if (hostname) {
		ret = gnutls_openpgp_crt_check_hostname2(key, hostname, verify_flags);
		if (ret == 0)
			*status |= GNUTLS_CERT_UNEXPECTED_OWNER;
	}

	ret = 0;

      leave:
	gnutls_openpgp_crt_deinit(key);

	return ret;
}

/*-
 * gnutls_openpgp_fingerprint:
 * @cert: the raw data that contains the OpenPGP public key.
 * @fpr: the buffer to save the fingerprint.
 * @fprlen: the integer to save the length of the fingerprint.
 *
 * Returns the fingerprint of the OpenPGP key. Depence on the algorithm,
 * the fingerprint can be 16 or 20 bytes.
 -*/
int
_gnutls_openpgp_fingerprint(const gnutls_datum_t * cert,
			    unsigned char *fpr, size_t * fprlen)
{
	gnutls_openpgp_crt_t key;
	int ret;

	ret = gnutls_openpgp_crt_init(&key);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_openpgp_crt_import(key, cert, GNUTLS_OPENPGP_FMT_RAW);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_openpgp_crt_get_fingerprint(key, fpr, fprlen);
	gnutls_openpgp_crt_deinit(key);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/*-
 * gnutls_openpgp_get_raw_key_creation_time:
 * @cert: the raw data that contains the OpenPGP public key.
 *
 * Returns the timestamp when the OpenPGP key was created.
 -*/
time_t
_gnutls_openpgp_get_raw_key_creation_time(const gnutls_datum_t * cert)
{
	gnutls_openpgp_crt_t key;
	int ret;
	time_t tim;

	ret = gnutls_openpgp_crt_init(&key);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_openpgp_crt_import(key, cert, GNUTLS_OPENPGP_FMT_RAW);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	tim = gnutls_openpgp_crt_get_creation_time(key);

	gnutls_openpgp_crt_deinit(key);

	return tim;
}


/*-
 * gnutls_openpgp_get_raw_key_expiration_time:
 * @cert: the raw data that contains the OpenPGP public key.
 *
 * Returns the time when the OpenPGP key expires. A value of '0' means
 * that the key doesn't expire at all.
 -*/
time_t
_gnutls_openpgp_get_raw_key_expiration_time(const gnutls_datum_t * cert)
{
	gnutls_openpgp_crt_t key;
	int ret;
	time_t tim;

	ret = gnutls_openpgp_crt_init(&key);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_openpgp_crt_import(key, cert, GNUTLS_OPENPGP_FMT_RAW);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	tim = gnutls_openpgp_crt_get_expiration_time(key);

	gnutls_openpgp_crt_deinit(key);

	return tim;
}
