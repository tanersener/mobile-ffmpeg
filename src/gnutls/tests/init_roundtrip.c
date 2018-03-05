/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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

/* See <http://bugs.gentoo.org/272388>. */

void doit(void)
{
	int res;

	res = global_init();
	if (res != 0)
		fail("global_init\n");

	gnutls_global_deinit();

	res = global_init();
	if (res != 0)
		fail("global_init2\n");

	gnutls_global_deinit();

	if (debug)
		success("init-deinit round-trip success\n");
}
