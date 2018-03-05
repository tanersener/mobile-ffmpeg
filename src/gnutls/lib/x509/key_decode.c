/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Red Hat
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

#include "gnutls_int.h"
#include "errors.h"
#include <global.h>
#include <libtasn1.h>
#include <datum.h>
#include "common.h"
#include "x509_int.h"
#include <num.h>
#include <ecc.h>

static int _gnutls_x509_read_rsa_pubkey(uint8_t * der, int dersize,
					gnutls_pk_params_st * params);
static int _gnutls_x509_read_dsa_pubkey(uint8_t * der, int dersize,
					gnutls_pk_params_st * params);
static int _gnutls_x509_read_ecc_pubkey(uint8_t * der, int dersize,
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
	return _gnutls_ecc_ansi_x963_import(der, dersize,
					    &params->params[ECC_X],
					    &params->params[ECC_Y]);
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

	/* FIXME: If the parameters are not included in the certificate
	 * then the issuer's parameters should be used. This is not
	 * done yet.
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

int _gnutls_x509_read_pubkey(gnutls_pk_algorithm_t algo, uint8_t * der,
			     int dersize, gnutls_pk_params_st * params)
{
	int ret;

	switch (algo) {
	case GNUTLS_PK_RSA:
		ret = _gnutls_x509_read_rsa_pubkey(der, dersize, params);
		if (ret >= 0) {
			params->algo = GNUTLS_PK_RSA;
			params->params_nr = RSA_PUBLIC_PARAMS;
		}
		break;
	case GNUTLS_PK_DSA:
		ret = _gnutls_x509_read_dsa_pubkey(der, dersize, params);
		if (ret >= 0) {
			params->algo = GNUTLS_PK_DSA;
			params->params_nr = DSA_PUBLIC_PARAMS;
		}
		break;
	case GNUTLS_PK_EC:
		ret = _gnutls_x509_read_ecc_pubkey(der, dersize, params);
		if (ret >= 0) {
			params->algo = GNUTLS_PK_ECDSA;
			params->params_nr = ECC_PUBLIC_PARAMS;
		}
		break;
	default:
		ret = gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
		break;
	}
	return ret;
}

int _gnutls_x509_read_pubkey_params(gnutls_pk_algorithm_t algo,
				    uint8_t * der, int dersize,
				    gnutls_pk_params_st * params)
{
	switch (algo) {
	case GNUTLS_PK_RSA:
		return 0;
	case GNUTLS_PK_DSA:
		return _gnutls_x509_read_dsa_params(der, dersize, params);
	case GNUTLS_PK_EC:
		return _gnutls_x509_read_ecc_params(der, dersize, &params->flags);
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
	/* do not set a number */
	params->params_nr = 0;
	return _gnutls_x509_read_der_int(der, dersize, &params->params[3]);
}
