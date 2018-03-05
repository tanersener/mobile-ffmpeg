/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: David Marín Carreño
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <assert.h>

#include "utils.h"

#define MAX_TRIES 2

static int sec_param[MAX_TRIES] =
#ifdef ENABLE_FIPS140
    { GNUTLS_SEC_PARAM_MEDIUM, GNUTLS_SEC_PARAM_HIGH };
#else
    { GNUTLS_SEC_PARAM_LOW, GNUTLS_SEC_PARAM_MEDIUM };
#endif

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", "crq_key_id", level, str);
}

const gnutls_datum_t raw_data = {
	(void *) "hello there",
	11
};

static void sign_verify_data(gnutls_x509_privkey_t pkey)
{
	int ret;
	gnutls_privkey_t privkey;
	gnutls_pubkey_t pubkey;
	gnutls_datum_t signature;

	/* sign arbitrary data */
	assert(gnutls_privkey_init(&privkey) >= 0);

	ret = gnutls_privkey_import_x509(privkey, pkey, 0);
	if (ret < 0)
		fail("gnutls_privkey_import_x509\n");

	ret = gnutls_privkey_sign_data(privkey, GNUTLS_DIG_SHA256, 0,
					&raw_data, &signature);
	if (ret < 0)
		fail("gnutls_x509_privkey_sign_data\n");

	/* verify data */
	assert(gnutls_pubkey_init(&pubkey) >= 0);

	ret = gnutls_pubkey_import_privkey(pubkey, privkey, 0, 0);
	if (ret < 0)
		fail("gnutls_pubkey_import_privkey\n");

	ret = gnutls_pubkey_verify_data2(pubkey, gnutls_pk_to_sign(gnutls_pubkey_get_pk_algorithm(pubkey, NULL),GNUTLS_DIG_SHA256),
				0, &raw_data, &signature);
	if (ret < 0)
		fail("gnutls_pubkey_verify_data2\n");

	gnutls_pubkey_deinit(pubkey);
	gnutls_privkey_deinit(privkey);
	gnutls_free(signature.data);
}

void doit(void)
{
	gnutls_x509_privkey_t pkey, dst;
	int ret, algorithm, i;

	ret = global_init();
	if (ret < 0)
		fail("global_init: %d\n", ret);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	for (i = 0; i < MAX_TRIES; i++) {
		for (algorithm = GNUTLS_PK_RSA; algorithm <= GNUTLS_PK_EC;
		     algorithm++) {
			if (algorithm == GNUTLS_PK_DH)
				continue;

			ret = gnutls_x509_privkey_init(&pkey);
			if (ret < 0) {
				fail("gnutls_x509_privkey_init: %d\n",
				     ret);
			}

			ret = gnutls_x509_privkey_init(&dst);
			if (ret < 0) {
				fail("gnutls_x509_privkey_init: %d\n",
				     ret);
			}

			ret =
			    gnutls_x509_privkey_generate(pkey, algorithm,
							 gnutls_sec_param_to_pk_bits
							 (algorithm,
							  sec_param[i]),
							 0);
			if (ret < 0) {
				fail("gnutls_x509_privkey_generate (%s-%d): %s (%d)\n", gnutls_pk_algorithm_get_name(algorithm),
					gnutls_sec_param_to_pk_bits(algorithm,sec_param[i]), gnutls_strerror(ret), ret);
			} else if (debug) {
				success("Key[%s] generation ok: %d\n",
					gnutls_pk_algorithm_get_name
					(algorithm), ret);
			}

			ret = gnutls_x509_privkey_verify_params(pkey);
			if (ret < 0) {
				fail("gnutls_x509_privkey_generate (%s): %s (%d)\n", gnutls_pk_algorithm_get_name(algorithm), gnutls_strerror(ret), ret);
			}

			/* include test of cpy */
			ret = gnutls_x509_privkey_cpy(dst, pkey);
			if (ret < 0) {
				fail("gnutls_x509_privkey_cpy (%s): %s (%d)\n", gnutls_pk_algorithm_get_name(algorithm), gnutls_strerror(ret), ret);
			}

			ret = gnutls_x509_privkey_verify_params(pkey);
			if (ret < 0) {
				fail("gnutls_x509_privkey_generate after cpy (%s): %s (%d)\n", gnutls_pk_algorithm_get_name(algorithm), gnutls_strerror(ret), ret);
			}

			sign_verify_data(pkey);
			sign_verify_data(dst);

			gnutls_x509_privkey_deinit(pkey);
			gnutls_x509_privkey_deinit(dst);
			success("Generated key with %s-%d\n", gnutls_pk_algorithm_get_name(algorithm), gnutls_sec_param_to_pk_bits(algorithm,sec_param[i]));
		}
	}

	gnutls_global_deinit();
}
