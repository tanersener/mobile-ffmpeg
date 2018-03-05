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

#include "gnutls_int.h"
#include "errors.h"
#include <global.h>
#include <libtasn1.h>
#include <datum.h>
#include "common.h"
#include "x509_int.h"
#include <num.h>

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

	gnutls_pk_params_init(params);

	result = _gnutls_x509_get_pk_algorithm(asn, root, NULL);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	pk_algorithm = result;

	/* Read the algorithm's parameters
	 */
	_asnstr_append_name(name, sizeof(name), root, ".subjectPublicKey");
	result = _gnutls_x509_read_value(asn, name, &tmp);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	if ((result =
	     _gnutls_x509_read_pubkey(pk_algorithm, tmp.data, tmp.size,
				      params)) < 0) {
		gnutls_assert();
		goto error;
	}

	/* Now read the parameters
	 */
	_gnutls_free_datum(&tmp);

	_asnstr_append_name(name, sizeof(name), root,
			    ".algorithm.parameters");

	/* FIXME: If the parameters are not included in the certificate
	 * then the issuer's parameters should be used. This is not
	 * done yet.
	 */
	if (pk_algorithm != GNUTLS_PK_RSA) {	/* RSA doesn't use parameters */
		result = _gnutls_x509_read_value(asn, name, &tmp);
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
 * This function writes and encodes the parameters for DSS or RSA keys.
 * This is the "signatureAlgorithm" fields.
 *
 * If @legacy is non-zero then the legacy value for PKCS#7 signatures
 * will be written for RSA signatures.
 */
int
_gnutls_x509_write_sig_params(ASN1_TYPE dst, const char *dst_name,
			      gnutls_pk_algorithm_t pk_algorithm,
			      gnutls_digest_algorithm_t dig, unsigned legacy)
{
	int result;
	char name[128];
	const char *oid;

	_gnutls_str_cpy(name, sizeof(name), dst_name);
	_gnutls_str_cat(name, sizeof(name), ".algorithm");

	if (legacy && pk_algorithm == GNUTLS_PK_RSA)
		oid = PK_PKIX1_RSA_OID;
	else
		oid = gnutls_sign_get_oid(gnutls_pk_to_sign(pk_algorithm, dig));
	if (oid == NULL) {
		gnutls_assert();
		_gnutls_debug_log
		    ("Cannot find OID for sign algorithm pk: %d dig: %d\n",
		     (int) pk_algorithm, (int) dig);
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* write the OID.
	 */
	result = asn1_write_value(dst, name, oid, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}


	_gnutls_str_cpy(name, sizeof(name), dst_name);
	_gnutls_str_cat(name, sizeof(name), ".parameters");

	if (pk_algorithm == GNUTLS_PK_RSA)
		result =
		    asn1_write_value(dst, name, ASN1_NULL, ASN1_NULL_SIZE);
	else
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

/* this function reads a (small) unsigned integer
 * from asn1 structs. Combines the read and the convertion
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
		gnutls_assert();
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
	uint8_t tmpstr[4];
	int result;

	_gnutls_write_uint32(num, tmpstr);

	result = asn1_write_value(node, value, tmpstr, 4);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}
