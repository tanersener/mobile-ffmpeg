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
#include "cert-common.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

const gnutls_datum_t raw_data = {
	(void *) "hello there",
	11
};

static int sign_verify_data(gnutls_x509_privkey_t pkey, gnutls_sign_algorithm_t algo, unsigned vflags)
{
	int ret;
	gnutls_privkey_t privkey;
	gnutls_pubkey_t pubkey = NULL;
	gnutls_datum_t signature;
	gnutls_pk_algorithm_t pk;
	gnutls_digest_algorithm_t dig;
	unsigned sflags = 0;

	/* sign arbitrary data */
	assert(gnutls_privkey_init(&privkey) >= 0);

	pk = gnutls_sign_get_pk_algorithm(algo);
	dig = gnutls_sign_get_hash_algorithm(algo);

	if (pk == GNUTLS_PK_RSA_PSS)
		sflags |= GNUTLS_PRIVKEY_SIGN_FLAG_RSA_PSS;

	ret = gnutls_privkey_import_x509(privkey, pkey, 0);
	if (ret < 0)
		fail("gnutls_pubkey_import_x509\n");

	ret = gnutls_privkey_sign_data(privkey, dig, sflags,
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
				vflags, &raw_data, &signature);
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

void doit(void)
{
	gnutls_x509_privkey_t pkey;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("global_init: %d\n", ret);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0) {
		fail("gnutls_x509_privkey_init: %d\n", ret);
	}

	ret = gnutls_x509_privkey_import(pkey, &key_dat, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_privkey_generate: %s\n", gnutls_strerror(ret));
	}

	if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_SHA1, 0) < 0)
		fail("failed verification with SHA1!\n");

	if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_MD5, 0) >= 0)
		fail("succeeded verification with MD5!\n");

	if (!gnutls_fips140_mode_enabled()) {
		if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_MD5, GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD5) < 0)
			fail("failed verification with MD5 and override flags!\n");

		if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_MD5, GNUTLS_VERIFY_ALLOW_BROKEN) < 0)
			fail("failed verification with MD5 and override flags2!\n");
	}

	if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_SHA256, 0) < 0)
		fail("failed verification with SHA256!\n");

	if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_SHA512, 0) < 0)
		fail("failed verification with SHA512!\n");

	if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_SHA3_256, 0) < 0)
		fail("failed verification with SHA3-256!\n");

	if (sign_verify_data(pkey, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, 0) < 0)
		fail("failed verification with SHA256 with PSS!\n");

	gnutls_x509_privkey_deinit(pkey);

	gnutls_global_deinit();
}
