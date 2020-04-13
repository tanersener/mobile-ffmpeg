/*
 * Copyright (C) 2017 Red Hat
 * Copyright (C) 1995-2017 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 * Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, August 1995.
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
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
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
#include <stdlib.h>
#include <rnd-common.h>

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

struct r48_rand_data {
	unsigned short int __x[3];	/* Current state.  */
	unsigned short int __old_x[3];	/* Old state.  */
	unsigned short int __c;	/* Additive const. in congruential formula.  */
	unsigned short int __init;	/* Flag for initializing.  */
	__extension__ unsigned long long int __a;	/* Factor in congruential
							   formula.  */
};

#ifdef __clang__
__attribute__((no_sanitize("integer")))
#endif
static int
__r48_rand_iterate(unsigned short int xsubi[3], struct r48_rand_data *buffer)
{
	uint64_t X;
	uint64_t result;

	/* Initialize buffer, if not yet done.  */
	if (unlikely(!buffer->__init)) {
		buffer->__a = 0x5deece66dull;
		buffer->__c = 0xb;
		buffer->__init = 1;
	}

	/* Do the real work.  We choose a data type which contains at least
	   48 bits.  Because we compute the modulus it does not care how
	   many bits really are computed.  */

	X = (uint64_t) xsubi[2] << 32 | (uint32_t) xsubi[1] << 16 | xsubi[0];

	result = X * buffer->__a + buffer->__c;

	xsubi[0] = result & 0xffff;
	xsubi[1] = (result >> 16) & 0xffff;
	xsubi[2] = (result >> 32) & 0xffff;

	return 0;
}

#ifdef __clang__
__attribute__((no_sanitize("integer")))
#elif defined __GNUC__
__attribute__((no_sanitize("shift-base")))
#endif
static int
r48_r(unsigned short int xsubi[3], struct r48_rand_data *buffer,
      long int *result)
{
	/* Compute next state.  */
	if (__r48_rand_iterate(xsubi, buffer) < 0)
		return -1;

	/* Store the result.  */
	*result = (int32_t) ((xsubi[2] << 16) | xsubi[1]);

	return 0;
}

static int r48(struct r48_rand_data *buffer, long int *result)
{
	return r48_r(buffer->__x, buffer, result);
}

/* This is a dummy random generator intended to be reproducible
 * for use in fuzzying targets.
 */

static int _rngfuzz_init(void **_ctx)
{
	*_ctx = calloc(1, sizeof(struct r48_rand_data));

	return 0;
}

static int _rngfuzz_rnd(void *_ctx, int level, void *buffer, size_t length)
{
	struct r48_rand_data *ctx = _ctx;
	uint8_t *p = buffer;
	long r;
	unsigned i;

	memset(ctx, 0, sizeof(*ctx));

	for (i = 0; i < length; i++) {
		r48(ctx, &r);
		p[i] = r;
	}
	return 0;
}

static void _rngfuzz_deinit(void *_ctx)
{
	struct r48_rand_data *ctx = _ctx;

	free(ctx);
}

static void _rngfuzz_refresh(void *_ctx)
{
	/* this is predictable RNG. Don't refresh */
	return;
}

gnutls_crypto_rnd_st _gnutls_fuzz_rnd_ops = {
	.init = _rngfuzz_init,
	.deinit = _rngfuzz_deinit,
	.rnd = _rngfuzz_rnd,
	.rnd_refresh = _rngfuzz_refresh,
	.self_test = NULL,
};

#endif /* FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION */
