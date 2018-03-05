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
#include <assert.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>

#include "utils.h"

/* Tests the gnutls_pkcs11_obj_get_exts API */

#ifdef _WIN32
# define P11LIB "libpkcs11mock1.dll"
#else
# define P11LIB "libpkcs11mock1.so"
#endif

void doit(void)
{
	int ret;
	const char *lib;
	gnutls_x509_ext_st *exts;
	unsigned int exts_size, i;
	gnutls_pkcs11_obj_t obj;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	lib = getenv("P11MOCKLIB1");
	if (lib == NULL)
		lib = P11LIB;

	if (debug) {
		gnutls_global_set_log_level(4711);
		success("loading lib %s\n", lib);
	}

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

	assert(gnutls_pkcs11_obj_init(&obj)>=0);

	/* check extensions */
	ret = gnutls_pkcs11_obj_import_url(obj, "pkcs11:type=cert;object=cert1", 0);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_obj_get_exts(obj, &exts, &exts_size, 0);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	if (exts_size != 2) {
		fail("the expected extensions were not found (found %d)!\n", exts_size);
		exit(1);
	}

	if (strcmp(exts[0].oid, "2.5.29.19") != 0) {
		fail("Found OID for %d: %s\n", 0, exts[0].oid);
	}

	{
		unsigned ca;
		int pathlen;
		ret = gnutls_x509_ext_import_basic_constraints(&exts[0].data, &ca, &pathlen);
		if (ret < 0) {
			fail("%d: %s\n", ret, gnutls_strerror(ret));
			exit(1);
		}

		if (debug)
			success("ca: %d/%d\n", ca, pathlen);
		if (ca != 1) {
			fail("Extension does not set the CA constraint!\n");
		}
	}

	if (strcmp(exts[1].oid, "2.5.29.15") != 0) {
		fail("Found OID for %d: %s\n", 1, exts[1].oid);
	}

	{
		unsigned keyusage;
		ret = gnutls_x509_ext_import_key_usage(&exts[1].data, &keyusage);
		if (ret < 0) {
			fail("%d: %s\n", ret, gnutls_strerror(ret));
			exit(1);
		}

		if (debug)
			success("usage: %x\n", keyusage);
		if (keyusage != (GNUTLS_KEY_KEY_ENCIPHERMENT|GNUTLS_KEY_ENCIPHER_ONLY|GNUTLS_KEY_KEY_CERT_SIGN)) {
			fail("Extension does not have the expected key usage!\n");
		}
	}

	for (i=0;i<exts_size;i++)
		gnutls_x509_ext_deinit(&exts[i]);
	gnutls_free(exts);

	gnutls_pkcs11_obj_deinit(obj);
	if (debug)
		printf("done\n\n\n");

	gnutls_global_deinit();
}
