/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013-2017 Red Hat
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
#include "errors.h"
#include <global.h>
#include <libtasn1.h>
#include <datum.h>
#include "common.h"
#include "x509_int.h"
#include <num.h>
#include <pk.h>
#include <mpi.h>
#include <ecc.h>

static int _gnutls_x509_write_rsa_pubkey(const gnutls_pk_params_st * params,
					 gnutls_datum_t * der);
static int _gnutls_x509_write_dsa_params(const gnutls_pk_params_st * params,
					 gnutls_datum_t * der);
static int _gnutls_x509_write_dsa_pubkey(const gnutls_pk_params_st * params,
					 gnutls_datum_t * der);
static int _gnutls_x509_write_gost_params(const gnutls_pk_params_st * params,
					 gnutls_datum_t * der);
static int _gnutls_x509_write_gost_pubkey(const gnutls_pk_params_st * params,
					 gnutls_datum_t * der);

/*
 * some x509 certificate functions that relate to MPI parameter
 * setting. This writes the BIT STRING subjectPublicKey.
 * Needs 2 parameters (m,e).
 *
 * Allocates the space used to store the DER data.
 */
static int
_gnutls_x509_write_rsa_pubkey(const gnutls_pk_params_st * params,
			      gnutls_datum_t * der)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	der->data = NULL;
	der->size = 0;

	if (params->params_nr < RSA_PUBLIC_PARAMS) {
		gnutls_assert();
		result = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.RSAPublicKey", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    _gnutls_x509_write_int(spk, "modulus", params->params[0], 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result =
	    _gnutls_x509_write_int(spk, "publicExponent",
				   params->params[1], 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(spk, "", der, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	asn1_delete_structure(&spk);

	return result;
}

/*
 * some x509 certificate functions that relate to MPI parameter
 * setting. This writes an ECPoint.
 *
 * Allocates the space used to store the DER data.
 */
int
_gnutls_x509_write_ecc_pubkey(const gnutls_pk_params_st * params,
			      gnutls_datum_t * der)
{
	int result;

	der->data = NULL;
	der->size = 0;

	if (params->params_nr < ECC_PUBLIC_PARAMS)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	result =
	    _gnutls_ecc_ansi_x962_export(params->curve,
					 params->params[ECC_X],
					 params->params[ECC_Y], /*&out */
					 der);
	if (result < 0)
		return gnutls_assert_val(result);

	return 0;
}

/*
 * some x509 certificate functions that relate to MPI parameter
 * setting. This writes a raw public key.
 *
 * Allocates the space used to store the data.
 */
int
_gnutls_x509_write_eddsa_pubkey(const gnutls_pk_params_st * params,
			      gnutls_datum_t * raw)
{
	int ret;

	raw->data = NULL;
	raw->size = 0;

	if (params->raw_pub.size == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (params->curve != GNUTLS_ECC_CURVE_ED25519 &&
	    params->curve != GNUTLS_ECC_CURVE_ED448)
		return gnutls_assert_val(GNUTLS_E_ECC_UNSUPPORTED_CURVE);

	ret = _gnutls_set_datum(raw, params->raw_pub.data, params->raw_pub.size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

int
_gnutls_x509_write_gost_pubkey(const gnutls_pk_params_st * params,
			      gnutls_datum_t * der)
{
	bigint_t x, y;
	int numlen;
	int byte_size, ret;
	size_t size;
	int pos;

	der->data = NULL;
	der->size = 0;

	if (params->params_nr < GOST_PUBLIC_PARAMS)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	x = params->params[GOST_X];
	y = params->params[GOST_Y];
	numlen = gnutls_ecc_curve_get_size(params->curve);

	if (numlen == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	der->size = 1 + ASN1_MAX_LENGTH_SIZE + 2 * numlen;

	der->data = gnutls_malloc(der->size);
	if (der->data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	memset(der->data, 0, der->size);

	der->data[0] = ASN1_TAG_OCTET_STRING;
	asn1_length_der(2 * numlen, &der->data[1], &pos);
	pos += 1;

	/* pad and store x */
	byte_size = (_gnutls_mpi_get_nbits(x) + 7) / 8;
	if (numlen < byte_size) {
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	size = numlen;
	ret = _gnutls_mpi_print_le(x, &der->data[pos], &size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* pad and store y */
	byte_size = (_gnutls_mpi_get_nbits(y) + 7) / 8;
	if (numlen < byte_size) {
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	size = numlen;
	ret = _gnutls_mpi_print_le(y, &der->data[pos + numlen], &size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	der->size = pos + 2 * numlen;

	return 0;

 cleanup:
	_gnutls_free_datum(der);
	return ret;
}

int
_gnutls_x509_write_pubkey_params(const gnutls_pk_params_st * params,
				 gnutls_datum_t * der)
{
	switch (params->algo) {
	case GNUTLS_PK_DSA:
		return _gnutls_x509_write_dsa_params(params, der);
	case GNUTLS_PK_RSA:
		der->data = gnutls_malloc(ASN1_NULL_SIZE);
		if (der->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		memcpy(der->data, ASN1_NULL, ASN1_NULL_SIZE);
		der->size = ASN1_NULL_SIZE;
		return 0;
	case GNUTLS_PK_RSA_PSS:
		return _gnutls_x509_write_rsa_pss_params(&params->spki, der);
	case GNUTLS_PK_ECDSA:
		return _gnutls_x509_write_ecc_params(params->curve, der);
	case GNUTLS_PK_EDDSA_ED25519:
	case GNUTLS_PK_EDDSA_ED448:
		der->data = NULL;
		der->size = 0;

		return 0;
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		return _gnutls_x509_write_gost_params(params, der);
	default:
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}
}

int
_gnutls_x509_write_pubkey(const gnutls_pk_params_st * params,
			  gnutls_datum_t * der)
{
	switch (params->algo) {
	case GNUTLS_PK_DSA:
		return _gnutls_x509_write_dsa_pubkey(params, der);
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:
		return _gnutls_x509_write_rsa_pubkey(params, der);
	case GNUTLS_PK_ECDSA:
		return _gnutls_x509_write_ecc_pubkey(params, der);
	case GNUTLS_PK_EDDSA_ED25519:
	case GNUTLS_PK_EDDSA_ED448:
		return _gnutls_x509_write_eddsa_pubkey(params, der);
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		return _gnutls_x509_write_gost_pubkey(params, der);
	default:
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}
}

/*
 * This function writes the parameters for DSS keys.
 * Needs 3 parameters (p,q,g).
 *
 * Allocates the space used to store the DER data.
 */
static int
_gnutls_x509_write_dsa_params(const gnutls_pk_params_st * params,
			      gnutls_datum_t * der)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	der->data = NULL;
	der->size = 0;

	if (params->params_nr < DSA_PUBLIC_PARAMS - 1) {
		gnutls_assert();
		result = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.DSAParameters", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_write_int(spk, "p", params->params[0], 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_write_int(spk, "q", params->params[1], 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_write_int(spk, "g", params->params[2], 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(spk, "", der, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	asn1_delete_structure(&spk);
	return result;
}

/*
 * This function writes the parameters for ECC keys.
 * That is the ECParameters struct.
 *
 * Allocates the space used to store the DER data.
 */
int
_gnutls_x509_write_ecc_params(const gnutls_ecc_curve_t curve,
			      gnutls_datum_t * der)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;
	const char *oid;

	der->data = NULL;
	der->size = 0;

	oid = gnutls_ecc_curve_get_oid(curve);
	if (oid == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);


	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.ECParameters", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if ((result =
	     asn1_write_value(spk, "", "namedCurve", 1)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((result =
	     asn1_write_value(spk, "namedCurve", oid,
			      1)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(spk, "", der, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	asn1_delete_structure(&spk);
	return result;
}

int
_gnutls_x509_write_rsa_pss_params(const gnutls_x509_spki_st *params,
				  gnutls_datum_t *der)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	const char *oid;
	gnutls_datum_t tmp = { NULL, 0 };

	der->data = NULL;
	der->size = 0;

	if (params->pk != GNUTLS_PK_RSA_PSS)
		return 0;

	/* refuse to write parameters we cannot read */
	if (gnutls_pk_to_sign(GNUTLS_PK_RSA_PSS, params->rsa_pss_dig) == GNUTLS_SIGN_UNKNOWN)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.RSAPSSParameters", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	oid = gnutls_digest_get_oid(params->rsa_pss_dig);

	if ((result = asn1_write_value(spk, "hashAlgorithm.algorithm", oid, 1))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((result = asn1_write_value(spk, "hashAlgorithm.parameters", NULL, 0))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((result =
	     asn1_write_value(spk, "maskGenAlgorithm.algorithm",
			      PKIX1_RSA_PSS_MGF1_OID, 1))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.AlgorithmIdentifier", &c2))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((result = asn1_write_value(c2, "algorithm", oid, 1))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((result = asn1_write_value(c2, "parameters", NULL, 0))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(c2, "", &tmp, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if ((result =
	     asn1_write_value(spk, "maskGenAlgorithm.parameters",
			      tmp.data, tmp.size))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _gnutls_x509_write_uint32(spk, "saltLength",
					   params->salt_size);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_write_uint32(spk, "trailerField", 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(spk, "", der, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	_gnutls_free_datum(&tmp);
	asn1_delete_structure(&c2);
	asn1_delete_structure(&spk);
	return result;
}

static int
_gnutls_x509_write_gost_params(const gnutls_pk_params_st * params,
			      gnutls_datum_t * der)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;
	const char *oid;

	der->data = NULL;
	der->size = 0;

	oid = gnutls_ecc_curve_get_oid(params->curve);
	if (oid == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);


	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(),
	      params->algo == GNUTLS_PK_GOST_01 ?
	      "GNUTLS.GOSTParametersOld" :
	      "GNUTLS.GOSTParameters", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if ((result =
	     asn1_write_value(spk, "publicKeyParamSet", oid,
			      1)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* For compatibility per R 1323565.1.023—2018 provide digest OID only
	 * for GOST-2001 keys or GOST-2012 keys with CryptoPro curves. Do not
	 * set this optional paramter for TC26 curves */
	if (params->algo == GNUTLS_PK_GOST_01)
		oid = HASH_OID_GOST_R_3411_94_CRYPTOPRO_PARAMS;
	else if (params->algo == GNUTLS_PK_GOST_12_256 &&
		 (params->curve == GNUTLS_ECC_CURVE_GOST256CPA ||
		  params->curve == GNUTLS_ECC_CURVE_GOST256CPB ||
		  params->curve == GNUTLS_ECC_CURVE_GOST256CPC ||
		  params->curve == GNUTLS_ECC_CURVE_GOST256CPXA ||
		  params->curve == GNUTLS_ECC_CURVE_GOST256CPXB))
		oid = HASH_OID_STREEBOG_256;
	else if (params->algo == GNUTLS_PK_GOST_12_512 &&
		 (params->curve == GNUTLS_ECC_CURVE_GOST512A ||
		  params->curve == GNUTLS_ECC_CURVE_GOST512B))
		oid = HASH_OID_STREEBOG_512;
	else
		oid = NULL;

	if ((result = asn1_write_value(spk, "digestParamSet", oid, oid ? 1 : 0)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	oid = gnutls_gost_paramset_get_oid(params->gost_params);
	if (oid == NULL) {
		gnutls_assert();
		result = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	if (params->algo == GNUTLS_PK_GOST_01) {
		if (params->gost_params == _gnutls_gost_paramset_default(params->algo))
			oid = NULL;

		if ((result =
		     asn1_write_value(spk, "encryptionParamSet", oid,
				      oid ? 1 : 0)) != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}
	}

	result = _gnutls_x509_der_encode(spk, "", der, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	asn1_delete_structure(&spk);
	return result;
}

/*
 * This function writes the public parameters for DSS keys.
 * Needs 1 parameter (y).
 *
 * Allocates the space used to store the DER data.
 */
static int
_gnutls_x509_write_dsa_pubkey(const gnutls_pk_params_st * params,
			      gnutls_datum_t * der)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	der->data = NULL;
	der->size = 0;

	if (params->params_nr < DSA_PUBLIC_PARAMS) {
		gnutls_assert();
		result = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.DSAPublicKey", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_write_int(spk, "", params->params[3], 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(spk, "", der, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	asn1_delete_structure(&spk);
	return result;
}

/* Encodes the RSA parameters into an ASN.1 RSA private key structure.
 */
static int
_gnutls_asn1_encode_rsa(ASN1_TYPE * c2, gnutls_pk_params_st * params)
{
	int result, ret;
	uint8_t null = '\0';
	gnutls_pk_params_st pk_params;

	/* we do copy the parameters into a new structure to run _gnutls_pk_fixup,
	 * i.e., regenerate some parameters in case they were broken */
	gnutls_pk_params_init(&pk_params);

	ret = _gnutls_pk_params_copy(&pk_params, params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_pk_fixup(GNUTLS_PK_RSA, GNUTLS_EXPORT, &pk_params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Ok. Now we have the data. Create the asn1 structures
	 */

	/* first make sure that no previously allocated data are leaked */
	if (*c2 != ASN1_TYPE_EMPTY) {
		asn1_delete_structure(c2);
		*c2 = ASN1_TYPE_EMPTY;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.RSAPrivateKey", c2))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* Write PRIME
	 */
	ret =
	    _gnutls_x509_write_int(*c2, "modulus",
				   params->params[RSA_MODULUS], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_int(*c2, "publicExponent",
				   params->params[RSA_PUB], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int(*c2, "privateExponent",
				   params->params[RSA_PRIV], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int(*c2, "prime1",
				   params->params[RSA_PRIME1], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int(*c2, "prime2",
				   params->params[RSA_PRIME2], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int(*c2, "coefficient",
				   params->params[RSA_COEF], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int(*c2, "exponent1",
				   params->params[RSA_E1], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int(*c2, "exponent2",
				   params->params[RSA_E2], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if ((result = asn1_write_value(*c2, "otherPrimeInfos",
				       NULL, 0)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((result =
	     asn1_write_value(*c2, "version", &null, 1)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	ret = 0;

      cleanup:
	if (ret < 0)
		asn1_delete_structure2(c2, ASN1_DELETE_FLAG_ZEROIZE);

	gnutls_pk_params_clear(&pk_params);
	gnutls_pk_params_release(&pk_params);
	return ret;
}

/* Encodes the ECC parameters into an ASN.1 ECPrivateKey structure.
 */
static int
_gnutls_asn1_encode_ecc(ASN1_TYPE * c2, gnutls_pk_params_st * params)
{
	int ret;
	uint8_t one = '\x01';
	gnutls_datum_t pubkey = { NULL, 0 };
	const char *oid;

	oid = gnutls_ecc_curve_get_oid(params->curve);
	if (oid == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	/* first make sure that no previously allocated data are leaked */
	if (*c2 != ASN1_TYPE_EMPTY) {
		asn1_delete_structure(c2);
		*c2 = ASN1_TYPE_EMPTY;
	}

	if ((ret = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.ECPrivateKey", c2))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	if ((ret =
	     asn1_write_value(*c2, "Version", &one, 1)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	if (curve_is_eddsa(params->curve)) {
		if (params->raw_pub.size == 0 || params->raw_priv.size == 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		ret =
		    asn1_write_value(*c2, "privateKey", params->raw_priv.data, params->raw_priv.size);
		if (ret != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(ret);
			goto cleanup;
		}

		ret =
		    asn1_write_value(*c2, "publicKey", params->raw_pub.data, params->raw_pub.size*8);
		if (ret != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(ret);
			goto cleanup;
		}
	} else {
		if (params->params_nr != ECC_PRIVATE_PARAMS)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		ret =
		    _gnutls_ecc_ansi_x962_export(params->curve,
						 params->params[ECC_X],
						 params->params[ECC_Y], &pubkey);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    _gnutls_x509_write_key_int(*c2, "privateKey",
					   params->params[ECC_K], 1);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if ((ret =
		     asn1_write_value(*c2, "publicKey", pubkey.data,
				      pubkey.size * 8)) != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(ret);
			goto cleanup;
		}
	}

	/* write our choice */
	if ((ret =
	     asn1_write_value(*c2, "parameters", "namedCurve",
			      1)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	if ((ret =
	     asn1_write_value(*c2, "parameters.namedCurve", oid,
			      1)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	_gnutls_free_datum(&pubkey);
	return 0;

cleanup:
	asn1_delete_structure2(c2, ASN1_DELETE_FLAG_ZEROIZE);
	_gnutls_free_datum(&pubkey);

	return ret;
}

static int
_gnutls_asn1_encode_gost(ASN1_TYPE * c2, gnutls_pk_params_st * params)
{
	int ret;
	const char *oid;

	oid = gnutls_pk_get_oid(params->algo);

	if (params->params_nr != GOST_PRIVATE_PARAMS || oid == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	/* first make sure that no previously allocated data are leaked */
	if (*c2 != ASN1_TYPE_EMPTY) {
		asn1_delete_structure(c2);
		*c2 = ASN1_TYPE_EMPTY;
	}

	if ((ret = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.GOSTPrivateKey", c2))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int_le(*c2, "", params->params[GOST_K]);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}


	return 0;

cleanup:
	asn1_delete_structure2(c2, ASN1_DELETE_FLAG_ZEROIZE);

	return ret;
}

/* Encodes the DSA parameters into an ASN.1 DSAPrivateKey structure.
 */
static int
_gnutls_asn1_encode_dsa(ASN1_TYPE * c2, gnutls_pk_params_st * params)
{
	int result, ret;
	const uint8_t null = '\0';

	/* first make sure that no previously allocated data are leaked */
	if (*c2 != ASN1_TYPE_EMPTY) {
		asn1_delete_structure(c2);
		*c2 = ASN1_TYPE_EMPTY;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.DSAPrivateKey", c2))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* Write PRIME
	 */
	ret =
	    _gnutls_x509_write_int(*c2, "p",
				   params->params[DSA_P], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_int(*c2, "q",
				   params->params[DSA_Q], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_int(*c2, "g",
				   params->params[DSA_G], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_int(*c2, "Y",
				   params->params[DSA_Y], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_write_key_int(*c2, "priv",
				   params->params[DSA_X], 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if ((result =
	     asn1_write_value(*c2, "version", &null, 1)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	return 0;

cleanup:
	asn1_delete_structure2(c2, ASN1_DELETE_FLAG_ZEROIZE);

	return ret;
}

int _gnutls_asn1_encode_privkey(ASN1_TYPE * c2,
				gnutls_pk_params_st * params)
{
	switch (params->algo) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:
		return _gnutls_asn1_encode_rsa(c2, params);
	case GNUTLS_PK_DSA:
		return _gnutls_asn1_encode_dsa(c2, params);
	case GNUTLS_PK_ECDSA:
	case GNUTLS_PK_EDDSA_ED25519:
	case GNUTLS_PK_EDDSA_ED448:
		return _gnutls_asn1_encode_ecc(c2, params);
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		return _gnutls_asn1_encode_gost(c2, params);
	default:
		return GNUTLS_E_UNIMPLEMENTED_FEATURE;
	}
}
