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
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>

#include "utils.h"
#include "pkcs11-mock-ext.h"

/* Tests the private key import for sensitive keys in the common case and in
 * some problematic cases. */

#ifdef _WIN32
# define P11LIB "libpkcs11mock1.dll"
#else
# include <dlfcn.h>
# define P11LIB "libpkcs11mock1.so"
#endif

void doit(void)
{
	int ret;
	const char *lib;
	gnutls_pkcs11_obj_t *obj_list;
	unsigned int obj_list_size = 0;
	unsigned int i;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

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

	ret = gnutls_pkcs11_obj_list_import_url4(&obj_list, &obj_list_size, "pkcs11:", GNUTLS_PKCS11_OBJ_FLAG_PRIVKEY);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	for (i=0;i<obj_list_size;i++)
		gnutls_pkcs11_obj_deinit(obj_list[i]);
	gnutls_free(obj_list);
	obj_list = NULL;
	obj_list_size = 0;

#ifndef _WIN32
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

		*pflags = MOCK_FLAG_BROKEN_GET_ATTRIBUTES;

		ret = gnutls_pkcs11_obj_list_import_url4(&obj_list, &obj_list_size, "pkcs11:", GNUTLS_PKCS11_OBJ_FLAG_PRIVKEY);
		if (ret < 0) {
			fail("%d: %s\n", ret, gnutls_strerror(ret));
			exit(1);
		}

		for (i=0;i<obj_list_size;i++)
			gnutls_pkcs11_obj_deinit(obj_list[i]);
		gnutls_free(obj_list);
		obj_list = NULL;
		obj_list_size = 0;
	}
#endif

	if (debug)
		printf("done\n\n\n");

	gnutls_global_deinit();
}
