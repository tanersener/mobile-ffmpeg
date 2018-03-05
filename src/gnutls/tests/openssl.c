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

#include <gnutls/openssl.h>

void doit(void)
{
	MD5_CTX c;
	unsigned char md[MD5_DIGEST_LENGTH];

	if (global_init() != 0)
		fail("global_init\n");

	if (!gnutls_check_version(GNUTLS_VERSION))
		success("gnutls_check_version ERROR\n");

	MD5_Init(&c);
	MD5_Update(&c, "abc", 3);
	MD5_Final(&(md[0]), &c);

	if (memcmp(md, "\x90\x01\x50\x98\x3c\xd2\x4f\xb0"
		   "\xd6\x96\x3f\x7d\x28\xe1\x7f\x72", sizeof(md)) != 0) {
		hexprint(md, sizeof(md));
		fail("MD5 failure\n");
	} else if (debug)
		success("MD5 OK\n");

	gnutls_global_deinit();
}
