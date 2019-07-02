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

#include "utils.h"

/* Tests whether a gnutls_privkey_t will continue to work after
 * a fork(). */

#if defined(HAVE___REGISTER_ATFORK)

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
	gnutls_datum_t sig = {NULL, 0}, data;
	pid_t pid;

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

	gnutls_privkey_set_pin_function(key, pin_func, NULL);

	ret = gnutls_privkey_import_url(key, "pkcs11:object=test", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_privkey_sign_hash(key, GNUTLS_DIG_SHA1, 0, &data, &sig);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_free(sig.data);

	pid = fork();
	if (pid != 0) {
		int status;
		assert(waitpid(pid, &status, 0) >= 0);

		if (WEXITSTATUS(status) != 0) {
			fail("child return status was unexpected: %d\n", WEXITSTATUS(status));
			exit(1);
		}
	} else { /* child */
		ret = gnutls_privkey_sign_hash(key, GNUTLS_DIG_SHA1, 0, &data, &sig);
		if (ret < 0) {
			fail("%d: %s\n", ret, gnutls_strerror(ret));
			exit(1);
		}

		gnutls_free(sig.data);		
		gnutls_privkey_deinit(key);
		gnutls_pkcs11_deinit();
		gnutls_global_deinit();
		exit(0);
	}

	if (debug)
		printf("done\n\n\n");

	gnutls_privkey_deinit(key);
	gnutls_pkcs11_deinit();
	gnutls_global_deinit();
}
#else
void doit(void)
{
	exit(77);
}
#endif
