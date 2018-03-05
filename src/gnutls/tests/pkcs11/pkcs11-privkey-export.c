/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"

/* This checks whether the public parts of RSA private and public keys
 * can be properly extracted from a PKCS#11 module. */

#define PIN "1234"
#ifdef _WIN32
# define P11LIB "libpkcs11mock1.dll"
#else
# include <dlfcn.h>
# define P11LIB "libpkcs11mock1.so"
#endif


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
	int ret;
	const char *lib;
	gnutls_privkey_t key;
	gnutls_pubkey_t pub;
	gnutls_datum_t m1, e1;
	gnutls_datum_t m2, e2;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	lib = getenv("P11MOCKLIB1");
	if (lib == NULL)
		lib = P11LIB;

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

	ret = gnutls_privkey_init(&key);
	assert(ret>=0);
	ret = gnutls_pubkey_init(&pub);
	assert(ret>=0);

	gnutls_privkey_set_pin_function(key, pin_func, NULL);

	ret = gnutls_privkey_import_url(key, "pkcs11:object=test", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pubkey_export_rsa_raw(pub, &m1, &e1);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pubkey_deinit(pub);
	gnutls_privkey_deinit(key);

	/* try again using gnutls_pubkey_import_url */	
	ret = gnutls_pubkey_init(&pub);
	assert(ret>=0);

	ret = gnutls_pubkey_import_url(pub, "pkcs11:object=test;type=public", 0);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pubkey_export_rsa_raw(pub, &m2, &e2);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(m1.size == m2.size);
	assert(e1.size == e2.size);
	assert(memcmp(e1.data, e2.data, e2.size)==0);
	assert(memcmp(m1.data, m2.data, m2.size)==0);

	gnutls_pubkey_deinit(pub);
	gnutls_free(m1.data);
	gnutls_free(e1.data);
	gnutls_free(m2.data);
	gnutls_free(e2.data);
	gnutls_pkcs11_deinit();
	gnutls_global_deinit();
}
