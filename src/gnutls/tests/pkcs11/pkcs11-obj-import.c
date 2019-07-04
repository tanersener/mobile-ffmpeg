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

#define CONFIG_NAME "softhsm-obj-import"
#define CONFIG CONFIG_NAME".config"

#include "../utils.h"
#include "softhsm.h"

/* Tests whether gnutls_pubkey_import_privkey works well for 
 * RSA keys under PKCS #11 */


#include "../cert-common.h"

#define PIN "1234"

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

void doit(void)
{
	static char buf[1024];
	int ret;
	const char *lib, *bin;
	gnutls_x509_crt_t crt;
	gnutls_pkcs11_obj_t obj;
	char *url;
	gnutls_datum_t tmp, tmp2;
	size_t buf_size;

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
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(crt, &server_cert,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_import: %s\n",
			gnutls_strerror(ret));
			exit(1);
	}

	if (debug) {
		gnutls_x509_crt_print(crt,
			      GNUTLS_CRT_PRINT_ONELINE,
			      &tmp);

		printf("\tCertificate: %.*s\n",
		       tmp.size, tmp.data);
		gnutls_free(tmp.data);
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

	ret = gnutls_pkcs11_copy_x509_crt(SOFTHSM_URL, crt, "cert",
					  GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_PRIVATE|GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_crt: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs11_set_pin_function(NULL, NULL);

	assert(gnutls_pkcs11_obj_init(&obj) >= 0);

	ret = gnutls_pkcs11_obj_import_url(obj, SOFTHSM_URL";object=cert", 0);
	if (ret < 0) {
		fprintf(stderr, "error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_pkcs11_obj_export_url(obj, GNUTLS_PKCS11_URL_GENERIC, &url) >= 0);
	assert(url != NULL);

	gnutls_free(url);

	assert(gnutls_pkcs11_obj_export3(obj, GNUTLS_X509_FMT_DER, &tmp) >= 0);
	assert(gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &tmp2) >= 0);

	assert(tmp2.size == tmp.size);
	assert(memcmp(tmp.data, tmp2.data, tmp.size) == 0);
	gnutls_free(tmp2.data);

	/* check gnutls_pkcs11_obj_export */
	buf_size = 4;
	assert(gnutls_pkcs11_obj_export(obj, buf, &buf_size) == GNUTLS_E_SHORT_MEMORY_BUFFER);

	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_export(obj, buf, &buf_size)>=0);
	assert(buf_size == tmp.size);
	assert(memcmp(buf, tmp.data, tmp.size) == 0);

	gnutls_free(tmp.data);

	/* The ID is constant and copied from the certificate */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_ID_HEX, buf, &buf_size) >= 0);
	assert(buf_size == 60);
	assert(memcmp(buf, "95:d1:ad:a4:52:e4:c5:61:12:a6:91:13:8d:80:dd:2d:81:22:3e:d4", 60) == 0);

	/* label */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_LABEL, buf, &buf_size) >= 0);
	assert(buf_size == 4);
	assert(memcmp(buf, "cert", 4) == 0);

	/* token-label */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_TOKEN_LABEL, buf, &buf_size) >= 0);
	assert(buf_size == 4);
	assert(memcmp(buf, "test", 4) == 0);

	/* token-serial */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_TOKEN_SERIAL, buf, &buf_size) >= 0);
	assert(buf_size != 0);
	assert(strlen(buf) != 0);

	/* token-model */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_TOKEN_MODEL, buf, &buf_size) >= 0);
	assert(buf_size != 0);
	assert(strlen(buf) != 0);

	/* token-manufacturer */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_TOKEN_MANUFACTURER, buf, &buf_size) >= 0);
	assert(buf_size != 0);
	assert(strlen(buf) != 0);

	/* token-ID */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_ID, buf, &buf_size) >= 0);
	assert(buf_size != 0);

	/* library description */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_LIBRARY_DESCRIPTION, buf, &buf_size) >= 0);
	assert(buf_size != 0);
	assert(strlen(buf) != 0);

	/* library manufacturer */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_LIBRARY_MANUFACTURER, buf, &buf_size) >= 0);
	assert(buf_size != 0);
	assert(strlen(buf) != 0);

	/* library version */
	buf_size = sizeof(buf);
	assert(gnutls_pkcs11_obj_get_info(obj, GNUTLS_PKCS11_OBJ_LIBRARY_VERSION, buf, &buf_size) >= 0);
	assert(buf_size != 0);
	assert(strlen(buf) != 0);

	gnutls_pkcs11_obj_deinit(obj);
	gnutls_x509_crt_deinit(crt);

	gnutls_global_deinit();

	remove(CONFIG);
}

