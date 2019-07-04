/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Red Hat, Inc.
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

#include "gnutls_int.h"
#include "errors.h"
#include <locks.h>
#include <num.h>
#include <nettle/chacha.h>
#include <rnd-common.h>
#include <system.h>
#include <atfork.h>
#include <errno.h>
#include <minmax.h>

#define PRNG_KEY_SIZE CHACHA_KEY_SIZE

/* For a high level description see the documentation and
 * the 'Random number generation' section of chapter
 * 'Using GnuTLS as a cryptographic library'.
 */

/* We have two "refresh" operations for the PRNG:
 *  re-seed: the random generator obtains a new key from the system or another PRNG
 *           (occurs when a time or data-based limit is reached for the GNUTLS_RND_RANDOM
 *            and GNUTLS_RND_KEY levels and data-based for the nonce level)
 *  re-key:  the random generator obtains a new key by utilizing its own output.
 *           This only happens for the GNUTLS_RND_KEY level, on every operation.
 */

/* after this number of bytes PRNG will rekey using the system RNG */
static const unsigned prng_reseed_limits[] = {
	[GNUTLS_RND_NONCE] = 16*1024*1024, /* 16 MB - we re-seed using the GNUTLS_RND_RANDOM output */
	[GNUTLS_RND_RANDOM] = 2*1024*1024, /* 2MB - we re-seed by time as well */
	[GNUTLS_RND_KEY] = 2*1024*1024 /* same as GNUTLS_RND_RANDOM - but we re-key on every operation */
};

static const time_t prng_reseed_time[] = {
	[GNUTLS_RND_NONCE] = 14400, /* 4 hours */
	[GNUTLS_RND_RANDOM] = 7200, /* 2 hours */
	[GNUTLS_RND_KEY] = 7200 /* same as RANDOM */
};

struct prng_ctx_st {
	struct chacha_ctx ctx;
	size_t counter;
	unsigned int forkid;
	time_t last_reseed;
};

struct generators_ctx_st {
	struct prng_ctx_st nonce;  /* GNUTLS_RND_NONCE */
	struct prng_ctx_st normal; /* GNUTLS_RND_RANDOM, GNUTLS_RND_KEY */
};


static void wrap_nettle_rnd_deinit(void *_ctx)
{
	gnutls_free(_ctx);
}

/* Initializes the nonce level random generator.
 *
 * the @new_key must be provided.
 *
 * @init must be non zero on first initialization, and
 * zero on any subsequent reinitializations.
 */
static int single_prng_init(struct prng_ctx_st *ctx,
			    uint8_t new_key[PRNG_KEY_SIZE],
			    unsigned new_key_size,
			    unsigned init)
{
	uint8_t nonce[CHACHA_NONCE_SIZE];

	memset(nonce, 0, sizeof(nonce)); /* to prevent valgrind from whinning */

	if (init == 0) {
		/* use the previous key to generate IV as well */
		chacha_crypt(&ctx->ctx, sizeof(nonce), nonce, nonce);

		/* Add key continuity by XORing the new key with data generated
		 * from the old key */
		chacha_crypt(&ctx->ctx, new_key_size, new_key, new_key);
	} else {
		struct timespec now; /* current time */

		ctx->forkid = _gnutls_get_forkid();

		gnutls_gettime(&now);
		memcpy(nonce, &now, MIN(sizeof(nonce), sizeof(now)));
		ctx->last_reseed = now.tv_sec;
	}

	chacha_set_key(&ctx->ctx, new_key);
	chacha_set_nonce(&ctx->ctx, nonce);

	zeroize_key(new_key, new_key_size);

	ctx->counter = 0;

	return 0;
}

/* API functions */

static int wrap_nettle_rnd_init(void **_ctx)
{
	int ret;
	uint8_t new_key[PRNG_KEY_SIZE*2];
	struct generators_ctx_st *ctx;

	ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	/* initialize the nonce RNG */
	ret = _rnd_get_system_entropy(new_key, sizeof(new_key));
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret = single_prng_init(&ctx->nonce, new_key, PRNG_KEY_SIZE, 1);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	/* initialize the random/key RNG */
	ret = single_prng_init(&ctx->normal, new_key+PRNG_KEY_SIZE, PRNG_KEY_SIZE, 1);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	*_ctx = ctx;

	return 0;
 fail:
	gnutls_free(ctx);
	return ret;
}

static int
wrap_nettle_rnd(void *_ctx, int level, void *data, size_t datasize)
{
	struct generators_ctx_st *ctx = _ctx;
	struct prng_ctx_st *prng_ctx;
	int ret, reseed = 0;
	uint8_t new_key[PRNG_KEY_SIZE];
	time_t now;

	if (level == GNUTLS_RND_RANDOM || level == GNUTLS_RND_KEY)
		prng_ctx = &ctx->normal;
	else if (level == GNUTLS_RND_NONCE)
		prng_ctx = &ctx->nonce;
	else
		return gnutls_assert_val(GNUTLS_E_RANDOM_FAILED);

	/* Two reasons for this memset():
	 *  1. avoid getting filled with valgrind warnings
	 *  2. avoid a cipher/PRNG failure to expose stack data
	 */
	memset(data, 0, datasize);

	now = gnutls_time(0);

	/* We re-seed based on time in addition to output data. That is,
	 * to prevent a temporal state compromise to become permanent for low
	 * traffic sites */
	if (unlikely(_gnutls_detect_fork(prng_ctx->forkid))) {
		reseed = 1;
	} else {
		if (now > prng_ctx->last_reseed + prng_reseed_time[level])
			reseed = 1;
	}

	if (reseed != 0 || prng_ctx->counter > prng_reseed_limits[level]) {
		if (level == GNUTLS_RND_NONCE) {
			ret = wrap_nettle_rnd(_ctx, GNUTLS_RND_RANDOM, new_key, sizeof(new_key));
		} else {

			/* we also use the system entropy to reduce the impact
			 * of a temporal state compromise for these two levels. */
			ret = _rnd_get_system_entropy(new_key, sizeof(new_key));
		}

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = single_prng_init(prng_ctx, new_key, sizeof(new_key), 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		prng_ctx->last_reseed = now;
		prng_ctx->forkid = _gnutls_get_forkid();
	}

	chacha_crypt(&prng_ctx->ctx, datasize, data, data);
	prng_ctx->counter += datasize;

	if (level == GNUTLS_RND_KEY) { /* prevent backtracking */
		ret = wrap_nettle_rnd(_ctx, GNUTLS_RND_RANDOM, new_key, sizeof(new_key));
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = single_prng_init(prng_ctx, new_key, sizeof(new_key), 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = 0;

cleanup:
	return ret;
}

static void wrap_nettle_rnd_refresh(void *_ctx)
{
	struct generators_ctx_st *ctx = _ctx;
	char tmp;

	/* force reseed */
	ctx->nonce.counter = prng_reseed_limits[GNUTLS_RND_NONCE]+1;
	ctx->normal.counter = prng_reseed_limits[GNUTLS_RND_RANDOM]+1;

	wrap_nettle_rnd(_ctx, GNUTLS_RND_NONCE, &tmp, 1);
	wrap_nettle_rnd(_ctx, GNUTLS_RND_RANDOM, &tmp, 1);

	return;
}

int crypto_rnd_prio = INT_MAX;

gnutls_crypto_rnd_st _gnutls_rnd_ops = {
	.init = wrap_nettle_rnd_init,
	.deinit = wrap_nettle_rnd_deinit,
	.rnd = wrap_nettle_rnd,
	.rnd_refresh = wrap_nettle_rnd_refresh,
	.self_test = NULL,
};
