/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2014-2016 Red Hat
 * Copyright (C) 2014-2016 Nikos Mavrogiannopoulos
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

#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <x509.h>
#include <x509_b64.h>
#include "x509_int.h"
#include "pkcs7_int.h"
#include <algorithms.h>
#include <num.h>
#include <random.h>
#include <pk.h>

#define PBES1_DES_MD5_OID "1.2.840.113549.1.5.3"

#define PBES2_OID "1.2.840.113549.1.5.13"
#define PBKDF2_OID "1.2.840.113549.1.5.12"
#define DES_EDE3_CBC_OID "1.2.840.113549.3.7"
#define AES_128_CBC_OID "2.16.840.1.101.3.4.1.2"
#define AES_192_CBC_OID "2.16.840.1.101.3.4.1.22"
#define AES_256_CBC_OID "2.16.840.1.101.3.4.1.42"
#define DES_CBC_OID "1.3.14.3.2.7"

/* oid_pbeWithSHAAnd3_KeyTripleDES_CBC */
#define PKCS12_PBE_3DES_SHA1_OID "1.2.840.113549.1.12.1.3"
#define PKCS12_PBE_ARCFOUR_SHA1_OID "1.2.840.113549.1.12.1.1"
#define PKCS12_PBE_RC2_40_SHA1_OID "1.2.840.113549.1.12.1.6"

static const struct pkcs_cipher_schema_st avail_pkcs_cipher_schemas[] = {
	{
	 .schema = PBES1_DES_MD5,
	 .name = "PBES1-DES-CBC-MD5",
	 .flag = GNUTLS_PKCS_PBES1_DES_MD5,
	 .cipher = GNUTLS_CIPHER_DES_CBC,
	 .pbes2 = 0,
	 .cipher_oid = PBES1_DES_MD5_OID,
	 .write_oid = PBES1_DES_MD5_OID,
	 .desc = NULL,
	 .iv_name = NULL,
	 .decrypt_only = 1},
	{
	 .schema = PBES2_3DES,
	 .name = "PBES2-3DES-CBC",
	 .flag = GNUTLS_PKCS_PBES2_3DES,
	 .cipher = GNUTLS_CIPHER_3DES_CBC,
	 .pbes2 = 1,
	 .cipher_oid = DES_EDE3_CBC_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.pkcs-5-des-EDE3-CBC-params",
	 .iv_name = "",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_DES,
	 .name = "PBES2-DES-CBC",
	 .flag = GNUTLS_PKCS_PBES2_DES,
	 .cipher = GNUTLS_CIPHER_DES_CBC,
	 .pbes2 = 1,
	 .cipher_oid = DES_CBC_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.pkcs-5-des-CBC-params",
	 .iv_name = "",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_AES_128,
	 .name = "PBES2-AES128-CBC",
	 .flag = GNUTLS_PKCS_PBES2_AES_128,
	 .cipher = GNUTLS_CIPHER_AES_128_CBC,
	 .pbes2 = 1,
	 .cipher_oid = AES_128_CBC_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.pkcs-5-aes128-CBC-params",
	 .iv_name = "",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_AES_192,
	 .name = "PBES2-AES192-CBC",
	 .flag = GNUTLS_PKCS_PBES2_AES_192,
	 .cipher = GNUTLS_CIPHER_AES_192_CBC,
	 .pbes2 = 1,
	 .cipher_oid = AES_192_CBC_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.pkcs-5-aes192-CBC-params",
	 .iv_name = "",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_AES_256,
	 .name = "PBES2-AES256-CBC",
	 .flag = GNUTLS_PKCS_PBES2_AES_256,
	 .cipher = GNUTLS_CIPHER_AES_256_CBC,
	 .pbes2 = 1,
	 .cipher_oid = AES_256_CBC_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.pkcs-5-aes256-CBC-params",
	 .iv_name = "",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_GOST28147_89_TC26Z,
	 .name = "PBES2-GOST28147-89-TC26Z",
	 .flag = GNUTLS_PKCS_PBES2_GOST_TC26Z,
	 .cipher = GNUTLS_CIPHER_GOST28147_TC26Z_CFB,
	 .pbes2 = 1,
	 .cipher_oid = GOST28147_89_TC26Z_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.Gost28147-89-Parameters",
	 .iv_name = "iv",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_GOST28147_89_CPA,
	 .name = "PBES2-GOST28147-89-CPA",
	 .flag = GNUTLS_PKCS_PBES2_GOST_CPA,
	 .cipher = GNUTLS_CIPHER_GOST28147_CPA_CFB,
	 .pbes2 = 1,
	 .cipher_oid = GOST28147_89_CPA_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.Gost28147-89-Parameters",
	 .iv_name = "iv",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_GOST28147_89_CPB,
	 .name = "PBES2-GOST28147-89-CPB",
	 .flag = GNUTLS_PKCS_PBES2_GOST_CPB,
	 .cipher = GNUTLS_CIPHER_GOST28147_CPB_CFB,
	 .pbes2 = 1,
	 .cipher_oid = GOST28147_89_CPB_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.Gost28147-89-Parameters",
	 .iv_name = "iv",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_GOST28147_89_CPC,
	 .name = "PBES2-GOST28147-89-CPC",
	 .flag = GNUTLS_PKCS_PBES2_GOST_CPC,
	 .cipher = GNUTLS_CIPHER_GOST28147_CPC_CFB,
	 .pbes2 = 1,
	 .cipher_oid = GOST28147_89_CPC_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.Gost28147-89-Parameters",
	 .iv_name = "iv",
	 .decrypt_only = 0},
	{
	 .schema = PBES2_GOST28147_89_CPD,
	 .name = "PBES2-GOST28147-89-CPD",
	 .flag = GNUTLS_PKCS_PBES2_GOST_CPD,
	 .cipher = GNUTLS_CIPHER_GOST28147_CPD_CFB,
	 .pbes2 = 1,
	 .cipher_oid = GOST28147_89_CPD_OID,
	 .write_oid = PBES2_OID,
	 .desc = "PKIX1.Gost28147-89-Parameters",
	 .iv_name = "iv",
	 .decrypt_only = 0},
	{
	 .schema = PKCS12_ARCFOUR_SHA1,
	 .name = "PKCS12-ARCFOUR-SHA1",
	 .flag = GNUTLS_PKCS_PKCS12_ARCFOUR,
	 .cipher = GNUTLS_CIPHER_ARCFOUR,
	 .pbes2 = 0,
	 .cipher_oid = PKCS12_PBE_ARCFOUR_SHA1_OID,
	 .write_oid = PKCS12_PBE_ARCFOUR_SHA1_OID,
	 .desc = NULL,
	 .iv_name = NULL,
	 .decrypt_only = 0},
	{
	 .schema = PKCS12_RC2_40_SHA1,
	 .name = "PKCS12-RC2-40-SHA1",
	 .flag = GNUTLS_PKCS_PKCS12_RC2_40,
	 .cipher = GNUTLS_CIPHER_RC2_40_CBC,
	 .pbes2 = 0,
	 .cipher_oid = PKCS12_PBE_RC2_40_SHA1_OID,
	 .write_oid = PKCS12_PBE_RC2_40_SHA1_OID,
	 .desc = NULL,
	 .iv_name = NULL,
	 .decrypt_only = 0},
	{
	 .schema = PKCS12_3DES_SHA1,
	 .name = "PKCS12-3DES-SHA1",
	 .flag = GNUTLS_PKCS_PKCS12_3DES,
	 .cipher = GNUTLS_CIPHER_3DES_CBC,
	 .pbes2 = 0,
	 .cipher_oid = PKCS12_PBE_3DES_SHA1_OID,
	 .write_oid = PKCS12_PBE_3DES_SHA1_OID,
	 .desc = NULL,
	 .iv_name = NULL,
	 .decrypt_only = 0},
	{0, 0, 0, 0, 0}
};

#define PBES2_SCHEMA_LOOP(b) { \
	const struct pkcs_cipher_schema_st * _p; \
		for (_p=avail_pkcs_cipher_schemas;_p->schema != 0;_p++) { b; } \
	}

#define PBES2_SCHEMA_FIND_FROM_FLAGS(fl, what) \
	PBES2_SCHEMA_LOOP( if (_p->flag == GNUTLS_PKCS_CIPHER_MASK(fl)) { what; } )

int _gnutls_pkcs_flags_to_schema(unsigned int flags)
{
	PBES2_SCHEMA_FIND_FROM_FLAGS(flags, return _p->schema;
	    );

	gnutls_assert();
	_gnutls_debug_log
	    ("Selecting default encryption PKCS12_3DES_SHA1 (flags: %u).\n",
	     flags);
	return PKCS12_3DES_SHA1;
}

/**
 * gnutls_pkcs_schema_get_name:
 * @schema: Holds the PKCS #12 or PBES2 schema (%gnutls_pkcs_encrypt_flags_t)
 *
 * This function will return a human readable description of the
 * PKCS12 or PBES2 schema.
 *
 * Returns: a constrant string or %NULL on error.
 *
 * Since: 3.4.0
 */
const char *gnutls_pkcs_schema_get_name(unsigned int schema)
{
	PBES2_SCHEMA_FIND_FROM_FLAGS(schema, return _p->name;
	    );
	return NULL;
}

/**
 * gnutls_pkcs_schema_get_oid:
 * @schema: Holds the PKCS #12 or PBES2 schema (%gnutls_pkcs_encrypt_flags_t)
 *
 * This function will return the object identifier of the
 * PKCS12 or PBES2 schema.
 *
 * Returns: a constrant string or %NULL on error.
 *
 * Since: 3.4.0
 */
const char *gnutls_pkcs_schema_get_oid(unsigned int schema)
{
	PBES2_SCHEMA_FIND_FROM_FLAGS(schema, return _p->cipher_oid;
	    );
	return NULL;
}

static const struct pkcs_cipher_schema_st *algo_to_pbes2_cipher_schema(unsigned
								       cipher)
{
	PBES2_SCHEMA_LOOP(if (_p->cipher == cipher && _p->pbes2 != 0) {
			  return _p;}
	) ;

	gnutls_assert();
	return NULL;
}

/* Converts a PKCS#7 encryption schema OID to an internal
 * schema_id or returns a negative value */
int _gnutls_check_pkcs_cipher_schema(const char *oid)
{
	if (strcmp(oid, PBES2_OID) == 0)
		return PBES2_GENERIC;	/* PBES2 ciphers are under an umbrella OID */

	PBES2_SCHEMA_LOOP(if (_p->pbes2 == 0 && strcmp(oid, _p->write_oid) == 0) {
			  return _p->schema;}
	) ;
	_gnutls_debug_log
	    ("PKCS #12 encryption schema OID '%s' is unsupported.\n", oid);

	return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

const struct pkcs_cipher_schema_st *_gnutls_pkcs_schema_get(schema_id schema)
{
	PBES2_SCHEMA_LOOP(if (schema == _p->schema) return _p;) ;

	gnutls_assert();
	return NULL;
}

/* Converts an OID to a gnutls cipher type.
 */
static int
pbes2_cipher_oid_to_algo(const char *oid, gnutls_cipher_algorithm_t * algo)
{

	*algo = 0;
	PBES2_SCHEMA_LOOP(if
			  (_p->pbes2 != 0 && strcmp(_p->cipher_oid, oid) == 0) {
			  *algo = _p->cipher; return 0;}
	) ;

	_gnutls_debug_log("PKCS #8 encryption OID '%s' is unsupported.\n", oid);
	return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

/* Decrypts a PKCS #7 encryptedData. The output is allocated
 * and stored in dec.
 */
int
_gnutls_pkcs7_decrypt_data(const gnutls_datum_t * data,
			   const char *password, gnutls_datum_t * dec)
{
	int result, len;
	char enc_oid[MAX_OID_SIZE];
	gnutls_datum_t tmp;
	ASN1_TYPE pasn = ASN1_TYPE_EMPTY, pkcs7_asn = ASN1_TYPE_EMPTY;
	int params_start, params_end, params_len;
	struct pbkdf2_params kdf_params;
	struct pbe_enc_params enc_params;
	schema_id schema;

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.pkcs-7-EncryptedData",
				 &pkcs7_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = asn1_der_decoding(&pkcs7_asn, data->data, data->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* Check the encryption schema OID
	 */
	len = sizeof(enc_oid);
	result =
	    asn1_read_value(pkcs7_asn,
			    "encryptedContentInfo.contentEncryptionAlgorithm.algorithm",
			    enc_oid, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	if ((result = _gnutls_check_pkcs_cipher_schema(enc_oid)) < 0) {
		gnutls_assert();
		goto error;
	}
	schema = result;

	/* Get the DER encoding of the parameters.
	 */
	result =
	    asn1_der_decoding_startEnd(pkcs7_asn, data->data, data->size,
				       "encryptedContentInfo.contentEncryptionAlgorithm.parameters",
				       &params_start, &params_end);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	params_len = params_end - params_start + 1;

	result =
	    _gnutls_read_pkcs_schema_params(&schema, password,
					    &data->data[params_start],
					    params_len, &kdf_params,
					    &enc_params);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	/* Parameters have been decoded. Now
	 * decrypt the EncryptedData.
	 */

	result =
	    _gnutls_pkcs_raw_decrypt_data(schema, pkcs7_asn,
					  "encryptedContentInfo.encryptedContent",
					  password, &kdf_params, &enc_params,
					  &tmp);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	asn1_delete_structure2(&pkcs7_asn, ASN1_DELETE_FLAG_ZEROIZE);

	*dec = tmp;

	return 0;

 error:
	asn1_delete_structure(&pasn);
	asn1_delete_structure2(&pkcs7_asn, ASN1_DELETE_FLAG_ZEROIZE);
	return result;
}

int
_gnutls_pkcs7_data_enc_info(const gnutls_datum_t * data,
			    const struct pkcs_cipher_schema_st **p,
			    struct pbkdf2_params *kdf_params, char **oid)
{
	int result, len;
	char enc_oid[MAX_OID_SIZE];
	ASN1_TYPE pasn = ASN1_TYPE_EMPTY, pkcs7_asn = ASN1_TYPE_EMPTY;
	int params_start, params_end, params_len;
	struct pbe_enc_params enc_params;
	schema_id schema;

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.pkcs-7-EncryptedData",
				 &pkcs7_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = asn1_der_decoding(&pkcs7_asn, data->data, data->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* Check the encryption schema OID
	 */
	len = sizeof(enc_oid);
	result =
	    asn1_read_value(pkcs7_asn,
			    "encryptedContentInfo.contentEncryptionAlgorithm.algorithm",
			    enc_oid, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	if (oid) {
		*oid = gnutls_strdup(enc_oid);
	}

	if ((result = _gnutls_check_pkcs_cipher_schema(enc_oid)) < 0) {
		gnutls_assert();
		goto error;
	}
	schema = result;

	/* Get the DER encoding of the parameters.
	 */
	result =
	    asn1_der_decoding_startEnd(pkcs7_asn, data->data, data->size,
				       "encryptedContentInfo.contentEncryptionAlgorithm.parameters",
				       &params_start, &params_end);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	params_len = params_end - params_start + 1;

	result =
	    _gnutls_read_pkcs_schema_params(&schema, NULL,
					    &data->data[params_start],
					    params_len, kdf_params,
					    &enc_params);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	*p = _gnutls_pkcs_schema_get(schema);
	if (*p == NULL) {
		gnutls_assert();
		result = GNUTLS_E_UNKNOWN_CIPHER_TYPE;
		goto error;
	}

	asn1_delete_structure2(&pkcs7_asn, ASN1_DELETE_FLAG_ZEROIZE);

	return 0;

 error:
	asn1_delete_structure(&pasn);
	asn1_delete_structure2(&pkcs7_asn, ASN1_DELETE_FLAG_ZEROIZE);
	return result;
}

/* Encrypts to a PKCS #7 encryptedData. The output is allocated
 * and stored in enc.
 */
int
_gnutls_pkcs7_encrypt_data(schema_id schema,
			   const gnutls_datum_t * data,
			   const char *password, gnutls_datum_t * enc)
{
	int result;
	gnutls_datum_t key = { NULL, 0 };
	gnutls_datum_t tmp = { NULL, 0 };
	ASN1_TYPE pkcs7_asn = ASN1_TYPE_EMPTY;
	struct pbkdf2_params kdf_params;
	struct pbe_enc_params enc_params;
	const struct pkcs_cipher_schema_st *s;

	s = _gnutls_pkcs_schema_get(schema);
	if (s == NULL || s->decrypt_only) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.pkcs-7-EncryptedData",
				 &pkcs7_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result =
	    asn1_write_value(pkcs7_asn,
			     "encryptedContentInfo.contentEncryptionAlgorithm.algorithm",
			     s->write_oid, 1);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* Generate a symmetric key.
	 */

	result =
	    _gnutls_pkcs_generate_key(schema, password, &kdf_params,
				      &enc_params, &key);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	result = _gnutls_pkcs_write_schema_params(schema, pkcs7_asn,
						  "encryptedContentInfo.contentEncryptionAlgorithm.parameters",
						  &kdf_params, &enc_params);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	/* Parameters have been encoded. Now
	 * encrypt the Data.
	 */
	result = _gnutls_pkcs_raw_encrypt_data(data, &enc_params, &key, &tmp);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	/* write the encrypted data.
	 */
	result =
	    asn1_write_value(pkcs7_asn,
			     "encryptedContentInfo.encryptedContent",
			     tmp.data, tmp.size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	_gnutls_free_datum(&tmp);
	_gnutls_free_key_datum(&key);

	/* Now write the rest of the pkcs-7 stuff.
	 */

	result = _gnutls_x509_write_uint32(pkcs7_asn, "version", 0);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	result =
	    asn1_write_value(pkcs7_asn, "encryptedContentInfo.contentType",
			     DATA_OID, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = asn1_write_value(pkcs7_asn, "unprotectedAttrs", NULL, 0);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* Now encode and copy the DER stuff.
	 */
	result = _gnutls_x509_der_encode(pkcs7_asn, "", enc, 0);

	asn1_delete_structure2(&pkcs7_asn, ASN1_DELETE_FLAG_ZEROIZE);

	if (result < 0) {
		gnutls_assert();
		goto error;
	}

 error:
	_gnutls_free_key_datum(&key);
	_gnutls_free_datum(&tmp);
	asn1_delete_structure2(&pkcs7_asn, ASN1_DELETE_FLAG_ZEROIZE);
	return result;
}

/* Reads the PBKDF2 parameters.
 */
static int
read_pbkdf2_params(ASN1_TYPE pasn,
		   const gnutls_datum_t * der, struct pbkdf2_params *params)
{
	int params_start, params_end;
	int params_len, len, result;
	ASN1_TYPE pbkdf2_asn = ASN1_TYPE_EMPTY;
	char oid[MAX_OID_SIZE];

	memset(params, 0, sizeof(*params));

	params->mac = GNUTLS_MAC_SHA1;

	/* Check the key derivation algorithm
	 */
	len = sizeof(oid);
	result =
	    asn1_read_value(pasn, "keyDerivationFunc.algorithm", oid, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}
	_gnutls_hard_log("keyDerivationFunc.algorithm: %s\n", oid);

	if (strcmp(oid, PBKDF2_OID) != 0) {
		gnutls_assert();
		_gnutls_debug_log
		    ("PKCS #8 key derivation OID '%s' is unsupported.\n", oid);
		return _gnutls_asn2err(result);
	}

	result =
	    asn1_der_decoding_startEnd(pasn, der->data, der->size,
				       "keyDerivationFunc.parameters",
				       &params_start, &params_end);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}
	params_len = params_end - params_start + 1;

	/* Now check the key derivation and the encryption
	 * functions.
	 */
	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.pkcs-5-PBKDF2-params",
				 &pbkdf2_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    _asn1_strict_der_decode(&pbkdf2_asn, &der->data[params_start],
				    params_len, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* read the salt */
	params->salt_size = sizeof(params->salt);
	result =
	    asn1_read_value(pbkdf2_asn, "salt.specified", params->salt,
			    &params->salt_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	_gnutls_hard_log("salt.specified.size: %d\n", params->salt_size);

	if (params->salt_size < 0) {
		result = gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
		goto error;
	}

	/* read the iteration count 
	 */
	result =
	    _gnutls_x509_read_uint(pbkdf2_asn, "iterationCount",
				   &params->iter_count);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	if (params->iter_count >= MAX_ITER_COUNT || params->iter_count == 0) {
		result = gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
		goto error;
	}

	_gnutls_hard_log("iterationCount: %d\n", params->iter_count);

	/* read the keylength, if it is set.
	 */
	result =
	    _gnutls_x509_read_uint(pbkdf2_asn, "keyLength", &params->key_size);
	if (result < 0) {
		params->key_size = 0;
	}

	if (params->key_size > MAX_CIPHER_KEY_SIZE) {
		result = gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
		goto error;
	}

	_gnutls_hard_log("keyLength: %d\n", params->key_size);

	len = sizeof(oid);
	result = asn1_read_value(pbkdf2_asn, "prf.algorithm", oid, &len);
	if (result != ASN1_SUCCESS) {
		/* use the default MAC */
		result = 0;
		goto error;
	}

	params->mac = gnutls_oid_to_mac(oid);
	if (params->mac == GNUTLS_MAC_UNKNOWN) {
		gnutls_assert();
		_gnutls_debug_log("Unsupported hash algorithm: %s\n", oid);
		result = GNUTLS_E_UNKNOWN_HASH_ALGORITHM;
		goto error;
	}

	result = 0;

 error:
	asn1_delete_structure(&pbkdf2_asn);
	return result;

}

/* Reads the PBE parameters from PKCS-12 schemas (*&#%*&#% RSA).
 */
static int read_pkcs12_kdf_params(ASN1_TYPE pasn, struct pbkdf2_params *params)
{
	int result;

	memset(params, 0, sizeof(*params));

	/* read the salt */
	params->salt_size = sizeof(params->salt);
	result =
	    asn1_read_value(pasn, "salt", params->salt, &params->salt_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (params->salt_size < 0)
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	_gnutls_hard_log("salt.size: %d\n", params->salt_size);

	/* read the iteration count 
	 */
	result =
	    _gnutls_x509_read_uint(pasn, "iterations", &params->iter_count);
	if (result < 0)
		return gnutls_assert_val(result);

	if (params->iter_count >= MAX_ITER_COUNT || params->iter_count == 0)
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	_gnutls_hard_log("iterationCount: %d\n", params->iter_count);

	params->key_size = 0;

	return 0;
}

/* Writes the PBE parameters for PKCS-12 schemas.
 */
static int
write_pkcs12_kdf_params(ASN1_TYPE pasn, const struct pbkdf2_params *kdf_params)
{
	int result;

	/* write the salt 
	 */
	result =
	    asn1_write_value(pasn, "salt",
			     kdf_params->salt, kdf_params->salt_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	_gnutls_hard_log("salt.size: %d\n", kdf_params->salt_size);

	/* write the iteration count 
	 */
	result =
	    _gnutls_x509_write_uint32(pasn, "iterations",
				      kdf_params->iter_count);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}
	_gnutls_hard_log("iterationCount: %d\n", kdf_params->iter_count);

	return 0;

 error:
	return result;

}

static int
read_pbes2_gost_oid(uint8_t *der, size_t len, char *oid, int oid_size)
{
	int result;
	ASN1_TYPE pbe_asn = ASN1_TYPE_EMPTY;

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.Gost28147-89-Parameters", &pbe_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    _asn1_strict_der_decode(&pbe_asn, der, len, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = asn1_read_value(pbe_asn, "encryptionParamSet",
				 oid, &oid_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = 0;

 error:
	asn1_delete_structure(&pbe_asn);
	return result;
}

static int
read_pbes2_enc_params(ASN1_TYPE pasn,
		      const gnutls_datum_t * der, struct pbe_enc_params *params)
{
	int params_start, params_end;
	int params_len, len, result;
	ASN1_TYPE pbe_asn = ASN1_TYPE_EMPTY;
	const struct pkcs_cipher_schema_st *p;

	memset(params, 0, sizeof(*params));

	/* Check the encryption algorithm
	 */
	len = sizeof(params->pbes2_oid);
	result = asn1_read_value(pasn, "encryptionScheme.algorithm", params->pbes2_oid, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}
	_gnutls_hard_log("encryptionScheme.algorithm: %s\n", params->pbes2_oid);

	result =
	    asn1_der_decoding_startEnd(pasn, der->data, der->size,
				       "encryptionScheme.parameters",
				       &params_start, &params_end);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}
	params_len = params_end - params_start + 1;

	/* For GOST we have to read params to determine actual cipher */
	if (!strcmp(params->pbes2_oid, GOST28147_89_OID)) {
		len = sizeof(params->pbes2_oid);
		result = read_pbes2_gost_oid(&der->data[params_start],
					     params_len, params->pbes2_oid, len);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	if ((result = pbes2_cipher_oid_to_algo(params->pbes2_oid, &params->cipher)) < 0) {
		gnutls_assert();
		return result;
	}

	/* Now check the encryption parameters.
	 */
	p = algo_to_pbes2_cipher_schema(params->cipher);
	if (p == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 p->desc, &pbe_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    _asn1_strict_der_decode(&pbe_asn, &der->data[params_start],
				    params_len, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* read the IV */
	params->iv_size = sizeof(params->iv);
	result = asn1_read_value(pbe_asn,
			p->iv_name,
			params->iv, &params->iv_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	_gnutls_hard_log("IV.size: %d\n", params->iv_size);

	result = 0;

 error:
	asn1_delete_structure(&pbe_asn);
	return result;
}

/* Read the parameters cipher, IV, salt etc using the given
 * schema ID. Initially the schema ID should have PBES2_GENERIC, for
 * PBES2 schemas, and will be updated by this function for details.
 */
int
_gnutls_read_pkcs_schema_params(schema_id * schema, const char *password,
				const uint8_t * data, int data_size,
				struct pbkdf2_params *kdf_params,
				struct pbe_enc_params *enc_params)
{
	ASN1_TYPE pasn = ASN1_TYPE_EMPTY;
	int result;
	gnutls_datum_t tmp;
	const struct pkcs_cipher_schema_st *p;

	if (*schema == PBES2_GENERIC) {
		/* Now check the key derivation and the encryption
		 * functions.
		 */
		if ((result =
		     asn1_create_element(_gnutls_get_pkix(),
					 "PKIX1.pkcs-5-PBES2-params",
					 &pasn)) != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto error;
		}

		/* Decode the parameters.
		 */
		result = _asn1_strict_der_decode(&pasn, data, data_size, NULL);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto error;
		}

		tmp.data = (uint8_t *) data;
		tmp.size = data_size;

		result = read_pbkdf2_params(pasn, &tmp, kdf_params);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		result = read_pbes2_enc_params(pasn, &tmp, enc_params);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		asn1_delete_structure2(&pasn, ASN1_DELETE_FLAG_ZEROIZE);

		p = algo_to_pbes2_cipher_schema(enc_params->cipher);
		if (p == NULL) {
			result = GNUTLS_E_INVALID_REQUEST;
			gnutls_assert();
			goto error;
		}

		*schema = p->schema;
		return 0;
	} else if (*schema == PBES1_DES_MD5) {
		return _gnutls_read_pbkdf1_params(data, data_size, kdf_params,
						  enc_params);
	} else {		/* PKCS #12 schema */
		memset(enc_params, 0, sizeof(*enc_params));

		p = _gnutls_pkcs_schema_get(*schema);
		if (p == NULL) {
			gnutls_assert();
			result = GNUTLS_E_UNKNOWN_CIPHER_TYPE;
			goto error;
		}
		enc_params->cipher = p->cipher;
		enc_params->iv_size = gnutls_cipher_get_iv_size(p->cipher);

		if ((result =
		     asn1_create_element(_gnutls_get_pkix(),
					 "PKIX1.pkcs-12-PbeParams",
					 &pasn)) != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto error;
		}

		/* Decode the parameters.
		 */
		result = _asn1_strict_der_decode(&pasn, data, data_size, NULL);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto error;
		}

		result = read_pkcs12_kdf_params(pasn, kdf_params);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		if (enc_params->iv_size) {
			result =
			    _gnutls_pkcs12_string_to_key(mac_to_entry
							 (GNUTLS_MAC_SHA1),
							 2 /*IV*/,
							 kdf_params->salt,
							 kdf_params->salt_size,
							 kdf_params->iter_count,
							 password,
							 enc_params->iv_size,
							 enc_params->iv);
			if (result < 0) {
				gnutls_assert();
				goto error;
			}

		}

		asn1_delete_structure(&pasn);

		return 0;
	}			/* switch */

 error:
	asn1_delete_structure(&pasn);
	return result;
}

static int
_gnutls_pbes2_string_to_key(unsigned int pass_len, const char *password,
			    const struct pbkdf2_params *kdf_params,
			    int key_size, uint8_t *key)
{
	gnutls_datum_t _key;
	gnutls_datum_t salt;

	_key.data = (void *)password;
	_key.size = pass_len;
	salt.data = (void *)kdf_params->salt;
	salt.size = kdf_params->salt_size;

	return gnutls_pbkdf2(kdf_params->mac, &_key, &salt,
			     kdf_params->iter_count, key, key_size);
}

int
_gnutls_pkcs_raw_decrypt_data(schema_id schema, ASN1_TYPE pkcs8_asn,
			      const char *root, const char *_password,
			      const struct pbkdf2_params *kdf_params,
			      const struct pbe_enc_params *enc_params,
			      gnutls_datum_t * decrypted_data)
{
	gnutls_datum_t enc = { NULL, 0 };
	uint8_t *key = NULL;
	gnutls_datum_t dkey, d_iv;
	cipher_hd_st ch;
	int ch_init = 0;
	int key_size, ret;
	unsigned int pass_len = 0;
	const struct pkcs_cipher_schema_st *p;
	unsigned block_size;
	const cipher_entry_st *ce;
	char *password;

	if (_password) {
		gnutls_datum_t pout;
		ret = _gnutls_utf8_password_normalize(_password, strlen(_password), &pout, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		password = (char*)pout.data;
		pass_len = pout.size;
	} else {
		password = NULL;
		pass_len = 0;
	}

	ret = _gnutls_x509_read_value(pkcs8_asn, root, &enc);
	if (ret < 0) {
		gnutls_assert();
		enc.data = NULL;
		goto cleanup;
	}

	if (schema == PBES1_DES_MD5) {
		ret = _gnutls_decrypt_pbes1_des_md5_data(password, pass_len,
							  kdf_params,
							  enc_params, &enc,
							  decrypted_data);
		if (ret < 0)
			goto error;
		goto cleanup;
	}

	if (kdf_params->key_size == 0) {
		key_size = gnutls_cipher_get_key_size(enc_params->cipher);
	} else
		key_size = kdf_params->key_size;

	key = gnutls_malloc(key_size);
	if (key == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto error;
	}

	/* generate the key
	 */
	p = _gnutls_pkcs_schema_get(schema);
	if (p != NULL && p->pbes2 != 0) {	/* PBES2 */
		ret = _gnutls_pbes2_string_to_key(pass_len, password,
						     kdf_params,
						     key_size, key);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else if (p != NULL) {	/* PKCS 12 schema */
		ret =
		    _gnutls_pkcs12_string_to_key(mac_to_entry(GNUTLS_MAC_SHA1),
						 1 /*KEY*/,
						 kdf_params->salt,
						 kdf_params->salt_size,
						 kdf_params->iter_count,
						 password, key_size, key);

		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	} else {
		gnutls_assert();
		ret = GNUTLS_E_UNKNOWN_CIPHER_TYPE;
		goto error;
	}

	ce = cipher_to_entry(enc_params->cipher);
	block_size = _gnutls_cipher_get_block_size(ce);

	if (ce->type == CIPHER_BLOCK) {
		if (enc.size % block_size != 0 || (unsigned)enc_params->iv_size != block_size) {
			gnutls_assert();
			ret = GNUTLS_E_DECRYPTION_FAILED;
			goto error;
		}
	} else {
		unsigned iv_size = _gnutls_cipher_get_iv_size(ce);
		if (iv_size > (unsigned)enc_params->iv_size) {
			gnutls_assert();
			ret = GNUTLS_E_DECRYPTION_FAILED;
			goto error;
		}
	}

	/* do the decryption.
	 */
	dkey.data = key;
	dkey.size = key_size;

	d_iv.data = (uint8_t *) enc_params->iv;
	d_iv.size = enc_params->iv_size;

	ret =
	    _gnutls_cipher_init(&ch, ce, &dkey, &d_iv, 0);

	gnutls_free(key);

	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ch_init = 1;

	ret = _gnutls_cipher_decrypt(&ch, enc.data, enc.size);
	if (ret < 0) {
		gnutls_assert();
		ret = GNUTLS_E_DECRYPTION_FAILED;
		goto error;
	}

	decrypted_data->data = enc.data;

	if (ce->type == CIPHER_BLOCK && block_size != 1) {
		unsigned pslen = (uint8_t)enc.data[enc.size - 1];
		unsigned i;

		if (pslen > block_size || pslen >= enc.size  || pslen == 0) {
			gnutls_assert();
			ret = GNUTLS_E_DECRYPTION_FAILED;
			goto error;
		}

		/* verify padding according to rfc2898 */
		decrypted_data->size = enc.size - pslen;
		for (i=0;i<pslen;i++) {
			if (enc.data[enc.size-1-i] != pslen) {
				gnutls_assert();
				ret = GNUTLS_E_DECRYPTION_FAILED;
				goto error;
			}
		}
	} else {
		decrypted_data->size = enc.size;
	}

	_gnutls_cipher_deinit(&ch);

	ret = 0;

 cleanup:
	gnutls_free(password);

	return ret;

 error:
	gnutls_free(password);
	gnutls_free(enc.data);
	gnutls_free(key);
	if (ch_init != 0)
		_gnutls_cipher_deinit(&ch);
	return ret;
}

/* Writes the PBKDF2 parameters.
 */
static int
write_pbkdf2_params(ASN1_TYPE pasn, const struct pbkdf2_params *kdf_params)
{
	int result;
	ASN1_TYPE pbkdf2_asn = ASN1_TYPE_EMPTY;
	uint8_t tmp[MAX_OID_SIZE];
	const mac_entry_st *me;

	/* Write the key derivation algorithm
	 */
	result =
	    asn1_write_value(pasn, "keyDerivationFunc.algorithm",
			     PBKDF2_OID, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* Now write the key derivation and the encryption
	 * functions.
	 */
	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.pkcs-5-PBKDF2-params",
				 &pbkdf2_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_write_value(pbkdf2_asn, "salt", "specified", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* write the salt 
	 */
	result =
	    asn1_write_value(pbkdf2_asn, "salt.specified",
			     kdf_params->salt, kdf_params->salt_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	_gnutls_hard_log("salt.specified.size: %d\n", kdf_params->salt_size);

	/* write the iteration count 
	 */
	_gnutls_write_uint32(kdf_params->iter_count, tmp);

	result = asn1_write_value(pbkdf2_asn, "iterationCount", tmp, 4);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	_gnutls_hard_log("iterationCount: %d\n", kdf_params->iter_count);

	/* write the keylength, if it is set.
	 */
	result = asn1_write_value(pbkdf2_asn, "keyLength", NULL, 0);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	me = _gnutls_mac_to_entry(kdf_params->mac);
	if (!me || !me->mac_oid) {
		gnutls_assert();
		result = GNUTLS_E_INTERNAL_ERROR;
		goto error;
	}

	result = asn1_write_value(pbkdf2_asn, "prf.algorithm",
				  me->mac_oid, strlen(me->mac_oid));
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = asn1_write_value(pbkdf2_asn, "prf.parameters", NULL, 0);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* now encode them an put the DER output
	 * in the keyDerivationFunc.parameters
	 */
	result = _gnutls_x509_der_encode_and_copy(pbkdf2_asn, "",
						  pasn,
						  "keyDerivationFunc.parameters",
						  0);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	result = 0;

 error:
	asn1_delete_structure(&pbkdf2_asn);
	return result;

}

static int
write_pbes2_enc_params(ASN1_TYPE pasn, const struct pbe_enc_params *params)
{
	int result;
	ASN1_TYPE pbe_asn = ASN1_TYPE_EMPTY;
	const struct pkcs_cipher_schema_st *p;
	const char *cipher_oid;

	/* Write the encryption algorithm
	 */
	p = algo_to_pbes2_cipher_schema(params->cipher);
	if (p == NULL || p->pbes2 == 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Now check the encryption parameters.
	 */
	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 p->desc, &pbe_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (p->schema == PBES2_GOST28147_89_TC26Z ||
	    p->schema == PBES2_GOST28147_89_CPA ||
	    p->schema == PBES2_GOST28147_89_CPB ||
	    p->schema == PBES2_GOST28147_89_CPC ||
	    p->schema == PBES2_GOST28147_89_CPD) {
		cipher_oid = GOST28147_89_OID;
		result = asn1_write_value(pbe_asn, "encryptionParamSet",
					  p->cipher_oid, 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto error;
		}
	} else {
		cipher_oid = p->cipher_oid;
	}

	result =
	    asn1_write_value(pasn, "encryptionScheme.algorithm",
			     cipher_oid,
			     1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		goto error;
	}
	_gnutls_hard_log("encryptionScheme.algorithm: %s\n", cipher_oid);

	/* read the salt */
	result = asn1_write_value(pbe_asn, p->iv_name,
				  params->iv, params->iv_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}
	_gnutls_hard_log("IV.size: %d\n", params->iv_size);

	/* now encode them an put the DER output
	 * in the encryptionScheme.parameters
	 */
	result = _gnutls_x509_der_encode_and_copy(pbe_asn, "",
						  pasn,
						  "encryptionScheme.parameters",
						  0);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	result = 0;

 error:
	asn1_delete_structure(&pbe_asn);
	return result;

}

/* Generates a key and also stores the key parameters.
 */
int
_gnutls_pkcs_generate_key(schema_id schema,
			  const char *_password,
			  struct pbkdf2_params *kdf_params,
			  struct pbe_enc_params *enc_params,
			  gnutls_datum_t * key)
{
	unsigned char rnd[2];
	unsigned int pass_len = 0;
	int ret;
	const struct pkcs_cipher_schema_st *p;
	char *password = NULL;

	if (_password) {
		gnutls_datum_t pout;
		ret = _gnutls_utf8_password_normalize(_password, strlen(_password), &pout, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);

		password = (char*)pout.data;
		pass_len = pout.size;
	} else {
		password = NULL;
		pass_len = 0;
	}

	ret = gnutls_rnd(GNUTLS_RND_RANDOM, rnd, 2);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* generate salt */
	kdf_params->salt_size =
	    MIN(sizeof(kdf_params->salt), (unsigned)(12 + (rnd[1] % 10)));

	p = _gnutls_pkcs_schema_get(schema);
	if (p != NULL && p->pbes2 != 0) {	/* PBES2 */
		enc_params->cipher = p->cipher;
	} else if (p != NULL) {
		/* non PBES2 algorithms */
		enc_params->cipher = p->cipher;
		kdf_params->salt_size = 8;
	} else {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	ret = gnutls_rnd(GNUTLS_RND_RANDOM, kdf_params->salt,
			  kdf_params->salt_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	kdf_params->iter_count = 5 * 1024 + rnd[0];
	key->size = kdf_params->key_size =
	    gnutls_cipher_get_key_size(enc_params->cipher);

	enc_params->iv_size = gnutls_cipher_get_iv_size(enc_params->cipher);
	key->data = gnutls_malloc(key->size);
	if (key->data == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	/* now generate the key. 
	 */

	if (p->pbes2 != 0) {
		if (p->schema == PBES2_GOST28147_89_TC26Z)
			kdf_params->mac = GNUTLS_MAC_STREEBOG_256;
		else if (p->schema == PBES2_GOST28147_89_CPA ||
			 p->schema == PBES2_GOST28147_89_CPB ||
			 p->schema == PBES2_GOST28147_89_CPC ||
			 p->schema == PBES2_GOST28147_89_CPD)
			kdf_params->mac = GNUTLS_MAC_GOSTR_94;
		else
			kdf_params->mac = GNUTLS_MAC_SHA1;
		ret = _gnutls_pbes2_string_to_key(pass_len, password,
						  kdf_params,
						  kdf_params->key_size,
						  key->data);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		if (enc_params->iv_size) {
			ret = gnutls_rnd(GNUTLS_RND_NONCE,
					  enc_params->iv, enc_params->iv_size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}
	} else {		/* PKCS 12 schema */
		ret =
		    _gnutls_pkcs12_string_to_key(mac_to_entry(GNUTLS_MAC_SHA1),
						 1 /*KEY*/,
						 kdf_params->salt,
						 kdf_params->salt_size,
						 kdf_params->iter_count,
						 password,
						 kdf_params->key_size,
						 key->data);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Now generate the IV
		 */
		if (enc_params->iv_size) {
			ret =
			    _gnutls_pkcs12_string_to_key(mac_to_entry
							 (GNUTLS_MAC_SHA1),
							 2 /*IV*/,
							 kdf_params->salt,
							 kdf_params->salt_size,
							 kdf_params->iter_count,
							 password,
							 enc_params->iv_size,
							 enc_params->iv);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}
	}

	ret = 0;

 cleanup:
	gnutls_free(password);
	return ret;
}

/* Encodes the parameters to be written in the encryptionAlgorithm.parameters
 * part.
 */
int
_gnutls_pkcs_write_schema_params(schema_id schema, ASN1_TYPE pkcs8_asn,
				 const char *where,
				 const struct pbkdf2_params *kdf_params,
				 const struct pbe_enc_params *enc_params)
{
	int result;
	ASN1_TYPE pasn = ASN1_TYPE_EMPTY;
	const struct pkcs_cipher_schema_st *p;

	p = _gnutls_pkcs_schema_get(schema);

	if (p != NULL && p->pbes2 != 0) {	/* PBES2 */
		if ((result =
		     asn1_create_element(_gnutls_get_pkix(),
					 "PKIX1.pkcs-5-PBES2-params",
					 &pasn)) != ASN1_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		result = write_pbkdf2_params(pasn, kdf_params);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		result = write_pbes2_enc_params(pasn, enc_params);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		result = _gnutls_x509_der_encode_and_copy(pasn, "",
							  pkcs8_asn, where, 0);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		asn1_delete_structure(&pasn);

	} else if (p != NULL) {	/* PKCS #12 */

		if ((result =
		     asn1_create_element(_gnutls_get_pkix(),
					 "PKIX1.pkcs-12-PbeParams",
					 &pasn)) != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto error;
		}

		result = write_pkcs12_kdf_params(pasn, kdf_params);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		result = _gnutls_x509_der_encode_and_copy(pasn, "",
							  pkcs8_asn, where, 0);
		if (result < 0) {
			gnutls_assert();
			goto error;
		}

		asn1_delete_structure(&pasn);
	}

	return 0;

 error:
	asn1_delete_structure(&pasn);
	return result;

}

int
_gnutls_pkcs_raw_encrypt_data(const gnutls_datum_t * plain,
			      const struct pbe_enc_params *enc_params,
			      const gnutls_datum_t * key, gnutls_datum_t * encrypted)
{
	int result;
	int data_size;
	uint8_t *data = NULL;
	gnutls_datum_t d_iv;
	cipher_hd_st ch;
	int ch_init = 0;
	uint8_t pad, pad_size;
	const cipher_entry_st *ce;

	ce = cipher_to_entry(enc_params->cipher);
	pad_size = _gnutls_cipher_get_block_size(ce);

	if (pad_size == 1 || ce->type == CIPHER_STREAM)	/* stream */
		pad_size = 0;

	data = gnutls_malloc(plain->size + pad_size);
	if (data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	memcpy(data, plain->data, plain->size);

	if (pad_size > 0) {
		pad = pad_size - (plain->size % pad_size);
		if (pad == 0)
			pad = pad_size;
		memset(&data[plain->size], pad, pad);
	} else
		pad = 0;

	data_size = plain->size + pad;

	d_iv.data = (uint8_t *) enc_params->iv;
	d_iv.size = enc_params->iv_size;
	result =
	    _gnutls_cipher_init(&ch, cipher_to_entry(enc_params->cipher),
				key, &d_iv, 1);

	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	ch_init = 1;

	result = _gnutls_cipher_encrypt(&ch, data, data_size);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	encrypted->data = data;
	encrypted->size = data_size;

	_gnutls_cipher_deinit(&ch);

	return 0;

 error:
	gnutls_free(data);
	if (ch_init != 0)
		_gnutls_cipher_deinit(&ch);
	return result;
}
