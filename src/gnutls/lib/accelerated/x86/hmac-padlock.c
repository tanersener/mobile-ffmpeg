/*
 * Copyright (C) 2008, 2010-2012 Free Software Foundation, Inc.
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

/* This file provides the backend hash/mac implementation for
 * VIA Padlock hardware acceleration.
 */

#include "gnutls_int.h"
#include <hash_int.h>
#include "errors.h"
#include <nettle/sha.h>
#include <nettle/hmac.h>
#include <nettle/macros.h>
#include <nettle/memxor.h>
#include <aes-padlock.h>
#include <sha-padlock.h>
#include <algorithms.h>

#ifdef HAVE_LIBNETTLE

#define IPAD 0x36
#define OPAD 0x5c
#define MAX_SHA_DIGEST_SIZE (512/8)

typedef void (*update_func) (void *, size_t, const uint8_t *);
typedef void (*digest_func) (void *, size_t, uint8_t *);
typedef void (*set_key_func) (void *, size_t, const uint8_t *);

struct padlock_hmac_ctx {
	union {
		struct hmac_sha224_ctx sha224;
		struct hmac_sha256_ctx sha256;
		struct hmac_sha384_ctx sha384;
		struct hmac_sha512_ctx sha512;
		struct hmac_sha1_ctx sha1;
	} ctx;

	void *ctx_ptr;
	gnutls_mac_algorithm_t algo;
	size_t length;
	update_func update;
	digest_func digest;
	set_key_func setkey;
};

static void
padlock_hmac_sha1_set_key(struct hmac_sha1_ctx *ctx,
			  size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &padlock_sha1, key_length, key);
}

static void
padlock_hmac_sha1_update(struct hmac_sha1_ctx *ctx,
			 size_t length, const uint8_t * data)
{
	padlock_sha1_update(&ctx->state, length, data);
}

static void
padlock_hmac_sha1_digest(struct hmac_sha1_ctx *ctx,
			 size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &padlock_sha1, length, digest);
}

static void
padlock_hmac_sha256_set_key(struct hmac_sha256_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &padlock_sha256, key_length, key);
}

static void
padlock_hmac_sha256_update(struct hmac_sha256_ctx *ctx,
			   size_t length, const uint8_t * data)
{
	padlock_sha256_update(&ctx->state, length, data);
}

static void
padlock_hmac_sha256_digest(struct hmac_sha256_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &padlock_sha256, length, digest);
}

static void
padlock_hmac_sha224_set_key(struct hmac_sha224_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &padlock_sha224, key_length, key);
}

static void
padlock_hmac_sha224_digest(struct hmac_sha224_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &padlock_sha224, length, digest);
}

static void
padlock_hmac_sha384_set_key(struct hmac_sha384_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &padlock_sha384, key_length, key);
}

static void
padlock_hmac_sha384_digest(struct hmac_sha384_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &padlock_sha384, length, digest);
}

static void
padlock_hmac_sha512_set_key(struct hmac_sha512_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &padlock_sha512, key_length, key);
}

static void
padlock_hmac_sha512_update(struct hmac_sha512_ctx *ctx,
			   size_t length, const uint8_t * data)
{
	padlock_sha512_update(&ctx->state, length, data);
}

static void
padlock_hmac_sha512_digest(struct hmac_sha512_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &padlock_sha512, length, digest);
}

static int
_hmac_ctx_init(gnutls_mac_algorithm_t algo, struct padlock_hmac_ctx *ctx)
{
	switch (algo) {
	case GNUTLS_MAC_SHA1:
		ctx->update = (update_func) padlock_hmac_sha1_update;
		ctx->digest = (digest_func) padlock_hmac_sha1_digest;
		ctx->setkey = (set_key_func) padlock_hmac_sha1_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha1;
		ctx->length = SHA1_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA224:
		ctx->update = (update_func) padlock_hmac_sha256_update;
		ctx->digest = (digest_func) padlock_hmac_sha224_digest;
		ctx->setkey = (set_key_func) padlock_hmac_sha224_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha224;
		ctx->length = SHA224_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA256:
		ctx->update = (update_func) padlock_hmac_sha256_update;
		ctx->digest = (digest_func) padlock_hmac_sha256_digest;
		ctx->setkey = (set_key_func) padlock_hmac_sha256_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha256;
		ctx->length = SHA256_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA384:
		ctx->update = (update_func) padlock_hmac_sha512_update;
		ctx->digest = (digest_func) padlock_hmac_sha384_digest;
		ctx->setkey = (set_key_func) padlock_hmac_sha384_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha384;
		ctx->length = SHA384_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA512:
		ctx->update = (update_func) padlock_hmac_sha512_update;
		ctx->digest = (digest_func) padlock_hmac_sha512_digest;
		ctx->setkey = (set_key_func) padlock_hmac_sha512_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha512;
		ctx->length = SHA512_DIGEST_SIZE;
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return 0;
}


static int wrap_padlock_hmac_init(gnutls_mac_algorithm_t algo, void **_ctx)
{
	struct padlock_hmac_ctx *ctx;
	int ret;

	ctx = gnutls_calloc(1, sizeof(struct padlock_hmac_ctx));
	if (ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ctx->algo = algo;

	ret = _hmac_ctx_init(algo, ctx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	*_ctx = ctx;

	return 0;
}

static void *
wrap_padlock_hmac_copy(const void *_ctx)
{
	struct padlock_hmac_ctx *new_ctx;
	const struct padlock_hmac_ctx *ctx=_ctx;
	ptrdiff_t off = (uint8_t *)ctx->ctx_ptr - (uint8_t *)(&ctx->ctx);

	new_ctx = gnutls_malloc(sizeof(struct padlock_hmac_ctx));
	if (new_ctx == NULL) {
		gnutls_assert();
		return NULL;
	}

	memcpy(new_ctx, ctx, sizeof(*new_ctx));
	new_ctx->ctx_ptr = (uint8_t *)&new_ctx->ctx + off;

	return new_ctx;
}

static int
wrap_padlock_hmac_setkey(void *_ctx, const void *key, size_t keylen)
{
	struct padlock_hmac_ctx *ctx = _ctx;

	ctx->setkey(ctx->ctx_ptr, keylen, key);

	return GNUTLS_E_SUCCESS;
}

static int
wrap_padlock_hmac_update(void *_ctx, const void *text, size_t textsize)
{
	struct padlock_hmac_ctx *ctx = _ctx;

	ctx->update(ctx->ctx_ptr, textsize, text);

	return GNUTLS_E_SUCCESS;
}

static int
wrap_padlock_hmac_output(void *src_ctx, void *digest, size_t digestsize)
{
	struct padlock_hmac_ctx *ctx;
	ctx = src_ctx;

	if (digestsize < ctx->length) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ctx->digest(ctx->ctx_ptr, digestsize, digest);

	return 0;
}

static void wrap_padlock_hmac_deinit(void *hd)
{
	gnutls_free(hd);
}

static int
wrap_padlock_hmac_fast(gnutls_mac_algorithm_t algo,
		       const void *nonce, size_t nonce_size,
		       const void *key, size_t key_size, const void *text,
		       size_t text_size, void *digest)
{
	if (algo == GNUTLS_MAC_SHA1 || algo == GNUTLS_MAC_SHA256) {
		unsigned char *pad;
		unsigned char pad2[SHA1_DATA_SIZE + MAX_SHA_DIGEST_SIZE];
		unsigned char hkey[MAX_SHA_DIGEST_SIZE];
		unsigned int digest_size =
		    _gnutls_mac_get_algo_len(mac_to_entry(algo));

		if (key_size > SHA1_DATA_SIZE) {
			wrap_padlock_hash_fast((gnutls_digest_algorithm_t)
					       algo, key, key_size, hkey);
			key = hkey;
			key_size = digest_size;
		}

		pad = gnutls_malloc(text_size + SHA1_DATA_SIZE);
		if (pad == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		memset(pad, IPAD, SHA1_DATA_SIZE);
		memxor(pad, key, key_size);

		memcpy(&pad[SHA1_DATA_SIZE], text, text_size);

		wrap_padlock_hash_fast((gnutls_digest_algorithm_t) algo,
				       pad, text_size + SHA1_DATA_SIZE,
				       &pad2[SHA1_DATA_SIZE]);

		gnutls_free(pad);

		memset(pad2, OPAD, SHA1_DATA_SIZE);
		memxor(pad2, key, key_size);

		wrap_padlock_hash_fast((gnutls_digest_algorithm_t) algo,
				       pad2, digest_size + SHA1_DATA_SIZE,
				       digest);

	} else {
		struct padlock_hmac_ctx ctx;
		int ret;

		ret = _hmac_ctx_init(algo, &ctx);
		if (ret < 0)
			return gnutls_assert_val(ret);
		ctx.algo = algo;

		wrap_padlock_hmac_setkey(&ctx, key, key_size);

		wrap_padlock_hmac_update(&ctx, text, text_size);

		wrap_padlock_hmac_output(&ctx, digest, ctx.length);
		wrap_padlock_hmac_deinit(&ctx);

		zeroize_temp_key(&ctx, sizeof(ctx));
	}

	return 0;
}

const gnutls_crypto_mac_st _gnutls_hmac_sha_padlock = {
	.init = NULL,
	.setkey = NULL,
	.setnonce = NULL,
	.hash = NULL,
	.output = NULL,
	.deinit = NULL,
	.fast = wrap_padlock_hmac_fast
};

const gnutls_crypto_mac_st _gnutls_hmac_sha_padlock_nano = {
	.init = wrap_padlock_hmac_init,
	.setkey = wrap_padlock_hmac_setkey,
	.setnonce = NULL,
	.hash = wrap_padlock_hmac_update,
	.output = wrap_padlock_hmac_output,
	.copy = wrap_padlock_hmac_copy,
	.deinit = wrap_padlock_hmac_deinit,
	.fast = wrap_padlock_hmac_fast,
};

#endif				/* HAVE_LIBNETTLE */
