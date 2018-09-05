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

/* Tests whether a protected object is imported with PIN obtained using
 * pin-value or pin-source. */

#define CONFIG_NAME "softhsm-import-with-pin"
#define CONFIG CONFIG_NAME".config"

#include "../cert-common.h"

#define PIN "1234"

static const gnutls_datum_t testdata = {(void*)"test test", 9};

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
		unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

static void write_pin(const char *file, const char *pin)
{
	FILE *fp = fopen(file, "w");
	assert(fp != NULL);
	fputs(pin, fp);
	fclose(fp);
}

void doit(void)
{
	char buf[512];
	int ret;
	const char *lib, *bin;
	gnutls_x509_privkey_t key;
	gnutls_datum_t sig;
	gnutls_privkey_t pkey;
	char file[TMPNAME_SIZE];

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
	snprintf(buf, sizeof(buf), "%s --init-token --slot 0 --label test --so-pin "PIN" --pin "PIN, bin);
	system(buf);

	ret = gnutls_pkcs11_add_provider(lib, "trusted");
	if (ret < 0) {
		fprintf(stderr, "add_provider: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_privkey_import(key, &server_key,
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

	ret = gnutls_pkcs11_token_set_pin(SOFTHSM_URL, NULL, PIN, GNUTLS_PIN_USER);
	if (ret < 0) {
		fail("gnutls_pkcs11_token_set_pin: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_copy_x509_privkey(SOFTHSM_URL, key, "cert", GNUTLS_KEY_DIGITAL_SIGNATURE|GNUTLS_KEY_KEY_ENCIPHERMENT,
					      GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE|GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE|GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_privkey: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_privkey_deinit(key);
	gnutls_pkcs11_set_pin_function(NULL, NULL);

	assert(gnutls_privkey_init(&pkey) == 0);

	/* Test 1
	 * Try importing with wrong pin-value */
	ret = gnutls_privkey_import_pkcs11_url(pkey, SOFTHSM_URL";object=cert;object-type=private;pin-value=XXXX");
	if (ret != GNUTLS_E_PKCS11_PIN_ERROR) {
		fprintf(stderr, "unexpected error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}
	gnutls_privkey_deinit(pkey);
	assert(gnutls_privkey_init(&pkey) == 0);

	/* Test 2
	 * Try importing with pin-value */
	ret = gnutls_privkey_import_pkcs11_url(pkey, SOFTHSM_URL";object=cert;object-type=private;pin-value="PIN);
	if (ret < 0) {
		fprintf(stderr, "error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	/* check whether privkey is operational by signing */
	assert(gnutls_privkey_sign_data(pkey, GNUTLS_DIG_SHA256, 0, &testdata, &sig) == 0);
	gnutls_free(sig.data);
	gnutls_privkey_deinit(pkey);

	/* Test 3
	 * Try importing with wrong pin-source */
	track_temp_files();
	get_tmpname(file);

	write_pin(file, "XXXX");

	assert(gnutls_privkey_init(&pkey) == 0);
	snprintf(buf, sizeof(buf), "%s;object=cert;object-type=private;pin-source=%s", SOFTHSM_URL, file);
	ret = gnutls_privkey_import_pkcs11_url(pkey, buf);
	if (ret != GNUTLS_E_PKCS11_PIN_ERROR) {
		fprintf(stderr, "error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_privkey_deinit(pkey);

	/* Test 4
	 * Try importing with pin-source */
	write_pin(file, PIN);

	assert(gnutls_privkey_init(&pkey) == 0);
	snprintf(buf, sizeof(buf), "%s;object=cert;object-type=private;pin-source=%s", SOFTHSM_URL, file);
	ret = gnutls_privkey_import_pkcs11_url(pkey, buf);
	if (ret < 0) {
		fprintf(stderr, "error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	/* check whether privkey is operational by signing */
	assert(gnutls_privkey_sign_data(pkey, GNUTLS_DIG_SHA256, 0, &testdata, &sig) == 0);
	gnutls_free(sig.data);
	gnutls_privkey_deinit(pkey);

	gnutls_global_deinit();
	delete_temp_files();

	remove(CONFIG);
}

