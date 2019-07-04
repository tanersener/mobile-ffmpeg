/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
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

#include "gnutls_int.h"
#include "errors.h"
#include <cipher_int.h>
#include <datum.h>
#include <gnutls/crypto.h>
#include <algorithms.h>
#include <random.h>
#include <crypto.h>
#include <fips.h>
#include "crypto-api.h"

typedef struct api_cipher_hd_st {
	cipher_hd_st ctx_enc;
	cipher_hd_st ctx_dec;
} api_cipher_hd_st;

/**
 * gnutls_cipher_init:
 * @handle: is a #gnutls_cipher_hd_t type
 * @cipher: the encryption algorithm to use
 * @key: the key to be used for encryption/decryption
 * @iv: the IV to use (if not applicable set NULL)
 *
 * This function will initialize the @handle context to be usable
 * for encryption/decryption of data. This will effectively use the
 * current crypto backend in use by gnutls or the cryptographic
 * accelerator in use.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int
gnutls_cipher_init(gnutls_cipher_hd_t * handle,
		   gnutls_cipher_algorithm_t cipher,
		   const gnutls_datum_t * key, const gnutls_datum_t * iv)
{
	api_cipher_hd_st *h;
	int ret;
	const cipher_entry_st* e;

	if (is_cipher_algo_forbidden(cipher))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	e = cipher_to_entry(cipher);
	if (e == NULL || e->only_aead)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	*handle = gnutls_calloc(1, sizeof(api_cipher_hd_st));
	if (*handle == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	h = *handle;
	ret =
	    _gnutls_cipher_init(&h->ctx_enc, e, key,
				iv, 1);

	if (ret >= 0 && _gnutls_cipher_type(e) == CIPHER_BLOCK)
		ret =
		    _gnutls_cipher_init(&h->ctx_dec, e, key, iv, 0);

	return ret;
}

/**
 * gnutls_cipher_tag:
 * @handle: is a #gnutls_cipher_hd_t type
 * @tag: will hold the tag
 * @tag_size: the length of the tag to return
 *
 * This function operates on authenticated encryption with
 * associated data (AEAD) ciphers and will return the
 * output tag.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 3.0
 **/
int
gnutls_cipher_tag(gnutls_cipher_hd_t handle, void *tag, size_t tag_size)
{
	api_cipher_hd_st *h = handle;

	if (_gnutls_cipher_is_aead(&h->ctx_enc) == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	_gnutls_cipher_tag(&h->ctx_enc, tag, tag_size);

	return 0;
}

/**
 * gnutls_cipher_add_auth:
 * @handle: is a #gnutls_cipher_hd_t type
 * @ptext: the data to be authenticated
 * @ptext_size: the length of the data
 *
 * This function operates on authenticated encryption with
 * associated data (AEAD) ciphers and authenticate the
 * input data. This function can only be called once
 * and before any encryption operations.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 3.0
 **/
int
gnutls_cipher_add_auth(gnutls_cipher_hd_t handle, const void *ptext,
		       size_t ptext_size)
{
	api_cipher_hd_st *h = handle;

	if (_gnutls_cipher_is_aead(&h->ctx_enc) == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	return _gnutls_cipher_auth(&h->ctx_enc, ptext, ptext_size);
}

/**
 * gnutls_cipher_set_iv:
 * @handle: is a #gnutls_cipher_hd_t type
 * @iv: the IV to set
 * @ivlen: the length of the IV
 *
 * This function will set the IV to be used for the next
 * encryption block.
 *
 * Since: 3.0
 **/
void
gnutls_cipher_set_iv(gnutls_cipher_hd_t handle, void *iv, size_t ivlen)
{
	api_cipher_hd_st *h = handle;

	if (_gnutls_cipher_setiv(&h->ctx_enc, iv, ivlen) < 0) {
		_gnutls_switch_lib_state(LIB_STATE_ERROR);
	}

	if (_gnutls_cipher_type(h->ctx_enc.e) == CIPHER_BLOCK)
		if (_gnutls_cipher_setiv(&h->ctx_dec, iv, ivlen) < 0) {
			_gnutls_switch_lib_state(LIB_STATE_ERROR);
		}
}

/*-
 * _gnutls_cipher_get_iv:
 * @handle: is a #gnutls_cipher_hd_t type
 * @iv: the IV to set
 * @ivlen: the length of the IV
 *
 * This function will retrieve the internally calculated IV value. It is
 * intended to be used  for modes like CFB. @iv must have @ivlen length
 * at least.
 *
 * This is solely for validation purposes of our crypto
 * implementation.  For other purposes, the IV can be typically
 * calculated from the initial IV value and the subsequent ciphertext
 * values.  As such, this function only works with the internally
 * registered ciphers.
 *
 * Returns: The length of IV or a negative error code on error.
 *
 * Since: 3.6.8
 -*/
int
_gnutls_cipher_get_iv(gnutls_cipher_hd_t handle, void *iv, size_t ivlen)
{
	api_cipher_hd_st *h = handle;

	return _gnutls_cipher_getiv(&h->ctx_enc, iv, ivlen);
}

/**
 * gnutls_cipher_encrypt:
 * @handle: is a #gnutls_cipher_hd_t type
 * @ptext: the data to encrypt
 * @ptext_len: the length of data to encrypt
 *
 * This function will encrypt the given data using the algorithm
 * specified by the context.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int
gnutls_cipher_encrypt(gnutls_cipher_hd_t handle, void *ptext,
		      size_t ptext_len)
{
	api_cipher_hd_st *h = handle;

	return _gnutls_cipher_encrypt(&h->ctx_enc, ptext, ptext_len);
}

/**
 * gnutls_cipher_decrypt:
 * @handle: is a #gnutls_cipher_hd_t type
 * @ctext: the data to decrypt
 * @ctext_len: the length of data to decrypt
 *
 * This function will decrypt the given data using the algorithm
 * specified by the context.
 *
 * Note that in AEAD ciphers, this will not check the tag. You will
 * need to compare the tag sent with the value returned from gnutls_cipher_tag().
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int
gnutls_cipher_decrypt(gnutls_cipher_hd_t handle, void *ctext,
		      size_t ctext_len)
{
	api_cipher_hd_st *h = handle;

	if (_gnutls_cipher_type(h->ctx_enc.e) != CIPHER_BLOCK)
		return _gnutls_cipher_decrypt(&h->ctx_enc, ctext,
					      ctext_len);
	else
		return _gnutls_cipher_decrypt(&h->ctx_dec, ctext,
					      ctext_len);
}

/**
 * gnutls_cipher_encrypt2:
 * @handle: is a #gnutls_cipher_hd_t type
 * @ptext: the data to encrypt
 * @ptext_len: the length of data to encrypt
 * @ctext: the encrypted data
 * @ctext_len: the available length for encrypted data
 *
 * This function will encrypt the given data using the algorithm
 * specified by the context. For block ciphers the @ptext_len must be
 * a multiple of the block size. For the supported ciphers the encrypted
 * data length will equal the plaintext size.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.12.0
 **/
int
gnutls_cipher_encrypt2(gnutls_cipher_hd_t handle, const void *ptext,
		       size_t ptext_len, void *ctext,
		       size_t ctext_len)
{
	api_cipher_hd_st *h = handle;

	return _gnutls_cipher_encrypt2(&h->ctx_enc, ptext, ptext_len,
				       ctext, ctext_len);
}

/**
 * gnutls_cipher_decrypt2:
 * @handle: is a #gnutls_cipher_hd_t type
 * @ctext: the data to decrypt
 * @ctext_len: the length of data to decrypt
 * @ptext: the decrypted data
 * @ptext_len: the available length for decrypted data
 *
 * This function will decrypt the given data using the algorithm
 * specified by the context. For block ciphers the @ctext_len must be
 * a multiple of the block size. For the supported ciphers the plaintext
 * data length will equal the ciphertext size.
 *
 * Note that in AEAD ciphers, this will not check the tag. You will
 * need to compare the tag sent with the value returned from gnutls_cipher_tag().
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.12.0
 **/
int
gnutls_cipher_decrypt2(gnutls_cipher_hd_t handle, const void *ctext,
		       size_t ctext_len, void *ptext, size_t ptext_len)
{
	api_cipher_hd_st *h = handle;

	if (_gnutls_cipher_type(h->ctx_enc.e) != CIPHER_BLOCK)
		return _gnutls_cipher_decrypt2(&h->ctx_enc, ctext,
					       ctext_len, ptext,
					       ptext_len);
	else
		return _gnutls_cipher_decrypt2(&h->ctx_dec, ctext,
					       ctext_len, ptext,
					       ptext_len);
}

/**
 * gnutls_cipher_deinit:
 * @handle: is a #gnutls_cipher_hd_t type
 *
 * This function will deinitialize all resources occupied by the given
 * encryption context.
 *
 * Since: 2.10.0
 **/
void gnutls_cipher_deinit(gnutls_cipher_hd_t handle)
{
	api_cipher_hd_st *h = handle;

	_gnutls_cipher_deinit(&h->ctx_enc);
	if (_gnutls_cipher_type(h->ctx_enc.e) == CIPHER_BLOCK)
		_gnutls_cipher_deinit(&h->ctx_dec);
	gnutls_free(handle);
}


/* HMAC */


/**
 * gnutls_hmac_init:
 * @dig: is a #gnutls_hmac_hd_t type
 * @algorithm: the HMAC algorithm to use
 * @key: the key to be used for encryption
 * @keylen: the length of the key
 *
 * This function will initialize an context that can be used to
 * produce a Message Authentication Code (MAC) of data.  This will
 * effectively use the current crypto backend in use by gnutls or the
 * cryptographic accelerator in use.
 *
 * Note that despite the name of this function, it can be used
 * for other MAC algorithms than HMAC.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int
gnutls_hmac_init(gnutls_hmac_hd_t * dig,
		 gnutls_mac_algorithm_t algorithm,
		 const void *key, size_t keylen)
{
	/* MD5 is only allowed internally for TLS */
	if (is_mac_algo_forbidden(algorithm))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	*dig = gnutls_malloc(sizeof(mac_hd_st));
	if (*dig == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return _gnutls_mac_init(((mac_hd_st *) * dig),
				mac_to_entry(algorithm), key, keylen);
}

/**
 * gnutls_hmac_set_nonce:
 * @handle: is a #gnutls_hmac_hd_t type
 * @nonce: the data to set as nonce
 * @nonce_len: the length of data
 *
 * This function will set the nonce in the MAC algorithm.
 *
 * Since: 3.2.0
 **/
void
gnutls_hmac_set_nonce(gnutls_hmac_hd_t handle, const void *nonce,
		      size_t nonce_len)
{
	_gnutls_mac_set_nonce((mac_hd_st *) handle, nonce, nonce_len);
}

/**
 * gnutls_hmac:
 * @handle: is a #gnutls_hmac_hd_t type
 * @ptext: the data to hash
 * @ptext_len: the length of data to hash
 *
 * This function will hash the given data using the algorithm
 * specified by the context.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int gnutls_hmac(gnutls_hmac_hd_t handle, const void *ptext, size_t ptext_len)
{
	return _gnutls_mac((mac_hd_st *) handle, ptext, ptext_len);
}

/**
 * gnutls_hmac_output:
 * @handle: is a #gnutls_hmac_hd_t type
 * @digest: is the output value of the MAC
 *
 * This function will output the current MAC value
 * and reset the state of the MAC.
 *
 * Since: 2.10.0
 **/
void gnutls_hmac_output(gnutls_hmac_hd_t handle, void *digest)
{
	_gnutls_mac_output((mac_hd_st *) handle, digest);
}

/**
 * gnutls_hmac_deinit:
 * @handle: is a #gnutls_hmac_hd_t type
 * @digest: is the output value of the MAC
 *
 * This function will deinitialize all resources occupied by
 * the given hmac context.
 *
 * Since: 2.10.0
 **/
void gnutls_hmac_deinit(gnutls_hmac_hd_t handle, void *digest)
{
	_gnutls_mac_deinit((mac_hd_st *) handle, digest);
	gnutls_free(handle);
}

/**
 * gnutls_hmac_get_len:
 * @algorithm: the hmac algorithm to use
 *
 * This function will return the length of the output data
 * of the given hmac algorithm.
 *
 * Returns: The length or zero on error.
 *
 * Since: 2.10.0
 **/
unsigned gnutls_hmac_get_len(gnutls_mac_algorithm_t algorithm)
{
	return _gnutls_mac_get_algo_len(mac_to_entry(algorithm));
}

/**
 * gnutls_hmac_fast:
 * @algorithm: the hash algorithm to use
 * @key: the key to use
 * @keylen: the length of the key
 * @ptext: the data to hash
 * @ptext_len: the length of data to hash
 * @digest: is the output value of the hash
 *
 * This convenience function will hash the given data and return output
 * on a single call.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int
gnutls_hmac_fast(gnutls_mac_algorithm_t algorithm,
		 const void *key, size_t keylen,
		 const void *ptext, size_t ptext_len, void *digest)
{
	if (is_mac_algo_forbidden(algorithm))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	return _gnutls_mac_fast(algorithm, key, keylen, ptext, ptext_len,
				digest);
}

/* HASH */

/**
 * gnutls_hash_init:
 * @dig: is a #gnutls_hash_hd_t type
 * @algorithm: the hash algorithm to use
 *
 * This function will initialize an context that can be used to
 * produce a Message Digest of data.  This will effectively use the
 * current crypto backend in use by gnutls or the cryptographic
 * accelerator in use.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int
gnutls_hash_init(gnutls_hash_hd_t * dig,
		 gnutls_digest_algorithm_t algorithm)
{
	if (is_mac_algo_forbidden(algorithm))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	*dig = gnutls_malloc(sizeof(digest_hd_st));
	if (*dig == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return _gnutls_hash_init(((digest_hd_st *) * dig),
				 hash_to_entry(algorithm));
}

/**
 * gnutls_hash:
 * @handle: is a #gnutls_hash_hd_t type
 * @ptext: the data to hash
 * @ptext_len: the length of data to hash
 *
 * This function will hash the given data using the algorithm
 * specified by the context.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int gnutls_hash(gnutls_hash_hd_t handle, const void *ptext, size_t ptext_len)
{
	return _gnutls_hash((digest_hd_st *) handle, ptext, ptext_len);
}

/**
 * gnutls_hash_output:
 * @handle: is a #gnutls_hash_hd_t type
 * @digest: is the output value of the hash
 *
 * This function will output the current hash value
 * and reset the state of the hash.
 *
 * Since: 2.10.0
 **/
void gnutls_hash_output(gnutls_hash_hd_t handle, void *digest)
{
	_gnutls_hash_output((digest_hd_st *) handle, digest);
}

/**
 * gnutls_hash_deinit:
 * @handle: is a #gnutls_hash_hd_t type
 * @digest: is the output value of the hash
 *
 * This function will deinitialize all resources occupied by
 * the given hash context.
 *
 * Since: 2.10.0
 **/
void gnutls_hash_deinit(gnutls_hash_hd_t handle, void *digest)
{
	_gnutls_hash_deinit((digest_hd_st *) handle, digest);
	gnutls_free(handle);
}

/**
 * gnutls_hash_get_len:
 * @algorithm: the hash algorithm to use
 *
 * This function will return the length of the output data
 * of the given hash algorithm.
 *
 * Returns: The length or zero on error.
 *
 * Since: 2.10.0
 **/
unsigned gnutls_hash_get_len(gnutls_digest_algorithm_t algorithm)
{
	return _gnutls_hash_get_algo_len(hash_to_entry(algorithm));
}

/**
 * gnutls_hash_fast:
 * @algorithm: the hash algorithm to use
 * @ptext: the data to hash
 * @ptext_len: the length of data to hash
 * @digest: is the output value of the hash
 *
 * This convenience function will hash the given data and return output
 * on a single call.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 2.10.0
 **/
int
gnutls_hash_fast(gnutls_digest_algorithm_t algorithm,
		 const void *ptext, size_t ptext_len, void *digest)
{
	if (is_mac_algo_forbidden(algorithm))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	return _gnutls_hash_fast(algorithm, ptext, ptext_len, digest);
}

/**
 * gnutls_key_generate:
 * @key: is a pointer to a #gnutls_datum_t which will contain a newly
 * created key
 * @key_size: the number of bytes of the key
 *
 * Generates a random key of @key_size bytes.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 * error code.
 *
 * Since: 3.0
 **/
int gnutls_key_generate(gnutls_datum_t * key, unsigned int key_size)
{
	int ret;

	FAIL_IF_LIB_ERROR;

#ifdef ENABLE_FIPS140
	/* The FIPS140 approved RNGs are not allowed to be used
	 * to extract key sizes longer than their original seed.
	 */
	if (_gnutls_fips_mode_enabled() != 0 &&
	    key_size > FIPS140_RND_KEY_SIZE)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
#endif

	key->size = key_size;
	key->data = gnutls_malloc(key->size);
	if (!key->data) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = gnutls_rnd(GNUTLS_RND_RANDOM, key->data, key->size);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(key);
		return ret;
	}

	return 0;
}

/* AEAD API */

/**
 * gnutls_aead_cipher_init:
 * @handle: is a #gnutls_aead_cipher_hd_t type.
 * @cipher: the authenticated-encryption algorithm to use
 * @key: The key to be used for encryption
 *
 * This function will initialize an context that can be used for
 * encryption/decryption of data. This will effectively use the
 * current crypto backend in use by gnutls or the cryptographic
 * accelerator in use.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 3.4.0
 **/
int gnutls_aead_cipher_init(gnutls_aead_cipher_hd_t *handle,
			    gnutls_cipher_algorithm_t cipher,
			    const gnutls_datum_t *key)
{
	api_aead_cipher_hd_st *h;
	const cipher_entry_st *e;

	if (is_cipher_algo_forbidden(cipher))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	e = cipher_to_entry(cipher);
	if (e == NULL || e->type != CIPHER_AEAD)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	*handle = gnutls_calloc(1, sizeof(api_aead_cipher_hd_st));
	if (*handle == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	h = *handle;

	return _gnutls_aead_cipher_init(h, cipher, key);
}

/**
 * gnutls_aead_cipher_decrypt:
 * @handle: is a #gnutls_aead_cipher_hd_t type.
 * @nonce: the nonce to set
 * @nonce_len: The length of the nonce
 * @auth: additional data to be authenticated
 * @auth_len: The length of the data
 * @tag_size: The size of the tag to use (use zero for the default)
 * @ctext: the data to decrypt (including the authentication tag)
 * @ctext_len: the length of data to decrypt (includes tag size)
 * @ptext: the decrypted data
 * @ptext_len: the length of decrypted data (initially must hold the maximum available size)
 *
 * This function will decrypt the given data using the algorithm
 * specified by the context. This function must be provided the complete
 * data to be decrypted, including the authentication tag. On several
 * AEAD ciphers, the authentication tag is appended to the ciphertext,
 * though this is not a general rule. This function will fail if
 * the tag verification fails.
 *
 * Returns: Zero or a negative error code on verification failure or other error.
 *
 * Since: 3.4.0
 **/
int
gnutls_aead_cipher_decrypt(gnutls_aead_cipher_hd_t handle,
			   const void *nonce, size_t nonce_len,
			   const void *auth, size_t auth_len,
			   size_t tag_size,
			   const void *ctext, size_t ctext_len,
			   void *ptext, size_t *ptext_len)
{
	int ret;
	api_aead_cipher_hd_st *h = handle;

	if (tag_size == 0)
		tag_size = _gnutls_cipher_get_tag_size(h->ctx_enc.e);
	else if (tag_size > (unsigned)_gnutls_cipher_get_tag_size(h->ctx_enc.e))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (unlikely(ctext_len < tag_size))
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	ret = _gnutls_aead_cipher_decrypt(&h->ctx_enc,
					  nonce, nonce_len,
					  auth, auth_len,
					  tag_size,
					  ctext, ctext_len,
					  ptext, *ptext_len);
	if (unlikely(ret < 0))
		return gnutls_assert_val(ret);

	/* That assumes that AEAD ciphers are stream */
	*ptext_len = ctext_len - tag_size;

	return 0;
}


/**
 * gnutls_aead_cipher_encrypt:
 * @handle: is a #gnutls_aead_cipher_hd_t type.
 * @nonce: the nonce to set
 * @nonce_len: The length of the nonce
 * @auth: additional data to be authenticated
 * @auth_len: The length of the data
 * @tag_size: The size of the tag to use (use zero for the default)
 * @ptext: the data to encrypt
 * @ptext_len: The length of data to encrypt
 * @ctext: the encrypted data including authentication tag
 * @ctext_len: the length of encrypted data (initially must hold the maximum available size, including space for tag)
 *
 * This function will encrypt the given data using the algorithm
 * specified by the context. The output data will contain the
 * authentication tag.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 3.4.0
 **/
int
gnutls_aead_cipher_encrypt(gnutls_aead_cipher_hd_t handle,
			   const void *nonce, size_t nonce_len,
			   const void *auth, size_t auth_len,
			   size_t tag_size,
			   const void *ptext, size_t ptext_len,
			   void *ctext, size_t *ctext_len)
{
	api_aead_cipher_hd_st *h = handle;
	int ret;

	if (tag_size == 0)
		tag_size = _gnutls_cipher_get_tag_size(h->ctx_enc.e);
	else if (tag_size > (unsigned)_gnutls_cipher_get_tag_size(h->ctx_enc.e))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (unlikely(*ctext_len < ptext_len + tag_size))
		return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);

	ret = _gnutls_aead_cipher_encrypt(&h->ctx_enc,
					  nonce, nonce_len,
					  auth, auth_len,
					  tag_size,
					  ptext, ptext_len,
					  ctext, *ctext_len);
	if (unlikely(ret < 0))
		return gnutls_assert_val(ret);

	/* That assumes that AEAD ciphers are stream */
	*ctext_len = ptext_len + tag_size;

	return 0;
}

struct iov_store_st {
	void *data;
	size_t size;
	unsigned allocated;
};

static void iov_store_free(struct iov_store_st *s)
{
	if (s->allocated) {
		gnutls_free(s->data);
		s->allocated = 0;
	}
}

static int copy_iov(struct iov_store_st *dst, const giovec_t *iov, int iovcnt)
{
	memset(dst, 0, sizeof(*dst));
	if (iovcnt == 0) {
		return 0;
	} else if (iovcnt == 1) {
		dst->data = iov[0].iov_base;
		dst->size = iov[0].iov_len;
		/* implies: dst->allocated = 0; */
		return 0;
	} else {
		int i;
		uint8_t *p;

		dst->size = 0;
		for (i=0;i<iovcnt;i++)
			dst->size += iov[i].iov_len;
		dst->data = gnutls_malloc(dst->size);
		if (dst->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		p = dst->data;
		for (i=0;i<iovcnt;i++) {
			memcpy(p, iov[i].iov_base, iov[i].iov_len);
			p += iov[i].iov_len;
		}

		dst->allocated = 1;
		return 0;
	}
}

#define AUTH_UPDATE_FINAL(ctx) do { \
	if (index) { \
		ret = _gnutls_cipher_auth(ctx, cache, index); \
		if (unlikely(ret < 0)) \
			return gnutls_assert_val(ret); \
	} \
	} while(0)

#define AUTH_UPDATE(ctx, data, length) do { \
	if (index) { \
		ssize_t left = blocksize - index; \
		if (length < left) { \
			memcpy(cache+index, data, \
			       length); \
			index += length; \
			goto __update_done; \
		} else { \
			memcpy(cache+index, data, left); \
			ret = _gnutls_cipher_auth(ctx, cache, blocksize); \
			if (unlikely(ret < 0)) \
				return gnutls_assert_val(ret); \
			data += left; \
			length -= left; \
		} \
	} \
	if (length >= blocksize) { \
		ssize_t to_proc = (length/blocksize)*blocksize; \
		ret = _gnutls_cipher_auth(ctx, data, to_proc); \
		if (unlikely(ret < 0)) \
			return gnutls_assert_val(ret); \
		data += to_proc; \
		length -= to_proc; \
	} \
	if (length) \
		memcpy(cache, data, length); \
	index = length; \
 __update_done: \
	; \
	} while(0)

#define ENCRYPT_FINAL(ctx, dst, dst_size) do { \
	if (index) { \
		if (unlikely(dst_size < (ssize_t)index)) \
			return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER); \
		ret = _gnutls_cipher_encrypt2(ctx, cache, index, dst, dst_size); \
		if (unlikely(ret < 0)) \
			return gnutls_assert_val(ret); \
		dst += index; \
		dst_size -= index; \
	} \
	} while(0)

#define ENCRYPT(ctx, data, length, dst, dst_size) do { \
	if (index) { \
		ssize_t left = blocksize - index; \
		if (length < left) { \
			memcpy(cache+index, data, \
			       length); \
			index += length; \
			goto __encrypt_done; \
		} else { \
			if (unlikely(dst_size < blocksize)) \
				return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER); \
			memcpy(cache+index, data, left); \
			ret = _gnutls_cipher_encrypt2(ctx, cache, blocksize, dst, dst_size); \
			if (unlikely(ret < 0)) \
				return gnutls_assert_val(ret); \
			data += left; \
			length -= left; \
			dst += blocksize; \
			dst_size -= blocksize; \
		} \
	} \
	if (length >= blocksize) { \
		ssize_t to_proc = (length/blocksize)*blocksize; \
		if (unlikely(dst_size < to_proc)) \
			return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER); \
		ret = _gnutls_cipher_encrypt2(ctx, data, to_proc, dst, dst_size); \
		if (unlikely(ret < 0)) \
			return gnutls_assert_val(ret); \
		data += to_proc; \
		length -= to_proc; \
		dst += to_proc; \
		dst_size -= to_proc; \
	} \
	if (length) \
		memcpy(cache, data, length); \
	index = length; \
 __encrypt_done: \
	; \
	} while(0)


/**
 * gnutls_aead_cipher_encryptv:
 * @handle: is a #gnutls_aead_cipher_hd_t type.
 * @nonce: the nonce to set
 * @nonce_len: The length of the nonce
 * @auth_iov: additional data to be authenticated
 * @auth_iovcnt: The number of buffers in @auth_iov
 * @tag_size: The size of the tag to use (use zero for the default)
 * @iov: the data to be encrypted
 * @iovcnt: The number of buffers in @iov
 * @ctext: the encrypted data including authentication tag
 * @ctext_len: the length of encrypted data (initially must hold the maximum available size, including space for tag)
 *
 * This function will encrypt the provided data buffers using the algorithm
 * specified by the context. The output data will contain the
 * authentication tag.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 3.6.3
 **/
int
gnutls_aead_cipher_encryptv(gnutls_aead_cipher_hd_t handle,
			    const void *nonce, size_t nonce_len,
			    const giovec_t *auth_iov, int auth_iovcnt,
			    size_t tag_size,
			    const giovec_t *iov, int iovcnt,
			    void *ctext, size_t *ctext_len)
{
	api_aead_cipher_hd_st *h = handle;
	int ret;
	uint8_t *dst;
	ssize_t dst_size, total = 0, len;
	uint8_t *p;
	unsigned i;
	uint8_t cache[MAX_CIPHER_BLOCK_SIZE];
	unsigned index;
	ssize_t blocksize = handle->ctx_enc.e->blocksize;

	/* Limitation: this function provides an optimization under the internally registered
	 * AEAD ciphers. When an AEAD cipher is used registered with gnutls_crypto_register_aead_cipher(),
	 * then this becomes a convenience function as it missed the lower-level primitives
	 * necessary for piecemeal encryption. */

	if (tag_size == 0)
		tag_size = _gnutls_cipher_get_tag_size(h->ctx_enc.e);
	else if (tag_size > (unsigned)_gnutls_cipher_get_tag_size(h->ctx_enc.e))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (handle->ctx_enc.e->only_aead || handle->ctx_enc.encrypt == NULL) {
		/* ciphertext cannot be produced in a piecemeal approach */
		struct iov_store_st auth;
		struct iov_store_st ptext;

		ret = copy_iov(&auth, auth_iov, auth_iovcnt);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = copy_iov(&ptext, iov, iovcnt);
		if (ret < 0) {
			iov_store_free(&auth);
			return gnutls_assert_val(ret);
		}

		ret = gnutls_aead_cipher_encrypt(handle, nonce, nonce_len,
						 auth.data, auth.size,
						 tag_size,
						 ptext.data, ptext.size,
						 ctext, ctext_len);
		iov_store_free(&auth);
		iov_store_free(&ptext);

		return ret;
	}

	ret = _gnutls_cipher_setiv(&handle->ctx_enc, nonce, nonce_len);
	if (unlikely(ret < 0))
		return gnutls_assert_val(ret);

	index = 0;
	for (i = 0; i < (unsigned)auth_iovcnt; i++) {
		p = auth_iov[i].iov_base;
		len = auth_iov[i].iov_len;
		AUTH_UPDATE(&handle->ctx_enc, p, len);
	}
	AUTH_UPDATE_FINAL(&handle->ctx_enc);

	dst = ctext;
	dst_size = *ctext_len;

	index = 0;
	for (i = 0; i < (unsigned)iovcnt; i++) {
		p = iov[i].iov_base;
		len = iov[i].iov_len;
		ENCRYPT(&handle->ctx_enc, p, len, dst, dst_size);
		total += iov[i].iov_len;
	}
	ENCRYPT_FINAL(&handle->ctx_enc, dst, dst_size);

	if ((size_t)dst_size < tag_size)
		return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);

	_gnutls_cipher_tag(&handle->ctx_enc, dst, tag_size);

	total += tag_size;
	*ctext_len = total;

	return 0;
}

/**
 * gnutls_aead_cipher_deinit:
 * @handle: is a #gnutls_aead_cipher_hd_t type.
 *
 * This function will deinitialize all resources occupied by the given
 * authenticated-encryption context.
 *
 * Since: 3.4.0
 **/
void gnutls_aead_cipher_deinit(gnutls_aead_cipher_hd_t handle)
{
	_gnutls_aead_cipher_deinit(handle);
	gnutls_free(handle);
}
