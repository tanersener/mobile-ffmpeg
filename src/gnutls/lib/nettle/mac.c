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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
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
#include <fips.h>

typedef void (*update_func) (void *, size_t, const uint8_t *);
typedef void (*digest_func) (void *, size_t, uint8_t *);
typedef void (*set_key_func) (void *, size_t, const uint8_t *);
typedef void (*set_nonce_func) (void *, size_t, const uint8_t *);

static int wrap_nettle_hash_init(gnutls_digest_algorithm_t algo,
				 void **_ctx);

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
		struct umac96_ctx umac96;
		struct umac128_ctx umac128;
	} ctx;

	void *ctx_ptr;
	gnutls_mac_algorithm_t algo;
	size_t length;
	update_func update;
	digest_func digest;
	set_key_func set_key;
	set_nonce_func set_nonce;
};

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

static int _mac_ctx_init(gnutls_mac_algorithm_t algo,
			 struct nettle_mac_ctx *ctx)
{
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
	case GNUTLS_MAC_UMAC_96:
		if (_gnutls_fips_mode_enabled() != 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	
		ctx->update = (update_func) umac96_update;
		ctx->digest = (digest_func) umac96_digest;
		ctx->set_key = _wrap_umac96_set_key;
		ctx->set_nonce = (set_nonce_func) umac96_set_nonce;
		ctx->ctx_ptr = &ctx->ctx.umac96;
		ctx->length = 12;
		break;
	case GNUTLS_MAC_UMAC_128:
		if (_gnutls_fips_mode_enabled() != 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		ctx->update = (update_func) umac128_update;
		ctx->digest = (digest_func) umac128_digest;
		ctx->set_key = _wrap_umac128_set_key;
		ctx->set_nonce = (set_nonce_func) umac128_set_nonce;
		ctx->ctx_ptr = &ctx->ctx.umac128;
		ctx->length = 16;
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

	if (ctx.set_nonce)
		ctx.set_nonce(&ctx, nonce_size, nonce);
	ctx.set_key(&ctx, key_size, key);
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
		return 1;

	case GNUTLS_MAC_UMAC_96:
	case GNUTLS_MAC_UMAC_128:
		if (_gnutls_fips_mode_enabled() != 0)
			return 0;
		else
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
		return GNUTLS_E_INVALID_REQUEST;

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
		if (_gnutls_fips_mode_enabled() != 0)
			return 0;
		else
			return 1;
	default:
		return 0;
	}
}

static int _ctx_init(gnutls_digest_algorithm_t algo,
		     struct nettle_hash_ctx *ctx)
{
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
		if (_gnutls_fips_mode_enabled() != 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		md2_init(&ctx->ctx.md2);
		ctx->update = (update_func) md2_update;
		ctx->digest = (digest_func) md2_digest;
		ctx->ctx_ptr = &ctx->ctx.md2;
		ctx->length = MD2_DIGEST_SIZE;
		break;
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
};

gnutls_crypto_digest_st _gnutls_digest_ops = {
	.init = wrap_nettle_hash_init,
	.hash = wrap_nettle_hash_update,
	.output = wrap_nettle_hash_output,
	.deinit = wrap_nettle_hash_deinit,
	.fast = wrap_nettle_hash_fast,
	.exists = wrap_nettle_hash_exists,
};
