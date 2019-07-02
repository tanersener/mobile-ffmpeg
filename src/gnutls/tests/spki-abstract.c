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
#include <assert.h>
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"
#include "cert-common.h"

static void pubkey_check(void)
{
	int ret;
	gnutls_pubkey_t pubkey;
	gnutls_x509_spki_t spki;
	unsigned salt_size;
	gnutls_digest_algorithm_t dig;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_spki_init(&spki);
	assert(ret>=0);

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_pubkey_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_pubkey_import_x509_raw(pubkey, &server_ca3_rsa_pss2_cert,
				   GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_pubkey_import: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pubkey_get_spki(pubkey, spki, 0);
	assert(ret >= 0);

	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, &salt_size) >= 0);
	assert(salt_size == 32);
	assert(dig == GNUTLS_DIG_SHA256);

	gnutls_pubkey_deinit(pubkey);
	gnutls_x509_spki_deinit(spki);
	gnutls_global_deinit();
}

static void key_check(void)
{
	int ret;
	gnutls_privkey_t key;
	gnutls_x509_spki_t spki;
	unsigned salt_size;
	gnutls_digest_algorithm_t dig;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_spki_init(&spki);
	assert(ret>=0);

	ret = gnutls_privkey_init(&key);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_privkey_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_privkey_import_x509_raw(key, &server_ca3_rsa_pss2_key,
				           GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_privkey_import: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_privkey_get_spki(key, spki, 0);
	assert(ret >= 0);

	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, &salt_size) >= 0);
	assert(salt_size == 32);
	assert(dig == GNUTLS_DIG_SHA256);

	/* set and get */
	gnutls_x509_spki_set_rsa_pss_params(spki, GNUTLS_DIG_SHA1, 64);
	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, &salt_size) >= 0);
	assert(salt_size == 64);
	assert(dig == GNUTLS_DIG_SHA1);

	/* read one at a time */
	assert(gnutls_x509_spki_get_rsa_pss_params(spki, NULL, &salt_size) >= 0);
	assert(salt_size == 64);
	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, NULL) >= 0);
	assert(dig == GNUTLS_DIG_SHA1);

	gnutls_privkey_deinit(key);
	gnutls_x509_spki_deinit(spki);
}

void doit(void)
{
	pubkey_check();
	key_check();
}
