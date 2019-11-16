/*
 * Copyright (C) 2008-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
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
 * aarch64 hardware.
 */

#include "gnutls_int.h"
#include <hash_int.h>
#include "errors.h"
#include <nettle/sha.h>
#include <nettle/hmac.h>
#include <nettle/macros.h>
#include <sha-aarch64.h>
#include <algorithms.h>

#ifdef HAVE_LIBNETTLE

typedef void (*update_func) (void *, size_t, const uint8_t *);
typedef void (*digest_func) (void *, size_t, uint8_t *);
typedef void (*set_key_func) (void *, size_t, const uint8_t *);

struct aarch64_hmac_ctx {
	union {
		struct hmac_sha1_ctx sha1;
		struct hmac_sha224_ctx sha224;
		struct hmac_sha256_ctx sha256;
		struct hmac_sha384_ctx sha384;
		struct hmac_sha512_ctx sha512;
	} ctx;

	void *ctx_ptr;
	gnutls_mac_algorithm_t algo;
	size_t length;
	update_func update;
	digest_func digest;
	set_key_func setkey;
};

static void
aarch64_hmac_sha1_set_key(struct hmac_sha1_ctx *ctx,
			  size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &aarch64_sha1, key_length, key);
}

static void
aarch64_hmac_sha1_update(struct hmac_sha1_ctx *ctx,
			 size_t length, const uint8_t * data)
{
	aarch64_sha1_update(&ctx->state, length, data);
}

static void
aarch64_hmac_sha1_digest(struct hmac_sha1_ctx *ctx,
			 size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &aarch64_sha1, length, digest);
}

static void
aarch64_hmac_sha256_set_key(struct hmac_sha256_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &aarch64_sha256, key_length, key);
}

static void
aarch64_hmac_sha256_update(struct hmac_sha256_ctx *ctx,
			   size_t length, const uint8_t * data)
{
	aarch64_sha256_update(&ctx->state, length, data);
}

static void
aarch64_hmac_sha256_digest(struct hmac_sha256_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &aarch64_sha256, length, digest);
}

static void
aarch64_hmac_sha224_set_key(struct hmac_sha224_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &aarch64_sha224, key_length, key);
}

static void
aarch64_hmac_sha224_digest(struct hmac_sha224_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &aarch64_sha224, length, digest);
}

static void
aarch64_hmac_sha384_set_key(struct hmac_sha384_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &aarch64_sha384, key_length, key);
}

static void
aarch64_hmac_sha384_digest(struct hmac_sha384_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &aarch64_sha384, length, digest);
}

static void
aarch64_hmac_sha512_set_key(struct hmac_sha512_ctx *ctx,
			    size_t key_length, const uint8_t * key)
{
	HMAC_SET_KEY(ctx, &aarch64_sha512, key_length, key);
}

static void
aarch64_hmac_sha512_update(struct hmac_sha512_ctx *ctx,
			   size_t length, const uint8_t * data)
{
	aarch64_sha512_update(&ctx->state, length, data);
}

static void
aarch64_hmac_sha512_digest(struct hmac_sha512_ctx *ctx,
			   size_t length, uint8_t * digest)
{
	HMAC_DIGEST(ctx, &aarch64_sha512, length, digest);
}

static int
_hmac_ctx_init(gnutls_mac_algorithm_t algo, struct aarch64_hmac_ctx *ctx)
{
	switch (algo) {
	case GNUTLS_MAC_SHA1:
		ctx->update = (update_func) aarch64_hmac_sha1_update;
		ctx->digest = (digest_func) aarch64_hmac_sha1_digest;
		ctx->setkey = (set_key_func) aarch64_hmac_sha1_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha1;
		ctx->length = SHA1_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA224:
		ctx->update = (update_func) aarch64_hmac_sha256_update;
		ctx->digest = (digest_func) aarch64_hmac_sha224_digest;
		ctx->setkey = (set_key_func) aarch64_hmac_sha224_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha224;
		ctx->length = SHA224_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA256:
		ctx->update = (update_func) aarch64_hmac_sha256_update;
		ctx->digest = (digest_func) aarch64_hmac_sha256_digest;
		ctx->setkey = (set_key_func) aarch64_hmac_sha256_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha256;
		ctx->length = SHA256_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA384:
		ctx->update = (update_func) aarch64_hmac_sha512_update;
		ctx->digest = (digest_func) aarch64_hmac_sha384_digest;
		ctx->setkey = (set_key_func) aarch64_hmac_sha384_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha384;
		ctx->length = SHA384_DIGEST_SIZE;
		break;
	case GNUTLS_MAC_SHA512:
		ctx->update = (update_func) aarch64_hmac_sha512_update;
		ctx->digest = (digest_func) aarch64_hmac_sha512_digest;
		ctx->setkey = (set_key_func) aarch64_hmac_sha512_set_key;
		ctx->ctx_ptr = &ctx->ctx.sha512;
		ctx->length = SHA512_DIGEST_SIZE;
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return 0;
}


static int wrap_aarch64_hmac_init(gnutls_mac_algorithm_t algo, void **_ctx)
{
	struct aarch64_hmac_ctx *ctx;
	int ret;

	ctx = gnutls_calloc(1, sizeof(struct aarch64_hmac_ctx));
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
wrap_aarch64_hmac_copy(const void *_ctx)
{
	struct aarch64_hmac_ctx *new_ctx;
	const struct aarch64_hmac_ctx *ctx=_ctx;
	ptrdiff_t off = (uint8_t *)ctx->ctx_ptr - (uint8_t *)(&ctx->ctx);

	new_ctx = gnutls_malloc(sizeof(struct aarch64_hmac_ctx));
	if (new_ctx == NULL) {
		gnutls_assert();
		return NULL;
	}

	memcpy(new_ctx, ctx, sizeof(*new_ctx));
	new_ctx->ctx_ptr = (uint8_t *)&new_ctx->ctx + off;

	return new_ctx;
}


static int
wrap_aarch64_hmac_setkey(void *_ctx, const void *key, size_t keylen)
{
	struct aarch64_hmac_ctx *ctx = _ctx;

	ctx->setkey(ctx->ctx_ptr, keylen, key);

	return GNUTLS_E_SUCCESS;
}

static int
wrap_aarch64_hmac_update(void *_ctx, const void *text, size_t textsize)
{
	struct aarch64_hmac_ctx *ctx = _ctx;

	ctx->update(ctx->ctx_ptr, textsize, text);

	return GNUTLS_E_SUCCESS;
}

static int
wrap_aarch64_hmac_output(void *src_ctx, void *digest, size_t digestsize)
{
	struct aarch64_hmac_ctx *ctx;
	ctx = src_ctx;

	if (digestsize < ctx->length) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ctx->digest(ctx->ctx_ptr, digestsize, digest);

	return 0;
}

static void wrap_aarch64_hmac_deinit(void *hd)
{
	gnutls_free(hd);
}

static int wrap_aarch64_hmac_fast(gnutls_mac_algorithm_t algo,
				const void *nonce, size_t nonce_size,
				const void *key, size_t key_size,
				const void *text, size_t text_size,
				void *digest)
{
	struct aarch64_hmac_ctx ctx;
	int ret;

	ret = _hmac_ctx_init(algo, &ctx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ctx.setkey(&ctx, key_size, key);
	ctx.update(&ctx, text_size, text);
	ctx.digest(&ctx, ctx.length, digest);

	zeroize_temp_key(&ctx, sizeof(ctx));

	return 0;
}

const gnutls_crypto_mac_st _gnutls_hmac_sha_aarch64 = {
	.init = wrap_aarch64_hmac_init,
	.setkey = wrap_aarch64_hmac_setkey,
	.setnonce = NULL,
	.hash = wrap_aarch64_hmac_update,
	.output = wrap_aarch64_hmac_output,
	.copy = wrap_aarch64_hmac_copy,
	.deinit = wrap_aarch64_hmac_deinit,
	.fast = wrap_aarch64_hmac_fast,
};

#endif				/* HAVE_LIBNETTLE */
