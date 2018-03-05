/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include "utils.h"

#include "../lib/gnutls_int.h"
#include "../lib/hash_int.h"
#include "../lib/debug.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

void doit(void)
{
	unsigned char digest[20];
	int err;

	/* XXX: We need this to fix secure memory. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	err =
	    gnutls_hmac_fast(GNUTLS_MAC_MD5, "keykeykey", 9, "abcdefgh", 8,
			     digest);
	if (err < 0)
		fail("gnutls_hmac_fast(MD5) failed: %d\n", err);
	else {
		if (memcmp(digest, "\x3c\xb0\x9d\x83\x28\x01\xef\xc0"
			   "\x7b\xb3\xaf\x42\x69\xe5\x93\x9a", 16) == 0) {
			if (debug)
				success("gnutls_hmac_fast(MD5) OK\n");
		} else {
			hexprint(digest, 16);
			fail("gnutls_hmac_fast(MD5) failure\n");
		}
	}

	err =
	    gnutls_hmac_fast(GNUTLS_MAC_SHA1, "keykeykey", 9, "abcdefgh",
			     8, digest);
	if (err < 0)
		fail("gnutls_hmac_fast(SHA1) failed: %d\n", err);
	else {
		if (memcmp(digest, "\x58\x93\x7a\x58\xfe\xea\x82\xf8"
			   "\x0e\x64\x62\x01\x40\x2b\x2c\xed\x5d\x54\xc1\xfa",
			   20) == 0) {
			if (debug)
				success("gnutls_hmac_fast(SHA1) OK\n");
		} else {
			hexprint(digest, 20);
			fail("gnutls_hmac_fast(SHA1) failure\n");
		}
	}

	gnutls_global_deinit();
}
