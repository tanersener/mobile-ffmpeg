/*
 * Copyright (C) 2015 Red Hat, Inc.
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
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>

#include "utils.h"

#define SELF_TEST_SIG(x) \
	ret = gnutls_oid_to_sign(gnutls_sign_get_oid(x)); \
	if (ret != x) { \
		fail("error testing %s\n", gnutls_sign_get_name(x)); \
	}

#define SELF_TEST_PK(x) \
	ret = gnutls_oid_to_pk(gnutls_pk_get_oid(x)); \
	if (ret != x) { \
		fail("error testing %s\n", gnutls_pk_get_name(x)); \
	}

#define SELF_TEST_DIG(x) \
	ret = gnutls_oid_to_digest(gnutls_digest_get_oid(x)); \
	if (ret != x) { \
		fail("error testing %s\n", gnutls_digest_get_name(x)); \
	}

void doit(void)
{
	int ret;

	SELF_TEST_SIG(GNUTLS_SIGN_RSA_SHA1);
	SELF_TEST_SIG(GNUTLS_SIGN_RSA_SHA256);

	/* RSA-PSS uses a single OID which is independent
	 * of the signature parameters, such as the digest.
	 * As such we cannot test all variants.
	 */
	SELF_TEST_SIG(GNUTLS_SIGN_RSA_PSS_SHA256);

	SELF_TEST_SIG(GNUTLS_SIGN_ECDSA_SHA1);
	SELF_TEST_SIG(GNUTLS_SIGN_ECDSA_SHA256);
	SELF_TEST_SIG(GNUTLS_SIGN_ECDSA_SHA512);

	SELF_TEST_SIG(GNUTLS_SIGN_EDDSA_ED25519);

	if (!gnutls_fips140_mode_enabled()) {
#ifdef ENABLE_GOST
		SELF_TEST_SIG(GNUTLS_SIGN_GOST_94);
		SELF_TEST_SIG(GNUTLS_SIGN_GOST_256);
		SELF_TEST_SIG(GNUTLS_SIGN_GOST_512);
#endif
	}

	SELF_TEST_PK(GNUTLS_PK_RSA);
	SELF_TEST_PK(GNUTLS_PK_DSA);
	SELF_TEST_PK(GNUTLS_PK_EC);
	SELF_TEST_PK(GNUTLS_PK_RSA_PSS);
	SELF_TEST_PK(GNUTLS_PK_EDDSA_ED25519);

	if (!gnutls_fips140_mode_enabled()) {
#ifdef ENABLE_GOST
		SELF_TEST_PK(GNUTLS_PK_GOST_01);
		SELF_TEST_PK(GNUTLS_PK_GOST_12_256);
		SELF_TEST_PK(GNUTLS_PK_GOST_12_512);
#endif
	}

	SELF_TEST_DIG(GNUTLS_DIG_MD5);
	SELF_TEST_DIG(GNUTLS_DIG_SHA1);
	SELF_TEST_DIG(GNUTLS_DIG_SHA256);
	SELF_TEST_DIG(GNUTLS_DIG_SHA512);
	SELF_TEST_DIG(GNUTLS_DIG_SHA3_224);
	SELF_TEST_DIG(GNUTLS_DIG_SHA3_256);
	SELF_TEST_DIG(GNUTLS_DIG_SHA3_384);
	SELF_TEST_DIG(GNUTLS_DIG_SHA3_512);

	if (!gnutls_fips140_mode_enabled()) {
#ifdef ENABLE_GOST
		SELF_TEST_DIG(GNUTLS_DIG_GOSTR_94);
		SELF_TEST_DIG(GNUTLS_DIG_STREEBOG_256);
		SELF_TEST_DIG(GNUTLS_DIG_STREEBOG_512);
#endif
	}
}
