/*
 * Copyright (C) 2011-2016 Free Software Foundation, Inc.
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

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#include <nettle/sha.h>
#include <nettle/macros.h>
#include <nettle/nettle-meta.h>
#include <sha-aarch64.h>
#include <aarch64-common.h>

void sha1_block_data_order(void *c, const void *p, size_t len);
void sha256_block_data_order(void *c, const void *p, size_t len);
void sha512_block_data_order(void *c, const void *p, size_t len);

typedef void (*update_func) (void *, size_t, const uint8_t *);
typedef void (*digest_func) (void *, size_t, uint8_t *);
typedef void (*set_key_func) (void *, size_t, const uint8_t *);
typedef void (*init_func) (void *);

struct aarch64_hash_ctx {
	union {
		struct sha1_ctx sha1;
		struct sha224_ctx sha224;
		struct sha256_ctx sha256;
		struct sha384_ctx sha384;
		struct sha512_ctx sha512;
	} ctx;
	void *ctx_ptr;
	gnutls_digest_algorithm_t algo;
	size_t length;
	update_func update;
	digest_func digest;
	init_func init;
};

static int
wrap_aarch64_hash_update(void *_ctx, const void *text, size_t textsize)
{
	struct aarch64_hash_ctx *ctx = _ctx;

	ctx->update(ctx->ctx_ptr, textsize, text);

	return GNUTLS_E_SUCCESS;
}

static void wrap_aarch64_hash_deinit(void *hd)
{
	gnutls_free(hd);
}

void aarch64_sha1_update(struct sha1_ctx *ctx, size_t length,
		     const uint8_t * data)
{
	struct {
		uint32_t h0, h1, h2, h3, h4;
		uint32_t Nl, Nh;
		uint32_t data[16];
		unsigned int num;
	} octx;
	size_t res;
	unsigned t2, i;

	if ((res = ctx->index)) {
		res = SHA1_DATA_SIZE - res;
		if (length < res)
			res = length;
		sha1_update(ctx, res, data);
		data += res;
		length -= res;
	}

	octx.h0 = ctx->state[0];
	octx.h1 = ctx->state[1];
	octx.h2 = ctx->state[2];
	octx.h3 = ctx->state[3];
	octx.h4 = ctx->state[4];

	memcpy(octx.data, ctx->block, SHA1_DATA_SIZE);
	octx.num = ctx->index;

	res = length % SHA1_DATA_SIZE;
	length -= res;

	if (length > 0) {

		t2 = length / SHA1_DATA_SIZE;

		sha1_block_data_order(&octx, data, t2);

		for (i=0;i<t2;i++)
			ctx->count++;
		data += length;
	}

	ctx->state[0] = octx.h0;
	ctx->state[1] = octx.h1;
	ctx->state[2] = octx.h2;
	ctx->state[3] = octx.h3;
	ctx->state[4] = octx.h4;

	memcpy(ctx->block, octx.data, octx.num);
	ctx->index = octx.num;

	if (res > 0) {
		sha1_update(ctx, res, data);
	}

}

void aarch64_sha256_update(struct sha256_ctx *ctx, size_t length,
		     const uint8_t * data)
{
	struct {
		uint32_t h[8];
		uint32_t Nl, Nh;
		uint32_t data[16];
		unsigned int num;
		unsigned md_len;
	} octx;
	size_t res;
	unsigned t2, i;

	if ((res = ctx->index)) {
		res = SHA256_DATA_SIZE - res;
		if (length < res)
			res = length;
		sha256_update(ctx, res, data);
		data += res;
		length -= res;
	}

	memcpy(octx.h, ctx->state, sizeof(octx.h));
	memcpy(octx.data, ctx->block, SHA256_DATA_SIZE);
	octx.num = ctx->index;

	res = length % SHA256_DATA_SIZE;
	length -= res;

	if (length > 0) {
		t2 = length / SHA1_DATA_SIZE;
		sha256_block_data_order(&octx, data, t2);
		
		for (i=0;i<t2;i++)
			ctx->count++;
		data += length;
	}

	memcpy(ctx->state, octx.h, sizeof(octx.h));

	memcpy(ctx->block, octx.data, octx.num);
	ctx->index = octx.num;

	if (res > 0) {
		sha256_update(ctx, res, data);
	}
}

void aarch64_sha512_update(struct sha512_ctx *ctx, size_t length,
		     const uint8_t * data)
{
	struct {
		uint64_t h[8];
		uint64_t Nl, Nh;
		union {
			uint64_t d[16];
			uint8_t p[16*8];
		} u;
		unsigned int num;
		unsigned md_len;
	} octx;
	size_t res;
	unsigned t2, i;

	if ((res = ctx->index)) {
		res = SHA512_DATA_SIZE - res;
		if (length < res)
			res = length;
		sha512_update(ctx, res, data);
		data += res;
		length -= res;
	}

	memcpy(octx.h, ctx->state, sizeof(octx.h));
	memcpy(octx.u.p, ctx->block, SHA512_DATA_SIZE);
	octx.num = ctx->index;

	res = length % SHA512_DATA_SIZE;
	length -= res;

	if (length > 0) {
		t2 = length / SHA512_DATA_SIZE;
		sha512_block_data_order(&octx, data, t2);
		
		for (i=0;i<t2;i++)
			MD_INCR(ctx);
		data += length;
	}

	memcpy(ctx->state, octx.h, sizeof(octx.h));

	memcpy(ctx->block, octx.u.p, octx.num);
	ctx->index = octx.num;

	if (res > 0) {
		sha512_update(ctx, res, data);
	}
}

static int _ctx_init(gnutls_digest_algorithm_t algo,
		     struct aarch64_hash_ctx *ctx)
{
	switch (algo) {
	case GNUTLS_DIG_SHA1:
		sha1_init(&ctx->ctx.sha1);
		ctx->update = (update_func) aarch64_sha1_update;
		ctx->digest = (digest_func) sha1_digest;
		ctx->init = (init_func) sha1_init;
		ctx->ctx_ptr = &ctx->ctx.sha1;
		ctx->length = SHA1_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA224:
		sha224_init(&ctx->ctx.sha224);
		ctx->update = (update_func) aarch64_sha256_update;
		ctx->digest = (digest_func) sha224_digest;
		ctx->init = (init_func) sha224_init;
		ctx->ctx_ptr = &ctx->ctx.sha224;
		ctx->length = SHA224_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA256:
		sha256_init(&ctx->ctx.sha256);
		ctx->update = (update_func) aarch64_sha256_update;
		ctx->digest = (digest_func) sha256_digest;
		ctx->init = (init_func) sha256_init;
		ctx->ctx_ptr = &ctx->ctx.sha256;
		ctx->length = SHA256_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA384:
		sha384_init(&ctx->ctx.sha384);
		ctx->update = (update_func) aarch64_sha512_update;
		ctx->digest = (digest_func) sha512_digest;
		ctx->init = (init_func) sha384_init;
		ctx->ctx_ptr = &ctx->ctx.sha384;
		ctx->length = SHA384_DIGEST_SIZE;
		break;
	case GNUTLS_DIG_SHA512:
		sha512_init(&ctx->ctx.sha512);
		ctx->update = (update_func) aarch64_sha512_update;
		ctx->digest = (digest_func) sha512_digest;
		ctx->init = (init_func) sha512_init;
		ctx->ctx_ptr = &ctx->ctx.sha512;
		ctx->length = SHA512_DIGEST_SIZE;
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return 0;
}


static int wrap_aarch64_hash_init(gnutls_digest_algorithm_t algo, void **_ctx)
{
	struct aarch64_hash_ctx *ctx;
	int ret;

	ctx = gnutls_malloc(sizeof(struct aarch64_hash_ctx));
	if (ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ctx->algo = algo;

	if ((ret = _ctx_init(algo, ctx)) < 0) {
		gnutls_assert();
		return ret;
	}

	*_ctx = ctx;

	return 0;
}

static int
wrap_aarch64_hash_output(void *src_ctx, void *digest, size_t digestsize)
{
	struct aarch64_hash_ctx *ctx;
	ctx = src_ctx;

	if (digestsize < ctx->length)
		return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);

	ctx->digest(ctx->ctx_ptr, digestsize, digest);

	return 0;
}

static int wrap_aarch64_hash_fast(gnutls_digest_algorithm_t algo,
				 const void *text, size_t text_size,
				 void *digest)
{
	struct aarch64_hash_ctx ctx;
	int ret;

	ret = _ctx_init(algo, &ctx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ctx.update(&ctx, text_size, text);
	ctx.digest(&ctx, ctx.length, digest);

	return 0;
}

const struct nettle_hash aarch64_sha1 =
NN_HASH(sha1, aarch64_sha1_update, sha1_digest, SHA1);
const struct nettle_hash aarch64_sha224 =
NN_HASH(sha224, aarch64_sha256_update, sha224_digest, SHA224);
const struct nettle_hash aarch64_sha256 =
NN_HASH(sha256, aarch64_sha256_update, sha256_digest, SHA256);

const struct nettle_hash aarch64_sha384 =
NN_HASH(sha384, aarch64_sha512_update, sha384_digest, SHA384);
const struct nettle_hash aarch64_sha512 =
NN_HASH(sha512, aarch64_sha512_update, sha512_digest, SHA512);

const gnutls_crypto_digest_st _gnutls_sha_aarch64 = {
	.init = wrap_aarch64_hash_init,
	.hash = wrap_aarch64_hash_update,
	.output = wrap_aarch64_hash_output,
	.deinit = wrap_aarch64_hash_deinit,
	.fast = wrap_aarch64_hash_fast,
};
