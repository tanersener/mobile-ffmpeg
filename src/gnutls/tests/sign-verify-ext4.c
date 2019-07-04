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

/* This tests the gnutls_privkey_import_ext4() APIs */

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
#include "common-key-tests.h"
#include "utils.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

/* sha1 hash of "hello" string */
const gnutls_datum_t sha1_hash_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xd9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t sha256_hash_data = {
	(void *)
	    "\x2c\xf2\x4d\xba\x5f\xb0\xa3\x0e\x26\xe8\x3b\x2a\xc5\xb9\xe2\x9e"
	    "\x1b\x16\x1e\x5c\x1f\xa7\x42\x5e\x73\x04\x33\x62\x93\x8b\x98\x24",
	32
};

const gnutls_datum_t raw_data = {
	(void *) "hello",
	5
};


struct key_cb_data {
	gnutls_privkey_t rkey; /* the real thing */
	unsigned pk;
	unsigned sig;
	unsigned bits;
};

static int key_cb_info_func(gnutls_privkey_t key, unsigned int flags, void *userdata)
{
	struct key_cb_data *p = userdata;

	if (flags & GNUTLS_PRIVKEY_INFO_PK_ALGO)
		return p->pk;
	else if (flags & GNUTLS_PRIVKEY_INFO_PK_ALGO_BITS)
		return p->bits;
	else if (flags & GNUTLS_PRIVKEY_INFO_HAVE_SIGN_ALGO) {
		unsigned sig = GNUTLS_FLAGS_TO_SIGN_ALGO(flags);

		if (sig == p->sig)
			return 1;

		return 0;
	}

	return -1;
}

static
int key_cb_sign_data_func (gnutls_privkey_t key, gnutls_sign_algorithm_t sig,
			   void* userdata, unsigned int flags, const gnutls_datum_t *data,
			   gnutls_datum_t *signature)
{
	struct key_cb_data *p = userdata;

	if (debug)
		fprintf(stderr, "signing data with: %s\n", gnutls_sign_get_name(sig));
	return gnutls_privkey_sign_data2(p->rkey, sig, 0, data, signature);
}

static
int key_cb_sign_hash_func (gnutls_privkey_t key, gnutls_sign_algorithm_t sig,
			   void* userdata, unsigned int flags, const gnutls_datum_t *data,
			   gnutls_datum_t *signature)
{
	struct key_cb_data *p = userdata;

	if (sig == GNUTLS_SIGN_RSA_RAW) {
		if (debug)
			fprintf(stderr, "signing digestinfo with: raw RSA\n");
		return gnutls_privkey_sign_hash(p->rkey, 0, GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA, data, signature);
	} else {
		if (debug)
			fprintf(stderr, "signing hash with: %s\n", gnutls_sign_get_name(sig));
		return gnutls_privkey_sign_hash2(p->rkey, sig, 0, data, signature);
	}
}

static void key_cb_deinit_func(gnutls_privkey_t key, void* userdata)
{
	struct key_cb_data *p = userdata;
	gnutls_privkey_deinit(p->rkey);
	free(userdata);
}

static gnutls_privkey_t load_virt_privkey(const gnutls_datum_t *txtkey, gnutls_pk_algorithm_t pk,
					  gnutls_sign_algorithm_t sig)
{
	gnutls_privkey_t privkey;
	struct key_cb_data *userdata;
	int ret;

	userdata = calloc(1, sizeof(struct key_cb_data));
	if (userdata == NULL) {
		fail("memory error\n");
	}

	ret = gnutls_privkey_init(&privkey);
	if (ret < 0)
		fail("gnutls_privkey_init\n");

	ret = gnutls_privkey_init(&userdata->rkey);
	if (ret < 0)
		fail("gnutls_privkey_init\n");

	ret =
	    gnutls_privkey_import_x509_raw(userdata->rkey, txtkey, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("gnutls_privkey_import\n");

	gnutls_privkey_get_pk_algorithm(userdata->rkey, &userdata->bits);

	userdata->pk = pk;
	userdata->sig = sig;

	ret = gnutls_privkey_import_ext4(privkey, userdata,
					 key_cb_sign_data_func,
					 key_cb_sign_hash_func,
					 NULL,
					 key_cb_deinit_func, key_cb_info_func,
					 GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);
	if (ret < 0)
		fail("gnutls_privkey_import_ext4\n");

	return privkey;
}

#define tests common_key_tests
#define testfail(fmt, ...) \
	fail("%s: "fmt, tests[i].name, ##__VA_ARGS__)

void doit(void)
{
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t privkey;
	gnutls_sign_algorithm_t sign_algo;
	gnutls_datum_t signature;
	gnutls_datum_t signature2;
	int ret;
	size_t i;
	const gnutls_datum_t *hash_data;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		if (debug)
			success("loop %d: %s\n", (int) i, tests[i].name);

		if (tests[i].digest == GNUTLS_DIG_SHA1) {
			hash_data = &sha1_hash_data;
		} else {
			hash_data = &sha256_hash_data;
		}

		privkey = load_virt_privkey(&tests[i].key, tests[i].pk, tests[i].sigalgo);

		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0)
			testfail("gnutls_privkey_init\n");


		ret = gnutls_privkey_sign_data2(privkey, tests[i].sigalgo, tests[i].sign_flags,
						&raw_data, &signature);
		if (ret < 0)
			testfail("gnutls_x509_privkey_sign_hash\n");

		ret = gnutls_pubkey_import_x509_raw(pubkey, &tests[i].cert, GNUTLS_X509_FMT_PEM, 0);
		if (ret < 0)
			testfail("gnutls_x509_pubkey_import\n");

		ret =
		    gnutls_pubkey_verify_data2(pubkey, tests[i].sigalgo, GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1, &raw_data,
					       &signature);
		if (ret < 0)
			testfail("gnutls_pubkey_verify_data2\n");

		gnutls_free(signature.data);


		if (!tests[i].data_only) {
			ret = gnutls_privkey_sign_hash(privkey, tests[i].digest, tests[i].sign_flags,
							hash_data, &signature2);
			if (ret < 0)
				testfail("gnutls_privkey_sign_hash\n");

			ret =
			    gnutls_pubkey_verify_hash2(pubkey, tests[i].sigalgo, GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1, hash_data,
						      &signature2);
			if (ret < 0)
				testfail("gnutls_pubkey_verify_hash2-1 (hashed data)\n");

			gnutls_free(signature2.data);
		}

		if (gnutls_pubkey_get_pk_algorithm(pubkey, NULL) ==
		    GNUTLS_PK_RSA) {

			ret =
			    gnutls_privkey_sign_hash(privkey,
						     tests[i].digest,
						     GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
						     hash_data,
						     &signature);
			if (ret < 0)
				testfail("gnutls_privkey_sign_hash: %s\n",
				     gnutls_strerror(ret));

			sign_algo =
			    gnutls_pk_to_sign
			    (gnutls_pubkey_get_pk_algorithm(pubkey, NULL),
			     tests[i].digest);

			ret =
			    gnutls_pubkey_verify_hash2(pubkey, sign_algo,
							GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA,
							hash_data,
							&signature);
			if (ret < 0)
				testfail("gnutls_pubkey_verify_hash-3 (raw hashed data)\n");

			gnutls_free(signature.data);
			/* test the legacy API */
			ret =
			    gnutls_privkey_sign_raw_data(privkey, 0,
							 hash_data,
							 &signature);
			if (ret < 0)
				testfail("gnutls_privkey_sign_raw_data: %s\n",
				     gnutls_strerror(ret));

			ret =
			    gnutls_pubkey_verify_hash2(pubkey, sign_algo,
							GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA,
							hash_data,
							&signature);
			if (ret < 0)
				testfail("gnutls_pubkey_verify_hash-4 (legacy raw hashed data)\n");
		}
		gnutls_free(signature.data);
		gnutls_free(signature2.data);
		gnutls_privkey_deinit(privkey);
		gnutls_pubkey_deinit(pubkey);
	}

	gnutls_global_deinit();
}
