/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
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

#ifndef GNUTLS_LIB_X509_PKCS7_INT_H
#define GNUTLS_LIB_X509_PKCS7_INT_H

#include <gnutls/x509.h>

/* PKCS #7
 */
#define DATA_OID "1.2.840.113549.1.7.1"
#define ENC_DATA_OID "1.2.840.113549.1.7.6"

#define SIGNED_DATA_OID "1.2.840.113549.1.7.2"
#define DIGESTED_DATA_OID "1.2.840.113549.1.7.5"


typedef enum schema_id {
	PBES2_GENERIC=1,	/* when the algorithm is unknown, temporal use when reading only */
	PBES2_DES,		/* the stuff in PKCS #5 */
	PBES2_3DES,
	PBES2_AES_128,
	PBES2_AES_192,
	PBES2_AES_256,
	PBES2_GOST28147_89_TC26Z,
	PBES2_GOST28147_89_CPA,
	PBES2_GOST28147_89_CPB,
	PBES2_GOST28147_89_CPC,
	PBES2_GOST28147_89_CPD,
	PKCS12_3DES_SHA1,	/* the stuff in PKCS #12 */
	PKCS12_ARCFOUR_SHA1,
	PKCS12_RC2_40_SHA1,
	PBES1_DES_MD5		/* openssl before 1.1.0 uses that by default */
} schema_id;

struct pkcs_cipher_schema_st {
	unsigned int schema;
	const char *name;
	unsigned int flag;
	unsigned int cipher;
	unsigned pbes2;
	const char *cipher_oid;
	const char *write_oid;
	const char *desc;
	const char *iv_name;
	unsigned decrypt_only;
};

const struct pkcs_cipher_schema_st *_gnutls_pkcs_schema_get(schema_id schema);

struct pbe_enc_params {
	gnutls_cipher_algorithm_t cipher;
	uint8_t iv[MAX_CIPHER_BLOCK_SIZE];
	int iv_size;
	char pbes2_oid[MAX_OID_SIZE]; /* when reading params, the OID is stored for info purposes */
};

int
_gnutls_decrypt_pbes1_des_md5_data(const char *password,
			   unsigned password_len,
			   const struct pbkdf2_params *kdf_params,
			   const struct pbe_enc_params *enc_params,
			   const gnutls_datum_t *encrypted_data,
			   gnutls_datum_t *decrypted_data);

int _gnutls_check_pkcs_cipher_schema(const char *oid);

int
_gnutls_pkcs_raw_decrypt_data(schema_id schema, ASN1_TYPE pkcs8_asn,
	     const char *root, const char *password,
	     const struct pbkdf2_params *kdf_params,
	     const struct pbe_enc_params *enc_params,
	     gnutls_datum_t *decrypted_data);

int
_gnutls_pkcs_raw_encrypt_data(const gnutls_datum_t * plain,
	     const struct pbe_enc_params *enc_params,
	     const gnutls_datum_t * key, gnutls_datum_t * encrypted);

int _gnutls_pkcs7_decrypt_data(const gnutls_datum_t * data,
			       const char *password, gnutls_datum_t * dec);

int _gnutls_read_pbkdf1_params(const uint8_t * data, int data_size,
		       struct pbkdf2_params *kdf_params,
		       struct pbe_enc_params *enc_params);

int
_gnutls_read_pkcs_schema_params(schema_id * schema, const char *password,
			const uint8_t * data, int data_size,
			struct pbkdf2_params *kdf_params,
			struct pbe_enc_params *enc_params);

int
_gnutls_pkcs_write_schema_params(schema_id schema, ASN1_TYPE pkcs8_asn,
		    const char *where,
		    const struct pbkdf2_params *kdf_params,
		    const struct pbe_enc_params *enc_params);

int
_gnutls_pkcs_generate_key(schema_id schema,
	     const char *password,
	     struct pbkdf2_params *kdf_params,
	     struct pbe_enc_params *enc_params, gnutls_datum_t * key);

int _gnutls_pkcs_flags_to_schema(unsigned int flags);
int _gnutls_pkcs7_encrypt_data(schema_id schema,
			       const gnutls_datum_t * data,
			       const char *password, gnutls_datum_t * enc);

int
_gnutls_pkcs7_data_enc_info(const gnutls_datum_t * data, const struct pkcs_cipher_schema_st **p,
	struct pbkdf2_params *kdf_params, char **oid);

#endif /* GNUTLS_LIB_X509_PKCS7_INT_H */
