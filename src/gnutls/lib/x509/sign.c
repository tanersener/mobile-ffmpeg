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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
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
#include <pk.h>

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

int
_gnutls_x509_crt_get_spki_params(gnutls_x509_crt_t crt,
				 const gnutls_x509_spki_st *key_params,
				 gnutls_x509_spki_st *params)
{
	int result;
	gnutls_x509_spki_st crt_params;

	result = _gnutls_x509_crt_read_spki_params(crt, &crt_params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	if (crt_params.pk == GNUTLS_PK_RSA_PSS) {
		if (key_params->pk == GNUTLS_PK_RSA_PSS) {
			if (crt_params.rsa_pss_dig != key_params->rsa_pss_dig) {
				gnutls_assert();
				return GNUTLS_E_CERTIFICATE_ERROR;
			}

			if (crt_params.salt_size < key_params->salt_size) {
				gnutls_assert();
				return GNUTLS_E_CERTIFICATE_ERROR;
			}
		} else if (key_params->pk != GNUTLS_PK_RSA && key_params->pk != GNUTLS_PK_UNKNOWN) {
			gnutls_assert();
			return GNUTLS_E_CERTIFICATE_ERROR;
		}
		memcpy(params, &crt_params, sizeof(gnutls_x509_spki_st));
	} else {
		memcpy(params, key_params, sizeof(gnutls_x509_spki_st));
	}

	return 0;
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
		       unsigned int flags,
		       gnutls_x509_crt_t issuer,
		       gnutls_privkey_t issuer_key)
{
	int result;
	gnutls_datum_t signature;
	gnutls_datum_t tbs;
	char name[128];
	gnutls_pk_algorithm_t pk;
	gnutls_x509_spki_st key_params, params;
	const gnutls_sign_entry_st *se;

	pk = gnutls_x509_crt_get_pk_algorithm(issuer, NULL);
	if (pk == GNUTLS_PK_UNKNOWN)
		pk = gnutls_privkey_get_pk_algorithm(issuer_key, NULL);

	result = _gnutls_privkey_get_spki_params(issuer_key, &key_params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = _gnutls_x509_crt_get_spki_params(issuer, &key_params, &params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = _gnutls_privkey_update_spki_params(issuer_key, pk, dig, flags,
						  &params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

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

	se = _gnutls_pk_to_sign_entry(params.pk, dig);
	if (se == NULL)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM);

	_gnutls_debug_log("signing structure using %s\n", se->name);

	result = _gnutls_x509_write_sign_params(src, name, se, &params);
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

	FIX_SIGN_PARAMS(params, flags, dig);

	if (_gnutls_pk_is_not_prehashed(params.pk)) {
		result = privkey_sign_raw_data(issuer_key, se, &tbs, &signature, &params);
	} else {
		result = privkey_sign_and_hash_data(issuer_key, se,
						    &tbs, &signature, &params);
	}
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

	result = _gnutls_x509_write_sign_params(src, "signatureAlgorithm",
						se, &params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}
