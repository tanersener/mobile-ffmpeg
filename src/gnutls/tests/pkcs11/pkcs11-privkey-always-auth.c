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

# include "utils.h"
# include "pkcs11-mock-ext.h"

/* Tests whether a gnutls_privkey_t will work properly with a key marked
 * as always authenticate */

static unsigned pin_called = 0;
static const char *_pin = "1234";

# include <dlfcn.h>
# define P11LIB "libpkcs11mock1.so"


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
	int ret;
	const char *lib;
	gnutls_privkey_t key;
	gnutls_pkcs11_obj_t obj;
	gnutls_datum_t sig = {NULL, 0}, data;
	unsigned flags = 0;

	lib = getenv("P11MOCKLIB1");
	if (lib == NULL)
		lib = P11LIB;

	{
		void *dl;
		unsigned int *pflags;

		dl = dlopen(lib, RTLD_NOW);
		if (dl == NULL) {
			fail("could not dlopen %s\n", lib);
			exit(1);
		}

		pflags = dlsym(dl, "pkcs11_mock_flags");
		if (pflags == NULL) {
			fail("could find pkcs11_mock_flags\n");
			exit(1);
		}

		*pflags = MOCK_FLAG_ALWAYS_AUTH;
	}

	data.data = (void*)"\x38\x17\x0c\x08\xcb\x45\x8f\xd4\x87\x9c\x34\xb6\xf6\x08\x29\x4c\x50\x31\x2b\xbb";
	data.size = 20;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

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

	ret = gnutls_pkcs11_obj_init(&obj);
	assert(ret>=0);

	gnutls_pkcs11_obj_set_pin_function(obj, pin_func, NULL);

	ret = gnutls_pkcs11_obj_import_url(obj, "pkcs11:object=test;type=private", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	assert(ret>=0);

	ret = gnutls_pkcs11_obj_get_flags(obj, &flags);
	assert(ret>=0);

	if (!(flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_ALWAYS_AUTH)) {
		fail("key object doesn't have the always authenticate flag\n");
	}
	gnutls_pkcs11_obj_deinit(obj);


	ret = gnutls_privkey_init(&key);
	assert(ret>=0);

	gnutls_privkey_set_pin_function(key, pin_func, NULL);

	ret = gnutls_privkey_import_url(key, "pkcs11:object=test", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	pin_called = 0;

	ret = gnutls_privkey_sign_hash(key, GNUTLS_DIG_SHA1, 0, &data, &sig);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	if (pin_called == 0) {
		fail("PIN function wasn't called!\n");
	}
	pin_called = 0;

	gnutls_free(sig.data);

	/* call again - should re-authenticate */
	ret = gnutls_privkey_sign_hash(key, GNUTLS_DIG_SHA1, 0, &data, &sig);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	if (pin_called == 0) {
		fail("PIN function wasn't called twice!\n");
	}
	pin_called = 0;

	gnutls_free(sig.data);

	if (debug)
		printf("done\n\n\n");

	gnutls_privkey_deinit(key);
	gnutls_pkcs11_deinit();
	gnutls_global_deinit();
}
#endif
