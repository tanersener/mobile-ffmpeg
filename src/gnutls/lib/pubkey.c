/*
 * GnuTLS public key support
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
 * 
 * Author: Nikos Mavrogiannopoulos
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
 */

#include "gnutls_int.h"
#include <gnutls/pkcs11.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"
#include <datum.h>
#include <pkcs11_int.h>
#include <gnutls/abstract.h>
#include <tls-sig.h>
#include <pk.h>
#include <x509_int.h>
#include <num.h>
#include <x509/common.h>
#include <x509_b64.h>
#include <abstract_int.h>
#include <fips.h>
#include "urls.h"
#include <ecc.h>

static int
pubkey_verify_hashed_data(const gnutls_sign_entry_st *se,
			  const mac_entry_st *me,
			  const gnutls_datum_t * hash,
			  const gnutls_datum_t * signature,
			  gnutls_pk_params_st * params,
			  gnutls_x509_spki_st * sign_params,
			  unsigned flags);

static
int pubkey_supports_sig(gnutls_pubkey_t pubkey,
			const gnutls_sign_entry_st *se);

unsigned pubkey_to_bits(const gnutls_pk_params_st * params)
{
	switch (params->algo) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:
		return _gnutls_mpi_get_nbits(params->params[RSA_MODULUS]);
	case GNUTLS_PK_DSA:
		return _gnutls_mpi_get_nbits(params->params[DSA_P]);
	case GNUTLS_PK_ECDSA:
	case GNUTLS_PK_EDDSA_ED25519:
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		return gnutls_ecc_curve_get_size(params->curve) * 8;
	default:
		return 0;
	}
}

/**
 * gnutls_pubkey_get_pk_algorithm:
 * @key: should contain a #gnutls_pubkey_t type
 * @bits: If set will return the number of bits of the parameters (may be NULL)
 *
 * This function will return the public key algorithm of a public
 * key and if possible will return a number of bits that indicates
 * the security parameter of the key.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative error code on error.
 *
 * Since: 2.12.0
 **/
int gnutls_pubkey_get_pk_algorithm(gnutls_pubkey_t key, unsigned int *bits)
{
	if (bits)
		*bits = key->bits;

	return key->params.algo;
}

/**
 * gnutls_pubkey_get_key_usage:
 * @key: should contain a #gnutls_pubkey_t type
 * @usage: If set will return the number of bits of the parameters (may be NULL)
 *
 * This function will return the key usage of the public key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_pubkey_get_key_usage(gnutls_pubkey_t key, unsigned int *usage)
{
	if (usage)
		*usage = key->key_usage;

	return 0;
}

/**
 * gnutls_pubkey_init:
 * @key: A pointer to the type to be initialized
 *
 * This function will initialize a public key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_pubkey_init(gnutls_pubkey_t * key)
{
	FAIL_IF_LIB_ERROR;

	*key = gnutls_calloc(1, sizeof(struct gnutls_pubkey_st));
	if (*key == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

/**
 * gnutls_pubkey_deinit:
 * @key: The key to be deinitialized
 *
 * This function will deinitialize a public key structure.
 *
 * Since: 2.12.0
 **/
void gnutls_pubkey_deinit(gnutls_pubkey_t key)
{
	if (!key)
		return;
	gnutls_pk_params_release(&key->params);
	gnutls_free(key);
}

/**
 * gnutls_pubkey_import_x509:
 * @key: The public key
 * @crt: The certificate to be imported
 * @flags: should be zero
 *
 * This function will import the given public key to the abstract
 * #gnutls_pubkey_t type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_import_x509(gnutls_pubkey_t key, gnutls_x509_crt_t crt,
			  unsigned int flags)
{
	int ret;

	gnutls_pk_params_release(&key->params);
	/* params initialized in _gnutls_x509_crt_get_mpis */

	ret = gnutls_x509_crt_get_pk_algorithm(crt, &key->bits);
	if (ret < 0)
		return gnutls_assert_val(ret);

	key->params.algo = ret;

	ret = gnutls_x509_crt_get_key_usage(crt, &key->key_usage, NULL);
	if (ret < 0)
		key->key_usage = 0;

	ret = _gnutls_x509_crt_get_mpis(crt, &key->params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_pubkey_import_x509_crq:
 * @key: The public key
 * @crq: The certificate to be imported
 * @flags: should be zero
 *
 * This function will import the given public key to the abstract
 * #gnutls_pubkey_t type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.5
 **/
int
gnutls_pubkey_import_x509_crq(gnutls_pubkey_t key, gnutls_x509_crq_t crq,
			      unsigned int flags)
{
	int ret;

	gnutls_pk_params_release(&key->params);
	/* params initialized in _gnutls_x509_crq_get_mpis */

	key->params.algo = gnutls_x509_crq_get_pk_algorithm(crq, &key->bits);

	ret = gnutls_x509_crq_get_key_usage(crq, &key->key_usage, NULL);
	if (ret < 0)
		key->key_usage = 0;

	ret = _gnutls_x509_crq_get_mpis(crq, &key->params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_pubkey_import_privkey:
 * @key: The public key
 * @pkey: The private key
 * @usage: GNUTLS_KEY_* key usage flags.
 * @flags: should be zero
 *
 * Imports the public key from a private.  This function will import
 * the given public key to the abstract #gnutls_pubkey_t type.
 *
 * Note that in certain keys this operation may not be possible, e.g.,
 * in other than RSA PKCS#11 keys.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_import_privkey(gnutls_pubkey_t key, gnutls_privkey_t pkey,
			     unsigned int usage, unsigned int flags)
{
	gnutls_pk_params_release(&key->params);
	gnutls_pk_params_init(&key->params);

	key->key_usage = usage;
	key->params.algo = gnutls_privkey_get_pk_algorithm(pkey, &key->bits);

	return _gnutls_privkey_get_public_mpis(pkey, &key->params);
}

/**
 * gnutls_pubkey_get_preferred_hash_algorithm:
 * @key: Holds the certificate
 * @hash: The result of the call with the hash algorithm used for signature
 * @mand: If non zero it means that the algorithm MUST use this hash. May be NULL.
 *
 * This function will read the certificate and return the appropriate digest
 * algorithm to use for signing with this certificate. Some certificates (i.e.
 * DSA might not be able to sign without the preferred algorithm).
 *
 * To get the signature algorithm instead of just the hash use gnutls_pk_to_sign()
 * with the algorithm of the certificate/key and the provided @hash.
 *
 * Returns: the 0 if the hash algorithm is found. A negative error code is
 * returned on error.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_get_preferred_hash_algorithm(gnutls_pubkey_t key,
					   gnutls_digest_algorithm_t *
					   hash, unsigned int *mand)
{
	int ret;
	const mac_entry_st *me;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (mand)
		*mand = 0;

	switch (key->params.algo) {
	case GNUTLS_PK_DSA:
		if (mand)
			*mand = 1;
		FALLTHROUGH;
	case GNUTLS_PK_ECDSA:

		me = _gnutls_dsa_q_to_hash(&key->params, NULL);
		if (hash)
			*hash = (gnutls_digest_algorithm_t)me->id;

		ret = 0;
		break;
	case GNUTLS_PK_EDDSA_ED25519:
		if (hash)
			*hash = GNUTLS_DIG_SHA512;

		ret = 0;
		break;
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		if (hash)
			*hash = _gnutls_gost_digest(key->params.algo);
		if (mand)
			*mand = 1;

		ret = 0;
		break;
	case GNUTLS_PK_RSA_PSS:
		if (mand && key->params.spki.rsa_pss_dig)
			*mand = 1;

		if (hash) {
			if (key->params.spki.rsa_pss_dig) {
				*hash = key->params.spki.rsa_pss_dig;
			} else {
				*hash = _gnutls_pk_bits_to_sha_hash(pubkey_to_bits(&key->params));
			}
		}
		ret = 0;
		break;
	case GNUTLS_PK_RSA:
		if (hash)
			*hash = _gnutls_pk_bits_to_sha_hash(pubkey_to_bits(&key->params));
		ret = 0;
		break;

	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
	}

	return ret;
}

#ifdef ENABLE_PKCS11

/**
 * gnutls_pubkey_import_pkcs11:
 * @key: The public key
 * @obj: The parameters to be imported
 * @flags: should be zero
 *
 * Imports a public key from a pkcs11 key. This function will import
 * the given public key to the abstract #gnutls_pubkey_t type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_import_pkcs11(gnutls_pubkey_t key,
			    gnutls_pkcs11_obj_t obj, unsigned int flags)
{
	int ret, type;

	type = gnutls_pkcs11_obj_get_type(obj);
	if (type != GNUTLS_PKCS11_OBJ_PUBKEY
	    && type != GNUTLS_PKCS11_OBJ_X509_CRT) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (type == GNUTLS_PKCS11_OBJ_X509_CRT) {
		gnutls_x509_crt_t xcrt;

		ret = gnutls_x509_crt_init(&xcrt);
		if (ret < 0) {
			gnutls_assert()
			    return ret;
		}

		ret = gnutls_x509_crt_import_pkcs11(xcrt, obj);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup_crt;
		}

		ret = gnutls_pubkey_import_x509(key, xcrt, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup_crt;
		}

		ret = gnutls_x509_crt_get_key_usage(xcrt, &key->key_usage, NULL);
		if (ret < 0)
		  key->key_usage = 0;

		ret = 0;
	      cleanup_crt:
		gnutls_x509_crt_deinit(xcrt);
		return ret;
	}

	key->key_usage = obj->key_usage;

	switch (obj->pk_algorithm) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:
		ret = gnutls_pubkey_import_rsa_raw(key, &obj->pubkey[0],
						   &obj->pubkey[1]);
		break;
	case GNUTLS_PK_DSA:
		ret = gnutls_pubkey_import_dsa_raw(key, &obj->pubkey[0],
						   &obj->pubkey[1],
						   &obj->pubkey[2],
						   &obj->pubkey[3]);
		break;
	case GNUTLS_PK_EC:
		ret = gnutls_pubkey_import_ecc_x962(key, &obj->pubkey[0],
						    &obj->pubkey[1]);
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_UNIMPLEMENTED_FEATURE;
	}

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

#endif				/* ENABLE_PKCS11 */

/**
 * gnutls_pubkey_export:
 * @key: Holds the certificate
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a certificate PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will export the public key to DER or PEM format.
 * The contents of the exported data is the SubjectPublicKeyInfo
 * X.509 structure.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *output_data_size is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN CERTIFICATE".
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_export(gnutls_pubkey_t key,
		     gnutls_x509_crt_fmt_t format, void *output_data,
		     size_t * output_data_size)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.SubjectPublicKeyInfo", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    _gnutls_x509_encode_and_copy_PKI_params(spk, "",
						    &key->params);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_export_int_named(spk, "",
					       format, PEM_PK,
					       output_data,
					       output_data_size);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	asn1_delete_structure(&spk);

	return result;
}

/**
 * gnutls_pubkey_export2:
 * @key: Holds the certificate
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a certificate PEM or DER encoded
 *
 * This function will export the public key to DER or PEM format.
 * The contents of the exported data is the SubjectPublicKeyInfo
 * X.509 structure.
 *
 * The output buffer will be allocated using gnutls_malloc().
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN CERTIFICATE".
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 *
 * Since: 3.1.3
 **/
int
gnutls_pubkey_export2(gnutls_pubkey_t key,
		      gnutls_x509_crt_fmt_t format, gnutls_datum_t * out)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.SubjectPublicKeyInfo", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    _gnutls_x509_encode_and_copy_PKI_params(spk, "",
						    &key->params);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_export_int_named2(spk, "",
						format, PEM_PK,
						out);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	asn1_delete_structure(&spk);

	return result;
}

/**
 * gnutls_pubkey_get_key_id:
 * @key: Holds the public key
 * @flags: should be one of the flags from %gnutls_keyid_flags_t
 * @output_data: will contain the key ID
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will return a unique ID that depends on the public
 * key parameters. This ID can be used in checking whether a
 * certificate corresponds to the given public key.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *output_data_size is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.  The output will normally be a SHA-1 hash output,
 * which is 20 bytes.
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_get_key_id(gnutls_pubkey_t key, unsigned int flags,
			 unsigned char *output_data,
			 size_t * output_data_size)
{
	int ret = 0;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _gnutls_get_key_id(&key->params,
			       output_data, output_data_size, flags);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_pubkey_export_rsa_raw2:
 * @key: Holds the certificate
 * @m: will hold the modulus (may be %NULL)
 * @e: will hold the public exponent (may be %NULL)
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the RSA public key's parameters found in
 * the given structure.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * This function allows for %NULL parameters since 3.4.1.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.0
 **/
int
gnutls_pubkey_export_rsa_raw2(gnutls_pubkey_t key,
			     gnutls_datum_t * m, gnutls_datum_t * e,
			     unsigned flags)
{
	int ret;
	mpi_dprint_func dprint = _gnutls_mpi_dprint_lz;

	if (flags & GNUTLS_EXPORT_FLAG_NO_LZ)
		dprint = _gnutls_mpi_dprint;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (!GNUTLS_PK_IS_RSA(key->params.algo)) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (m) {
		ret = dprint(key->params.params[0], m);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	if (e) {
		ret = dprint(key->params.params[1], e);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(m);
			return ret;
		}
	}

	return 0;
}

/**
 * gnutls_pubkey_export_rsa_raw:
 * @key: Holds the certificate
 * @m: will hold the modulus (may be %NULL)
 * @e: will hold the public exponent (may be %NULL)
 *
 * This function will export the RSA public key's parameters found in
 * the given structure.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * This function allows for %NULL parameters since 3.4.1.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.3.0
 **/
int
gnutls_pubkey_export_rsa_raw(gnutls_pubkey_t key,
			     gnutls_datum_t * m, gnutls_datum_t * e)
{
	return gnutls_pubkey_export_rsa_raw2(key, m, e, 0);
}


/**
 * gnutls_pubkey_export_dsa_raw:
 * @key: Holds the public key
 * @p: will hold the p (may be %NULL)
 * @q: will hold the q (may be %NULL)
 * @g: will hold the g (may be %NULL)
 * @y: will hold the y (may be %NULL)
 *
 * This function will export the DSA public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * This function allows for %NULL parameters since 3.4.1.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.3.0
 **/
int
gnutls_pubkey_export_dsa_raw(gnutls_pubkey_t key,
			     gnutls_datum_t * p, gnutls_datum_t * q,
			     gnutls_datum_t * g, gnutls_datum_t * y)
{
	return gnutls_pubkey_export_dsa_raw2(key, p, q, g, y, 0);
}

/**
 * gnutls_pubkey_export_dsa_raw2:
 * @key: Holds the public key
 * @p: will hold the p (may be %NULL)
 * @q: will hold the q (may be %NULL)
 * @g: will hold the g (may be %NULL)
 * @y: will hold the y (may be %NULL)
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the DSA public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * This function allows for %NULL parameters since 3.4.1.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.0
 **/
int
gnutls_pubkey_export_dsa_raw2(gnutls_pubkey_t key,
			     gnutls_datum_t * p, gnutls_datum_t * q,
			     gnutls_datum_t * g, gnutls_datum_t * y,
			     unsigned flags)
{
	int ret;
	mpi_dprint_func dprint = _gnutls_mpi_dprint_lz;

	if (flags & GNUTLS_EXPORT_FLAG_NO_LZ)
		dprint = _gnutls_mpi_dprint;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key->params.algo != GNUTLS_PK_DSA) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* P */
	if (p) {
		ret = dprint(key->params.params[0], p);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	/* Q */
	if (q) {
		ret = dprint(key->params.params[1], q);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(p);
			return ret;
		}
	}

	/* G */
	if (g) {
		ret = dprint(key->params.params[2], g);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(p);
			_gnutls_free_datum(q);
			return ret;
		}
	}

	/* Y */
	if (y) {
		ret = dprint(key->params.params[3], y);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(p);
			_gnutls_free_datum(g);
			_gnutls_free_datum(q);
			return ret;
		}
	}

	return 0;
}

/**
 * gnutls_pubkey_export_ecc_raw:
 * @key: Holds the public key
 * @curve: will hold the curve (may be %NULL)
 * @x: will hold x-coordinate (may be %NULL)
 * @y: will hold y-coordinate (may be %NULL)
 *
 * This function will export the ECC public key's parameters found in
 * the given key.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * In EdDSA curves the @y parameter will be %NULL and the other parameters
 * will be in the native format for the curve.
 *
 * This function allows for %NULL parameters since 3.4.1.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.0
 **/
int
gnutls_pubkey_export_ecc_raw(gnutls_pubkey_t key,
			     gnutls_ecc_curve_t * curve,
			     gnutls_datum_t * x, gnutls_datum_t * y)
{
	return gnutls_pubkey_export_ecc_raw2(key, curve, x, y, 0);
}

/**
 * gnutls_pubkey_export_ecc_raw2:
 * @key: Holds the public key
 * @curve: will hold the curve (may be %NULL)
 * @x: will hold x-coordinate (may be %NULL)
 * @y: will hold y-coordinate (may be %NULL)
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the ECC public key's parameters found in
 * the given key.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * In EdDSA curves the @y parameter will be %NULL and the other parameters
 * will be in the native format for the curve.
 *
 * This function allows for %NULL parameters since 3.4.1.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.0
 **/
int
gnutls_pubkey_export_ecc_raw2(gnutls_pubkey_t key,
			     gnutls_ecc_curve_t * curve,
			     gnutls_datum_t * x, gnutls_datum_t * y,
			     unsigned int flags)
{
	int ret;
	mpi_dprint_func dprint = _gnutls_mpi_dprint_lz;

	if (flags & GNUTLS_EXPORT_FLAG_NO_LZ)
		dprint = _gnutls_mpi_dprint;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (!IS_EC(key->params.algo)) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (curve)
		*curve = key->params.curve;

	if (key->params.algo == GNUTLS_PK_EDDSA_ED25519) {
		if (x) {
			ret = _gnutls_set_datum(x, key->params.raw_pub.data, key->params.raw_pub.size);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
		if (y) {
			y->data = NULL;
			y->size = 0;
		}
		return 0;
	}

	/* ECDSA */

	/* X */
	if (x) {
		ret = dprint(key->params.params[ECC_X], x);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	/* Y */
	if (y) {
		ret = dprint(key->params.params[ECC_Y], y);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(x);
			return ret;
		}
	}

	return 0;
}

/**
 * gnutls_pubkey_export_ecc_x962:
 * @key: Holds the public key
 * @parameters: DER encoding of an ANSI X9.62 parameters
 * @ecpoint: DER encoding of ANSI X9.62 ECPoint
 *
 * This function will export the ECC public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.3.0
 **/
int gnutls_pubkey_export_ecc_x962(gnutls_pubkey_t key,
				  gnutls_datum_t * parameters,
				  gnutls_datum_t * ecpoint)
{
	int ret;
	gnutls_datum_t raw_point = {NULL,0};

	if (key == NULL || key->params.algo != GNUTLS_PK_EC)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = _gnutls_x509_write_ecc_pubkey(&key->params, &raw_point);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_x509_encode_string(ASN1_ETYPE_OCTET_STRING,
					 raw_point.data, raw_point.size, ecpoint);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_write_ecc_params(key->params.curve, parameters);
	if (ret < 0) {
		_gnutls_free_datum(ecpoint);
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(raw_point.data);
	return ret;
}

/**
 * gnutls_pubkey_export_gost_raw2:
 * @key: Holds the public key
 * @curve: will hold the curve (may be %NULL)
 * @digest: will hold the curve (may be %NULL)
 * @paramset: will hold the parameters id (may be %NULL)
 * @x: will hold the x-coordinate (may be %NULL)
 * @y: will hold the y-coordinate (may be %NULL)
 * @flags: flags from %gnutls_abstract_export_flags_t
 *
 * This function will export the GOST public key's parameters found in
 * the given key.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Note: parameters will be stored with least significant byte first. On
 * version 3.6.3 this was incorrectly returned in big-endian format.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.3
 **/
int
gnutls_pubkey_export_gost_raw2(gnutls_pubkey_t key,
			       gnutls_ecc_curve_t * curve,
			       gnutls_digest_algorithm_t * digest,
			       gnutls_gost_paramset_t * paramset,
			       gnutls_datum_t * x, gnutls_datum_t * y,
			       unsigned int flags)
{
	int ret;

	mpi_dprint_func dprint = _gnutls_mpi_dprint_le;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key->params.algo != GNUTLS_PK_GOST_01 &&
	    key->params.algo != GNUTLS_PK_GOST_12_256 &&
	    key->params.algo != GNUTLS_PK_GOST_12_512) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (curve)
		*curve = key->params.curve;

	if (digest)
		*digest = _gnutls_gost_digest(key->params.algo);

	if (paramset)
		*paramset = key->params.gost_params;

	/* X */
	if (x) {
		ret = dprint(key->params.params[GOST_X], x);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	/* Y */
	if (y) {
		ret = dprint(key->params.params[GOST_Y], y);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(x);
			return ret;
		}
	}

	return 0;
}

/**
 * gnutls_pubkey_import:
 * @key: The public key. 
 * @data: The DER or PEM encoded certificate. 
 * @format: One of DER or PEM 
 * 
 * This function will import the provided public key in
 * a SubjectPublicKeyInfo X.509 structure to a native
 * %gnutls_pubkey_t type. The output will be stored 
 * in @key. If the public key is PEM encoded it should have a header 
 * of "PUBLIC KEY". 
 * 
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 * negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_import(gnutls_pubkey_t key,
		     const gnutls_datum_t * data,
		     gnutls_x509_crt_fmt_t format)
{
	int result = 0, need_free = 0;
	gnutls_datum_t _data;
	ASN1_TYPE spk;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	_data.data = data->data;
	_data.size = data->size;

	/* If the Certificate is in PEM format then decode it
	 */
	if (format == GNUTLS_X509_FMT_PEM) {
		/* Try the first header */
		result =
		    _gnutls_fbase64_decode(PEM_PK, data->data,
					   data->size, &_data);

		if (result < 0) {
			gnutls_assert();
			return result;
		}

		need_free = 1;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.SubjectPublicKeyInfo", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _asn1_strict_der_decode(&spk, _data.data, _data.size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _gnutls_get_asn_mpis(spk, "", &key->params);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	key->bits = pubkey_to_bits(&key->params);
	result = 0;

      cleanup:
	asn1_delete_structure(&spk);

	if (need_free)
		_gnutls_free_datum(&_data);
	return result;
}

/**
 * gnutls_x509_crt_set_pubkey:
 * @crt: should contain a #gnutls_x509_crt_t type
 * @key: holds a public key
 *
 * This function will set the public parameters from the given public
 * key to the certificate. The @key can be deallocated after that.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_x509_crt_set_pubkey(gnutls_x509_crt_t crt, gnutls_pubkey_t key)
{
	int result;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = _gnutls_x509_encode_and_copy_PKI_params(crt->cert,
							 "tbsCertificate.subjectPublicKeyInfo",
							 &key->params);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	if (key->key_usage)
		gnutls_x509_crt_set_key_usage(crt, key->key_usage);

	return 0;
}

/**
 * gnutls_x509_crq_set_pubkey:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @key: holds a public key
 *
 * This function will set the public parameters from the given public
 * key to the request. The @key can be deallocated after that.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_x509_crq_set_pubkey(gnutls_x509_crq_t crq, gnutls_pubkey_t key)
{
	int result;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = _gnutls_x509_encode_and_copy_PKI_params
	    (crq->crq,
	     "certificationRequestInfo.subjectPKInfo",
	     &key->params);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	if (key->key_usage)
		gnutls_x509_crq_set_key_usage(crq, key->key_usage);

	return 0;
}

/**
 * gnutls_pubkey_set_key_usage:
 * @key: a certificate of type #gnutls_x509_crt_t
 * @usage: an ORed sequence of the GNUTLS_KEY_* elements.
 *
 * This function will set the key usage flags of the public key. This
 * is only useful if the key is to be exported to a certificate or
 * certificate request.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_pubkey_set_key_usage(gnutls_pubkey_t key, unsigned int usage)
{
	key->key_usage = usage;

	return 0;
}

#ifdef ENABLE_PKCS11

#if 0
/**
 * gnutls_pubkey_import_pkcs11_url:
 * @key: A key of type #gnutls_pubkey_t
 * @url: A PKCS 11 url
 * @flags: One of GNUTLS_PKCS11_OBJ_* flags
 *
 * This function will import a PKCS 11 certificate to a #gnutls_pubkey_t
 * structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_import_pkcs11_url(gnutls_pubkey_t key, const char *url,
				unsigned int flags)
{
	int x;
}
#endif

static int
_gnutls_pubkey_import_pkcs11_url(gnutls_pubkey_t key, const char *url,
				unsigned int flags)
{
	gnutls_pkcs11_obj_t pcrt;
	int ret;

	ret = gnutls_pkcs11_obj_init(&pcrt);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (key->pin.cb)
		gnutls_pkcs11_obj_set_pin_function(pcrt, key->pin.cb,
						   key->pin.data);

	ret = gnutls_pkcs11_obj_import_url(pcrt, url, flags|GNUTLS_PKCS11_OBJ_FLAG_EXPECT_PUBKEY);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_import_pkcs11(key, pcrt, flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
      cleanup:

	gnutls_pkcs11_obj_deinit(pcrt);

	return ret;
}

#endif				/* ENABLE_PKCS11 */

/**
 * gnutls_pubkey_import_url:
 * @key: A key of type #gnutls_pubkey_t
 * @url: A PKCS 11 url
 * @flags: One of GNUTLS_PKCS11_OBJ_* flags
 *
 * This function will import a public key from the provided URL.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 **/
int
gnutls_pubkey_import_url(gnutls_pubkey_t key, const char *url,
			 unsigned int flags)
{
	unsigned i;

	for (i=0;i<_gnutls_custom_urls_size;i++) {
		if (strncmp(url, _gnutls_custom_urls[i].name, _gnutls_custom_urls[i].name_size) == 0) {
			if (_gnutls_custom_urls[i].import_pubkey)
				return _gnutls_custom_urls[i].import_pubkey(key, url, flags);
		}
	}

	if (strncmp(url, PKCS11_URL, PKCS11_URL_SIZE) == 0)
#ifdef ENABLE_PKCS11
		return _gnutls_pubkey_import_pkcs11_url(key, url, flags);
#else
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
#endif

	if (strncmp(url, TPMKEY_URL, TPMKEY_URL_SIZE) == 0)
#ifdef HAVE_TROUSERS
		return gnutls_pubkey_import_tpm_url(key, url, NULL, 0);
#else
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
#endif

	return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
}

/**
 * gnutls_pubkey_import_rsa_raw:
 * @key: The key
 * @m: holds the modulus
 * @e: holds the public exponent
 *
 * This function will replace the parameters in the given structure.
 * The new parameters should be stored in the appropriate
 * gnutls_datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an negative error code.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_import_rsa_raw(gnutls_pubkey_t key,
			     const gnutls_datum_t * m,
			     const gnutls_datum_t * e)
{
	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_release(&key->params);
	gnutls_pk_params_init(&key->params);

	if (_gnutls_mpi_init_scan_nz(&key->params.params[0], m->data, m->size)) {
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	if (_gnutls_mpi_init_scan_nz(&key->params.params[1], e->data, e->size)) {
		gnutls_assert();
		_gnutls_mpi_release(&key->params.params[0]);
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	key->params.params_nr = RSA_PUBLIC_PARAMS;
	key->params.algo = GNUTLS_PK_RSA;
	key->bits = pubkey_to_bits(&key->params);

	return 0;
}

/**
 * gnutls_pubkey_import_ecc_raw:
 * @key: The structure to store the parsed key
 * @curve: holds the curve
 * @x: holds the x-coordinate
 * @y: holds the y-coordinate
 *
 * This function will convert the given elliptic curve parameters to a
 * #gnutls_pubkey_t.  The output will be stored in @key.
 *
 * In EdDSA curves the @y parameter should be %NULL and the @x parameter must
 * be the value in the native format for the curve.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_pubkey_import_ecc_raw(gnutls_pubkey_t key,
			     gnutls_ecc_curve_t curve,
			     const gnutls_datum_t * x,
			     const gnutls_datum_t * y)
{
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_release(&key->params);
	gnutls_pk_params_init(&key->params);

	if (curve_is_eddsa(curve)) {
		unsigned size = gnutls_ecc_curve_get_size(curve);
		if (x->size != size) {
			ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
			goto cleanup;
		}

		ret = _gnutls_set_datum(&key->params.raw_pub, x->data, x->size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		key->params.algo = GNUTLS_PK_EDDSA_ED25519;
		key->params.curve = curve;
		key->bits = pubkey_to_bits(&key->params);

		return 0;
	}

	/* ECDSA */
	key->params.curve = curve;

	if (_gnutls_mpi_init_scan_nz
	    (&key->params.params[ECC_X], x->data, x->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	if (_gnutls_mpi_init_scan_nz
	    (&key->params.params[ECC_Y], y->data, y->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;
	key->params.algo = GNUTLS_PK_ECDSA;
	key->bits = pubkey_to_bits(&key->params);

	return 0;

      cleanup:
	gnutls_pk_params_release(&key->params);
	return ret;
}

/**
 * gnutls_pubkey_import_ecc_x962:
 * @key: The structure to store the parsed key
 * @parameters: DER encoding of an ANSI X9.62 parameters
 * @ecpoint: DER encoding of ANSI X9.62 ECPoint
 *
 * This function will convert the given elliptic curve parameters to a
 * #gnutls_pubkey_t.  The output will be stored in @key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_pubkey_import_ecc_x962(gnutls_pubkey_t key,
			      const gnutls_datum_t * parameters,
			      const gnutls_datum_t * ecpoint)
{
	int ret;
	gnutls_datum_t raw_point = {NULL,0};

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_release(&key->params);
	gnutls_pk_params_init(&key->params);

	key->params.params_nr = 0;

	ret =
	    _gnutls_x509_read_ecc_params(parameters->data,
					 parameters->size, &key->params.curve);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_decode_string(ASN1_ETYPE_OCTET_STRING,
					 ecpoint->data, ecpoint->size, &raw_point, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_ecc_ansi_x962_import(raw_point.data, raw_point.size,
					   &key->params.params[ECC_X],
					   &key->params.params[ECC_Y]);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}
	key->params.params_nr += 2;
	key->params.algo = GNUTLS_PK_EC;

	gnutls_free(raw_point.data);
	return 0;

      cleanup:
	gnutls_pk_params_release(&key->params);
	gnutls_free(raw_point.data);
	return ret;
}

/**
 * gnutls_pubkey_import_gost_raw:
 * @key: The structure to store the parsed key
 * @curve: holds the curve
 * @digest: holds the digest
 * @paramset: holds the parameters id
 * @x: holds the x-coordinate
 * @y: holds the y-coordinate
 *
 * This function will convert the given GOST public key's parameters to a
 * #gnutls_pubkey_t.  The output will be stored in @key.  @digest should be
 * one of GNUTLS_DIG_GOSR_94, GNUTLS_DIG_STREEBOG_256 or
 * GNUTLS_DIG_STREEBOG_512.  If @paramset is set to GNUTLS_GOST_PARAMSET_UNKNOWN
 * default one will be selected depending on @digest.
 *
 * Note: parameters should be stored with least significant byte first. On
 * version 3.6.3 big-endian format was used incorrectly.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.3
 **/
int
gnutls_pubkey_import_gost_raw(gnutls_pubkey_t key,
			     gnutls_ecc_curve_t curve,
			     gnutls_digest_algorithm_t digest,
			     gnutls_gost_paramset_t paramset,
			     const gnutls_datum_t * x,
			     const gnutls_datum_t * y)
{
	int ret;
	gnutls_pk_algorithm_t pk_algo;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	pk_algo = _gnutls_digest_gost(digest);
	if (pk_algo == GNUTLS_PK_UNKNOWN)
		return GNUTLS_E_ILLEGAL_PARAMETER;

	if (paramset == GNUTLS_GOST_PARAMSET_UNKNOWN)
		paramset = _gnutls_gost_paramset_default(pk_algo);

	gnutls_pk_params_release(&key->params);
	gnutls_pk_params_init(&key->params);

	key->params.curve = curve;
	key->params.gost_params = paramset;

	if (_gnutls_mpi_init_scan_le
	    (&key->params.params[GOST_X], x->data, x->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	if (_gnutls_mpi_init_scan_le
	    (&key->params.params[GOST_Y], y->data, y->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;
	key->params.algo = pk_algo;

	return 0;

      cleanup:
	gnutls_pk_params_release(&key->params);
	return ret;
}

/**
 * gnutls_pubkey_import_dsa_raw:
 * @key: The structure to store the parsed key
 * @p: holds the p
 * @q: holds the q
 * @g: holds the g
 * @y: holds the y
 *
 * This function will convert the given DSA raw parameters to the
 * native #gnutls_pubkey_t format.  The output will be stored
 * in @key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_pubkey_import_dsa_raw(gnutls_pubkey_t key,
			     const gnutls_datum_t * p,
			     const gnutls_datum_t * q,
			     const gnutls_datum_t * g,
			     const gnutls_datum_t * y)
{
	size_t siz = 0;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_release(&key->params);
	gnutls_pk_params_init(&key->params);

	siz = p->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[0], p->data, siz)) {
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	siz = q->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[1], q->data, siz)) {
		gnutls_assert();
		_gnutls_mpi_release(&key->params.params[0]);
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	siz = g->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[2], g->data, siz)) {
		gnutls_assert();
		_gnutls_mpi_release(&key->params.params[1]);
		_gnutls_mpi_release(&key->params.params[0]);
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	siz = y->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[3], y->data, siz)) {
		gnutls_assert();
		_gnutls_mpi_release(&key->params.params[2]);
		_gnutls_mpi_release(&key->params.params[1]);
		_gnutls_mpi_release(&key->params.params[0]);
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	key->params.params_nr = DSA_PUBLIC_PARAMS;
	key->params.algo = GNUTLS_PK_DSA;
	key->bits = pubkey_to_bits(&key->params);

	return 0;

}

/* Updates the gnutls_x509_spki_st parameters based on the signature
 * information, and reports any incompatibilities between the existing
 * parameters (if any) with the signature algorithm */
static
int fixup_spki_params(const gnutls_pk_params_st *key_params, const gnutls_sign_entry_st *se,
		       const mac_entry_st *me, gnutls_x509_spki_st *params)
{
	unsigned bits;

	if (se->pk != key_params->algo) {
		if (!sign_supports_priv_pk_algorithm(se, key_params->algo)) {
			_gnutls_debug_log("have key: %s/%d, with sign %s/%d\n",
					gnutls_pk_get_name(key_params->algo), key_params->algo,
					se->name, se->id);
			return gnutls_assert_val(GNUTLS_E_CONSTRAINT_ERROR);
		}
	}

	if (params->pk == GNUTLS_PK_RSA_PSS) {
		int ret;
		if (!GNUTLS_PK_IS_RSA(key_params->algo))
			return gnutls_assert_val(GNUTLS_E_CONSTRAINT_ERROR);

		/* The requested sign algorithm is RSA-PSS, while the
		 * pubkey doesn't include parameter information. Fill
		 * it with the same way as gnutls_privkey_sign*. */
		if (key_params->algo == GNUTLS_PK_RSA || params->rsa_pss_dig == 0) {
			bits = pubkey_to_bits(key_params);
			params->rsa_pss_dig = se->hash;
			ret = _gnutls_find_rsa_pss_salt_size(bits, me, 0);
			if (ret < 0)
				return gnutls_assert_val(ret);

			params->salt_size = ret;
		}

		if (params->rsa_pss_dig != se->hash)
			return gnutls_assert_val(GNUTLS_E_CONSTRAINT_ERROR);
	}

	return 0;
}

/**
 * gnutls_pubkey_verify_data2:
 * @pubkey: Holds the public key
 * @algo: The signature algorithm used
 * @flags: Zero or an OR list of #gnutls_certificate_verify_flags
 * @data: holds the signed data
 * @signature: contains the signature
 *
 * This function will verify the given signed data, using the
 * parameters from the certificate.
 *
 * Returns: In case of a verification failure %GNUTLS_E_PK_SIG_VERIFY_FAILED 
 * is returned, and zero or positive code on success. For known to be insecure
 * signatures this function will return %GNUTLS_E_INSUFFICIENT_SECURITY unless
 * the flag %GNUTLS_VERIFY_ALLOW_BROKEN is specified.
 *
 * Since: 3.0
 **/
int
gnutls_pubkey_verify_data2(gnutls_pubkey_t pubkey,
			   gnutls_sign_algorithm_t algo,
			   unsigned int flags,
			   const gnutls_datum_t * data,
			   const gnutls_datum_t * signature)
{
	int ret;
	const mac_entry_st *me;
	gnutls_x509_spki_st params;
	const gnutls_sign_entry_st *se;

	if (pubkey == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (flags & GNUTLS_VERIFY_USE_TLS1_RSA)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memcpy(&params, &pubkey->params.spki, sizeof(gnutls_x509_spki_st));

	se = _gnutls_sign_to_entry(algo);
	if (se == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = pubkey_supports_sig(pubkey, se);
	if (ret < 0)
		return gnutls_assert_val(ret);

	params.pk = se->pk;

	me = hash_to_entry(se->hash);
	if (me == NULL && !_gnutls_pk_is_not_prehashed(se->pk))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = pubkey_verify_data(se, me, data, signature, &pubkey->params,
				 &params, flags);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_pubkey_verify_hash2:
 * @key: Holds the public key
 * @algo: The signature algorithm used
 * @flags: Zero or an OR list of #gnutls_certificate_verify_flags
 * @hash: holds the hash digest to be verified
 * @signature: contains the signature
 *
 * This function will verify the given signed digest, using the
 * parameters from the public key. Note that unlike gnutls_privkey_sign_hash(),
 * this function accepts a signature algorithm instead of a digest algorithm.
 * You can use gnutls_pk_to_sign() to get the appropriate value.
 *
 * Returns: In case of a verification failure %GNUTLS_E_PK_SIG_VERIFY_FAILED 
 * is returned, and zero or positive code on success. For known to be insecure
 * signatures this function will return %GNUTLS_E_INSUFFICIENT_SECURITY unless
 * the flag %GNUTLS_VERIFY_ALLOW_BROKEN is specified.
 *
 * Since: 3.0
 **/
int
gnutls_pubkey_verify_hash2(gnutls_pubkey_t key,
			   gnutls_sign_algorithm_t algo,
			   unsigned int flags,
			   const gnutls_datum_t * hash,
			   const gnutls_datum_t * signature)
{
	const mac_entry_st *me;
	gnutls_x509_spki_st params;
	const gnutls_sign_entry_st *se;
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (_gnutls_pk_is_not_prehashed(key->params.algo)) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	memcpy(&params, &key->params.spki, sizeof(gnutls_x509_spki_st));

	if (flags & GNUTLS_VERIFY_USE_TLS1_RSA) {
		if (!GNUTLS_PK_IS_RSA(key->params.algo))
			return gnutls_assert_val(GNUTLS_E_INCOMPATIBLE_SIG_WITH_KEY);
		params.pk = GNUTLS_PK_RSA;
		/* we do not check for insecure algorithms with this flag */
		return _gnutls_pk_verify(params.pk, hash, signature,
					 &key->params, &params);
	} else {
		se = _gnutls_sign_to_entry(algo);
		if (se == NULL)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		ret = pubkey_supports_sig(key, se);
		if (ret < 0)
			return gnutls_assert_val(ret);

		params.pk = se->pk;

		me = hash_to_entry(se->hash);
		if (me == NULL && !_gnutls_pk_is_not_prehashed(se->pk))
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		ret = pubkey_verify_hashed_data(se, me, hash, signature,
						&key->params,
						&params, flags);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	return 0;
}

/**
 * gnutls_pubkey_encrypt_data:
 * @key: Holds the public key
 * @flags: should be 0 for now
 * @plaintext: The data to be encrypted
 * @ciphertext: contains the encrypted data
 *
 * This function will encrypt the given data, using the public
 * key. On success the @ciphertext will be allocated using gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_pubkey_encrypt_data(gnutls_pubkey_t key, unsigned int flags,
			   const gnutls_datum_t * plaintext,
			   gnutls_datum_t * ciphertext)
{
	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_pk_encrypt(key->params.algo, ciphertext,
				  plaintext, &key->params);
}

static
int pubkey_supports_sig(gnutls_pubkey_t pubkey,
			const gnutls_sign_entry_st *se)
{
	if (pubkey->params.algo == GNUTLS_PK_ECDSA && se->curve) {
		gnutls_ecc_curve_t curve = pubkey->params.curve;

		if (curve != se->curve) {
			_gnutls_handshake_log("have key: ECDSA with %s/%d, with sign %s/%d\n",
				gnutls_ecc_curve_get_name(curve), (int)curve,
				se->name, se->id);
			return gnutls_assert_val(GNUTLS_E_INCOMPATIBLE_SIG_WITH_KEY);
		}
	}

	if (se->pk != pubkey->params.algo) { /* if the PK algorithm of the signature differs to the one on the pubkey */
		if (!sign_supports_priv_pk_algorithm(se, pubkey->params.algo)) {
			_gnutls_handshake_log("have key: %s/%d, with sign %s/%d\n",
					gnutls_pk_get_name(pubkey->params.algo), pubkey->params.algo,
					se->name, se->id);
			return gnutls_assert_val(GNUTLS_E_INCOMPATIBLE_SIG_WITH_KEY);
		}
	}

	return 0;
}

/* Checks whether the public key given is compatible with the
 * signature algorithm used. The session is only used for audit logging, and
 * it may be null.
 */
int _gnutls_pubkey_compatible_with_sig(gnutls_session_t session,
				       gnutls_pubkey_t pubkey,
				       const version_entry_st * ver,
				       gnutls_sign_algorithm_t sign)
{
	unsigned int hash_size = 0;
	unsigned int sig_hash_size;
	const mac_entry_st *me;
	const gnutls_sign_entry_st *se;

	se = _gnutls_sign_to_entry(sign);
	if (se == NULL && _gnutls_version_has_selectable_sighash(ver))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (pubkey->params.algo == GNUTLS_PK_DSA) {
		me = _gnutls_dsa_q_to_hash(&pubkey->params, &hash_size);

		/* DSA keys over 1024 bits cannot be used with TLS 1.x, x<2 */
		if (!_gnutls_version_has_selectable_sighash(ver)) {
			if (me->id != GNUTLS_MAC_SHA1)
				return
				    gnutls_assert_val
				    (GNUTLS_E_INCOMPAT_DSA_KEY_WITH_TLS_PROTOCOL);
		} else if (se != NULL) {
			me = hash_to_entry(se->hash);

			sig_hash_size = _gnutls_hash_get_algo_len(me);
			if (sig_hash_size < hash_size)
				_gnutls_audit_log(session,
						  "The hash size used in signature (%u) is less than the expected (%u)\n",
						  sig_hash_size,
						  hash_size);
		}

	} else if (pubkey->params.algo == GNUTLS_PK_ECDSA) {
		if (_gnutls_version_has_selectable_sighash(ver)
		    && se != NULL) {
			_gnutls_dsa_q_to_hash(&pubkey->params, &hash_size);

			me = hash_to_entry(se->hash);

			sig_hash_size = _gnutls_hash_get_algo_len(me);

			if (sig_hash_size < hash_size)
				_gnutls_audit_log(session,
						  "The hash size used in signature (%u) is less than the expected (%u)\n",
						  sig_hash_size,
						  hash_size);
		}

	} else if (pubkey->params.algo == GNUTLS_PK_GOST_01 ||
		   pubkey->params.algo == GNUTLS_PK_GOST_12_256 ||
		   pubkey->params.algo == GNUTLS_PK_GOST_12_512) {
		if (_gnutls_version_has_selectable_sighash(ver)
		    && se != NULL) {
			if (_gnutls_gost_digest(pubkey->params.algo) != se->hash) {
				_gnutls_audit_log(session,
						  "The hash algo used in signature (%u) is not expected (%u)\n",
						  se->hash, _gnutls_gost_digest(pubkey->params.algo));
				return gnutls_assert_val(GNUTLS_E_CONSTRAINT_ERROR);
			}
		}

	} else if (pubkey->params.algo == GNUTLS_PK_RSA_PSS) {
		if (!_gnutls_version_has_selectable_sighash(ver))
			/* this should not have happened */
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		/* RSA PSS public keys are restricted to a single digest, i.e., signature */

		if (pubkey->params.spki.rsa_pss_dig && pubkey->params.spki.rsa_pss_dig != se->hash) {
			return gnutls_assert_val(GNUTLS_E_CONSTRAINT_ERROR);
		}
	}

	if (se != NULL)
		return pubkey_supports_sig(pubkey, se);

	return 0;
}

/* Returns the public key. 
 */
int
_gnutls_pubkey_get_mpis(gnutls_pubkey_t key, gnutls_pk_params_st * params)
{
	return _gnutls_pk_params_copy(params, &key->params);
}

/* if hash==MD5 then we do RSA-MD5
 * if hash==SHA then we do RSA-SHA
 * params[0] is modulus
 * params[1] is public key
 */
static int
_pkcs1_rsa_verify_sig(gnutls_pk_algorithm_t pk,
		      const mac_entry_st * me,
		      const gnutls_datum_t * text,
		      const gnutls_datum_t * prehash,
		      const gnutls_datum_t * signature,
		      gnutls_pk_params_st * params,
		      gnutls_x509_spki_st * sign_params)
{
	int ret;
	uint8_t md[MAX_HASH_SIZE], *cmp;
	unsigned int digest_size;
	gnutls_datum_t d, di;

	if (unlikely(me == NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	digest_size = _gnutls_hash_get_algo_len(me);
	if (prehash) {
		if (prehash->data == NULL || prehash->size != digest_size)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		cmp = prehash->data;
	} else {
		if (!text) {
			gnutls_assert();
			return GNUTLS_E_INVALID_REQUEST;
		}

		ret = _gnutls_hash_fast((gnutls_digest_algorithm_t)me->id,
					text->data, text->size, md);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		cmp = md;
	}

	d.data = cmp;
	d.size = digest_size;

	if (pk == GNUTLS_PK_RSA) {
		/* decrypted is a BER encoded data of type DigestInfo
		 */
		ret = encode_ber_digest_info(me, &d, &di);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_pk_verify(pk, &di, signature, params,
					sign_params);
		_gnutls_free_datum(&di);
	} else {
		ret = _gnutls_pk_verify(pk, &d, signature, params,
					sign_params);
	}

	return ret;
}

/* Hashes input data and verifies a signature.
 */
static int
dsa_verify_hashed_data(gnutls_pk_algorithm_t pk,
		       const mac_entry_st * algo,
		       const gnutls_datum_t * hash,
		       const gnutls_datum_t * signature,
		       gnutls_pk_params_st * params,
		       gnutls_x509_spki_st * sign_params)
{
	gnutls_datum_t digest;
	unsigned int hash_len;

	if (algo == NULL)
		algo = _gnutls_dsa_q_to_hash(params, &hash_len);
	else
		hash_len = _gnutls_hash_get_algo_len(algo);

	/* SHA1 or better allowed */
	if (!hash->data || hash->size < hash_len) {
		gnutls_assert();
		_gnutls_debug_log
		    ("Hash size (%d) does not correspond to hash %s(%d) or better.\n",
		     (int) hash->size, _gnutls_mac_get_name(algo),
		     hash_len);

		if (hash->size != 20)	/* SHA1 is allowed */
			return
			    gnutls_assert_val
			    (GNUTLS_E_PK_SIG_VERIFY_FAILED);
	}

	digest.data = hash->data;
	digest.size = hash->size;

	return _gnutls_pk_verify(pk, &digest, signature, params, sign_params);
}

static int
dsa_verify_data(gnutls_pk_algorithm_t pk,
		const mac_entry_st * algo,
		const gnutls_datum_t * data,
		const gnutls_datum_t * signature,
		gnutls_pk_params_st * params,
		gnutls_x509_spki_st * sign_params)
{
	int ret;
	uint8_t _digest[MAX_HASH_SIZE];
	gnutls_datum_t digest;

	if (algo == NULL)
		algo = _gnutls_dsa_q_to_hash(params, NULL);

	ret = _gnutls_hash_fast((gnutls_digest_algorithm_t)algo->id,
				data->data, data->size, _digest);
	if (ret < 0)
		return gnutls_assert_val(ret);

	digest.data = _digest;
	digest.size = _gnutls_hash_get_algo_len(algo);

	return _gnutls_pk_verify(pk, &digest, signature, params, sign_params);
}

/* Verifies the signature data, and returns GNUTLS_E_PK_SIG_VERIFY_FAILED if 
 * not verified, or 1 otherwise.
 */
static int
pubkey_verify_hashed_data(const gnutls_sign_entry_st *se,
			  const mac_entry_st *me,
			  const gnutls_datum_t * hash,
			  const gnutls_datum_t * signature,
			  gnutls_pk_params_st * params,
			  gnutls_x509_spki_st * sign_params,
			  unsigned flags)
{
	int ret;

	if (unlikely(me==NULL))
		return gnutls_assert_val(GNUTLS_E_UNKNOWN_HASH_ALGORITHM);

	ret = fixup_spki_params(params, se, me, sign_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	switch (se->pk) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:

		if (_pkcs1_rsa_verify_sig
		    (se->pk, me, NULL, hash, signature, params, sign_params) != 0)
		{
			gnutls_assert();
			return GNUTLS_E_PK_SIG_VERIFY_FAILED;
		}

		break;

	case GNUTLS_PK_ECDSA:
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
	case GNUTLS_PK_DSA:
		if (dsa_verify_hashed_data
		    (se->pk, me, hash, signature, params, sign_params) != 0) {
			gnutls_assert();
			return GNUTLS_E_PK_SIG_VERIFY_FAILED;
		}

		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;

	}

	if (_gnutls_sign_is_secure2(se, 0) == 0 && _gnutls_is_broken_sig_allowed(se, flags) == 0) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_SECURITY);
	}

	return 1;
}

/* Verifies the signature data, and returns GNUTLS_E_PK_SIG_VERIFY_FAILED if 
 * not verified, or 1 otherwise.
 */
int
pubkey_verify_data(const gnutls_sign_entry_st *se,
		   const mac_entry_st *me,
		   const gnutls_datum_t * data,
		   const gnutls_datum_t * signature,
		   gnutls_pk_params_st * params,
		   gnutls_x509_spki_st * sign_params,
		   unsigned flags)
{
	int ret;

	if (unlikely(me == NULL))
		return gnutls_assert_val(GNUTLS_E_UNKNOWN_HASH_ALGORITHM);

	ret = fixup_spki_params(params, se, me, sign_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	switch (se->pk) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:
		if (_pkcs1_rsa_verify_sig
		    (se->pk, me, data, NULL, signature, params, sign_params) != 0) {
			gnutls_assert();
			return GNUTLS_E_PK_SIG_VERIFY_FAILED;
		}

		break;

	case GNUTLS_PK_EDDSA_ED25519:
		if (_gnutls_pk_verify(se->pk, data, signature, params, sign_params) != 0) {
			gnutls_assert();
			return GNUTLS_E_PK_SIG_VERIFY_FAILED;
		}

		break;

	case GNUTLS_PK_EC:
	case GNUTLS_PK_DSA:
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		if (dsa_verify_data
		    (se->pk, me, data, signature, params, sign_params) != 0) {
			gnutls_assert();
			return GNUTLS_E_PK_SIG_VERIFY_FAILED;
		}

		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;

	}

	if (_gnutls_sign_is_secure2(se,0) == 0 && _gnutls_is_broken_sig_allowed(se, flags) == 0) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_SECURITY);
	}

	return 1;
}

const mac_entry_st *_gnutls_dsa_q_to_hash(const gnutls_pk_params_st *
					  params, unsigned int *hash_len)
{
	int bits = 0;
	int ret;

	if (params->algo == GNUTLS_PK_DSA)
		bits = _gnutls_mpi_get_nbits(params->params[1]);
	else if (params->algo == GNUTLS_PK_EC)
		bits = gnutls_ecc_curve_get_size(params->curve) * 8;

	if (bits <= 160) {
		if (hash_len)
			*hash_len = 20;
		ret = GNUTLS_DIG_SHA1;
	} else if (bits <= 192) {
		if (hash_len)
			*hash_len = 24;
		ret = GNUTLS_DIG_SHA256;
	} else if (bits <= 224) {
		if (hash_len)
			*hash_len = 28;
		ret = GNUTLS_DIG_SHA256;
	} else if (bits <= 256) {
		if (hash_len)
			*hash_len = 32;
		ret = GNUTLS_DIG_SHA256;
	} else if (bits <= 384) {
		if (hash_len)
			*hash_len = 48;
		ret = GNUTLS_DIG_SHA384;
	} else {
		if (hash_len)
			*hash_len = 64;
		ret = GNUTLS_DIG_SHA512;
	}

	return mac_to_entry(ret);
}

/**
 * gnutls_pubkey_set_pin_function:
 * @key: A key of type #gnutls_pubkey_t
 * @fn: the callback
 * @userdata: data associated with the callback
 *
 * This function will set a callback function to be used when
 * required to access the object. This function overrides any other
 * global PIN functions.
 *
 * Note that this function must be called right after initialization
 * to have effect.
 *
 * Since: 3.1.0
 *
 **/
void gnutls_pubkey_set_pin_function(gnutls_pubkey_t key,
				    gnutls_pin_callback_t fn,
				    void *userdata)
{
	key->pin.cb = fn;
	key->pin.data = userdata;
}

/**
 * gnutls_pubkey_import_x509_raw:
 * @pkey: The public key
 * @data: The public key data to be imported
 * @format: The format of the public key
 * @flags: should be zero
 *
 * This function will import the given public key to the abstract
 * #gnutls_pubkey_t type. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.3
 **/
int gnutls_pubkey_import_x509_raw(gnutls_pubkey_t pkey,
				  const gnutls_datum_t * data,
				  gnutls_x509_crt_fmt_t format,
				  unsigned int flags)
{
	gnutls_x509_crt_t xpriv;
	int ret;

	ret = gnutls_x509_crt_init(&xpriv);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_crt_import(xpriv, data, format);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_import_x509(pkey, xpriv, flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

      cleanup:
	gnutls_x509_crt_deinit(xpriv);

	return ret;
}

/**
 * gnutls_pubkey_verify_params:
 * @key: should contain a #gnutls_pubkey_t type
 *
 * This function will verify the public key parameters.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_pubkey_verify_params(gnutls_pubkey_t key)
{
	int ret;

	ret = _gnutls_pk_verify_pub_params(key->params.algo, &key->params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_pubkey_get_spki:
 * @pubkey: a public key of type #gnutls_pubkey_t
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_pubkey_spki_t
 * @flags: must be zero
 *
 * This function will return the public key information if available.
 * The provided @spki must be initialized.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.0
 **/
int
gnutls_pubkey_get_spki(gnutls_pubkey_t pubkey, gnutls_x509_spki_t spki, unsigned int flags)
{
	if (pubkey == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (pubkey->params.spki.pk == GNUTLS_PK_UNKNOWN)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	memcpy(spki, &pubkey->params.spki, sizeof(gnutls_x509_spki_st));

	return 0;
}

/**
 * gnutls_pubkey_set_spki:
 * @pubkey: a public key of type #gnutls_pubkey_t
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_pubkey_spki_t
 * @flags: must be zero
 *
 * This function will set the public key information.
 * The provided @spki must be initialized.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.0
 **/
int
gnutls_pubkey_set_spki(gnutls_pubkey_t pubkey, const gnutls_x509_spki_t spki, unsigned int flags)
{
	if (pubkey == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (!_gnutls_pk_are_compat(pubkey->params.algo, spki->pk))
                return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memcpy(&pubkey->params.spki, spki, sizeof(gnutls_x509_spki_st));

	pubkey->params.algo = spki->pk;

	return 0;
}
