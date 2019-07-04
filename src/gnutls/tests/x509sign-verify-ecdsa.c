/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <sys/types.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <unistd.h>
#endif
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"

#include "x509sign-verify-common.h"

void doit(void)
{
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA1, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1));
	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1));
	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP384R1));
	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP521R1));

	gnutls_global_deinit();
}
