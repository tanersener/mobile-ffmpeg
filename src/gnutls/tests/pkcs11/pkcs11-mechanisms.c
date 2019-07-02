/*
 * Copyright (C) 2016-2017 Red Hat, Inc.
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"

/* Tests whether a gnutls_privkey_t will continue to work after
 * a fork(), when gnutls_pkcs11_reinit() is manually called. */

#if defined(HAVE___REGISTER_ATFORK)

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

void doit(void)
{
	int ret;
	const char *lib;
	unsigned long mech;
	unsigned i;

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

	for (i=0;;i++) {
		ret = gnutls_pkcs11_token_get_mechanism("pkcs11:", i, &mech);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;
		success("mech: %lu\n", mech);
		ret = gnutls_pkcs11_token_check_mechanism("pkcs11:", mech, NULL, 0, 0);
		if (ret == 0) {
			fail("mechanism %ld was reported are supported, but is not found!\n", mech);
		}
	}
	if (debug)
		printf("done\n\n\n");

	ret = gnutls_pkcs11_token_check_mechanism("pkcs11:", -1, NULL, 0, 0);
	if (ret != 0)
		fail("found invalid mechanism1\n");

	ret = gnutls_pkcs11_token_check_mechanism("pkcs11:", -3, NULL, 0, 0);
	if (ret != 0)
		fail("found invalid mechanism2\n");

	gnutls_pkcs11_deinit();
	gnutls_global_deinit();
}
#else
void doit(void)
{
	exit(77);
}
#endif
