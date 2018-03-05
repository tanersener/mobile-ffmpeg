/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include "utils.h"

#if defined(ENABLE_FIPS140) || !defined(__linux__) || !defined(__GNUC__)

void doit(void)
{
	exit(77);
}

#else

static int _rnd_called = 0;

/* Tests whether gnutls_rnd() is called during gnutls library initialization.
 * Normally it shouldn't be called to prevent any blocking due to getrandom()
 * calls.
 */
int __attribute__ ((visibility ("protected")))
gnutls_rnd(gnutls_rnd_level_t level, void *data, size_t len)
{
	_rnd_called = 1;

	memset(data, 0xff, len);
	return 0;
}

void doit(void)
{
	global_init();

	if (_rnd_called != 0)
		fail("gnutls_rnd was called during gnutls_global_init()!\n");

	gnutls_global_deinit();
}
#endif				/* _WIN32 */
