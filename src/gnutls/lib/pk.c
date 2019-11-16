/*
 * Copyright (C) 2001-2014 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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

/* This file contains the functions needed for RSA/DSA public key
 * encryption and signatures. 
 */

#include "gnutls_int.h"
#include <mpi.h>
#include <pk.h>
#include "errors.h"
#include <datum.h>
#include <global.h>
#include <num.h>
#include "debug.h"
#include <x509/x509_int.h>
#include <x509/common.h>
#include <random.h>
#include <gnutls/crypto.h>

/**
 * gnutls_encode_rs_value:
 * @sig_value: will hold a Dss-Sig-Value DER encoded structure
 * @r: must contain the r value
 * @s: must contain the s value
 *
 * This function will encode the provided r and s values, 
 * into a Dss-Sig-Value structure, used for DSA and ECDSA
 * signatures.
 *
 * The output value should be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.6.0
 *
 **/
int
gnutls_encode_rs_value(gnutls_datum_t * sig_value,
			const gnutls_datum_t * r,
			const gnutls_datum_t * s)
{
	return _gnutls_encode_ber_rs_raw(sig_value, r, s);
}

/* same as gnutls_encode_rs_value(), but kept since it used
 * to be exported for FIPS140 CAVS testing.
 */
int
_gnutls_encode_ber_rs_raw(gnutls_datum_t * sig_value,
			  const gnutls_datum_t * r,
			  const gnutls_datum_t * s)
{
	ASN1_TYPE sig;
	int result, ret;
	uint8_t *tmp = NULL;

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.DSASignatureValue",
				 &sig)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (s->data[0] >= 0x80 || r->data[0] >= 0x80) {
		tmp = gnutls_malloc(MAX(r->size, s->size)+1);
		if (tmp == NULL) {
			ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
			goto cleanup;
		}
	}

	if (r->data[0] >= 0x80) {
		tmp[0] = 0;
		memcpy(&tmp[1], r->data, r->size);
		result = asn1_write_value(sig, "r", tmp, 1+r->size);
	} else {
		result = asn1_write_value(sig, "r", r->data, r->size);
	}

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}


	if (s->data[0] >= 0x80) {
		tmp[0] = 0;
		memcpy(&tmp[1], s->data, s->size);
		result = asn1_write_value(sig, "s", tmp, 1+s->size);
	} else {
		result = asn1_write_value(sig, "s", s->data, s->size);
	}

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	ret = _gnutls_x509_der_encode(sig, "", sig_value, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(tmp);
	asn1_delete_structure(&sig);
	return ret;
}

int
_gnutls_encode_ber_rs(gnutls_datum_t * sig_value, bigint_t r, bigint_t s)
{
	ASN1_TYPE sig;
	int result;

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.DSASignatureValue",
				 &sig)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_write_int(sig, "r", r, 1);
	if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&sig);
		return result;
	}

	result = _gnutls_x509_write_int(sig, "s", s, 1);
	if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&sig);
		return result;
	}

	result = _gnutls_x509_der_encode(sig, "", sig_value, 0);
	asn1_delete_structure(&sig);

	if (result < 0)
		return gnutls_assert_val(result);

	return 0;
}


/* decodes the Dss-Sig-Value structure
 */
int
_gnutls_decode_ber_rs(const gnutls_datum_t * sig_value, bigint_t * r,
		      bigint_t * s)
{
	ASN1_TYPE sig;
	int result;

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.DSASignatureValue",
				 &sig)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* rfc3279 doesn't specify whether Dss-Sig-Value is encoded
	 * as DER or BER. As such we do not restrict to the DER subset. */
	result =
	    asn1_der_decoding(&sig, sig_value->data, sig_value->size,
			      NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&sig);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_read_int(sig, "r", r);
	if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&sig);
		return result;
	}

	result = _gnutls_x509_read_int(sig, "s", s);
	if (result < 0) {
		gnutls_assert();
		_gnutls_mpi_release(r);
		asn1_delete_structure(&sig);
		return result;
	}

	asn1_delete_structure(&sig);

	return 0;
}

/**
 * gnutls_decode_rs_value:
 * @sig_value: holds a Dss-Sig-Value DER or BER encoded structure
 * @r: will contain the r value
 * @s: will contain the s value
 *
 * This function will decode the provided @sig_value, 
 * into @r and @s elements. The Dss-Sig-Value is used for DSA and ECDSA
 * signatures.
 *
 * The output values may be padded with a zero byte to prevent them
 * from being interpreted as negative values. The value
 * should be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.6.0
 *
 **/
int gnutls_decode_rs_value(const gnutls_datum_t * sig_value, gnutls_datum_t *r,
			   gnutls_datum_t *s)
{
	return _gnutls_decode_ber_rs_raw(sig_value, r, s);
}

/* same as gnutls_decode_rs_value(), but kept since it used
 * to be exported for FIPS140 CAVS testing.
 */
int
_gnutls_decode_ber_rs_raw(const gnutls_datum_t * sig_value, gnutls_datum_t *r,
			  gnutls_datum_t *s)
{
	ASN1_TYPE sig;
	int result;

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.DSASignatureValue",
				 &sig)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* rfc3279 doesn't specify whether Dss-Sig-Value is encoded
	 * as DER or BER. As such we do not restrict to the DER subset. */
	result =
	    asn1_der_decoding(&sig, sig_value->data, sig_value->size,
			      NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&sig);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_read_value(sig, "r", r);
	if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&sig);
		return result;
	}

	result = _gnutls_x509_read_value(sig, "s", s);
	if (result < 0) {
		gnutls_assert();
		gnutls_free(r->data);
		asn1_delete_structure(&sig);
		return result;
	}

	asn1_delete_structure(&sig);

	return 0;
}

int
_gnutls_encode_gost_rs(gnutls_datum_t * sig_value, bigint_t r, bigint_t s,
		       size_t intsize)
{
	uint8_t *data;
	int result;

	data = gnutls_malloc(intsize * 2);
	if (data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	if ((result = _gnutls_mpi_bprint_size(s, data, intsize)) < 0) {
		gnutls_assert();
		gnutls_free(data);
		return result;
	}

	if ((result = _gnutls_mpi_bprint_size(r, data + intsize, intsize)) < 0) {
		gnutls_assert();
		gnutls_free(data);
		return result;
	}

	sig_value->data = data;
	sig_value->size = intsize * 2;

	return 0;
}

int
_gnutls_decode_gost_rs(const gnutls_datum_t * sig_value, bigint_t * r,
		       bigint_t * s)
{
	int ret;
	unsigned halfsize = sig_value->size >> 1;

	if (sig_value->size % 2 != 0) {
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}

	ret = _gnutls_mpi_init_scan(s, sig_value->data, halfsize);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ret = _gnutls_mpi_init_scan(r, sig_value->data + halfsize, halfsize);
	if (ret < 0) {
		_gnutls_mpi_release(s);
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	return 0;
}

/**
 * gnutls_encode_gost_rs_value:
 * @sig_value: will hold a GOST signature according to RFC 4491 section 2.2.2
 * @r: must contain the r value
 * @s: must contain the s value
 *
 * This function will encode the provided r and s values, into binary
 * representation according to RFC 4491 section 2.2.2, used for GOST R
 * 34.10-2001 (and thus also for GOST R 34.10-2012) signatures.
 *
 * The output value should be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.6.0
 */
int gnutls_encode_gost_rs_value(gnutls_datum_t * sig_value, const gnutls_datum_t * r, const gnutls_datum_t  *s)
{
	uint8_t *data;
	size_t intsize = r->size;

	if (s->size != intsize) {
		gnutls_assert();
		return GNUTLS_E_ILLEGAL_PARAMETER;
	}

	data = gnutls_malloc(intsize * 2);
	if (data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	memcpy(data, s->data, intsize);
	memcpy(data + intsize, r->data, intsize);

	sig_value->data = data;
	sig_value->size = intsize * 2;

	return 0;
}

/**
 * gnutls_decode_gost_rs_value:
 * @sig_value: will holds a GOST signature according to RFC 4491 section 2.2.2
 * @r: will contain the r value
 * @s: will contain the s value
 *
 * This function will decode the provided @sig_value, into @r and @s elements.
 * See RFC 4491 section 2.2.2 for the format of signature value.
 *
 * The output values may be padded with a zero byte to prevent them
 * from being interpreted as negative values. The value
 * should be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.6.0
 */
int gnutls_decode_gost_rs_value(const gnutls_datum_t * sig_value, gnutls_datum_t * r, gnutls_datum_t * s)
{
	int ret;
	unsigned halfsize = sig_value->size >> 1;

	if (sig_value->size % 2 != 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	ret = _gnutls_set_datum(s, sig_value->data, halfsize);
	if (ret != 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_set_datum(r, sig_value->data + halfsize, halfsize);
	if (ret != 0) {
		_gnutls_free_datum(s);
		return gnutls_assert_val(ret);
	}

	return 0;
}

gnutls_digest_algorithm_t _gnutls_gost_digest(gnutls_pk_algorithm_t pk)
{
	if (pk == GNUTLS_PK_GOST_01)
		return GNUTLS_DIG_GOSTR_94;
	else if (pk == GNUTLS_PK_GOST_12_256)
		return GNUTLS_DIG_STREEBOG_256;
	else if (pk == GNUTLS_PK_GOST_12_512)
		return GNUTLS_DIG_STREEBOG_512;

	gnutls_assert();

	return GNUTLS_DIG_UNKNOWN;
}

gnutls_pk_algorithm_t _gnutls_digest_gost(gnutls_digest_algorithm_t digest)
{
	if (digest == GNUTLS_DIG_GOSTR_94)
		return GNUTLS_PK_GOST_01;
	else if (digest == GNUTLS_DIG_STREEBOG_256)
		return GNUTLS_PK_GOST_12_256;
	else if (digest == GNUTLS_DIG_STREEBOG_512)
		return GNUTLS_PK_GOST_12_512;

	gnutls_assert();

	return GNUTLS_PK_UNKNOWN;
}

gnutls_gost_paramset_t _gnutls_gost_paramset_default(gnutls_pk_algorithm_t pk)
{
	if (pk == GNUTLS_PK_GOST_01)
		return GNUTLS_GOST_PARAMSET_CP_A;
	else if (pk == GNUTLS_PK_GOST_12_256 ||
		 pk == GNUTLS_PK_GOST_12_512)
		return GNUTLS_GOST_PARAMSET_TC26_Z;
	else
		return gnutls_assert_val(GNUTLS_GOST_PARAMSET_UNKNOWN);
}

/* some generic pk functions */

int _gnutls_pk_params_copy(gnutls_pk_params_st * dst,
			   const gnutls_pk_params_st * src)
{
	unsigned int i, j;
	dst->params_nr = 0;

	if (src == NULL || (src->params_nr == 0 && src->raw_pub.size == 0)) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	dst->pkflags = src->pkflags;
	dst->curve = src->curve;
	dst->gost_params = src->gost_params;
	dst->qbits = src->qbits;
	dst->algo = src->algo;

	for (i = 0; i < src->params_nr; i++) {
		dst->params[i] = _gnutls_mpi_copy(src->params[i]);
		if (dst->params[i] == NULL) {
			goto fail;
		}

		dst->params_nr++;
	}

	if (_gnutls_set_datum(&dst->raw_priv, src->raw_priv.data, src->raw_priv.size) < 0) {
		gnutls_assert();
		goto fail;
	}

	if (_gnutls_set_datum(&dst->raw_pub, src->raw_pub.data, src->raw_pub.size) < 0) {
		gnutls_assert();
		goto fail;
	}

	if (src->seed_size) {
		dst->seed_size = src->seed_size;
		memcpy(dst->seed, src->seed, src->seed_size);
	}
	dst->palgo = src->palgo;

	memcpy(&dst->spki, &src->spki, sizeof(gnutls_x509_spki_st));

	return 0;

fail:
	for (j = 0; j < i; j++)
		_gnutls_mpi_release(&dst->params[j]);
	return GNUTLS_E_MEMORY_ERROR;
}

void gnutls_pk_params_init(gnutls_pk_params_st * p)
{
	memset(p, 0, sizeof(gnutls_pk_params_st));
}

void gnutls_pk_params_release(gnutls_pk_params_st * p)
{
	unsigned int i;
	for (i = 0; i < p->params_nr; i++) {
		_gnutls_mpi_release(&p->params[i]);
	}
	gnutls_free(p->raw_priv.data);
	gnutls_free(p->raw_pub.data);

	p->params_nr = 0;
}

void gnutls_pk_params_clear(gnutls_pk_params_st * p)
{
	unsigned int i;
	for (i = 0; i < p->params_nr; i++) {
		if (p->params[i] != NULL)
			_gnutls_mpi_clear(p->params[i]);
	}
	gnutls_memset(p->seed, 0, p->seed_size);
	p->seed_size = 0;
	if (p->raw_priv.data != NULL) {
		gnutls_memset(p->raw_priv.data, 0, p->raw_priv.size);
		p->raw_priv.size = 0;
	}
}

int
_gnutls_find_rsa_pss_salt_size(unsigned bits, const mac_entry_st *me,
			       unsigned salt_size)
{
	unsigned digest_size;
	int max_salt_size;
	unsigned key_size;

	digest_size = _gnutls_hash_get_algo_len(me);
	key_size = (bits + 7) / 8;

	if (key_size == 0) {
		return gnutls_assert_val(GNUTLS_E_PK_INVALID_PUBKEY);
	} else {
		max_salt_size = key_size - digest_size - 2;
		if (max_salt_size < 0)
			return gnutls_assert_val(GNUTLS_E_CONSTRAINT_ERROR);
	}

	if (salt_size < digest_size)
		salt_size = digest_size;

	if (salt_size > (unsigned)max_salt_size)
		salt_size = max_salt_size;

	return salt_size;
}

/* Writes the digest information and the digest in a DER encoded
 * structure. The digest info is allocated and stored into the info structure.
 */
int
encode_ber_digest_info(const mac_entry_st * e,
			const gnutls_datum_t * digest,
			gnutls_datum_t * output)
{
	ASN1_TYPE dinfo = ASN1_TYPE_EMPTY;
	int result;
	const char *algo;
	uint8_t *tmp_output;
	int tmp_output_size;

	/* prevent asn1_write_value() treating input as string */
	if (digest->size == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	algo = _gnutls_x509_mac_to_oid(e);
	if (algo == NULL) {
		gnutls_assert();
		_gnutls_debug_log("Hash algorithm: %d has no OID\n",
				  e->id);
		return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
	}

	if ((result = asn1_create_element(_gnutls_get_gnutls_asn(),
					  "GNUTLS.DigestInfo",
					  &dinfo)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    asn1_write_value(dinfo, "digestAlgorithm.algorithm", algo, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	/* Write an ASN.1 NULL in the parameters field.  This matches RFC
	   3279 and RFC 4055, although is arguable incorrect from a historic
	   perspective (see those documents for more information).
	   Regardless of what is correct, this appears to be what most
	   implementations do.  */
	result = asn1_write_value(dinfo, "digestAlgorithm.parameters",
				  ASN1_NULL, ASN1_NULL_SIZE);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	result =
	    asn1_write_value(dinfo, "digest", digest->data, digest->size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	tmp_output_size = 0;
	result = asn1_der_coding(dinfo, "", NULL, &tmp_output_size, NULL);
	if (result != ASN1_MEM_ERROR) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	tmp_output = gnutls_malloc(tmp_output_size);
	if (tmp_output == NULL) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return GNUTLS_E_MEMORY_ERROR;
	}

	result =
	    asn1_der_coding(dinfo, "", tmp_output, &tmp_output_size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	asn1_delete_structure(&dinfo);

	output->size = tmp_output_size;
	output->data = tmp_output;

	return 0;
}

/**
 * gnutls_encode_ber_digest_info:
 * @info: an RSA BER encoded DigestInfo structure
 * @hash: the hash algorithm that was used to get the digest
 * @digest: must contain the digest data
 * @output: will contain the allocated DigestInfo BER encoded data
 *
 * This function will encode the provided digest data, and its
 * algorithm into an RSA PKCS#1 1.5 DigestInfo structure. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.5.0
 *
 **/
int
gnutls_encode_ber_digest_info(gnutls_digest_algorithm_t hash,
			      const gnutls_datum_t * digest,
			      gnutls_datum_t * output)
{
	const mac_entry_st *e = hash_to_entry(hash);
	if (unlikely(e == NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	return encode_ber_digest_info(e , digest, output);
}

/**
 * gnutls_decode_ber_digest_info:
 * @info: an RSA BER encoded DigestInfo structure
 * @hash: will contain the hash algorithm of the structure
 * @digest: will contain the hash output of the structure
 * @digest_size: will contain the hash size of the structure; initially must hold the maximum size of @digest
 *
 * This function will parse an RSA PKCS#1 1.5 DigestInfo structure
 * and report the hash algorithm used as well as the digest data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.5.0
 *
 **/
int
gnutls_decode_ber_digest_info(const gnutls_datum_t * info,
		       gnutls_digest_algorithm_t * hash,
		       unsigned char * digest, unsigned int *digest_size)
{
	ASN1_TYPE dinfo = ASN1_TYPE_EMPTY;
	int result;
	char str[MAX(MAX_OID_SIZE, MAX_HASH_SIZE)];
	int len;

	if ((result = asn1_create_element(_gnutls_get_gnutls_asn(),
					  "GNUTLS.DigestInfo",
					  &dinfo)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* rfc2313 required BER encoding of that field, thus
	 * we don't restrict libtasn1 to DER subset */
	result = asn1_der_decoding(&dinfo, info->data, info->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	len = sizeof(str) - 1;
	result =
	    asn1_read_value(dinfo, "digestAlgorithm.algorithm", str, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	*hash = gnutls_oid_to_digest(str);

	if (*hash == GNUTLS_DIG_UNKNOWN) {

		_gnutls_debug_log("verify.c: HASH OID: %s\n", str);

		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return GNUTLS_E_UNKNOWN_HASH_ALGORITHM;
	}

	len = sizeof(str) - 1;
	result =
	    asn1_read_value(dinfo, "digestAlgorithm.parameters", str,
			    &len);
	/* To avoid permitting garbage in the parameters field, either the
	   parameters field is not present, or it contains 0x05 0x00. */
	if (!(result == ASN1_ELEMENT_NOT_FOUND ||
	      (result == ASN1_SUCCESS && len == ASN1_NULL_SIZE &&
	       memcmp(str, ASN1_NULL, ASN1_NULL_SIZE) == 0))) {
		gnutls_assert();
		asn1_delete_structure(&dinfo);
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	}

	len = *digest_size;
	result = asn1_read_value(dinfo, "digest", digest, &len);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		*digest_size = len;
		asn1_delete_structure(&dinfo);
		return _gnutls_asn2err(result);
	}

	*digest_size = len;
	asn1_delete_structure(&dinfo);

	return 0;
}

int
_gnutls_params_get_rsa_raw(const gnutls_pk_params_st* params,
				    gnutls_datum_t * m, gnutls_datum_t * e,
				    gnutls_datum_t * d, gnutls_datum_t * p,
				    gnutls_datum_t * q, gnutls_datum_t * u,
				    gnutls_datum_t * e1,
				    gnutls_datum_t * e2,
				    unsigned int flags)
{
	int ret;
	mpi_dprint_func dprint = _gnutls_mpi_dprint_lz;

	if (flags & GNUTLS_EXPORT_FLAG_NO_LZ)
		dprint = _gnutls_mpi_dprint;

	if (params == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (!GNUTLS_PK_IS_RSA(params->algo)) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (m) {
		ret = dprint(params->params[0], m);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	}

	/* E */
	if (e) {
		ret = dprint(params->params[1], e);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	}

	/* D */
	if (d && params->params[2]) {
		ret = dprint(params->params[2], d);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else if (d) {
		d->data = NULL;
		d->size = 0;
	}

	/* P */
	if (p && params->params[3]) {
		ret = dprint(params->params[3], p);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else if (p) {
		p->data = NULL;
		p->size = 0;
	}

	/* Q */
	if (q && params->params[4]) {
		ret = dprint(params->params[4], q);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else if (q) {
		q->data = NULL;
		q->size = 0;
	}

	/* U */
	if (u && params->params[5]) {
		ret = dprint(params->params[5], u);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else if (u) {
		u->data = NULL;
		u->size = 0;
	}

	/* E1 */
	if (e1 && params->params[6]) {
		ret = dprint(params->params[6], e1);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else if (e1) {
		e1->data = NULL;
		e1->size = 0;
	}

	/* E2 */
	if (e2 && params->params[7]) {
		ret = dprint(params->params[7], e2);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else if (e2) {
		e2->data = NULL;
		e2->size = 0;
	}

	return 0;

      error:
	_gnutls_free_datum(m);
	_gnutls_free_datum(d);
	_gnutls_free_datum(e);
	_gnutls_free_datum(e1);
	_gnutls_free_datum(e2);
	_gnutls_free_datum(p);
	_gnutls_free_datum(q);

	return ret;
}

int
_gnutls_params_get_dsa_raw(const gnutls_pk_params_st* params,
			     gnutls_datum_t * p, gnutls_datum_t * q,
			     gnutls_datum_t * g, gnutls_datum_t * y,
			     gnutls_datum_t * x, unsigned int flags)
{
	int ret;
	mpi_dprint_func dprint = _gnutls_mpi_dprint_lz;

	if (flags & GNUTLS_EXPORT_FLAG_NO_LZ)
		dprint = _gnutls_mpi_dprint;

	if (params == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (params->algo != GNUTLS_PK_DSA) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* P */
	if (p) {
		ret = dprint(params->params[0], p);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	/* Q */
	if (q) {
		ret = dprint(params->params[1], q);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(p);
			return ret;
		}
	}


	/* G */
	if (g) {
		ret = dprint(params->params[2], g);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(p);
			_gnutls_free_datum(q);
			return ret;
		}
	}


	/* Y */
	if (y) {
		ret = dprint(params->params[3], y);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(p);
			_gnutls_free_datum(g);
			_gnutls_free_datum(q);
			return ret;
		}
	}

	/* X */
	if (x) {
		ret = dprint(params->params[4], x);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(y);
			_gnutls_free_datum(p);
			_gnutls_free_datum(g);
			_gnutls_free_datum(q);
			return ret;
		}
	}

	return 0;
}

int _gnutls_params_get_ecc_raw(const gnutls_pk_params_st* params,
				       gnutls_ecc_curve_t * curve,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k,
				       unsigned int flags)
{
	int ret;
	mpi_dprint_func dprint = _gnutls_mpi_dprint_lz;
	const gnutls_ecc_curve_entry_st *e;

	if (flags & GNUTLS_EXPORT_FLAG_NO_LZ)
		dprint = _gnutls_mpi_dprint;

	if (params == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (curve)
		*curve = params->curve;

	e = _gnutls_ecc_curve_get_params(params->curve);

	if (_curve_is_eddsa(e)) {
		if (x) {
			ret = _gnutls_set_datum(x, params->raw_pub.data, params->raw_pub.size);
			if (ret < 0) {
				return gnutls_assert_val(ret);
			}
		}

		if (y) {
			y->data = NULL;
			y->size = 0;
		}

		if (k) {
			ret = _gnutls_set_datum(k, params->raw_priv.data, params->raw_priv.size);
			if (ret < 0) {
				_gnutls_free_datum(x);
				return gnutls_assert_val(ret);
			}
		}

		return 0;
	}

	if (unlikely(e == NULL || e->pk != GNUTLS_PK_ECDSA))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	/* X */
	if (x) {
		ret = dprint(params->params[ECC_X], x);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	/* Y */
	if (y) {
		ret = dprint(params->params[ECC_Y], y);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(x);
			return ret;
		}
	}


	/* K */
	if (k) {
		ret = dprint(params->params[ECC_K], k);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(x);
			_gnutls_free_datum(y);
			return ret;
		}
	}

	return 0;

}

int _gnutls_params_get_gost_raw(const gnutls_pk_params_st* params,
				       gnutls_ecc_curve_t * curve,
				       gnutls_digest_algorithm_t * digest,
				       gnutls_gost_paramset_t * paramset,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k,
				       unsigned int flags)
{
	int ret;
	mpi_dprint_func dprint = _gnutls_mpi_dprint_le;

	if (params == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (curve)
		*curve = params->curve;

	if (digest)
		*digest = _gnutls_gost_digest(params->algo);

	if (paramset)
		*paramset = params->gost_params;

	/* X */
	if (x) {
		ret = dprint(params->params[GOST_X], x);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	/* Y */
	if (y) {
		ret = dprint(params->params[GOST_Y], y);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(x);
			return ret;
		}
	}


	/* K */
	if (k) {
		ret = dprint(params->params[GOST_K], k);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_free_datum(x);
			_gnutls_free_datum(y);
			return ret;
		}
	}

	return 0;

}

int
pk_hash_data(gnutls_pk_algorithm_t pk, const mac_entry_st * hash,
	     gnutls_pk_params_st * params,
	     const gnutls_datum_t * data, gnutls_datum_t * digest)
{
	int ret;

	digest->size = _gnutls_hash_get_algo_len(hash);
	digest->data = gnutls_malloc(digest->size);
	if (digest->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret =
	    _gnutls_hash_fast((gnutls_digest_algorithm_t)hash->id, data->data, data->size,
			      digest->data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	gnutls_free(digest->data);
	return ret;
}


/* 
 * This function will do RSA PKCS #1 1.5 encoding
 * on the given digest. The given digest must be allocated
 * and will be freed if replacement is required.
 */
int
pk_prepare_hash(gnutls_pk_algorithm_t pk,
		const mac_entry_st * hash, gnutls_datum_t * digest)
{
	int ret;
	gnutls_datum_t old_digest = { digest->data, digest->size };

	switch (pk) {
	case GNUTLS_PK_RSA:
		if (unlikely(hash == NULL))
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		/* Encode the digest as a DigestInfo
		 */
		if ((ret =
		     encode_ber_digest_info(hash, &old_digest,
					    digest)) != 0) {
			gnutls_assert();
			return ret;
		}

		_gnutls_free_datum(&old_digest);
		break;
	case GNUTLS_PK_RSA_PSS:
	case GNUTLS_PK_DSA:
	case GNUTLS_PK_ECDSA:
	case GNUTLS_PK_EDDSA_ED25519:
	case GNUTLS_PK_GOST_01:
	case GNUTLS_PK_GOST_12_256:
	case GNUTLS_PK_GOST_12_512:
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_UNIMPLEMENTED_FEATURE;
	}

	return 0;
}
