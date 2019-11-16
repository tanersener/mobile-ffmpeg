/*
 * Copyright (C) 2017-2019 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Daiki Ueno
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
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/socket.h>
# include <arpa/inet.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include "utils.h"

/* verifies whether the sign-data and verify-data APIs
 * operate as expected with deterministic ECDSA/DSA (RFC 6979) */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

struct _key_tests_st {
	const char *name;
	gnutls_datum_t key;
	gnutls_datum_t msg;
	gnutls_datum_t sig;
	gnutls_pk_algorithm_t pk;
	gnutls_digest_algorithm_t digest;
	gnutls_sign_algorithm_t sigalgo;
	unsigned int sign_flags;
};

/* Test vectors from RFC 6979 */
static const char dsa_privkey_rfc6979[] =
    "-----BEGIN DSA PRIVATE KEY-----\n"
    "MIIBugIBAAKBgQCG9coD3P6yJQY/+DCgx2m53Z1hU62R184n94fEMni0R+ZTO4ax\n"
    "i+1uiki3hKFMJSxb4Nv2C4bWOFvS8S+3Y+2Ic6v9P1ui4KjApZCC6sBWk15Sna98\n"
    "YQRniZx3re38hGyIGHC3sZsrWPm+BSGhcALjvda4ZoXukLPZobAreCsXeQIVAJlv\n"
    "ln9sjjiNnijQHiBfupV6VpixAoGAB7D5JUYVC2JRS7dx4qDAzjh/A72mxWtQUgn/\n"
    "Jf08Ez2Ju82X6QTgkRTZp9796t/JB46lRNLkAa7sxAu5+794/YeZWhChwny3eJtZ\n"
    "S6fvtcQyap/lmgcOE223cXVGStykF75dzi9A0QpGo6OUPyarf9nAOY/4x27gpWgm\n"
    "qKiPHb0CgYBd9eAd7THQKX4nThaRwZL+WGj++eGahHdkVLEAzxb2U5IZWji5BSPi\n"
    "VC7mGHHARAy4fDIvxLTS7F4efsdm4b6NTOk1Q33BHDyP1CYziTPr/nOcs0ZfTTZo\n"
    "xeRzUIJTseaC9ly9xPrpPC6iEjkOVJBahuIiMXC0Tqp9pd2f/Pt/OwIUQRYCyxmm\n"
    "zMNElNedmO8eftWvJfc=\n"
    "-----END DSA PRIVATE KEY-----\n";

static const char ecdsa_secp256r1_privkey_rfc6979[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHgCAQEEIQDJr6nYRbp1FmtcIVdnsdaTTlDD2zbomxJ7imIrEg9nIaAKBggqhkjO\n"
    "PQMBB6FEA0IABGD+1LolWp0xyWHrdMY1bWjASbiSO2H6bOZpYi5g8p+2eQP+EAi4\n"
    "vJmkGunpVii8ZPLxsgwtfp9Rd6PClNRGIpk=\n"
    "-----END EC PRIVATE KEY-----\n";

static const char sample[] = "sample";

static const
struct _key_tests_st tests[] = {
	{
		.name = "dsa key",
		.key = {(void *) dsa_privkey_rfc6979, sizeof(dsa_privkey_rfc6979)-1},
		.msg = {(void *) sample, sizeof(sample)-1},
		.sig = {(void *) "\x30\x2d\x02\x15\x00\x81\xf2\xf5\x85\x0b\xe5\xbc\x12\x3c\x43\xf7\x1a\x30\x33\xe9\x38\x46\x11\xc5\x45\x02\x14\x4c\xdd\x91\x4b\x65\xeb\x6c\x66\xa8\xaa\xad\x27\x29\x9b\xee\x6b\x03\x5f\x5e\x89", 47},
		.pk = GNUTLS_PK_DSA,
		.digest = GNUTLS_DIG_SHA256,
		.sigalgo = GNUTLS_SIGN_DSA_SHA256,
		.sign_flags = GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE
	},
	{
		.name = "ecdsa key",
		.key = {(void *) ecdsa_secp256r1_privkey_rfc6979, sizeof(ecdsa_secp256r1_privkey_rfc6979)-1},
		.msg = {(void *) sample, sizeof(sample)-1},
		.sig = {(void *) "\x30\x46\x02\x21\x00\xef\xd4\x8b\x2a\xac\xb6\xa8\xfd\x11\x40\xdd\x9c\xd4\x5e\x81\xd6\x9d\x2c\x87\x7b\x56\xaa\xf9\x91\xc3\x4d\x0e\xa8\x4e\xaf\x37\x16\x02\x21\x00\xf7\xcb\x1c\x94\x2d\x65\x7c\x41\xd4\x36\xc7\xa1\xb6\xe2\x9f\x65\xf3\xe9\x00\xdb\xb9\xaf\xf4\x06\x4d\xc4\xab\x2f\x84\x3a\xcd\xa8", 72},
		.pk = GNUTLS_PK_ECDSA,
		.digest = GNUTLS_DIG_SHA256,
		.sigalgo = GNUTLS_SIGN_ECDSA_SECP256R1_SHA256,
		.sign_flags = GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE
	},
	{
		.name = "ecdsa key",
		.key = {(void *) ecdsa_secp256r1_privkey_rfc6979, sizeof(ecdsa_secp256r1_privkey_rfc6979)-1},
		.msg = {(void *) sample, sizeof(sample)-1},
		.sig = {(void *) "\x30\x46\x02\x21\x00\xef\xd4\x8b\x2a\xac\xb6\xa8\xfd\x11\x40\xdd\x9c\xd4\x5e\x81\xd6\x9d\x2c\x87\x7b\x56\xaa\xf9\x91\xc3\x4d\x0e\xa8\x4e\xaf\x37\x16\x02\x21\x00\xf7\xcb\x1c\x94\x2d\x65\x7c\x41\xd4\x36\xc7\xa1\xb6\xe2\x9f\x65\xf3\xe9\x00\xdb\xb9\xaf\xf4\x06\x4d\xc4\xab\x2f\x84\x3a\xcd\xa8", 72},
		.pk = GNUTLS_PK_ECDSA,
		.digest = GNUTLS_DIG_SHA256,
		.sigalgo = GNUTLS_SIGN_ECDSA_SHA256,
		.sign_flags = GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE
	},
	{
		.name = "ecdsa key (q bits < h bits)",
		.key = {(void *) ecdsa_secp256r1_privkey_rfc6979, sizeof(ecdsa_secp256r1_privkey_rfc6979)-1},
		.msg = {(void *) sample, sizeof(sample)-1},
		.sig = {(void *) "\x30\x44\x02\x20\x0e\xaf\xea\x03\x9b\x20\xe9\xb4\x23\x09\xfb\x1d\x89\xe2\x13\x05\x7c\xbf\x97\x3d\xc0\xcf\xc8\xf1\x29\xed\xdd\xc8\x00\xef\x77\x19\x02\x20\x48\x61\xf0\x49\x1e\x69\x98\xb9\x45\x51\x93\xe3\x4e\x7b\x0d\x28\x4d\xdd\x71\x49\xa7\x4b\x95\xb9\x26\x1f\x13\xab\xde\x94\x09\x54", 70},
		.pk = GNUTLS_PK_ECDSA,
		.digest = GNUTLS_DIG_SHA384,
		.sigalgo = GNUTLS_SIGN_ECDSA_SHA384,
		.sign_flags = GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE
	},
	{
		.name = "ecdsa key (q bits > h bits)",
		.key = {(void *) ecdsa_secp256r1_privkey_rfc6979, sizeof(ecdsa_secp256r1_privkey_rfc6979)-1},
		.msg = {(void *) sample, sizeof(sample)-1},
		.sig = {(void *) "\x30\x45\x02\x20\x53\xb2\xff\xf5\xd1\x75\x2b\x2c\x68\x9d\xf2\x57\xc0\x4c\x40\xa5\x87\xfa\xba\xbb\x3f\x6f\xc2\x70\x2f\x13\x43\xaf\x7c\xa9\xaa\x3f\x02\x21\x00\xb9\xaf\xb6\x4f\xdc\x03\xdc\x1a\x13\x1c\x7d\x23\x86\xd1\x1e\x34\x9f\x07\x0a\xa4\x32\xa4\xac\xc9\x18\xbe\xa9\x88\xbf\x75\xc7\x4c", 71},
		.pk = GNUTLS_PK_ECDSA,
		.digest = GNUTLS_DIG_SHA224,
		.sigalgo = GNUTLS_SIGN_ECDSA_SHA224,
		.sign_flags = GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE
	}
};

#define testfail(fmt, ...) \
	fail("%s: "fmt, tests[i].name, ##__VA_ARGS__)

void doit(void)
{
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t privkey;
	gnutls_datum_t signature;
	int ret;
	size_t i;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		success("testing: %s - %s", tests[i].name, gnutls_sign_algorithm_get_name(tests[i].sigalgo));

		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0)
			testfail("gnutls_pubkey_init\n");

		ret = gnutls_privkey_init(&privkey);
		if (ret < 0)
			testfail("gnutls_privkey_init\n");

		signature.data = NULL;
		signature.size = 0;

		ret = gnutls_privkey_import_x509_raw(privkey, &tests[i].key, GNUTLS_X509_FMT_PEM, NULL, 0);
		if (ret < 0)
			testfail("gnutls_privkey_import_x509_raw\n");

		ret = gnutls_privkey_sign_data(privkey, tests[i].digest, tests[i].sign_flags,
					       &tests[i].msg, &signature);
		if (gnutls_fips140_mode_enabled()) {
			/* deterministic ECDSA/DSA is prohibited under FIPS */
			if (ret != GNUTLS_E_INVALID_REQUEST)
				testfail("gnutls_privkey_sign_data unexpectedly succeeds\n");
			success(" - skipping\n");
			goto next;
		} else {
			if (ret < 0)
				testfail("gnutls_privkey_sign_data\n");
		}

		if (signature.size != tests[i].sig.size ||
		    memcmp(signature.data, tests[i].sig.data, signature.size) != 0)
			testfail("signature does not match");

		ret = gnutls_pubkey_import_privkey(pubkey, privkey, 0, 0);
		if (ret < 0)
			testfail("gnutls_pubkey_import_privkey\n");

		ret =
		    gnutls_pubkey_verify_data2(pubkey, tests[i].sigalgo, 0, &tests[i].msg,
					      &signature);
		if (ret < 0)
			testfail("gnutls_pubkey_verify_data2\n");
		success(" - pass");

	next:
		gnutls_free(signature.data);
		gnutls_privkey_deinit(privkey);
		gnutls_pubkey_deinit(pubkey);
	}

	gnutls_global_deinit();
}
