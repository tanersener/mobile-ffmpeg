/*
 * Copyright (C) 2017 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>

#include "utils.h"

#define CHECK_SECURE_SIG(sig) \
	ret = gnutls_sign_is_secure2(sig, 0); \
	if (ret == 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	} \
	ret = gnutls_sign_is_secure(sig); \
	if (ret == 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	}

#define CHECK_INSECURE_SIG(sig) \
	ret = gnutls_sign_is_secure2(sig, 0); \
	if (ret != 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	} \
	ret = gnutls_sign_is_secure2(sig, GNUTLS_SIGN_FLAG_SECURE_FOR_CERTS); \
	if (ret != 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	} \
	ret = gnutls_sign_is_secure(sig); \
	if (ret != 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	}

#ifndef ALLOW_SHA1
#define CHECK_INSECURE_FOR_CERTS_SIG(sig) \
	ret = gnutls_sign_is_secure2(sig, 0); \
	if (ret == 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	} \
	ret = gnutls_sign_is_secure2(sig, GNUTLS_SIGN_FLAG_SECURE_FOR_CERTS); \
	if (ret != 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	} \
	ret = gnutls_sign_is_secure(sig); \
	if (ret == 0) { \
		fail("error testing %d/%s\n", sig, gnutls_sign_get_name(sig)); \
	}
#else
#define CHECK_INSECURE_FOR_CERTS_SIG(sig)
#endif

void doit(void)
{
	int ret;
	unsigned i;

	CHECK_INSECURE_FOR_CERTS_SIG(GNUTLS_SIGN_RSA_SHA1);
	CHECK_INSECURE_FOR_CERTS_SIG(GNUTLS_SIGN_DSA_SHA1);
	CHECK_INSECURE_FOR_CERTS_SIG(GNUTLS_SIGN_ECDSA_SHA1);

	CHECK_INSECURE_SIG(GNUTLS_SIGN_RSA_MD5);
	CHECK_INSECURE_SIG(GNUTLS_SIGN_RSA_MD2);

	for (i=1;i<=GNUTLS_SIGN_MAX;i++) {
#ifndef ALLOW_SHA1
		if (i==GNUTLS_SIGN_RSA_SHA1||i==GNUTLS_SIGN_DSA_SHA1||i==GNUTLS_SIGN_ECDSA_SHA1)
			continue;
#endif
		if (i==GNUTLS_SIGN_RSA_MD5||i==GNUTLS_SIGN_RSA_MD2||i==GNUTLS_SIGN_UNKNOWN)
			continue;
		/* skip any unused elements */
		if (gnutls_sign_algorithm_get_name(i)==NULL)
			continue;
		CHECK_SECURE_SIG(i);
	}
}
