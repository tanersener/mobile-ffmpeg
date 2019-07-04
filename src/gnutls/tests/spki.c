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

static void crq_check(void)
{
	int ret;
	gnutls_x509_crq_t crq;
	gnutls_x509_spki_t spki;
	gnutls_datum_t tmp;
	gnutls_x509_privkey_t privkey;
	unsigned salt_size;
	gnutls_digest_algorithm_t dig;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_x509_privkey_init(&privkey)>=0);

	ret =
	    gnutls_x509_privkey_generate(privkey, GNUTLS_PK_RSA, 2048, 0);
	assert(ret>=0);

	assert(gnutls_x509_spki_init(&spki)>=0);

	gnutls_x509_spki_set_rsa_pss_params(spki, GNUTLS_DIG_SHA256, 32);

	ret = gnutls_x509_crq_init(&crq);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crq_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_x509_crq_set_version(crq, 1)>=0);
	assert(gnutls_x509_crq_set_key(crq, privkey)>=0);
	assert(gnutls_x509_crq_set_spki(crq, spki, 0)>=0);

	assert(gnutls_x509_crq_set_dn_by_oid(crq, GNUTLS_OID_X520_COMMON_NAME,
						0, "CN-Test", 7)>=0);
	gnutls_x509_spki_deinit(spki);

	assert(gnutls_x509_crq_sign2(crq, privkey, GNUTLS_DIG_SHA256, 0)>=0);

	if (debug) {
		gnutls_x509_crq_print(crq, GNUTLS_CRT_PRINT_ONELINE, &tmp);

		printf("\tCertificate: %.*s\n", tmp.size, tmp.data);
		gnutls_free(tmp.data);
	}

	/* read SPKI */
	assert(gnutls_x509_spki_init(&spki)>=0);

	ret = gnutls_x509_crq_get_spki(crq, spki, 0);
	assert(ret >= 0);

	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, &salt_size) >= 0);
	assert(salt_size == 32);
	assert(dig == GNUTLS_DIG_SHA256);

	/* set invalid */
	gnutls_x509_spki_set_rsa_pss_params(spki, GNUTLS_DIG_SHA256, 1024);
	assert(gnutls_x509_crq_set_spki(crq, spki, 0) == GNUTLS_E_PK_INVALID_PUBKEY_PARAMS);

	gnutls_x509_crq_deinit(crq);
	gnutls_x509_spki_deinit(spki);
	gnutls_x509_privkey_deinit(privkey);
	gnutls_global_deinit();
}


static void cert_check(void)
{
	int ret;
	gnutls_x509_crt_t crt;
	gnutls_x509_spki_t spki;
	gnutls_datum_t tmp;
	unsigned salt_size;
	gnutls_digest_algorithm_t dig;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_spki_init(&spki);
	assert(ret>=0);

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(crt, &server_ca3_rsa_pss2_cert,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_import: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (debug) {
		gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_ONELINE, &tmp);

		printf("\tCertificate: %.*s\n", tmp.size, tmp.data);
		gnutls_free(tmp.data);
	}

	ret = gnutls_x509_crt_get_spki(crt, spki, 0);
	assert(ret >= 0);

	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, &salt_size) >= 0);
	assert(salt_size == 32);
	assert(dig == GNUTLS_DIG_SHA256);

	/* set invalid */
	gnutls_x509_spki_set_rsa_pss_params(spki, GNUTLS_DIG_SHA256, 1024);
	assert(gnutls_x509_crt_set_spki(crt, spki, 0) == GNUTLS_E_PK_INVALID_PUBKEY_PARAMS);

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_spki_deinit(spki);
	gnutls_global_deinit();
}

static void key_check(void)
{
	int ret;
	gnutls_x509_privkey_t key;
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

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_privkey_import(key, &server_ca3_rsa_pss2_key,
				       GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_import: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_get_spki(key, spki, 0);
	assert(ret >= 0);

	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, &salt_size) >= 0);
	assert(salt_size == 32);
	assert(dig == GNUTLS_DIG_SHA256);

	/* set and get */
	gnutls_x509_spki_set_rsa_pss_params(spki, GNUTLS_DIG_SHA1, 64);
	assert(gnutls_x509_spki_get_rsa_pss_params(spki, &dig, &salt_size) >= 0);
	assert(salt_size == 64);
	assert(dig == GNUTLS_DIG_SHA1);

	/* set invalid */
	gnutls_x509_spki_set_rsa_pss_params(spki, GNUTLS_DIG_SHA1, 1024);
	assert(gnutls_x509_privkey_set_spki(key, spki, 0) == GNUTLS_E_PK_INVALID_PUBKEY_PARAMS);

	gnutls_x509_privkey_deinit(key);
	gnutls_x509_spki_deinit(spki);
}

void doit(void)
{
	cert_check();
	key_check();
	crq_check();
}
