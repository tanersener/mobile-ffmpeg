/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
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

/* This file handles all the internal functions that cope with random data.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <random.h>
#include "locks.h"
#include <fips.h>

#include "gthreads.h"

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
extern gnutls_crypto_rnd_st _gnutls_fuzz_rnd_ops;
#endif

/* Per thread context of random generator, and a flag to indicate initialization */
static _Thread_local void *gnutls_rnd_ctx;
static _Thread_local unsigned rnd_initialized = 0;

struct rnd_ctx_list_st {
	void *ctx;
	struct rnd_ctx_list_st *next;
};

/* A global list of all allocated contexts - to be
 * used during deinitialization. */
GNUTLS_STATIC_MUTEX(gnutls_rnd_ctx_list_mutex);
static struct rnd_ctx_list_st *head = NULL;

static int append(void *ctx)
{
	struct rnd_ctx_list_st *e = gnutls_malloc(sizeof(*e));

	if (e == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	e->ctx = ctx;
	e->next = head;

	head = e;

	return 0;
}

inline static int _gnutls_rnd_init(void)
{
	if (unlikely(!rnd_initialized)) {
		int ret;

		if (_gnutls_rnd_ops.init == NULL) {
			rnd_initialized = 1;
			return 0;
		}

		if (_gnutls_rnd_ops.init(&gnutls_rnd_ctx) < 0) {
			gnutls_assert();
			return GNUTLS_E_RANDOM_FAILED;
		}

		GNUTLS_STATIC_MUTEX_LOCK(gnutls_rnd_ctx_list_mutex);
		ret = append(gnutls_rnd_ctx);
		GNUTLS_STATIC_MUTEX_UNLOCK(gnutls_rnd_ctx_list_mutex);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_rnd_ops.deinit(gnutls_rnd_ctx);
			return ret;
		}

		rnd_initialized = 1;
	}
	return 0;
}

int _gnutls_rnd_preinit(void)
{
	int ret;

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
# warning Insecure PRNG is enabled
	ret = gnutls_crypto_rnd_register(100, &_gnutls_fuzz_rnd_ops);
	if (ret < 0)
		return ret;

#elif defined(ENABLE_FIPS140)
	/* The FIPS140 random generator is only enabled when we are compiled
	 * with FIPS support, _and_ the system requires FIPS140.
	 */
	if (_gnutls_fips_mode_enabled() == 1) {
		ret = gnutls_crypto_rnd_register(100, &_gnutls_fips_rnd_ops);
		if (ret < 0)
			return ret;
	}
#endif

	ret = _rnd_system_entropy_init();
	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_RANDOM_FAILED;
	}

	return 0;
}

void _gnutls_rnd_deinit(void)
{
	if (_gnutls_rnd_ops.deinit != NULL) {
		struct rnd_ctx_list_st *e = head, *next;

		while(e != NULL) {
			next = e->next;
			_gnutls_rnd_ops.deinit(e->ctx);
			gnutls_free(e);
			e = next;
		}
		head = NULL;
	}

	rnd_initialized = 0;
	_rnd_system_entropy_deinit();

	return;
}

/**
 * gnutls_rnd:
 * @level: a security level
 * @data: place to store random bytes
 * @len: The requested size
 *
 * This function will generate random data and store it to output
 * buffer. The value of @level should be one of %GNUTLS_RND_NONCE,
 * %GNUTLS_RND_RANDOM and %GNUTLS_RND_KEY. See the manual and
 * %gnutls_rnd_level_t for detailed information.
 *
 * This function is thread-safe and also fork-safe.
 *
 * Returns: Zero on success, or a negative error code on error.
 *
 * Since: 2.12.0
 **/
int gnutls_rnd(gnutls_rnd_level_t level, void *data, size_t len)
{
	int ret;
	FAIL_IF_LIB_ERROR;

	if (unlikely((ret=_gnutls_rnd_init()) < 0))
		return gnutls_assert_val(ret);

	if (likely(len > 0)) {
		return _gnutls_rnd_ops.rnd(gnutls_rnd_ctx, level, data,
					   len);
	}
	return 0;
}

/**
 * gnutls_rnd_refresh:
 *
 * This function refreshes the random generator state.
 * That is the current precise time, CPU usage, and
 * other values are input into its state.
 *
 * On a slower rate input from /dev/urandom is mixed too.
 *
 * Since: 3.1.7
 **/
void gnutls_rnd_refresh(void)
{
	if (rnd_initialized && _gnutls_rnd_ops.rnd_refresh)
		_gnutls_rnd_ops.rnd_refresh(gnutls_rnd_ctx);
}
