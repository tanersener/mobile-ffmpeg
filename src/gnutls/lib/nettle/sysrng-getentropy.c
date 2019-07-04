/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

/* The *BSD getentropy() system random generator. The simplest of all.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <locks.h>
#include <num.h>
#include <errno.h>
#include <rnd-common.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/random.h>
#endif

/* gnulib wants to claim strerror even if it cannot provide it. WTF */
#undef strerror

/* The POSIX (Linux-BSD) randomness gatherer.
 */

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

get_entropy_func _rnd_get_system_entropy = NULL;

static int _rnd_get_system_entropy_simple(void* _rnd, size_t size)
{
	if (getentropy(_rnd, size) < 0) {
		int e = errno;
		gnutls_assert();
		_gnutls_debug_log
			("Failed to use getentropy: %s\n",
					 strerror(e));
		return GNUTLS_E_RANDOM_DEVICE_ERROR;
	}
	return 0;
}

int _rnd_system_entropy_init(void)
{
	_rnd_get_system_entropy = _rnd_get_system_entropy_simple;
	return 0;
}

int _rnd_system_entropy_check(void)
{
	return 0;
}

void _rnd_system_entropy_deinit(void)
{
	return;
}

