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

/* This tests key import for gnutls_x509_privkey_t APIs */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>
#include <assert.h>
#include "cert-common.h"
#include "utils.h"

#define testfail(fmt, ...) \
	fail("%s: "fmt, name, ##__VA_ARGS__)

const gnutls_datum_t raw_data = {
	(void *) "hello there",
	11
};

static int sign_verify_data(gnutls_x509_privkey_t pkey, gnutls_sign_algorithm_t algo)
{
	int ret;
	gnutls_privkey_t privkey;
	gnutls_pubkey_t pubkey = NULL;
	gnutls_datum_t signature;

	/* sign arbitrary data */
	assert(gnutls_privkey_init(&privkey) >= 0);

	ret = gnutls_privkey_import_x509(privkey, pkey, 0);
	if (ret < 0)
		fail("gnutls_privkey_import_x509\n");

	ret = gnutls_privkey_sign_data2(privkey, algo, 0,
					&raw_data, &signature);
	if (ret < 0) {
		ret = -1;
		goto cleanup;
	}

	/* verify data */
	assert(gnutls_pubkey_init(&pubkey) >= 0);

	ret = gnutls_pubkey_import_privkey(pubkey, privkey, 0, 0);
	if (ret < 0)
		fail("gnutls_pubkey_import_privkey\n");

	ret = gnutls_pubkey_verify_data2(pubkey, algo,
				GNUTLS_VERIFY_ALLOW_BROKEN, &raw_data, &signature);
	if (ret < 0) {
		ret = -1;
		goto cleanup;
	}

	ret = 0;
 cleanup:
	if (pubkey)
		gnutls_pubkey_deinit(pubkey);
	gnutls_privkey_deinit(privkey);
	gnutls_free(signature.data);

	return ret;
}

static void load_privkey(const char *name, const gnutls_datum_t *txtkey, gnutls_pk_algorithm_t pk,
			 gnutls_sign_algorithm_t sig, int exp_key_err)
{
	gnutls_x509_privkey_t tmp;
	int ret;

	ret = gnutls_x509_privkey_init(&tmp);
	if (ret < 0)
		testfail("gnutls_privkey_init\n");

	ret = gnutls_x509_privkey_import(tmp, txtkey, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		if (exp_key_err) {
			testfail("did not fail in key import, although expected\n");
		}

		testfail("gnutls_privkey_import: %s\n", gnutls_strerror(ret));
	}

	if (gnutls_x509_privkey_get_pk_algorithm(tmp) != (int)pk) {
		testfail("pk algorithm doesn't match!\n");
	}

	ret = gnutls_x509_privkey_verify_params(tmp);
	if (ret < 0)
		testfail("gnutls_privkey_verify_params: %s\n", gnutls_strerror(ret));

	sign_verify_data(tmp, sig);
	
	gnutls_x509_privkey_deinit(tmp);

	return;
}

static void load_privkey_in_der(const char *name, const gnutls_datum_t *txtkey, gnutls_pk_algorithm_t pk,
			 gnutls_sign_algorithm_t sig, int exp_key_err)
{
	gnutls_x509_privkey_t tmp;
	gnutls_datum_t der;
	int ret;

	ret = gnutls_x509_privkey_init(&tmp);
	if (ret < 0)
		testfail("gnutls_privkey_init\n");

	ret = gnutls_pem_base64_decode2(NULL, txtkey, &der);
	if (ret < 0 || der.size == 0) {
		testfail("could not convert key to DER form: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_x509_privkey_import(tmp, &der, GNUTLS_X509_FMT_DER);
	gnutls_free(der.data);

	if (ret < 0) {
		if (exp_key_err) {
			testfail("did not fail in key import, although expected\n");
		}

		testfail("gnutls_privkey_import: %s\n", gnutls_strerror(ret));
	}

	if (gnutls_x509_privkey_get_pk_algorithm(tmp) != (int)pk) {
		testfail("pk algorithm doesn't match!\n");
	}

	ret = gnutls_x509_privkey_verify_params(tmp);
	if (ret < 0)
		testfail("gnutls_privkey_verify_params: %s\n", gnutls_strerror(ret));

	sign_verify_data(tmp, sig);

	gnutls_x509_privkey_deinit(tmp);

	return;
}

typedef struct test_st {
	const char *name;
	gnutls_pk_algorithm_t pk;
	gnutls_sign_algorithm_t sig;
	const gnutls_datum_t *key;
	int exp_key_err;
} test_st;

static const test_st tests[] = {
	{.name = "ecc key",
	 .pk = GNUTLS_PK_ECDSA,
	 .sig = GNUTLS_SIGN_ECDSA_SHA256,
	 .key = &server_ca3_ecc_key,
	},
	{.name = "rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .sig = GNUTLS_SIGN_RSA_SHA384,
	 .key = &server_ca3_key,
	},
	{.name = "rsa-pss-sign key (PKCS#8)",
	 .pk = GNUTLS_PK_RSA_PSS,
	 .sig = GNUTLS_SIGN_RSA_PSS_SHA256,
	 .key = &server_ca3_rsa_pss2_key,
	},
	{.name = "dsa key",
	 .pk = GNUTLS_PK_DSA,
	 .sig = GNUTLS_SIGN_DSA_SHA1,
	 .key = &dsa_key,
	},
	{.name = "ed25519 key (PKCS#8)",
	 .pk = GNUTLS_PK_EDDSA_ED25519,
	 .sig = GNUTLS_SIGN_EDDSA_ED25519,
	 .key = &server_ca3_eddsa_key,
	}
};

void doit(void)
{
	unsigned int i;

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		success("checking: %s\n", tests[i].name);

		load_privkey(tests[i].name, tests[i].key, tests[i].pk,
			     tests[i].sig, tests[i].exp_key_err);

		success("checking: %s in der form\n", tests[i].name);
		load_privkey_in_der(tests[i].name, tests[i].key, tests[i].pk,
			     tests[i].sig, tests[i].exp_key_err);
	}

	gnutls_global_deinit();
}
