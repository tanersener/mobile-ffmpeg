/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
 * Copyright (C) 2015-2017 Red Hat, Inc.
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
#include <limits.h>

/* Reads an Integer from the DER encoded data
 */

int _gnutls_x509_read_der_int(uint8_t * der, int dersize, bigint_t * out)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	/* == INTEGER */
	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.DSAPublicKey",
	      &spk)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&spk, der, dersize, NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return _gnutls_asn2err(result);
	}

	/* Read Y */

	if ((result = _gnutls_x509_read_int(spk, "", out)) < 0) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return _gnutls_asn2err(result);
	}

	asn1_delete_structure(&spk);

	return 0;

}

int _gnutls_x509_read_der_uint(uint8_t * der, int dersize, unsigned int *out)
{
	int result;
	ASN1_TYPE spk = ASN1_TYPE_EMPTY;

	/* == INTEGER */
	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.DSAPublicKey",
	      &spk)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&spk, der, dersize, NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return _gnutls_asn2err(result);
	}

	/* Read Y */

	if ((result = _gnutls_x509_read_uint(spk, "", out)) < 0) {
		gnutls_assert();
		asn1_delete_structure(&spk);
		return _gnutls_asn2err(result);
	}

	asn1_delete_structure(&spk);

	return 0;

}


/* Extracts DSA and RSA parameters from a certificate.
 */
int
_gnutls_get_asn_mpis(ASN1_TYPE asn, const char *root,
		     gnutls_pk_params_st * params)
{
	int result;
	char name[256];
	gnutls_datum_t tmp = { NULL, 0 };
	gnutls_pk_algorithm_t pk_algorithm;
	gnutls_ecc_curve_t curve;

	gnutls_pk_params_init(params);

	result = _gnutls_x509_get_pk_algorithm(asn, root, &curve, NULL);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	pk_algorithm = result;
	params->curve = curve;
	params->algo = pk_algorithm;

	/* Read the algorithm's parameters
	 */
	_asnstr_append_name(name, sizeof(name), root,
			    ".algorithm.parameters");

	if (pk_algorithm != GNUTLS_PK_RSA &&
	    pk_algorithm != GNUTLS_PK_EDDSA_ED25519 && pk_algorithm != GNUTLS_PK_ECDH_X25519 &&
	    pk_algorithm != GNUTLS_PK_EDDSA_ED448 && pk_algorithm != GNUTLS_PK_ECDH_X448) {
		/* RSA and EdDSA do not use parameters */
		result = _gnutls_x509_read_value(asn, name, &tmp);
		if (pk_algorithm == GNUTLS_PK_RSA_PSS && 
		    (result == GNUTLS_E_ASN1_VALUE_NOT_FOUND || result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)) {
			goto skip_params;
		}
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		result =
		     _gnutls_x509_read_pubkey_params(pk_algorithm,
						     tmp.data, tmp.size,
						     params);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		_gnutls_free_datum(&tmp);
	}

 skip_params:
	/* Now read the public key */
	_asnstr_append_name(name, sizeof(name), root, ".subjectPublicKey");

	result = _gnutls_x509_read_value(asn, name, &tmp);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	if ((result =
	     _gnutls_x509_read_pubkey(pk_algorithm, tmp.data, tmp.size,
				      params)) < 0) {
		gnutls_assert();
		goto error;
	}

	result = _gnutls_x509_check_pubkey_params(params);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	result = 0;

 error:
	if (result < 0)
		gnutls_pk_params_release(params);
	_gnutls_free_datum(&tmp);
	return result;
}

/* Extracts DSA and RSA parameters from a certificate.
 */
int
_gnutls_x509_crt_get_mpis(gnutls_x509_crt_t cert,
			  gnutls_pk_params_st * params)
{
	/* Read the algorithm's OID
	 */
	return _gnutls_get_asn_mpis(cert->cert,
				    "tbsCertificate.subjectPublicKeyInfo",
				    params);
}

/* Extracts DSA and RSA parameters from a certificate.
 */
int
_gnutls_x509_crq_get_mpis(gnutls_x509_crq_t cert,
			  gnutls_pk_params_st * params)
{
	/* Read the algorithm's OID
	 */
	return _gnutls_get_asn_mpis(cert->crq,
				    "certificationRequestInfo.subjectPKInfo",
				    params);
}

/*
 * This function reads and decodes the parameters for DSS or RSA keys.
 * This is the "signatureAlgorithm" fields.
 */
int
_gnutls_x509_read_pkalgo_params(ASN1_TYPE src, const char *src_name,
			      gnutls_x509_spki_st *spki, unsigned is_sig)
{
	int result;
	char name[128];
	char oid[MAX_OID_SIZE];
	int oid_size;

	memset(spki, 0, sizeof(*spki));

	_gnutls_str_cpy(name, sizeof(name), src_name);
	_gnutls_str_cat(name, sizeof(name), ".algorithm");

	oid_size = sizeof(oid);
	result = asn1_read_value(src, name, oid, &oid_size);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (strcmp (oid, PK_PKIX1_RSA_PSS_OID) == 0) {
		gnutls_datum_t tmp = { NULL, 0 };

		_gnutls_str_cpy(name, sizeof(name), src_name);
		_gnutls_str_cat(name, sizeof(name), ".parameters");

		result = _gnutls_x509_read_value(src, name, &tmp);
		if (result < 0) {
			if (!is_sig) {
				if (result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND ||
				    result != GNUTLS_E_ASN1_VALUE_NOT_FOUND) {
					/* it is ok to not have parameters in SPKI, but
					 * not in signatures */
					return 0;
				}
			}

			return gnutls_assert_val(result);
		}

		result = _gnutls_x509_read_rsa_pss_params(tmp.data, tmp.size,
							  spki);
		_gnutls_free_datum(&tmp);

		if (result < 0)
			gnutls_assert();

		return result;
	}

	return 0;
}

static int write_oid_and_params(ASN1_TYPE dst, const char *dst_name, const char *oid, gnutls_x509_spki_st *params)
{
	int result;
	char name[128];

	if (params == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	_gnutls_str_cpy(name, sizeof(name), dst_name);
	_gnutls_str_cat(name, sizeof(name), ".algorithm");

	/* write the OID.
	 */
	result = asn1_write_value(dst, name, oid, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	_gnutls_str_cpy(name, sizeof(name), dst_name);
	_gnutls_str_cat(name, sizeof(name), ".parameters");

	if (params->pk == GNUTLS_PK_RSA)
		result =
		    asn1_write_value(dst, name, ASN1_NULL, ASN1_NULL_SIZE);
	else if (params->pk == GNUTLS_PK_RSA_PSS) {
		gnutls_datum_t tmp = { NULL, 0 };

		result = _gnutls_x509_write_rsa_pss_params(params, &tmp);
		if (result < 0)
			return gnutls_assert_val(result);

		result = asn1_write_value(dst, name, tmp.data, tmp.size);
		_gnutls_free_datum(&tmp);
	} else
		result = asn1_write_value(dst, name, NULL, 0);

	if (result != ASN1_SUCCESS && result != ASN1_ELEMENT_NOT_FOUND) {
		/* Here we ignore the element not found error, since this
		 * may have been disabled before.
		 */
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

int
_gnutls_x509_write_spki_params(ASN1_TYPE dst, const char *dst_name,
			       gnutls_x509_spki_st *params)
{
	const char *oid;

	if (params->legacy && params->pk == GNUTLS_PK_RSA)
		oid = PK_PKIX1_RSA_OID;
	else if (params->pk == GNUTLS_PK_RSA_PSS)
		oid = PK_PKIX1_RSA_PSS_OID;
	else
		oid = gnutls_pk_get_oid(params->pk);

	if (oid == NULL) {
		gnutls_assert();
		_gnutls_debug_log
		    ("Cannot find OID for public key algorithm %s\n",
		     gnutls_pk_get_name(params->pk));
		return GNUTLS_E_INVALID_REQUEST;
	}

	return write_oid_and_params(dst, dst_name, oid, params);
}

int
_gnutls_x509_write_sign_params(ASN1_TYPE dst, const char *dst_name,
			       const gnutls_sign_entry_st *se, gnutls_x509_spki_st *params)
{
	const char *oid;

	if (params->legacy && params->pk == GNUTLS_PK_RSA)
		oid = PK_PKIX1_RSA_OID;
	else if (params->pk == GNUTLS_PK_RSA_PSS)
		oid = PK_PKIX1_RSA_PSS_OID;
	else
		oid = se->oid;

	if (oid == NULL) {
		gnutls_assert();
		_gnutls_debug_log
		    ("Cannot find OID for sign algorithm %s\n",
		     se->name);
		return GNUTLS_E_INVALID_REQUEST;
	}

	return write_oid_and_params(dst, dst_name, oid, params);
}

/* this function reads a (small) unsigned integer
 * from asn1 structs. Combines the read and the conversion
 * steps.
 */
int
_gnutls_x509_read_uint(ASN1_TYPE node, const char *value,
		       unsigned int *ret)
{
	int len, result;
	uint8_t *tmpstr;

	len = 0;
	result = asn1_read_value(node, value, NULL, &len);
	if (result != ASN1_MEM_ERROR) {
		return _gnutls_asn2err(result);
	}

	tmpstr = gnutls_malloc(len);
	if (tmpstr == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	result = asn1_read_value(node, value, tmpstr, &len);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(tmpstr);
		return _gnutls_asn2err(result);
	}

	if (len == 1)
		*ret = tmpstr[0];
	else if (len == 2)
		*ret = _gnutls_read_uint16(tmpstr);
	else if (len == 3)
		*ret = _gnutls_read_uint24(tmpstr);
	else if (len == 4)
		*ret = _gnutls_read_uint32(tmpstr);
	else {
		gnutls_assert();
		gnutls_free(tmpstr);
		return GNUTLS_E_INTERNAL_ERROR;
	}

	gnutls_free(tmpstr);

	return 0;
}

/* Writes the specified integer into the specified node.
 */
int
_gnutls_x509_write_uint32(ASN1_TYPE node, const char *value, uint32_t num)
{
	uint8_t tmpstr[5];
	int result;

	tmpstr[0] = 0;
	_gnutls_write_uint32(num, tmpstr+1);

	if (tmpstr[1] > SCHAR_MAX) {
		result = asn1_write_value(node, value, tmpstr, 5);
	} else {
		result = asn1_write_value(node, value, tmpstr+1, 4);
	}

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}
