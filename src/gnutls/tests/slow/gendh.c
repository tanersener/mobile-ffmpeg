/*
 * Copyright (C) 2010, 2012 Free Software Foundation, Inc.
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

void doit(void)
{
	gnutls_dh_params_t dh_params = NULL;
	int rc;

	rc = global_init();
	if (rc)
		fail("global_init\n");

	if (gnutls_dh_params_init(&dh_params) < 0)
		fail("Error in dh parameter initialization\n");

	if (gnutls_dh_params_generate2(dh_params, 2048) < 0)
		fail("Error in prime generation\n");

	gnutls_dh_params_deinit(dh_params);

	gnutls_global_deinit();

	if (debug)
		success("generated DH params OK\n");
}
