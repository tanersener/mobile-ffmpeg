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
#include "pk.h"
#include <num.h>
#include <ecc.h>

static int _gnutls_x509_read_rsa_pubkey(uint8_t * der, int dersize,
					gnutls_pk_params_st * params);
static int _gnutls_x509_read_dsa_pubkey(uint8_t * der, int dersize,
					gnutls_pk_params_st * params);
static int _gnutls_x509_read_ecc_pubkey(uint8_t * der, int dersize,
					gnutls_pk_params_st * params);
static int _gnutls_x509_read_eddsa_pubkey(gnutls_ecc_curve_t curve,
					  uint8_t * der, int dersize,
					  gnutls_pk_params_st * params);
static int _gnutls_x509_read_gost_pubkey(uint8_t * der, int dersize,
					gnutls_pk_params_st * params);

static int
_gnutls_x509_read_dsa_params(uint8_t * der, int dersize,
			     gnutls_pk_params_st * params);

/*
 * some x509 certificate parsing functions that relate to MPI parameter
 * extraction. This reads the BIT STRING subjectPublicKey.
 * Returns 2 parameters (m,e). It does not set params_nr.
 */
int
_gnutls_x509_read_rsa_pubkey(uint8_t * der, int dersize,
			     gnutls_pk_params_st * params)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.RSAPublicKey", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_der_decoding(&spk, der, dersize, NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return _gnutls_asn2err(result);
	}


	if ((result =
	     _gnutls_x509_read_int(spk, "modulus",
				   &params->params[0])) < 0) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	}

	if ((result = _gnutls_x509_read_int(spk, "publicExponent",
					    &params->params[1])) < 0) {
		gnutls_assert();
		_gnutls_mpi_release(&params->params[0]);
		asn1_delete_structure(&spk);
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	}

	asn1_delete_structure(&spk);

	return 0;

}

/*
 * some x509 certificate parsing functions that relate to MPI parameter
 * extraction. This reads the BIT STRING subjectPublicKey.
 * Returns 2 parameters (m,e). It does not set params_nr.
 */
int
_gnutls_x509_read_ecc_pubkey(uint8_t * der, int dersize,
			     gnutls_pk_params_st * params)
{
	/* RFC5480 defines the public key to be an ECPoint (i.e. OCTET STRING),
	 * Then it says that the OCTET STRING _value_ is converted to BIT STRING.
	 * That means that the value we place there is the raw X9.62 one. */
	return _gnutls_ecc_ansi_x962_import(der, dersize,
					    &params->params[ECC_X],
					    &params->params[ECC_Y]);
}

int _gnutls_x509_read_eddsa_pubkey(gnutls_ecc_curve_t curve,
				   uint8_t * der, int dersize,
				   gnutls_pk_params_st * params)
{
	int size = gnutls_ecc_curve_get_size(curve);
	if (dersize != size)
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	return _gnutls_set_datum(&params->raw_pub, der, dersize);
}

/* Pubkey is a concatenation of X (in little endian) and Y (also LE)
 * encoded into OCTET STRING. */
static int
_gnutls_x509_read_gost_pubkey(uint8_t * der, int dersize,
			     gnutls_pk_params_st * params)
{
	int ret;
	int len;
	bigint_t *x = &params->params[GOST_X];
	bigint_t *y = &params->params[GOST_Y];

	/* Quick and dirty parsing of OCTET STRING of 0x40 or 0x80 bytes */
	if (dersize < 1 || der[0] != ASN1_TAG_OCTET_STRING) {
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}

	der++;
	dersize--;

	ret = asn1_get_length_der(der, dersize, &len);
	if (ret <= 0 || ret % 2 != 0 || dersize != len + ret) {
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}

	der += len;
	dersize -= len;

	/* read data */
	ret = _gnutls_mpi_init_scan_le(x, der, dersize / 2);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ret = _gnutls_mpi_init_scan_le(y, der + dersize / 2, dersize / 2);
	if (ret < 0) {
		_gnutls_mpi_release(y);
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	return 0;
}

/* reads p,q and g 
 * from the certificate (subjectPublicKey BIT STRING).
 * params[0-2]. It does NOT set params_nr.
 */
static int
_gnutls_x509_read_dsa_params(uint8_t * der, int dersize,
			     gnutls_pk_params_st * params)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.Dss-Parms",
	      &spk)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_der_decoding(&spk, der, dersize, NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return _gnutls_asn2err(result);
	}

	/* If the parameters are not included in the certificate
	 * then the issuer's parameters should be used. This is not
	 * implemented, and is not used in practice (along with DSA).
	 */

	/* Read p */

	if ((result =
	     _gnutls_x509_read_int(spk, "p", &params->params[0])) < 0) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	}

	/* Read q */

	if ((result =
	     _gnutls_x509_read_int(spk, "q", &params->params[1])) < 0) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		_gnutls_mpi_release(&params->params[0]);
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	}

	/* Read g */

	if ((result =
	     _gnutls_x509_read_int(spk, "g", &params->params[2])) < 0) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		_gnutls_mpi_release(&params->params[0]);
		_gnutls_mpi_release(&params->params[1]);
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	}

	asn1_delete_structure(&spk);

	params->params_nr = 3; /* public key is missing */
	params->algo = GNUTLS_PK_DSA;

	return 0;

}

/* reads the curve from the certificate.
 * params[0-4]. It does NOT set params_nr.
 */
int
_gnutls_x509_read_ecc_params(uint8_t * der, int dersize,
			     unsigned int * curve)
{
	int ret;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;
	char oid[MAX_OID_SIZE];
	int oid_size;

	if ((ret = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.ECParameters",
	      &spk)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	ret = asn1_der_decoding(&spk, der, dersize, NULL);

	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	/* read the curve */
	oid_size = sizeof(oid);
	ret = asn1_read_value(spk, "namedCurve", oid, &oid_size);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	*curve = gnutls_oid_to_ecc_curve(oid);
	if (*curve == GNUTLS_ECC_CURVE_INVALID) {
		_gnutls_debug_log("Curve %s is not supported\n", oid);
		gnutls_assert();
		ret = GNUTLS_E_ECC_UNSUPPORTED_CURVE;
		goto cleanup;
	}

	ret = 0;

      cleanup:

	asn1_delete_structure(&spk);

	return ret;

}

/* Reads RSA-PSS parameters.
 */
int
_gnutls_x509_read_rsa_pss_params(uint8_t * der, int dersize,
				 gnutls_x509_spki_st * params)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	gnutls_digest_algorithm_t digest;
	char oid[MAX_OID_SIZE] = "";
	int size;
	unsigned int trailer;
	gnutls_datum_t value = { NULL, 0 };

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.RSAPSSParameters", &spk))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _asn1_strict_der_decode(&spk, der, dersize, NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	size = sizeof(oid);
	result = asn1_read_value(spk, "hashAlgorithm.algorithm", oid, &size);
	if (result == ASN1_SUCCESS)
		digest = gnutls_oid_to_digest(oid);
	else if (result == ASN1_ELEMENT_NOT_FOUND)
		/* The default hash algorithm is SHA-1 */
		digest = GNUTLS_DIG_SHA1;
	else {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (digest == GNUTLS_DIG_UNKNOWN) {
		gnutls_assert();
		_gnutls_debug_log("Unknown RSA-PSS hash: %s\n", oid);
		result = GNUTLS_E_UNKNOWN_HASH_ALGORITHM;
		goto cleanup;
	}

	size = sizeof(oid);
	result = asn1_read_value(spk, "maskGenAlgorithm.algorithm", oid, &size);
	if (result == ASN1_SUCCESS) {
		gnutls_digest_algorithm_t digest2;

		/* Error out if algorithm other than mgf1 is specified */
		if (strcmp(oid, PKIX1_RSA_PSS_MGF1_OID) != 0) {
			gnutls_assert();
			_gnutls_debug_log("Unknown mask algorithm: %s\n", oid);
			result = GNUTLS_E_UNKNOWN_ALGORITHM;
			goto cleanup;
		}

		/* Check if maskGenAlgorithm.parameters does exist and
		 * is identical to hashAlgorithm */
		result = _gnutls_x509_read_value(spk, "maskGenAlgorithm.parameters", &value);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if ((result = asn1_create_element
		     (_gnutls_get_pkix(), "PKIX1.AlgorithmIdentifier", &c2))
		    != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		result = _asn1_strict_der_decode(&c2, value.data, value.size, NULL);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		size = sizeof(oid);
		result = asn1_read_value(c2, "algorithm", oid, &size);
		if (result == ASN1_SUCCESS)
			digest2 = gnutls_oid_to_digest(oid);
		else if (result == ASN1_ELEMENT_NOT_FOUND)
			/* The default hash algorithm for mgf1 is SHA-1 */
			digest2 = GNUTLS_DIG_SHA1;
		else {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		if (digest != digest2) {
			gnutls_assert();
			result = GNUTLS_E_CONSTRAINT_ERROR;
			goto cleanup;
		}
	} else if (result != ASN1_ELEMENT_NOT_FOUND) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	memset(params, 0, sizeof(gnutls_x509_spki_st));
	params->pk = GNUTLS_PK_RSA_PSS;
	params->rsa_pss_dig = digest;

	result = _gnutls_x509_read_uint(spk, "saltLength", &params->salt_size);
	if (result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND ||
	    result == GNUTLS_E_ASN1_VALUE_NOT_FOUND)
		params->salt_size = 20;
	else if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_read_uint(spk, "trailerField", &trailer);
	if (result == GNUTLS_E_ASN1_VALUE_NOT_FOUND ||
	    result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
		trailer = 1;
	else if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}
	if (trailer != 1) {
		gnutls_assert();
		result = GNUTLS_E_CERTIFICATE_ERROR;
		goto cleanup;
	}

	result = 0;
 cleanup:
	_gnutls_free_datum(&value);
	asn1_delete_structure(&c2);
	asn1_delete_structure(&spk);
	return result;
}

/* reads the curve from the certificate.
 * It does NOT set params_nr.
 */
int
_gnutls_x509_read_gost_params(uint8_t * der, int dersize,
			      gnutls_pk_params_st * params,
			      gnutls_pk_algorithm_t algo)
{
	int ret;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;
	char oid[MAX_OID_SIZE];
	int oid_size;
	gnutls_ecc_curve_t curve;
	gnutls_gost_paramset_t param;

	if ((ret = asn1_create_element(_gnutls_get_gnutls_asn(),
				       algo == GNUTLS_PK_GOST_01 ?
				       "GNUTLS.GOSTParametersOld" :
				       "GNUTLS.GOSTParameters",
				       &spk)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	ret = _asn1_strict_der_decode(&spk, der, dersize, NULL);

	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	/* read the curve */
	oid_size = sizeof(oid);
	ret = asn1_read_value(spk, "publicKeyParamSet", oid, &oid_size);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	curve = gnutls_oid_to_ecc_curve(oid);
	if (curve == GNUTLS_ECC_CURVE_INVALID) {
		_gnutls_debug_log("Curve %s is not supported\n", oid);
		gnutls_assert();
		ret = GNUTLS_E_ECC_UNSUPPORTED_CURVE;
		goto cleanup;
	}

	/* Read the digest */
	oid_size = sizeof(oid);
	ret = asn1_read_value(spk, "digestParamSet", oid, &oid_size);
	if (ret != ASN1_SUCCESS &&
	    ret != ASN1_ELEMENT_NOT_FOUND) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}
	/* For now ignore the OID: we use pk OID instead */

	oid_size = sizeof(oid);
	ret = asn1_read_value(spk, "encryptionParamSet", oid, &oid_size);
	if (ret != ASN1_SUCCESS &&
	    ret != ASN1_ELEMENT_NOT_FOUND) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	if (ret != ASN1_ELEMENT_NOT_FOUND)
		param = gnutls_oid_to_gost_paramset(oid);
	else
		param = _gnutls_gost_paramset_default(algo);

	if (param == GNUTLS_GOST_PARAMSET_UNKNOWN) {
		gnutls_assert();
		ret = param;
		goto cleanup;
	}

	params->curve = curve;
	params->gost_params = param;
	ret = 0;

      cleanup:

	asn1_delete_structure(&spk);

	return ret;

}

/* This function must be called after _gnutls_x509_read_params()
 */
int _gnutls_x509_read_pubkey(gnutls_pk_algorithm_t algo, uint8_t * der,
			     int dersize, gnutls_pk_params_st * params)
{
	int ret;

	switch (algo) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_RSA_PSS:
		ret = _gnutls_x509_read_rsa_pubkey(der, dersize, params);
		if (ret >= 0) {
			params->algo = algo;
			params->params_nr = RSA_PUBLIC_PARAMS;
		}
		break;
	case GNUTLS_PK_DSA:
		if (params->params_nr != 3) /* _gnutls_x509_read_pubkey_params must have been called */
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		ret = _gnutls_x509_read_dsa_pubkey(der, dersize, params);
		if (ret >= 0) {
			params->algo = GNUTLS_PK_DSA;
			params->params_nr = DSA_PUBLIC_PARAMS;
		}
		break;
	case GNUTLS_PK_ECDSA:
		ret = _gnutls_x509_read_ecc_pubkey(der, dersize, params);
		if (ret >= 0) {
			params->algo = GNUTLS_PK_ECDSA;
			params->params_nr = ECC_PUBLIC_PARAMS;
		}
		break;
	case GNUTLS_PK_EDDSA_ED25519:
		ret = _gnutls_x509_read_eddsa_pubkey(GNUTLS_ECC_CURVE_ED25519, der, dersize, params);
		break;
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		ret = _gnutls_x509_read_gost_pubkey(der, dersize, params);
		if (ret >= 0) {
			params->algo = algo;
			params->params_nr = GOST_PUBLIC_PARAMS;
		}
		break;
	default:
		ret = gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
		break;
	}
	return ret;
}

/* This function must be called prior to _gnutls_x509_read_pubkey()
 */
int _gnutls_x509_read_pubkey_params(gnutls_pk_algorithm_t algo,
				    uint8_t * der, int dersize,
				    gnutls_pk_params_st * params)
{
	switch (algo) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_EDDSA_ED25519:
		return 0;
	case GNUTLS_PK_RSA_PSS:
		return _gnutls_x509_read_rsa_pss_params(der, dersize, &params->spki);
	case GNUTLS_PK_DSA:
		return _gnutls_x509_read_dsa_params(der, dersize, params);
	case GNUTLS_PK_EC:
		return _gnutls_x509_read_ecc_params(der, dersize, &params->curve);
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		return _gnutls_x509_read_gost_params(der, dersize, params, algo);
	default:
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}
}

/* This function must be called after _gnutls_x509_read_pubkey()
 */
int _gnutls_x509_check_pubkey_params(gnutls_pk_params_st * params)
{
	switch (params->algo) {
	case GNUTLS_PK_RSA_PSS: {
		unsigned bits;
		const mac_entry_st *me;
		size_t hash_size;

		if (params->spki.pk == GNUTLS_PK_UNKNOWN) /* no params present */
			return 0;

		bits = pubkey_to_bits(params);

		me = hash_to_entry(params->spki.rsa_pss_dig);
		if (unlikely(me == NULL))
			return gnutls_assert_val(GNUTLS_E_PK_INVALID_PUBKEY_PARAMS);

		hash_size = _gnutls_hash_get_algo_len(me);
		if (hash_size + params->spki.salt_size + 2 > (bits + 7) / 8)
			return gnutls_assert_val(GNUTLS_E_PK_INVALID_PUBKEY_PARAMS);
		return 0;
	}
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_DSA:
	case GNUTLS_PK_ECDSA:
	case GNUTLS_PK_EDDSA_ED25519:
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		return 0;
	default:
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}
}

/* reads DSA's Y
 * from the certificate 
 * only sets params[3]
 */
int
_gnutls_x509_read_dsa_pubkey(uint8_t * der, int dersize,
			     gnutls_pk_params_st * params)
{
	return _gnutls_x509_read_der_int(der, dersize, &params->params[3]);
}
