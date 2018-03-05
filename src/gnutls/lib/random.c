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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* This file handles all the internal functions that cope with random data.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <random.h>
#include "locks.h"
#include <fips.h>

void *gnutls_rnd_ctx;
GNUTLS_STATIC_MUTEX(gnutls_rnd_init_mutex);

#ifdef HAVE_STDATOMIC_H
static atomic_uint rnd_initialized = 0;

inline static int _gnutls_rnd_init(void)
{
	if (unlikely(!rnd_initialized)) {
		if (_gnutls_rnd_ops.init == NULL) {
			rnd_initialized = 1;
			return 0;
		}

		GNUTLS_STATIC_MUTEX_LOCK(gnutls_rnd_init_mutex);
		if (!rnd_initialized) {
			if (_gnutls_rnd_ops.init(&gnutls_rnd_ctx) < 0) {
				gnutls_assert();
				GNUTLS_STATIC_MUTEX_UNLOCK(gnutls_rnd_init_mutex);
				return GNUTLS_E_RANDOM_FAILED;
			}
			rnd_initialized = 1;
		}
		GNUTLS_STATIC_MUTEX_UNLOCK(gnutls_rnd_init_mutex);
	}
	return 0;
}
#else
static unsigned rnd_initialized = 0;

inline static int _gnutls_rnd_init(void)
{
	GNUTLS_STATIC_MUTEX_LOCK(gnutls_rnd_init_mutex);
	if (unlikely(!rnd_initialized)) {
		if (_gnutls_rnd_ops.init == NULL) {
			rnd_initialized = 1;
			GNUTLS_STATIC_MUTEX_UNLOCK(gnutls_rnd_init_mutex);
			return 0;
		}

		if (_gnutls_rnd_ops.init(&gnutls_rnd_ctx) < 0) {
			gnutls_assert();
			GNUTLS_STATIC_MUTEX_UNLOCK(gnutls_rnd_init_mutex);
			return GNUTLS_E_RANDOM_FAILED;
		}
		rnd_initialized = 1;
	}
	GNUTLS_STATIC_MUTEX_UNLOCK(gnutls_rnd_init_mutex);
	return 0;
}
#endif

int _gnutls_rnd_preinit(void)
{
	int ret;

#ifdef ENABLE_FIPS140
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
	if (rnd_initialized && _gnutls_rnd_ops.deinit != NULL) {
		_gnutls_rnd_ops.deinit(gnutls_rnd_ctx);
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
