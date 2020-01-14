/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

/* This file provides the backend hash/mac API for nettle.
 */

#include "gnutls_int.h"
#include <hash_int.h>
#include "errors.h"
#include <nettle/md5.h>
#include <nettle/md2.h>
#include <nettle/sha.h>
#include <nettle/sha3.h>
#include <nettle/hmac.h>
#include <nettle/umac.h>
#if ENABLE_GOST
#include "gost/hmac-gost.h"
#ifndef HAVE_NETTLE_GOSTHASH94CP_UPDATE
#include "gost/gosthash94.h"
#endif
#ifndef HAVE_NETTLE_STREEBOG512_UPDATE
#include "gost/streebog.h"
#endif
#ifndef HAVE_NETTLE_GOST28147_SET_KEY
#include "gost/gost28147.h"
#endif
#endif
#ifdef HAVE_NETTLE_CMAC128_UPDATE
#include <nettle/cmac.h>
#else
#include "cmac.h"
#endif /* HAVE_NETTLE_CMAC128_UPDATE */
#include <nettle/gcm.h>

typedef void (*update_func) (void *, size_t, const uint8_t *);
typedef void (*digest_func) (void *, size_t, uint8_t *);
typedef void (*set_key_func) (void *, size_t, const uint8_t *);
typedef void (*set_nonce_func) (void *, size_t, const uint8_t *);

static int wrap_nettle_hash_init(gnutls_digest_algorithm_t algo,
				 void **_ctx);

struct md5_sha1_ctx {
	struct md5_ctx md5;
	struct sha1_ctx sha1;
};

struct gmac_ctx {
	unsigned int pos;
	uint8_t buffer[GCM_BLOCK_SIZE];
	struct gcm_key key;
	struct gcm_ctx ctx;
	nettle_cipher_func *encrypt;
	union {
		struct aes128_ctx aes128;
		struct aes192_ctx aes192;
		struct aes256_ctx aes256;
	} cipher;
};

struct nettle_hash_ctx {
	union {
		struct md5_ctx md5;
		struct sha224_ctx sha224;
		struct sha256_ctx sha256;
		struct sha384_ctx sha384;
		struct sha512_ctx sha512;
		struct sha3_224_ctx sha3_224;
		struct sha3_256_ctx sha3_256;
		struct sha3_384_ctx sha3_384;
		struct sha3_512_ctx sha3_512;
		struct sha1_ctx sha1;
		struct md2_ctx md2;
		struct md5_sha1_ctx md5_sha1;
#if ENABLE_GOST
		struct gosthash94cp_ctx gosthash94cp;
		struct streebog256_ctx streebog256;
		struct streebog512_ctx streebog512;
#endif
	} ctx;
	void *ctx_ptr;
	gnutls_digest_algorithm_t algo;
	size_t length;
	update_func update;
	digest_func digest;
};

struct nettle_mac_ctx {
	union {
		struct hmac_md5_ctx md5;
		struct hmac_sha224_ctx sha224;
		struct hmac_sha256_ctx sha256;
		struct hmac_sha384_ctx sha384;
		struct hmac_sha512_ctx sha512;
		struct hmac_sha1_ctx sha1;
#if ENABLE_GOST
		struct hmac_gosthash94cp_ctx gosthash94cp;
		struct hmac_streebog256_ctx streebog256;
		struct hmac_streebog512_ctx streebog512;
		struct gost28147_imit_ctx gost28147_imit;
#endif
		struct umac96_ctx umac96;
		struct umac128_ctx umac128;
                struct cmac_aes128_ctx cmac128;
                struct cmac_aes256_ctx cmac256;
		struct gmac_ctx gmac;
	} ctx;

	void *ctx_ptr;
	gnutls_mac_algorithm_t algo;
	size_t length;
	update_func update;
	digest_func digest;
	set_key_func set_key;
	set_nonce_func set_nonce;
};

#if ENABLE_GOST
static void
_wrap_gost28147_imit_set_key_tc26z(void *ctx, size_t len, const uint8_t * key)
{
	gost28147_imit_set_key(ctx, len, key);
	gost28147_imit_set_param(ctx, &gost28147_param_TC26_Z);
}
#endif

static void
_wrap_umac96_set_key(void *ctx, size_t len, const uint8_t * key)
{
	if (unlikely(len != 16))
		abort();
	umac96_set_key(ctx, key);
}

static void
_wrap_umac128_set_key(void *ctx, size_t len, const uint8_t * key)
{
	if (unlikely(len != 16))
		abort();
	umac128_set_key(ctx, key);
}

static void
_wrap_cmac128_set_key(void *ctx, size_t len, const uint8_t * key)
{
	if (unlikely(len != 16))
		abort();
	cmac_aes128_set_key(ctx, key);
}

static void
_wrap_cmac256_set_key(void *ctx, size_t len, const uint8_t * key)
{
	if (unlikely(len != 32))
		abort();
	cmac_aes256_set_key(ctx, key);
}

static void
_wrap_gmac_aes128_set_key(void *_ctx, size_t len, const uint8_t * key)
{
	struct gmac_ctx *ctx = _ctx;

	if (unlikely(len != 16))
		abort();
	aes128_set_encrypt_key(&ctx->cipher.aes128, key);
	gcm_set_key(&ctx->key, &ctx->cipher, ctx->encrypt);
	ctx->pos = 0;
}

static void
_wrap_gmac_aes192_set_key(void *_ctx, size_t len, const uint8_t * key)
{
	struct gmac_ctx *ctx = _ctx;

	if (unlikely(len != 24))
		abort();
	aes192_set_encrypt_key(&ctx->cipher.aes192, key);
	gcm_set_key(&ctx->key, &ctx->cipher, ctx->encrypt);
	ctx->pos = 0;
}

static void
_wrap_gmac_aes256_set_key(void *_ctx, size_t len, const uint8_t * key)
{
	struct gmac_ctx *ctx = _ctx;

	if (unlikely(len != 32))
		abort();
	aes256_set_encrypt_key(&ctx->cipher.aes256, key);
	gcm_set_key(&ctx->key, &ctx->cipher, ctx->encrypt);
	ctx->pos = 0;
}

static void _wrap_gmac_set_nonce(void *_ctx, size_t nonce_length, const uint8_t *nonce)
{
	struct gmac_ctx *ctx = _ctx;

	gcm_set_iv(&ctx->ctx, &ctx->key, nonce_length, nonce);
}

static void _wrap_gmac_update(void *_ctx, size_t length, const uint8_t *data)
{
	struct gmac_ctx *ctx = _ctx;

	if (ctx->pos + length < GCM_BLOCK_SIZE) {
		memcpy(&ctx->buffer[ctx->pos], data, length);
		ctx->pos += length;
		return;
	}

	if (ctx->pos) {
		memcpy(&ctx->buffer[ctx->pos], data, GCM_BLOCK_SIZE - ctx->pos);
		gcm_update(&ctx->ctx, &ctx->key, GCM_BLOCK_SIZE, ctx->buffer);
		data += GCM_BLOCK_SIZE - ctx->pos;
		length -= GCM_BLOCK_SIZE - ctx->pos;
	}

	if (length >= GCM_BLOCK_SIZE) {
		gcm_update(&ctx->ctx, &ctx->key,
			   length / GCM_BLOCK_SIZE * GCM_BLOCK_SIZE,
			   data);
		data += length / GCM_BLOCK_SIZE * GCM_BLOCK_SIZE;
		length %= GCM_BLOCK_SIZE;
	}

	memcpy(ctx->buffer, data, length);
	ctx->pos = length;
}

static void _wrap_gmac_digest(void *_ctx, size_t length, uint8_t *digest)
{
	struct gmac_ctx *ctx = _ctx;

	if (ctx->pos)
		gcm_update(&ctx->ctx, &ctx->key, ctx->pos, ctx->buffer);
	gcm_digest(&ctx->ctx, &ctx->key, &ctx->cipher, ctx->encrypt, length, digest);
	ctx->pos = 0;
}

static int _mac_ctx_init(gnutls_mac_algorithm_t algo,
			 struct nettle_mac_ctx *ctx)
{
	/* Any FIPS140-2 related enforcement is performed on
	 * gnutls_hash_init() and gnutls_hmac_init() */

	ctx->set_nonce = NULL;
	switch (algo) {
	case GNUTLS_MAC_MD5:
		ctx->update = (update_func) hmac_md5_update;
		ctx->digest = (digest_func) hmac_md5_digest;
		ctx->set_key = (set_key_func) hmac_md5_set_key;
		ctx->ctx_ptr = &ctx->ctx.md5;
		ctx->length = MD5_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA1:
		ctx->update = (update_func) hmac_sha1_update;
		ctx->digest = (digest_func) hmac_sha1_digest;
		ctx->set_key = (set_key_func) hmac_sha1_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha1;
		ctx->length = SHA1_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA224:
		ctx->update = (update_func) hmac_sha224_update;
		ctx->digest = (digest_func) hmac_sha224_digest;
		ctx->set_key = (set_key_func) hmac_sha224_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha224;
		ctx->length = SHA224_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA256:
		ctx->update = (update_func) hmac_sha256_update;
		ctx->digest = (digest_func) hmac_sha256_digest;
		ctx->set_key = (set_key_func) hmac_sha256_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha256;
		ctx->length = SHA256_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA384:
		ctx->update = (update_func) hmac_sha384_update;
		ctx->digest = (digest_func) hmac_sha384_digest;
		ctx->set_key = (set_key_func) hmac_sha384_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha384;
		ctx->length = SHA384_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA512:
		ctx->update = (update_func) hmac_sha512_update;
		ctx->digest = (digest_func) hmac_sha512_digest;
		ctx->set_key = (set_key_func) hmac_sha512_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha512;
		ctx->length = SHA512_DIGEST_SIZE;
		break;
#if ENABLE_GOST
	case GNUTLS_MAC_GOSTR_94:
		ctx->update = (update_func) hmac_gosthash94cp_update;
		ctx->digest = (digest_func) hmac_gosthash94cp_digest;
		ctx->set_key = (set_key_func) hmac_gosthash94cp_set_key;
		ctx->ctx_ptr = &ctx->ctx.gosthash94cp;
		ctx->length = GOSTHASH94CP_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_STREEBOG_256:
		ctx->update = (update_func) hmac_streebog256_update;
		ctx->digest = (digest_func) hmac_streebog256_digest;
		ctx->set_key = (set_key_func) hmac_streebog256_set_key;
		ctx->ctx_ptr = &ctx->ctx.streebog256;
		ctx->length = STREEBOG256_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_STREEBOG_512:
		ctx->update = (update_func) hmac_streebog512_update;
		ctx->digest = (digest_func) hmac_streebog512_digest;
		ctx->set_key = (set_key_func) hmac_streebog512_set_key;
		ctx->ctx_ptr = &ctx->ctx.streebog512;
		ctx->length = STREEBOG512_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_GOST28147_TC26Z_IMIT:
		ctx->update = (update_func) gost28147_imit_update;
		ctx->digest = (digest_func) gost28147_imit_digest;
		ctx->set_key = _wrap_gost28147_imit_set_key_tc26z;
		ctx->ctx_ptr = &ctx->ctx.gost28147_imit;
		ctx->length = GOST28147_IMIT_DIGEST_SIZE;
		break;
#endif
	case GNUTLS_MAC_UMAC_96:
		ctx->update = (update_func) umac96_update;
		ctx->digest = (digest_func) umac96_digest;
		ctx->set_key = _wrap_umac96_set_key;
		ctx->set_nonce = (set_nonce_func) umac96_set_nonce;
		ctx->ctx_ptr = &ctx->ctx.umac96;
		ctx->length = 12;
		break;
	case GNUTLS_MAC_UMAC_128:
		ctx->update = (update_func) umac128_update;
		ctx->digest = (digest_func) umac128_digest;
		ctx->set_key = _wrap_umac128_set_key;
		ctx->set_nonce = (set_nonce_func) umac128_set_nonce;
		ctx->ctx_ptr = &ctx->ctx.umac128;
		ctx->length = 16;
		break;
	case GNUTLS_MAC_AES_CMAC_128:
		ctx->update = (update_func) cmac_aes128_update;
		ctx->digest = (digest_func) cmac_aes128_digest;
		ctx->set_key = _wrap_cmac128_set_key;
		ctx->ctx_ptr = &ctx->ctx.cmac128;
		ctx->length = CMAC128_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_AES_CMAC_256:
		ctx->update = (update_func) cmac_aes256_update;
		ctx->digest = (digest_func) cmac_aes256_digest;
		ctx->set_key = _wrap_cmac256_set_key;
		ctx->ctx_ptr = &ctx->ctx.cmac256;
		ctx->length = CMAC128_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_AES_GMAC_128:
		ctx->set_key = _wrap_gmac_aes128_set_key;
		ctx->set_nonce = _wrap_gmac_set_nonce;
		ctx->update = _wrap_gmac_update;
		ctx->digest = _wrap_gmac_digest;
		ctx->ctx_ptr = &ctx->ctx.gmac;
		ctx->length = GCM_DIGEST_SIZE;
		ctx->ctx.gmac.encrypt = (nettle_cipher_func *)aes128_encrypt;
		break;
	case GNUTLS_MAC_AES_GMAC_192:
		ctx->set_key = _wrap_gmac_aes192_set_key;
		ctx->set_nonce = _wrap_gmac_set_nonce;
		ctx->update = _wrap_gmac_update;
		ctx->digest = _wrap_gmac_digest;
		ctx->ctx_ptr = &ctx->ctx.gmac;
		ctx->length = GCM_DIGEST_SIZE;
		ctx->ctx.gmac.encrypt = (nettle_cipher_func *)aes192_encrypt;
		break;
	case GNUTLS_MAC_AES_GMAC_256:
		ctx->set_key = _wrap_gmac_aes256_set_key;
		ctx->set_nonce = _wrap_gmac_set_nonce;
		ctx->update = _wrap_gmac_update;
		ctx->digest = _wrap_gmac_digest;
		ctx->ctx_ptr = &ctx->ctx.gmac;
		ctx->length = GCM_DIGEST_SIZE;
		ctx->ctx.gmac.encrypt = (nettle_cipher_func *)aes256_encrypt;
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return 0;
}

static int wrap_nettle_mac_fast(gnutls_mac_algorithm_t algo,
				const void *nonce, size_t nonce_size,
				const void *key, size_t key_size,
				const void *text, size_t text_size,
				void *digest)
{
	struct nettle_mac_ctx ctx;
	int ret;

	ret = _mac_ctx_init(algo, &ctx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ctx.set_key(&ctx, key_size, key);
	if (ctx.set_nonce) {
		if (nonce == NULL || nonce_size == 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		ctx.set_nonce(&ctx, nonce_size, nonce);
	}
	ctx.update(&ctx, text_size, text);
	ctx.digest(&ctx, ctx.length, digest);
	
	zeroize_temp_key(&ctx, sizeof(ctx));

	return 0;
}

static int wrap_nettle_mac_exists(gnutls_mac_algorithm_t algo)
{
	switch (algo) {
	case GNUTLS_MAC_MD5:
	case GNUTLS_MAC_SHA1:
	case GNUTLS_MAC_SHA224:
	case GNUTLS_MAC_SHA256:
	case GNUTLS_MAC_SHA384:
	case GNUTLS_MAC_SHA512:
	case GNUTLS_MAC_UMAC_96:
	case GNUTLS_MAC_UMAC_128:
	case GNUTLS_MAC_AES_CMAC_128:
	case GNUTLS_MAC_AES_CMAC_256:
	case GNUTLS_MAC_AES_GMAC_128:
	case GNUTLS_MAC_AES_GMAC_192:
	case GNUTLS_MAC_AES_GMAC_256:
#if ENABLE_GOST
	case GNUTLS_MAC_GOSTR_94:
	case GNUTLS_MAC_STREEBOG_256:
	case GNUTLS_MAC_STREEBOG_512:
	case GNUTLS_MAC_GOST28147_TC26Z_IMIT:
#endif
		return 1;
	default:
		return 0;
	}
}

static int wrap_nettle_mac_init(gnutls_mac_algorithm_t algo, void **_ctx)
{
	struct nettle_mac_ctx *ctx;
	int ret;

	ctx = gnutls_calloc(1, sizeof(struct nettle_mac_ctx));
	if (ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ctx->algo = algo;

	ret = _mac_ctx_init(algo, ctx);
	if (ret < 0) {
		gnutls_free(ctx);
		return gnutls_assert_val(ret);
	}

	*_ctx = ctx;

	return 0;
}

static void *wrap_nettle_mac_copy(const void *_ctx)
{
	const struct nettle_mac_ctx *ctx = _ctx;
	struct nettle_mac_ctx *new_ctx;
	ptrdiff_t off = (uint8_t *)ctx->ctx_ptr - (uint8_t *)(&ctx->ctx);

	new_ctx = gnutls_calloc(1, sizeof(struct nettle_mac_ctx));
	if (new_ctx == NULL)
		return NULL;

	memcpy(new_ctx, ctx, sizeof(*ctx));
	new_ctx->ctx_ptr = (uint8_t *)&new_ctx->ctx + off;

	return new_ctx;
}

static int
wrap_nettle_mac_set_key(void *_ctx, const void *key, size_t keylen)
{
	struct nettle_mac_ctx *ctx = _ctx;

	ctx->set_key(ctx->ctx_ptr, keylen, key);
	return 0;
}

static int
wrap_nettle_mac_set_nonce(void *_ctx, const void *nonce, size_t noncelen)
{
	struct nettle_mac_ctx *ctx = _ctx;

	if (ctx->set_nonce == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (nonce == NULL || noncelen == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ctx->set_nonce(ctx->ctx_ptr, noncelen, nonce);

	return GNUTLS_E_SUCCESS;
}

static int
wrap_nettle_mac_update(void *_ctx, const void *text, size_t textsize)
{
	struct nettle_mac_ctx *ctx = _ctx;

	ctx->update(ctx->ctx_ptr, textsize, text);

	return GNUTLS_E_SUCCESS;
}

static int
wrap_nettle_mac_output(void *src_ctx, void *digest, size_t digestsize)
{
	struct nettle_mac_ctx *ctx;
	ctx = src_ctx;

	if (digestsize < ctx->length) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ctx->digest(ctx->ctx_ptr, digestsize, digest);

	return 0;
}

static void wrap_nettle_mac_deinit(void *hd)
{
	struct nettle_mac_ctx *ctx = hd;
	
	zeroize_temp_key(ctx, sizeof(*ctx));
	gnutls_free(ctx);
}

/* Hash functions 
 */
static int
wrap_nettle_hash_update(void *_ctx, const void *text, size_t textsize)
{
	struct nettle_hash_ctx *ctx = _ctx;

	ctx->update(ctx->ctx_ptr, textsize, text);

	return GNUTLS_E_SUCCESS;
}

static void wrap_nettle_hash_deinit(void *hd)
{
	gnutls_free(hd);
}

static int wrap_nettle_hash_exists(gnutls_digest_algorithm_t algo)
{
	switch (algo) {
	case GNUTLS_DIG_MD5:
	case GNUTLS_DIG_SHA1:
	case GNUTLS_DIG_MD5_SHA1:

	case GNUTLS_DIG_SHA224:
	case GNUTLS_DIG_SHA256:
	case GNUTLS_DIG_SHA384:
	case GNUTLS_DIG_SHA512:
		return 1;
	case GNUTLS_DIG_SHA3_224:
	case GNUTLS_DIG_SHA3_256:
	case GNUTLS_DIG_SHA3_384:
	case GNUTLS_DIG_SHA3_512:
#ifdef NETTLE_SHA3_FIPS202
		return 1;
#else
		return 0;
#endif
	case GNUTLS_DIG_MD2:
#if ENABLE_GOST
	case GNUTLS_DIG_GOSTR_94:
	case GNUTLS_DIG_STREEBOG_256:
	case GNUTLS_DIG_STREEBOG_512:
#endif
		return 1;
	default:
		return 0;
	}
}

static void _md5_sha1_update(void *_ctx, size_t len, const uint8_t *data)
{
	struct md5_sha1_ctx *ctx = _ctx;

	md5_update(&ctx->md5, len, data);
	sha1_update(&ctx->sha1, len, data);
}

static void _md5_sha1_digest(void *_ctx, size_t len, uint8_t *digest)
{
	struct md5_sha1_ctx *ctx = _ctx;

	md5_digest(&ctx->md5, len <= MD5_DIGEST_SIZE ? len : MD5_DIGEST_SIZE,
		   digest);

	if (len > MD5_DIGEST_SIZE)
		sha1_digest(&ctx->sha1, len - MD5_DIGEST_SIZE,
			    digest + MD5_DIGEST_SIZE);
}

static int _ctx_init(gnutls_digest_algorithm_t algo,
		     struct nettle_hash_ctx *ctx)
{
	/* Any FIPS140-2 related enforcement is performed on
	 * gnutls_hash_init() and gnutls_hmac_init() */
	switch (algo) {
	case GNUTLS_DIG_MD5:
		md5_init(&ctx->ctx.md5);
		ctx->update = (update_func) md5_update;
		ctx->digest = (digest_func) md5_digest;
		ctx->ctx_ptr = &ctx->ctx.md5;
		ctx->length = MD5_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA1:
		sha1_init(&ctx->ctx.sha1);
		ctx->update = (update_func) sha1_update;
		ctx->digest = (digest_func) sha1_digest;
		ctx->ctx_ptr = &ctx->ctx.sha1;
		ctx->length = SHA1_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_MD5_SHA1:
		md5_init(&ctx->ctx.md5_sha1.md5);
		sha1_init(&ctx->ctx.md5_sha1.sha1);
		ctx->update = (update_func) _md5_sha1_update;
		ctx->digest = (digest_func) _md5_sha1_digest;
		ctx->ctx_ptr = &ctx->ctx.md5_sha1;
		ctx->length = MD5_DIGEST_SIZE + SHA1_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA224:
		sha224_init(&ctx->ctx.sha224);
		ctx->update = (update_func) sha224_update;
		ctx->digest = (digest_func) sha224_digest;
		ctx->ctx_ptr = &ctx->ctx.sha224;
		ctx->length = SHA224_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA256:
		sha256_init(&ctx->ctx.sha256);
		ctx->update = (update_func) sha256_update;
		ctx->digest = (digest_func) sha256_digest;
		ctx->ctx_ptr = &ctx->ctx.sha256;
		ctx->length = SHA256_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA384:
		sha384_init(&ctx->ctx.sha384);
		ctx->update = (update_func) sha384_update;
		ctx->digest = (digest_func) sha384_digest;
		ctx->ctx_ptr = &ctx->ctx.sha384;
		ctx->length = SHA384_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA512:
		sha512_init(&ctx->ctx.sha512);
		ctx->update = (update_func) sha512_update;
		ctx->digest = (digest_func) sha512_digest;
		ctx->ctx_ptr = &ctx->ctx.sha512;
		ctx->length = SHA512_DIGEST_SIZE;
		break;
#ifdef NETTLE_SHA3_FIPS202
	case GNUTLS_DIG_SHA3_224:
		sha3_224_init(&ctx->ctx.sha3_224);
		ctx->update = (update_func) sha3_224_update;
		ctx->digest = (digest_func) sha3_224_digest;
		ctx->ctx_ptr = &ctx->ctx.sha3_224;
		ctx->length = SHA3_224_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA3_256:
		sha3_256_init(&ctx->ctx.sha3_256);
		ctx->update = (update_func) sha3_256_update;
		ctx->digest = (digest_func) sha3_256_digest;
		ctx->ctx_ptr = &ctx->ctx.sha3_256;
		ctx->length = SHA3_256_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA3_384:
		sha3_384_init(&ctx->ctx.sha3_384);
		ctx->update = (update_func) sha3_384_update;
		ctx->digest = (digest_func) sha3_384_digest;
		ctx->ctx_ptr = &ctx->ctx.sha3_384;
		ctx->length = SHA3_384_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA3_512:
		sha3_512_init(&ctx->ctx.sha3_512);
		ctx->update = (update_func) sha3_512_update;
		ctx->digest = (digest_func) sha3_512_digest;
		ctx->ctx_ptr = &ctx->ctx.sha3_512;
		ctx->length = SHA3_512_DIGEST_SIZE;
		break;
#endif
	case GNUTLS_DIG_MD2:
		md2_init(&ctx->ctx.md2);
		ctx->update = (update_func) md2_update;
		ctx->digest = (digest_func) md2_digest;
		ctx->ctx_ptr = &ctx->ctx.md2;
		ctx->length = MD2_DIGEST_SIZE;
		break;
#if ENABLE_GOST
	case GNUTLS_DIG_GOSTR_94:
		gosthash94cp_init(&ctx->ctx.gosthash94cp);
		ctx->update = (update_func) gosthash94cp_update;
		ctx->digest = (digest_func) gosthash94cp_digest;
		ctx->ctx_ptr = &ctx->ctx.gosthash94cp;
		ctx->length = GOSTHASH94_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_STREEBOG_256:
		streebog256_init(&ctx->ctx.streebog256);
		ctx->update = (update_func) streebog256_update;
		ctx->digest = (digest_func) streebog256_digest;
		ctx->ctx_ptr = &ctx->ctx.streebog256;
		ctx->length = STREEBOG256_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_STREEBOG_512:
		streebog512_init(&ctx->ctx.streebog512);
		ctx->update = (update_func) streebog512_update;
		ctx->digest = (digest_func) streebog512_digest;
		ctx->ctx_ptr = &ctx->ctx.streebog512;
		ctx->length = STREEBOG512_DIGEST_SIZE;
		break;
#endif
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return 0;
}

static int wrap_nettle_hash_fast(gnutls_digest_algorithm_t algo,
				 const void *text, size_t text_size,
				 void *digest)
{
	struct nettle_hash_ctx ctx;
	int ret;

	ret = _ctx_init(algo, &ctx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ctx.update(&ctx, text_size, text);
	ctx.digest(&ctx, ctx.length, digest);

	return 0;
}

static int
wrap_nettle_hash_init(gnutls_digest_algorithm_t algo, void **_ctx)
{
	struct nettle_hash_ctx *ctx;
	int ret;

	ctx = gnutls_malloc(sizeof(struct nettle_hash_ctx));
	if (ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ctx->algo = algo;

	if ((ret = _ctx_init(algo, ctx)) < 0) {
		gnutls_assert();
		gnutls_free(ctx);
		return ret;
	}

	*_ctx = ctx;

	return 0;
}

static void *wrap_nettle_hash_copy(const void *_ctx)
{
	const struct nettle_hash_ctx *ctx = _ctx;
	struct nettle_hash_ctx *new_ctx;
	ptrdiff_t off = (uint8_t *)ctx->ctx_ptr - (uint8_t *)(&ctx->ctx);

	new_ctx = gnutls_calloc(1, sizeof(struct nettle_hash_ctx));
	if (new_ctx == NULL)
		return NULL;

	memcpy(new_ctx, ctx, sizeof(*ctx));
	new_ctx->ctx_ptr = (uint8_t *)&new_ctx->ctx + off;

	return new_ctx;
}

static int
wrap_nettle_hash_output(void *src_ctx, void *digest, size_t digestsize)
{
	struct nettle_hash_ctx *ctx;
	ctx = src_ctx;

	if (digestsize < ctx->length) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ctx->digest(ctx->ctx_ptr, digestsize, digest);

	return 0;
}

gnutls_crypto_mac_st _gnutls_mac_ops = {
	.init = wrap_nettle_mac_init,
	.setkey = wrap_nettle_mac_set_key,
	.setnonce = wrap_nettle_mac_set_nonce,
	.hash = wrap_nettle_mac_update,
	.output = wrap_nettle_mac_output,
	.deinit = wrap_nettle_mac_deinit,
	.fast = wrap_nettle_mac_fast,
	.exists = wrap_nettle_mac_exists,
	.copy = wrap_nettle_mac_copy,
};

gnutls_crypto_digest_st _gnutls_digest_ops = {
	.init = wrap_nettle_hash_init,
	.hash = wrap_nettle_hash_update,
	.output = wrap_nettle_hash_output,
	.deinit = wrap_nettle_hash_deinit,
	.fast = wrap_nettle_hash_fast,
	.exists = wrap_nettle_hash_exists,
	.copy = wrap_nettle_hash_copy,
};
