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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <assert.h>

#include "utils.h"

/* This tests the key conversion from basic RSA to RSA-PSS.
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

const gnutls_datum_t raw_data = {
	(void *) "hello there",
	11
};

static void inv_sign_check(unsigned sigalgo,
			   gnutls_privkey_t privkey, int exp_error)
{
	int ret;
	gnutls_datum_t signature;

	ret = gnutls_privkey_sign_data2(privkey, sigalgo, 0,
					&raw_data, &signature);
	if (ret != exp_error)
		fail("gnutls_privkey_sign_data succeeded with %s and %s: %s\n", gnutls_pk_get_name(gnutls_privkey_get_pk_algorithm(privkey, NULL)),
			gnutls_sign_get_name(sigalgo), gnutls_strerror(ret));

	if (ret == 0)
		gnutls_free(signature.data);
}

static void inv_encryption_check(gnutls_pk_algorithm_t algorithm,
			     gnutls_privkey_t privkey, int exp_error)
{
	int ret;
	gnutls_datum_t ct;
	gnutls_pubkey_t pubkey;

	assert(gnutls_pubkey_init(&pubkey) >= 0);

	ret = gnutls_pubkey_import_privkey(pubkey, privkey, 0, 0);
	if (ret < 0)
		fail("gnutls_pubkey_import_privkey\n");

	ret = gnutls_pubkey_encrypt_data(pubkey, 0, &raw_data, &ct);
	if (ret != exp_error)
		fail("gnutls_pubkey_encrypt_data succeeded with %s: %s\n", gnutls_pk_get_name(algorithm),
			gnutls_strerror(ret));

	gnutls_pubkey_deinit(pubkey);

}

static void sign_verify_data(unsigned sigalgo, gnutls_privkey_t privkey)
{
	int ret;
	gnutls_pubkey_t pubkey;
	gnutls_datum_t signature;

	ret = gnutls_privkey_sign_data2(privkey, sigalgo, 0,
					&raw_data, &signature);
	if (ret < 0)
		fail("gnutls_x509_privkey_sign_data\n");

	/* verify data */
	assert(gnutls_pubkey_init(&pubkey) >= 0);

	ret = gnutls_pubkey_import_privkey(pubkey, privkey, 0, 0);
	if (ret < 0)
		fail("gnutls_pubkey_import_privkey\n");

	ret = gnutls_pubkey_verify_data2(pubkey, sigalgo,
				0, &raw_data, &signature);
	if (ret < 0)
		fail("gnutls_pubkey_verify_data2\n");

	gnutls_pubkey_deinit(pubkey);
	gnutls_free(signature.data);
}

void doit(void)
{
	gnutls_privkey_t pkey_rsa_pss;
	gnutls_privkey_t pkey_rsa;
	gnutls_x509_privkey_t tkey;
	int ret;
	gnutls_x509_spki_t spki;
	gnutls_datum_t tmp;

	ret = global_init();
	if (ret < 0)
		fail("global_init: %d\n", ret);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	assert(gnutls_x509_spki_init(&spki)>=0);

	assert(gnutls_privkey_init(&pkey_rsa) >=0);

	gnutls_x509_spki_set_rsa_pss_params(spki, GNUTLS_DIG_SHA256, 32);

	ret =
	    gnutls_privkey_generate(pkey_rsa, GNUTLS_PK_RSA, 2048, 0);
	if (ret < 0) {
		fail("gnutls_privkey_generate: %s\n", gnutls_strerror(ret));
	}

	assert(gnutls_privkey_set_spki(pkey_rsa, spki, 0)>=0);
	assert(gnutls_privkey_export_x509(pkey_rsa, &tkey) >=0);

	gnutls_x509_privkey_export2_pkcs8(tkey, GNUTLS_X509_FMT_PEM, NULL, 0, &tmp);

	/* import RSA-PSS version of key */
	assert(gnutls_privkey_init(&pkey_rsa_pss) >=0);
	assert(gnutls_privkey_import_x509_raw(pkey_rsa_pss, &tmp, GNUTLS_X509_FMT_PEM, NULL, 0) >= 0);

	gnutls_free(tmp.data);

	/* import RSA-PSS version of key */
	gnutls_privkey_deinit(pkey_rsa);
	gnutls_x509_privkey_export2(tkey, GNUTLS_X509_FMT_PEM, &tmp);
	assert(gnutls_privkey_init(&pkey_rsa) >=0);
	assert(gnutls_privkey_import_x509_raw(pkey_rsa, &tmp, GNUTLS_X509_FMT_PEM, NULL, 0) >= 0);

	gnutls_x509_privkey_deinit(tkey);
	gnutls_free(tmp.data);

	sign_verify_data(GNUTLS_SIGN_RSA_PSS_SHA256, pkey_rsa_pss);
	sign_verify_data(GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, pkey_rsa);
	sign_verify_data(GNUTLS_SIGN_RSA_PSS_SHA256, pkey_rsa);

	if (debug)
		success("success signing with RSA-PSS-SHA256\n");

	/* check whether the RSA-PSS key restrictions are being followed */
	inv_encryption_check(GNUTLS_PK_RSA_PSS, pkey_rsa_pss, GNUTLS_E_INVALID_REQUEST);
	inv_sign_check(GNUTLS_SIGN_RSA_SHA512, pkey_rsa_pss, GNUTLS_E_CONSTRAINT_ERROR);
	inv_sign_check(GNUTLS_SIGN_RSA_SHA256, pkey_rsa_pss, GNUTLS_E_CONSTRAINT_ERROR);
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_SHA384, pkey_rsa_pss, GNUTLS_E_CONSTRAINT_ERROR);
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_SHA512, pkey_rsa_pss, GNUTLS_E_CONSTRAINT_ERROR);
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_RSAE_SHA384, pkey_rsa_pss, GNUTLS_E_CONSTRAINT_ERROR);
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_RSAE_SHA512, pkey_rsa_pss, GNUTLS_E_CONSTRAINT_ERROR);

	/* check whether the RSA key is not being restricted */
	inv_sign_check(GNUTLS_SIGN_RSA_SHA512, pkey_rsa, 0);
	inv_sign_check(GNUTLS_SIGN_RSA_SHA256, pkey_rsa, 0);
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_RSAE_SHA384, pkey_rsa, 0);
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_RSAE_SHA512, pkey_rsa, 0);
	/* an RSA key can also generate "pure" for TLS RSA-PSS signatures
	 * as they are essentially the same thing, and we cannot always
	 * know whether a key is RSA-PSS only, or not (e.g., in PKCS#11
	 * keys). */
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_SHA384, pkey_rsa, 0);
	inv_sign_check(GNUTLS_SIGN_RSA_PSS_SHA512, pkey_rsa, 0);

	gnutls_privkey_deinit(pkey_rsa);
	gnutls_privkey_deinit(pkey_rsa_pss);
	gnutls_x509_spki_deinit(spki);

	gnutls_global_deinit();
}
