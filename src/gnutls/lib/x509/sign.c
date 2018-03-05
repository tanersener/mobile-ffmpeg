/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
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

/* All functions which relate to X.509 certificate signing stuff are
 * included here
 */

#include "gnutls_int.h"

#include "errors.h"
#include <libtasn1.h>
#include <global.h>
#include <num.h>		/* MAX */
#include <tls-sig.h>
#include <str.h>
#include <datum.h>
#include <x509_int.h>
#include <common.h>
#include <gnutls/abstract.h>

/* This is the same as the _gnutls_x509_sign, but this one will decode
 * the ASN1_TYPE given, and sign the DER data. Actually used to get the DER
 * of the TBS and sign it on the fly.
 */
int
_gnutls_x509_get_tbs(ASN1_TYPE cert, const char *tbs_name,
		     gnutls_datum_t * tbs)
{
	return _gnutls_x509_der_encode(cert, tbs_name, tbs, 0);
}

/*-
 * _gnutls_x509_pkix_sign - This function will sign a CRL or a certificate with a key
 * @src: should contain an ASN1_TYPE
 * @issuer: is the certificate of the certificate issuer
 * @issuer_key: holds the issuer's private key
 *
 * This function will sign a CRL or a certificate with the issuer's private key, and
 * will copy the issuer's information into the CRL or certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 -*/
int
_gnutls_x509_pkix_sign(ASN1_TYPE src, const char *src_name,
		       gnutls_digest_algorithm_t dig,
		       gnutls_x509_crt_t issuer,
		       gnutls_privkey_t issuer_key)
{
	int result;
	gnutls_datum_t signature;
	gnutls_datum_t tbs;
	char name[128];

	/* Step 1. Copy the issuer's name into the certificate.
	 */
	_gnutls_str_cpy(name, sizeof(name), src_name);
	_gnutls_str_cat(name, sizeof(name), ".issuer");

	result =
	    asn1_copy_node(src, name, issuer->cert,
			   "tbsCertificate.subject");
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* Step 1.5. Write the signature stuff in the tbsCertificate.
	 */
	_gnutls_str_cpy(name, sizeof(name), src_name);
	_gnutls_str_cat(name, sizeof(name), ".signature");

	result = _gnutls_x509_write_sig_params(src, name,
					       gnutls_privkey_get_pk_algorithm
					       (issuer_key, NULL), dig, 0);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* Step 2. Sign the certificate.
	 */
	result = _gnutls_x509_get_tbs(src, src_name, &tbs);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result =
	    gnutls_privkey_sign_data(issuer_key, dig, 0, &tbs, &signature);
	gnutls_free(tbs.data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* write the signature (bits)
	 */
	result =
	    asn1_write_value(src, "signature", signature.data,
			     signature.size * 8);

	_gnutls_free_datum(&signature);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* Step 3. Move up and write the AlgorithmIdentifier, which is also
	 * the same. 
	 */

	result = _gnutls_x509_write_sig_params(src, "signatureAlgorithm",
					       gnutls_privkey_get_pk_algorithm
					       (issuer_key, NULL), dig, 0);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}
