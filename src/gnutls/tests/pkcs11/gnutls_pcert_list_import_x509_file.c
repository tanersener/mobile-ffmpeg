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

#define CONFIG_NAME "softhsm-import-url"
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

static int comp_cert(gnutls_pcert_st *pcert, unsigned i)
{
	int ret;
	gnutls_datum_t data;
	gnutls_x509_crt_t crt2;

	if (debug)
		success("comparing cert %d\n", i);

	ret = gnutls_x509_crt_init(&crt2);
	if (ret < 0)
		return -1;

	data.data = (void*)nc_good2[i];
	data.size = strlen(nc_good2[i]);
	ret = gnutls_x509_crt_import(crt2, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		return -1;

	if (!gnutls_x509_crt_equals2(crt2, &pcert->cert)) {
		return -1;
	}

	gnutls_x509_crt_deinit(crt2);

	return 0;
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

static void write_certs(const char *file)
{
	FILE *fp = fopen(file, "w");
	assert(fp != NULL);
	fwrite(nc_good2[0], strlen(nc_good2[0]), 1, fp);
	fwrite(nc_good2[4], strlen(nc_good2[4]), 1, fp);
	fwrite(nc_good2[1], strlen(nc_good2[1]), 1, fp);
	fwrite(nc_good2[2], strlen(nc_good2[2]), 1, fp);
	fwrite(nc_good2[3], strlen(nc_good2[3]), 1, fp);
	fclose(fp);
}

void doit(void)
{
	char buf[512];
	int ret;
	const char *lib, *bin;
	unsigned int i;
	gnutls_pcert_st pcerts[16];
	unsigned int pcerts_size;
	char file[TMPNAME_SIZE];

	track_temp_files();
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

	success("import from URI\n");
	pcerts_size = 2;
	ret = gnutls_pcert_list_import_x509_file(pcerts, &pcerts_size, SOFTHSM_URL";object=cert-0",
						GNUTLS_X509_FMT_PEM, pin_func, NULL, 0);
	assert(ret == GNUTLS_E_SHORT_MEMORY_BUFFER);

	pcerts_size = sizeof(pcerts)/sizeof(pcerts[0]);
	ret = gnutls_pcert_list_import_x509_file(pcerts, &pcerts_size, SOFTHSM_URL";object=cert-0",
						GNUTLS_X509_FMT_PEM, pin_func, NULL, 0);
	if (ret < 0)
		fail("cannot load certs: %s\n", gnutls_strerror(ret));

	assert(pcerts_size == 5);

	for (i=0;i<pcerts_size;i++)
		assert(comp_cert(&pcerts[i], i)>=0);

	for (i=0;i<pcerts_size;i++)
		gnutls_pcert_deinit(&pcerts[i]);

	/* Try testing importing from file
	 */
	success("import from file\n");
	get_tmpname(file);

	write_certs(file);

	pcerts_size = 2;
	ret = gnutls_pcert_list_import_x509_file(pcerts, &pcerts_size, file,
						GNUTLS_X509_FMT_PEM, pin_func, NULL, 0);
	assert(ret == GNUTLS_E_SHORT_MEMORY_BUFFER);

	pcerts_size = sizeof(pcerts)/sizeof(pcerts[0]);
	ret = gnutls_pcert_list_import_x509_file(pcerts, &pcerts_size, file,
						GNUTLS_X509_FMT_PEM, pin_func, NULL, 0);
	if (ret < 0)
		fail("cannot load certs: %s\n", gnutls_strerror(ret));

	assert(pcerts_size == 5);

	for (i=0;i<pcerts_size;i++)
		assert(comp_cert(&pcerts[i], i)>=0);

	for (i=0;i<pcerts_size;i++)
		gnutls_pcert_deinit(&pcerts[i]);

	gnutls_global_deinit();
	delete_temp_files();

	remove(CONFIG);
}

