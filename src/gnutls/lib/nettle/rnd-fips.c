/*
 * Copyright (C) 2013 Red Hat
 *
 * This file is part of GnuTLS.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <drbg-aes.h>
#include <fips.h>

#include "gnutls_int.h"
#include "errors.h"
#include <nettle/aes.h>
#include <nettle/memxor.h>
#include <locks.h>
#include <atfork.h>
#include <rnd-common.h>

/* This provides a random generator for gnutls. It uses
 * three instances of the DRBG-AES-CTR generator, one for
 * each level of randomness. It uses /dev/urandom for their
 * seeding.
 */

#define RND_LOCK if (gnutls_mutex_lock(&rnd_mutex)!=0) abort()
#define RND_UNLOCK if (gnutls_mutex_unlock(&rnd_mutex)!=0) abort()

static void *rnd_mutex;

struct fips_ctx {
	struct drbg_aes_ctx nonce_context;
	struct drbg_aes_ctx normal_context;
	struct drbg_aes_ctx strong_context;
	unsigned int forkid;
};

static int _rngfips_ctx_reinit(struct fips_ctx *fctx);
static int _rngfips_ctx_init(struct fips_ctx *fctx);
static int drbg_reseed(struct drbg_aes_ctx *ctx);

static int get_random(struct drbg_aes_ctx *ctx, struct fips_ctx *fctx,
		      void *buffer, size_t length)
{
	int ret;

	if ( _gnutls_detect_fork(fctx->forkid) != 0) {
		ret = _rngfips_ctx_reinit(fctx);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	if (ctx->reseed_counter > DRBG_AES_RESEED_TIME) {
		ret = drbg_reseed(ctx);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	ret = drbg_aes_random(ctx, length, buffer);
	if (ret == 0)
		return gnutls_assert_val(GNUTLS_E_RANDOM_FAILED);

	return 0;
}

#define PSTRING "gnutls-rng"
#define PSTRING_SIZE (sizeof(PSTRING)-1)
static int drbg_init(struct drbg_aes_ctx *ctx)
{
	uint8_t buffer[DRBG_AES_SEED_SIZE];
	int ret;

	/* Get a key from the standard RNG or from the entropy source.  */
	ret = _rnd_get_system_entropy(buffer, sizeof(buffer));
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = drbg_aes_init(ctx, sizeof(buffer), buffer, PSTRING_SIZE, (void*)PSTRING);
	if (ret == 0)
		return gnutls_assert_val(GNUTLS_E_RANDOM_FAILED);

	zeroize_key(buffer, sizeof(buffer));

	return 0;
}

/* Reseed a generator. */
static int drbg_reseed(struct drbg_aes_ctx *ctx)
{
	uint8_t buffer[DRBG_AES_SEED_SIZE];
	int ret;

	/* The other two generators are seeded from /dev/random.  */
	ret = _rnd_get_system_entropy(buffer, sizeof(buffer));
	if (ret < 0)
		return gnutls_assert_val(ret);

	drbg_aes_reseed(ctx, sizeof(buffer), buffer, 0, NULL);

	return 0;
}

static int _rngfips_ctx_init(struct fips_ctx *fctx)
{
	int ret;

	/* strong */
	ret = drbg_init(&fctx->strong_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* normal */
	ret = drbg_init(&fctx->normal_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* nonce */
	ret = drbg_init(&fctx->nonce_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	fctx->forkid = _gnutls_get_forkid();

	return 0;
}

static int _rngfips_ctx_reinit(struct fips_ctx *fctx)
{
	int ret;

	/* strong */
	ret = drbg_reseed(&fctx->strong_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* normal */
	ret = drbg_reseed(&fctx->normal_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* nonce */
	ret = drbg_reseed(&fctx->nonce_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	fctx->forkid = _gnutls_get_forkid();

	return 0;
}

/* Initialize this random subsystem. */
static int _rngfips_init(void **_ctx)
{
/* Basic initialization is required to initialize mutexes and
   do a few checks on the implementation.  */
	struct fips_ctx *ctx;
	int ret;

	ctx = gnutls_calloc(1, sizeof(*ctx));
	if (ctx == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ret = gnutls_mutex_init(&rnd_mutex);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _rngfips_ctx_init(ctx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	*_ctx = ctx;

	return 0;
}

static int _rngfips_rnd(void *_ctx, int level, void *buffer, size_t length)
{
	struct fips_ctx *ctx = _ctx;
	int ret;

	RND_LOCK;
	switch (level) {
	case GNUTLS_RND_RANDOM:
		ret = get_random(&ctx->normal_context, ctx, buffer, length);
		break;
	case GNUTLS_RND_KEY:
		ret = get_random(&ctx->strong_context, ctx, buffer, length);
		break;
	default:
		ret = get_random(&ctx->nonce_context, ctx, buffer, length);
		break;
	}
	RND_UNLOCK;

	return ret;
}

static void _rngfips_deinit(void *_ctx)
{
	struct fips_ctx *ctx = _ctx;

	gnutls_mutex_deinit(&rnd_mutex);
	rnd_mutex = NULL;

	zeroize_key(ctx, sizeof(*ctx));
	free(ctx);
}

static void _rngfips_refresh(void *_ctx)
{
	/* this is predictable RNG. Don't refresh */
	return;
}

static int selftest_kat(void)
{
	int ret;

	RND_LOCK;
	ret = drbg_aes_self_test();
	RND_UNLOCK;

	if (ret == 0) {
		_gnutls_debug_log("DRBG-AES self test failed\n");
		return gnutls_assert_val(GNUTLS_E_RANDOM_FAILED);
	} else
		_gnutls_debug_log("DRBG-AES self test succeeded\n");

	return 0;
}

gnutls_crypto_rnd_st _gnutls_fips_rnd_ops = {
	.init = _rngfips_init,
	.deinit = _rngfips_deinit,
	.rnd = _rngfips_rnd,
	.rnd_refresh = _rngfips_refresh,
	.self_test = selftest_kat,
};

