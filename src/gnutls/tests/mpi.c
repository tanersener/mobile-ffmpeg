/*
 * Copyright (C) 2007-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Simon Josefsson
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

#include "utils.h"
#include "../lib/gnutls_int.h"
#include "../lib/mpi.h"
#include "../lib/errors.h"
#include "../lib/debug.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

void doit(void)
{
	bigint_t n1, n2, n3, n4;
	int ret;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(99);

	ret = _gnutls_mpi_init_multi(&n1, &n2, &n3, &n4, NULL);
	if (ret < 0)
		fail("mpi_new failed\n");

	ret = _gnutls_mpi_set_ui(n2, 2);
	if (ret < 0)
		fail("mpi_set_ui failed\n");

	ret = _gnutls_mpi_set_ui(n3, 5);
	if (ret < 0)
		fail("mpi_set_ui failed\n");

	ret = _gnutls_mpi_set_ui(n1, 12498924);
	if (ret < 0)
		fail("mpi_set_ui failed\n");

	ret = _gnutls_mpi_addm(n4, n1, n3, n2);
	if (ret < 0)
		fail("mpi_set_ui failed\n");

	if (_gnutls_mpi_cmp_ui(n4, 0) != 0
	    && _gnutls_mpi_cmp_ui(n4, 1) != 0)
		fail("mpi_cmp_ui failed\n");

	_gnutls_mpi_release(&n1);
	_gnutls_mpi_release(&n2);
	_gnutls_mpi_release(&n3);
	_gnutls_mpi_release(&n4);

	gnutls_global_deinit();

	if (debug)
		success("mpi ops ok\n");
}
