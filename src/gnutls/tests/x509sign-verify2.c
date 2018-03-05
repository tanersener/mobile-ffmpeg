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

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

/* sha1 hash of "hello" string */
const gnutls_datum_t sha1_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xd9\xae\xa9\x43\x4d",
	20
};

/* sha1 hash of "hello" string */
const gnutls_datum_t sha256_data = {
	(void *)
	    "\x2c\xf2\x4d\xba\x5f\xb0\xa3\x0e\x26\xe8"
	    "\x3b\x2a\xc5\xb9\xe2\x9e\x1b\x16\x1e\x5c"
	    "\x1f\xa7\x42\x5e\x73\x04\x33\x62\x93\x8b"
	    "\x98\x24",
	32
};

const gnutls_datum_t invalid_hash_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xca\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xb9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t raw_data = {
	(void *)"hello",
	5
};

#define ERR(x) fail("Failure at %d: %s (%s-%s) (iter: %d)\n", __LINE__, gnutls_sign_get_name(sign_algo), gnutls_pk_get_name(pk), gnutls_digest_get_name(hash), j);
static
void test_sig(gnutls_pk_algorithm_t pk, unsigned hash, unsigned bits)
{
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t privkey;
	gnutls_sign_algorithm_t sign_algo;
	gnutls_datum_t signature;
	const gnutls_datum_t *hash_data;
	int ret;
	unsigned j;

	if (hash == GNUTLS_DIG_SHA1)
		hash_data = &sha1_data;
	else if (hash == GNUTLS_DIG_SHA256)
		hash_data = &sha256_data;
	else
		abort();

	sign_algo =
	    gnutls_pk_to_sign(pk, hash);

	for (j = 0; j < 100; j++) {
		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0)
			ERR(__LINE__);

		ret = gnutls_privkey_init(&privkey);
		if (ret < 0)
			ERR(__LINE__);

		ret = gnutls_privkey_generate(privkey, pk, bits, 0);
		if (ret < 0)
			ERR(__LINE__);

		ret =
		    gnutls_privkey_sign_hash(privkey, hash,
					     0, hash_data,
					     &signature);
		if (ret < 0)
			ERR(__LINE__);

		ret = gnutls_pubkey_import_privkey(pubkey, privkey, GNUTLS_KEY_DIGITAL_SIGNATURE, 0);
		if (ret < 0)
			ERR(__LINE__);

		ret =
		    gnutls_pubkey_verify_hash2(pubkey,
						sign_algo, 0,
						hash_data, &signature);
		if (ret < 0)
			ERR(__LINE__);

		/* should fail */
		ret =
		    gnutls_pubkey_verify_hash2(pubkey,
						sign_algo, 0,
						&invalid_hash_data,
						&signature);
		if (ret != GNUTLS_E_PK_SIG_VERIFY_FAILED)
			ERR(__LINE__);

		sign_algo =
		    gnutls_pk_to_sign(gnutls_pubkey_get_pk_algorithm
				      (pubkey, NULL), hash);

		ret =
		    gnutls_pubkey_verify_hash2(pubkey, sign_algo, 0,
						hash_data, &signature);
		if (ret < 0)
			ERR(__LINE__);

		/* should fail */
		ret =
		    gnutls_pubkey_verify_hash2(pubkey, sign_algo, 0,
						&invalid_hash_data,
						&signature);
		if (ret != GNUTLS_E_PK_SIG_VERIFY_FAILED)
			ERR(__LINE__);

		/* test the raw interface */
		gnutls_free(signature.data);
		signature.data = NULL;

		if (pk == GNUTLS_PK_RSA) {
			ret =
			    gnutls_privkey_sign_hash(privkey,
						     hash,
						     GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
						     hash_data,
						     &signature);
			if (ret < 0)
				ERR(__LINE__);

			sign_algo =
			    gnutls_pk_to_sign
			    (gnutls_pubkey_get_pk_algorithm
			     (pubkey, NULL), hash);

			ret =
			    gnutls_pubkey_verify_hash2(pubkey,
							sign_algo,
							GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA,
							hash_data,
							&signature);
			if (ret < 0)
				ERR(__LINE__);

		}
		gnutls_free(signature.data);
		gnutls_privkey_deinit(privkey);
		gnutls_pubkey_deinit(pubkey);
	}

}

void doit(void)
{
	unsigned rsa_size1, rsa_size2;
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	if (gnutls_fips140_mode_enabled()) {
		rsa_size1 = 2048; /* minimum allowed */
		rsa_size2 = 2048; /* minimum allowed */
	} else {
		rsa_size1 = 512;
		rsa_size2 = 1024;
	}

	test_sig(GNUTLS_PK_RSA, GNUTLS_DIG_SHA1, rsa_size1);
	test_sig(GNUTLS_PK_RSA, GNUTLS_DIG_SHA256, rsa_size2);
	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA1, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1));
	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1));
	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP384R1));
	test_sig(GNUTLS_PK_EC, GNUTLS_DIG_SHA256, GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP521R1));

	gnutls_global_deinit();
}
