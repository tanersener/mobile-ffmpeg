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
#include <sys/types.h>
#include <sys/wait.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <gnutls/pkcs11.h>

#ifdef _WIN32

void doit(void)
{
	exit(77);
}

#else

#include "../utils.h"
#include "softhsm.h"
#include <assert.h>

#define CONFIG_NAME "softhsm-generate"
#define CONFIG CONFIG_NAME".config"
#define PIN "1234"
/* Tests whether a gnutls_privkey_generate3 will work generate a key
 * which is marked as sensitive.
 */

static unsigned pin_called = 0;
static const char *_pin = PIN;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
		unsigned flags, char *pin, size_t pin_max)
{
	if (_pin == NULL)
		return -1;

	strcpy(pin, _pin);
	pin_called++;
	return 0;
}

void doit(void)
{
	char buf[128];
	int ret;
	const char *lib, *bin;
	gnutls_datum_t out;
	unsigned flags;
	gnutls_pkcs11_obj_t obj;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	bin = softhsm_bin();

	lib = softhsm_lib();

	set_softhsm_conf(CONFIG);
	snprintf(buf, sizeof(buf), "%s --init-token --slot 0 --label test --so-pin "PIN" --pin "PIN, bin);
	system(buf);

	ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_add_provider(lib, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs11_set_pin_function(pin_func, NULL);

	/* generate sensitive */
	ret = gnutls_pkcs11_privkey_generate3("pkcs11:token=test", GNUTLS_PK_RSA, 2048,
					      "testkey", NULL, GNUTLS_X509_FMT_DER,
					      &out, 0, GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_pkcs11_obj_init(&obj) >= 0);
	assert(out.size > 0);

	gnutls_pkcs11_obj_set_pin_function(obj, pin_func, NULL);
	assert(gnutls_pkcs11_obj_import_url(obj, "pkcs11:token=test;object=testkey;type=private", GNUTLS_PKCS11_OBJ_FLAG_LOGIN) >= 0);

	assert(gnutls_pkcs11_obj_get_flags(obj, &flags) >= 0);

	assert(!(flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_SENSITIVE));
	assert(flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE);

	gnutls_free(out.data);
	gnutls_pkcs11_obj_deinit(obj);

	/* generate non-sensitive */
	ret = gnutls_pkcs11_privkey_generate3("pkcs11:token=test", GNUTLS_PK_RSA, 2048,
					      "testkey2", NULL, GNUTLS_X509_FMT_DER,
					      &out, 0, GNUTLS_PKCS11_OBJ_FLAG_LOGIN|GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_SENSITIVE);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_pkcs11_obj_init(&obj) >= 0);
	assert(out.size > 0);

	gnutls_pkcs11_obj_set_pin_function(obj, pin_func, NULL);
	assert(gnutls_pkcs11_obj_import_url(obj, "pkcs11:token=test;object=testkey2;type=private", GNUTLS_PKCS11_OBJ_FLAG_LOGIN) >= 0);

	assert(gnutls_pkcs11_obj_get_flags(obj, &flags) >= 0);

	assert(!(flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE));
	assert(flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_SENSITIVE);

	gnutls_free(out.data);
	gnutls_pkcs11_obj_deinit(obj);

	gnutls_pkcs11_deinit();
	gnutls_global_deinit();
	remove(CONFIG);
}
#endif
