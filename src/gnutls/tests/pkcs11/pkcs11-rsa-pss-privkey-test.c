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

#include "../utils.h"
#include "softhsm.h"

#define CONFIG_NAME "softhsm-privkey-rsa-pss-test"
#define CONFIG CONFIG_NAME".config"

/* Tests whether signing with PKCS#11 and RSA-PSS would
 * generate valid signatures */

#include "../cert-common.h"

#define PIN "1234"

static const gnutls_datum_t testdata = { (void *)"test test", 9 };

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static
int pin_func(void *userdata, int attempt, const char *url, const char *label,
	     unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

#define myfail(fmt, ...) \
	fail("%s (iter %d): "fmt, gnutls_sign_get_name(sigalgo), i, ##__VA_ARGS__)

static unsigned verify_rsa_pss_presence(void)
{
	unsigned i;
	unsigned long mechanism;
	int ret;

	i = 0;
	do {
		ret = gnutls_pkcs11_token_get_mechanism("pkcs11:", i++, &mechanism);
		if (ret >= 0 && mechanism == 0xd /* CKM_RSA_PKCS_PSS*/)
			return 1;
	} while(ret>=0);

	return 0;
}

void doit(void)
{
	char buf[128];
	int ret;
	const char *lib, *bin;
	gnutls_x509_crt_t crt;
	gnutls_x509_privkey_t key;
	gnutls_datum_t tmp, sig;
	gnutls_privkey_t pkey;
	gnutls_pubkey_t pubkey;
	gnutls_pubkey_t pubkey2;
	unsigned i, sigalgo;

	bin = softhsm_bin();

	lib = softhsm_lib();

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs11_set_pin_function(pin_func, NULL);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	set_softhsm_conf(CONFIG);
	snprintf(buf, sizeof(buf),
		 "%s --init-token --slot 0 --label test --so-pin " PIN " --pin "
		 PIN, bin);
	system(buf);

	ret = gnutls_pkcs11_add_provider(lib, NULL);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	if (verify_rsa_pss_presence() == 0) {
		fprintf(stderr, "Skipping test as no RSA-PSS mech is supported\n");
		exit(77);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(crt, &cli_ca3_rsa_pss_cert,
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

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_privkey_import(key, &cli_ca3_rsa_pss_key,
				       GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_import: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	/* initialize softhsm token */
	ret = gnutls_pkcs11_token_init(SOFTHSM_URL, PIN, "test");
	if (ret < 0) {
		fail("gnutls_pkcs11_token_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_pkcs11_token_set_pin(SOFTHSM_URL, NULL, PIN,
					GNUTLS_PIN_USER);
	if (ret < 0) {
		fail("gnutls_pkcs11_token_set_pin: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_copy_x509_crt(SOFTHSM_URL, crt, "cert",
					  GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE |
					  GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_crt: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_pkcs11_copy_x509_privkey(SOFTHSM_URL, key, "cert",
					    GNUTLS_KEY_DIGITAL_SIGNATURE |
					    GNUTLS_KEY_KEY_ENCIPHERMENT,
					    GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE
					    |
					    GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE
					    | GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_privkey: %s\n",
		     gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_privkey_deinit(key);
	gnutls_pkcs11_set_pin_function(NULL, NULL);

	assert(gnutls_privkey_init(&pkey) == 0);

	ret =
	    gnutls_privkey_import_pkcs11_url(pkey,
					     SOFTHSM_URL
					     ";object=cert;object-type=private;pin-value="
					     PIN);
	if (ret < 0) {
		fprintf(stderr, "error in %d: %s\n", __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_pubkey_init(&pubkey) == 0);
	assert(gnutls_pubkey_import_privkey(pubkey, pkey, 0, 0) == 0);

	assert(gnutls_pubkey_init(&pubkey2) == 0);
	assert(gnutls_pubkey_import_x509_raw
	       (pubkey2, &cli_ca3_rsa_pss_cert, GNUTLS_X509_FMT_PEM, 0) == 0);

	/* this is the algorithm supported by the certificate */
	sigalgo = GNUTLS_SIGN_RSA_PSS_SHA256;

	for (i = 0; i < 20; i++) {
		/* check whether privkey and pubkey are operational
		 * by signing and verifying */
		ret =
		    gnutls_privkey_sign_data2(pkey, sigalgo, 0,
					      &testdata, &sig);
		if (ret < 0)
			myfail("Error signing data %s\n", gnutls_strerror(ret));

		/* verify against the pubkey in PKCS #11 */
		ret =
		    gnutls_pubkey_verify_data2(pubkey, sigalgo, 0,
					       &testdata, &sig);
		if (ret < 0)
			myfail("Error verifying data1: %s\n",
			       gnutls_strerror(ret));

		/* verify against the raw pubkey */
		ret =
		    gnutls_pubkey_verify_data2(pubkey2, sigalgo, 0,
					       &testdata, &sig);
		if (ret < 0)
			myfail("Error verifying data2: %s\n",
			       gnutls_strerror(ret));

		gnutls_free(sig.data);
	}

	gnutls_pubkey_deinit(pubkey2);
	gnutls_pubkey_deinit(pubkey);
	gnutls_privkey_deinit(pkey);

	gnutls_global_deinit();

	remove(CONFIG);
}
