/*
 * Copyright (C) 2018 Nikos Mavrogiannopoulos
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

/* Tests whether gnutls_x509_crt_list_import_url() will return a well
 * sorted chain, out of values written to softhsm token.
 */

#define CONFIG_NAME "x509-crt-list-import-url"
#define CONFIG CONFIG_NAME".config"

#include "../test-chains.h"

#define PIN "123456"

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

static void comp_cert(gnutls_x509_crt_t crt1, unsigned i)
{
	int ret;
	gnutls_datum_t data;
	gnutls_x509_crt_t crt2;

	ret = gnutls_x509_crt_init(&crt2);
	if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));

	data.data = (void*)nc_good2[i];
	data.size = strlen(nc_good2[i]);
	ret = gnutls_x509_crt_import(crt2, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("error[%d]: %s\n", i, gnutls_strerror(ret));

	if (!gnutls_x509_crt_equals(crt1, crt2)) {
		fail("certificate doesn't match chain at %d\n", i);
	}

	gnutls_x509_crt_deinit(crt2);
}

static void load_cert(const char *url, unsigned i)
{
	int ret;
	gnutls_datum_t data;
	gnutls_x509_crt_t crt;
	char name[64];

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));

	data.data = (void*)nc_good2[i];
	data.size = strlen(nc_good2[i]);
	ret = gnutls_x509_crt_import(crt, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("error[%d]: %s\n", i, gnutls_strerror(ret));

	snprintf(name, sizeof(name), "cert-%d", i);
	ret = gnutls_pkcs11_copy_x509_crt(url, crt, name, GNUTLS_PKCS11_OBJ_FLAG_LOGIN|GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE);
	if (ret < 0)
		fail("error[%d]: %s\n", i, gnutls_strerror(ret));

	success("written cert-%d\n", i);

	gnutls_x509_crt_deinit(crt);
}

static void load_chain(const char *url)
{
	load_cert(url, 1);
	load_cert(url, 0);
	load_cert(url, 4);
	load_cert(url, 2);
	load_cert(url, 3);
}

void doit(void)
{
	char buf[512];
	int ret;
	const char *lib, *bin;
	gnutls_x509_crt_t *crts;
	unsigned int crts_size, i;

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

	ret = gnutls_pkcs11_add_provider(lib, NULL);
	if (ret < 0) {
		fprintf(stderr, "add_provider: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	/* initialize softhsm token */
	ret = gnutls_pkcs11_token_init(SOFTHSM_URL, PIN, "test");
	if (ret < 0) {
		fail("gnutls_pkcs11_token_init: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_pkcs11_token_set_pin(SOFTHSM_URL, NULL, PIN, GNUTLS_PIN_USER);
	if (ret < 0) {
		fail("gnutls_pkcs11_token_set_pin: %s\n", gnutls_strerror(ret));
	}

	load_chain(SOFTHSM_URL);
	gnutls_pkcs11_set_pin_function(NULL, NULL);

	/* try importing without login */
	ret = gnutls_x509_crt_list_import_url(&crts, &crts_size, SOFTHSM_URL";object=cert-0",
					      pin_func, NULL, 0);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
		fail("cannot load certs: %s\n", gnutls_strerror(ret));

	/* try importing with login */
	ret = gnutls_x509_crt_list_import_url(&crts, &crts_size, SOFTHSM_URL";object=cert-0",
					      pin_func, NULL, GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0)
		fail("cannot load certs: %s\n", gnutls_strerror(ret));

	assert(crts_size == 5);

	for (i=0;i<crts_size;i++)
		comp_cert(crts[i], i);

	for (i=0;i<crts_size;i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);

	gnutls_global_deinit();
	delete_temp_files();

	remove(CONFIG);
}

