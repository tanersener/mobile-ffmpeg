/*
 * Copyright (C) 2016 Red Hat
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
#include <nettle/md5.h>

/* This file includes support for PKCS#8 PBES1 with DES and MD5.
 * We only support decryption for compatibility with other software.
 */

int _gnutls_read_pbkdf1_params(const uint8_t * data, int data_size,
		       struct pbkdf2_params *kdf_params,
		       struct pbe_enc_params *enc_params)
{
	ASN1_TYPE pasn = ASN1_TYPE_EMPTY;
	int len;
	int ret, result;

	memset(kdf_params, 0, sizeof(*kdf_params));
	memset(enc_params, 0, sizeof(*enc_params));

	if ((result =
		     asn1_create_element(_gnutls_get_pkix(),
					 "PKIX1.pkcs-5-PBE-params",
					 &pasn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* Decode the parameters.
	 */
	result =
	    _asn1_strict_der_decode(&pasn, data, data_size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto error;
	}

	ret =
	    _gnutls_x509_read_uint(pasn, "iterationCount",
			   &kdf_params->iter_count);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	if (kdf_params->iter_count >= MAX_ITER_COUNT || kdf_params->iter_count == 0) {
		ret = gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
		goto error;
	}

	len = sizeof(kdf_params->salt);
	result =
	    asn1_read_value(pasn, "salt",
		    kdf_params->salt, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto error;
	}

	if (len != 8) {
		gnutls_assert();
		ret = GNUTLS_E_ILLEGAL_PARAMETER;
		goto error;
	}

	enc_params->cipher = GNUTLS_CIPHER_DES_CBC;

	ret = 0;
 error:
	asn1_delete_structure2(&pasn, ASN1_DELETE_FLAG_ZEROIZE);
	return ret;

}

static void pbkdf1_md5(const char *password, unsigned password_len,
	const uint8_t salt[8], unsigned iter_count, unsigned key_size, uint8_t *key)
{
	struct md5_ctx ctx;
	uint8_t tmp[16];
	unsigned i;

	if (key_size > sizeof(tmp))
		abort();

	for (i=0;i<iter_count;i++) {
		md5_init(&ctx);
		if (i==0) {
			md5_update(&ctx, password_len, (uint8_t*)password);
			md5_update(&ctx, 8, salt);
			md5_digest(&ctx, 16, tmp);
		} else {
			md5_update(&ctx, 16, tmp);
			md5_digest(&ctx, 16, tmp);
		}
	}

	memcpy(key, tmp, key_size);
	return;
}

int
_gnutls_decrypt_pbes1_des_md5_data(const char *password,
			   unsigned password_len,
			   const struct pbkdf2_params *kdf_params,
			   const struct pbe_enc_params *enc_params,
			   const gnutls_datum_t *encrypted_data,
			   gnutls_datum_t *decrypted_data)
{
	int result;
	gnutls_datum_t dkey, d_iv;
	cipher_hd_st ch;
	uint8_t key[16];
	const unsigned block_size = 8;

	if (enc_params->cipher != GNUTLS_CIPHER_DES_CBC)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (encrypted_data->size % block_size != 0)
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	/* generate the key
	 */
	pbkdf1_md5(password, password_len, kdf_params->salt, kdf_params->iter_count, sizeof(key), key);

	dkey.data = key;
	dkey.size = 8;
	d_iv.data = &key[8];
	d_iv.size = 8;
	result =
	    _gnutls_cipher_init(&ch, cipher_to_entry(GNUTLS_CIPHER_DES_CBC),
				&dkey, &d_iv, 0);
	if (result < 0)
		return gnutls_assert_val(result);

	result = _gnutls_cipher_decrypt(&ch, encrypted_data->data, encrypted_data->size);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	if ((int)encrypted_data->size - encrypted_data->data[encrypted_data->size - 1] < 0) {
		gnutls_assert();
		result = GNUTLS_E_ILLEGAL_PARAMETER;
		goto error;
	}

	decrypted_data->data = encrypted_data->data;
	decrypted_data->size = encrypted_data->size - encrypted_data->data[encrypted_data->size - 1];

	result = 0;
 error:
	_gnutls_cipher_deinit(&ch);

	return result;
}

