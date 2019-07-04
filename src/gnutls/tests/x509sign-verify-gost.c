/*
 * Copyright (C) 2016-2017 Free Software Foundation, Inc.
 *
 * Author: Dmitry Eremin-Solenikov
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
#ifndef ENABLE_GOST
	exit(77);
#else
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	test_sig(GNUTLS_PK_GOST_01, GNUTLS_DIG_GOSTR_94, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST256CPA));
	test_sig(GNUTLS_PK_GOST_12_256, GNUTLS_DIG_STREEBOG_256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST256CPA));
	test_sig(GNUTLS_PK_GOST_01, GNUTLS_DIG_GOSTR_94, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST256CPXA));
	test_sig(GNUTLS_PK_GOST_12_256, GNUTLS_DIG_STREEBOG_256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST256CPXA));
	test_sig(GNUTLS_PK_GOST_12_512, GNUTLS_DIG_STREEBOG_512, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST512A));

	gnutls_global_deinit();
#endif
}
