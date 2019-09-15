/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

/* This file handles all the internal functions that cope with hashes
 * and HMACs.
 */

#include "gnutls_int.h"
#include <hash_int.h>
#include "errors.h"
#include <algorithms.h>
#include <fips.h>

int _gnutls_hash_init(digest_hd_st * dig, const mac_entry_st * e)
{
	int result;
	const gnutls_crypto_digest_st *cc = NULL;

	FAIL_IF_LIB_ERROR;

	if (unlikely(e == NULL || e->id == GNUTLS_MAC_NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	dig->e = e;

	/* check if a digest has been registered 
	 */
	cc = _gnutls_get_crypto_digest((gnutls_digest_algorithm_t)e->id);
	if (cc != NULL && cc->init) {
		if (cc->init((gnutls_digest_algorithm_t)e->id, &dig->handle) < 0) {
			gnutls_assert();
			return GNUTLS_E_HASH_FAILED;
		}

		dig->hash = cc->hash;
		dig->output = cc->output;
		dig->deinit = cc->deinit;
		dig->copy = cc->copy;

		return 0;
	}

	result = _gnutls_digest_ops.init((gnutls_digest_algorithm_t)e->id, &dig->handle);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	dig->hash = _gnutls_digest_ops.hash;
	dig->output = _gnutls_digest_ops.output;
	dig->deinit = _gnutls_digest_ops.deinit;
	dig->copy = _gnutls_digest_ops.copy;

	return 0;
}

/* Returns true(non-zero) or false(0) if the 
 * provided hash exists
 */
int _gnutls_digest_exists(gnutls_digest_algorithm_t algo)
{
	const gnutls_crypto_digest_st *cc = NULL;

	if (is_mac_algo_forbidden(algo))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	cc = _gnutls_get_crypto_digest(algo);
	if (cc != NULL)
		return 1;

	return _gnutls_digest_ops.exists(algo);
}

int _gnutls_hash_copy(const digest_hd_st * handle, digest_hd_st * dst)
{
	if (handle->copy == NULL)
		return gnutls_assert_val(GNUTLS_E_HASH_FAILED);

	*dst = *handle; /* copy data */
	dst->handle = handle->copy(handle->handle);

	if (dst->handle == NULL)
		return GNUTLS_E_HASH_FAILED;

	return 0;
}

void _gnutls_hash_deinit(digest_hd_st * handle, void *digest)
{
	if (handle->handle == NULL) {
		return;
	}

	if (digest != NULL)
		_gnutls_hash_output(handle, digest);

	handle->deinit(handle->handle);
	handle->handle = NULL;
}

int
_gnutls_hash_fast(gnutls_digest_algorithm_t algorithm,
		  const void *text, size_t textlen, void *digest)
{
	int ret;
	const gnutls_crypto_digest_st *cc = NULL;
	
	FAIL_IF_LIB_ERROR;

	/* check if a digest has been registered 
	 */
	cc = _gnutls_get_crypto_digest(algorithm);
	if (cc != NULL) {
		if (cc->fast(algorithm, text, textlen, digest) < 0) {
			gnutls_assert();
			return GNUTLS_E_HASH_FAILED;
		}

		return 0;
	}

	ret = _gnutls_digest_ops.fast(algorithm, text, textlen, digest);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}


/* HMAC interface */

int
_gnutls_mac_fast(gnutls_mac_algorithm_t algorithm, const void *key,
		 int keylen, const void *text, size_t textlen,
		 void *digest)
{
	int ret;
	const gnutls_crypto_mac_st *cc = NULL;

	FAIL_IF_LIB_ERROR;

	/* check if a digest has been registered 
	 */
	cc = _gnutls_get_crypto_mac(algorithm);
	if (cc != NULL) {
		if (cc->
		    fast(algorithm, NULL, 0, key, keylen, text, textlen,
			 digest) < 0) {
			gnutls_assert();
			return GNUTLS_E_HASH_FAILED;
		}

		return 0;
	}

	ret =
	    _gnutls_mac_ops.fast(algorithm, NULL, 0, key, keylen, text,
				 textlen, digest);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;

}

/* Returns true(non-zero) or false(0) if the 
 * provided hash exists
 */
int _gnutls_mac_exists(gnutls_mac_algorithm_t algo)
{
	const gnutls_crypto_mac_st *cc = NULL;

	/* exceptionally it exists, as it is not a real MAC */
	if (algo == GNUTLS_MAC_AEAD)
		return 1;

	if (is_mac_algo_forbidden(algo))
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	cc = _gnutls_get_crypto_mac(algo);
	if (cc != NULL)
		return 1;

	return _gnutls_mac_ops.exists(algo);
}

int
_gnutls_mac_init(mac_hd_st * mac, const mac_entry_st * e,
		 const void *key, int keylen)
{
	int result;
	const gnutls_crypto_mac_st *cc = NULL;

	FAIL_IF_LIB_ERROR;

	if (unlikely(e == NULL || e->id == GNUTLS_MAC_NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	mac->e = e;
	mac->mac_len = _gnutls_mac_get_algo_len(e);

	/* check if a digest has been registered 
	 */
	cc = _gnutls_get_crypto_mac(e->id);
	if (cc != NULL && cc->init != NULL) {
		if (cc->init(e->id, &mac->handle) < 0) {
			gnutls_assert();
			return GNUTLS_E_HASH_FAILED;
		}

		if (cc->setkey(mac->handle, key, keylen) < 0) {
			gnutls_assert();
			cc->deinit(mac->handle);
			return GNUTLS_E_HASH_FAILED;
		}

		mac->hash = cc->hash;
		mac->setnonce = cc->setnonce;
		mac->output = cc->output;
		mac->deinit = cc->deinit;
		mac->copy = cc->copy;

		return 0;
	}

	result = _gnutls_mac_ops.init(e->id, &mac->handle);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	mac->hash = _gnutls_mac_ops.hash;
	mac->setnonce = _gnutls_mac_ops.setnonce;
	mac->output = _gnutls_mac_ops.output;
	mac->deinit = _gnutls_mac_ops.deinit;
	mac->copy = _gnutls_mac_ops.copy;

	if (_gnutls_mac_ops.setkey(mac->handle, key, keylen) < 0) {
		gnutls_assert();
		mac->deinit(mac->handle);
		return GNUTLS_E_HASH_FAILED;
	}

	return 0;
}

int _gnutls_mac_copy(const mac_hd_st * handle, mac_hd_st * dst)
{
	if (handle->copy == NULL)
		return gnutls_assert_val(GNUTLS_E_HASH_FAILED);

	*dst = *handle; /* copy data */
	dst->handle = handle->copy(handle->handle);

	if (dst->handle == NULL)
		return GNUTLS_E_HASH_FAILED;

	return 0;
}

void _gnutls_mac_deinit(mac_hd_st * handle, void *digest)
{
	if (handle->handle == NULL) {
		return;
	}

	if (digest)
		_gnutls_mac_output(handle, digest);

	handle->deinit(handle->handle);
	handle->handle = NULL;
}

#ifdef ENABLE_SSL3
inline static int get_padsize(gnutls_mac_algorithm_t algorithm)
{
	switch (algorithm) {
	case GNUTLS_MAC_MD5:
		return 48;
	case GNUTLS_MAC_SHA1:
		return 40;
	default:
		return 0;
	}
}

/* Special functions for SSL3 MAC
 */

int
_gnutls_mac_init_ssl3(digest_hd_st * ret, const mac_entry_st * e,
		      void *key, int keylen)
{
	uint8_t ipad[48];
	int padsize, result;

	FAIL_IF_LIB_ERROR;

	padsize = get_padsize(e->id);
	if (padsize == 0) {
		gnutls_assert();
		return GNUTLS_E_HASH_FAILED;
	}

	memset(ipad, 0x36, padsize);

	result = _gnutls_hash_init(ret, e);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	ret->key = key;
	ret->keysize = keylen;

	if (keylen > 0)
		_gnutls_hash(ret, key, keylen);
	_gnutls_hash(ret, ipad, padsize);

	return 0;
}

int _gnutls_mac_output_ssl3(digest_hd_st * handle, void *digest)
{
	uint8_t ret[MAX_HASH_SIZE];
	digest_hd_st td;
	uint8_t opad[48];
	int padsize;
	int block, rc;

	padsize = get_padsize(handle->e->id);
	if (padsize == 0) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	memset(opad, 0x5C, padsize);

	rc = _gnutls_hash_init(&td, handle->e);
	if (rc < 0) {
		gnutls_assert();
		return rc;
	}

	if (handle->keysize > 0)
		_gnutls_hash(&td, handle->key, handle->keysize);

	_gnutls_hash(&td, opad, padsize);
	block = _gnutls_mac_get_algo_len(handle->e);
	_gnutls_hash_output(handle, ret);	/* get the previous hash */
	_gnutls_hash(&td, ret, block);

	_gnutls_hash_deinit(&td, digest);

	/* reset handle */
	memset(opad, 0x36, padsize);

	if (handle->keysize > 0)
		_gnutls_hash(handle, handle->key, handle->keysize);
	_gnutls_hash(handle, opad, padsize);

	return 0;
}

int _gnutls_mac_deinit_ssl3(digest_hd_st * handle, void *digest)
{
	int ret = 0;

	if (digest != NULL)
		ret = _gnutls_mac_output_ssl3(handle, digest);
	_gnutls_hash_deinit(handle, NULL);

	return ret;
}

int
_gnutls_mac_deinit_ssl3_handshake(digest_hd_st * handle,
				  void *digest, uint8_t * key,
				  uint32_t key_size)
{
	uint8_t ret[MAX_HASH_SIZE];
	digest_hd_st td;
	uint8_t opad[48];
	uint8_t ipad[48];
	int padsize;
	int block, rc;

	padsize = get_padsize(handle->e->id);
	if (padsize == 0) {
		gnutls_assert();
		rc = GNUTLS_E_INTERNAL_ERROR;
		goto cleanup;
	}

	memset(opad, 0x5C, padsize);
	memset(ipad, 0x36, padsize);

	rc = _gnutls_hash_init(&td, handle->e);
	if (rc < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (key_size > 0)
		_gnutls_hash(&td, key, key_size);

	_gnutls_hash(&td, opad, padsize);
	block = _gnutls_mac_get_algo_len(handle->e);

	if (key_size > 0)
		_gnutls_hash(handle, key, key_size);
	_gnutls_hash(handle, ipad, padsize);
	_gnutls_hash_deinit(handle, ret);	/* get the previous hash */

	_gnutls_hash(&td, ret, block);

	_gnutls_hash_deinit(&td, digest);

	return 0;

      cleanup:
	_gnutls_hash_deinit(handle, NULL);
	return rc;
}

static int
ssl3_sha(int i, uint8_t * secret, int secret_len,
	 uint8_t * rnd, int rnd_len, void *digest)
{
	int j, ret;
	uint8_t text1[26];

	digest_hd_st td;

	for (j = 0; j < i + 1; j++) {
		text1[j] = 65 + i;	/* A==65 */
	}

	ret = _gnutls_hash_init(&td, mac_to_entry(GNUTLS_MAC_SHA1));
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_hash(&td, text1, i + 1);
	_gnutls_hash(&td, secret, secret_len);
	_gnutls_hash(&td, rnd, rnd_len);

	_gnutls_hash_deinit(&td, digest);
	return 0;
}

#define SHA1_DIGEST_OUTPUT 20
#define MD5_DIGEST_OUTPUT 16

static int
ssl3_md5(int i, uint8_t * secret, int secret_len,
	 uint8_t * rnd, int rnd_len, void *digest)
{
	uint8_t tmp[MAX_HASH_SIZE];
	digest_hd_st td;
	int ret;

	ret = _gnutls_hash_init(&td, mac_to_entry(GNUTLS_MAC_MD5));
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_hash(&td, secret, secret_len);

	ret = ssl3_sha(i, secret, secret_len, rnd, rnd_len, tmp);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_hash_deinit(&td, digest);
		return ret;
	}

	_gnutls_hash(&td, tmp, SHA1_DIGEST_OUTPUT);

	_gnutls_hash_deinit(&td, digest);
	return 0;
}

int
_gnutls_ssl3_generate_random(void *secret, int secret_len,
			     void *rnd, int rnd_len,
			     int ret_bytes, uint8_t * ret)
{
	int i = 0, copy, output_bytes;
	uint8_t digest[MAX_HASH_SIZE];
	int block = MD5_DIGEST_OUTPUT;
	int result, times;

	output_bytes = 0;
	do {
		output_bytes += block;
	}
	while (output_bytes < ret_bytes);

	times = output_bytes / block;

	for (i = 0; i < times; i++) {

		result =
		    ssl3_md5(i, secret, secret_len, rnd, rnd_len, digest);
		if (result < 0) {
			gnutls_assert();
			return result;
		}

		if ((1 + i) * block < ret_bytes) {
			copy = block;
		} else {
			copy = ret_bytes - (i) * block;
		}

		memcpy(&ret[i * block], digest, copy);
	}

	return 0;
}

#endif
