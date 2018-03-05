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

/* Tests the certificate extension override in "trusted" PKCS#11 modules */

#ifdef _WIN32
# define P11LIB "libpkcs11mock1.dll"
#else
# define P11LIB "libpkcs11mock1.so"
#endif

static time_t mytime(time_t * t)
{
	time_t then = 1424466893;

	if (t)
		*t = then;

	return then;
}

void doit(void)
{
	int ret;
	gnutls_x509_crt_t crt, ocrt;
	unsigned keyusage;
	const char *lib;
	gnutls_pkcs11_obj_t *plist;
	unsigned int plist_size;
	gnutls_pkcs11_obj_t *plist2;
	unsigned int plist2_size, i;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	lib = getenv("P11MOCKLIB1");
	if (lib == NULL)
		lib = P11LIB;

	gnutls_global_set_time_function(mytime);
	if (debug) {
		gnutls_global_set_log_level(4711);
		success("loading lib %s\n", lib);
	}

	ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_add_provider(lib, "trusted");
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_x509_crt_init(&crt)>=0);
	assert(gnutls_x509_crt_init(&ocrt)>=0);

	/* check low level certificate import functions */
	ret = gnutls_pkcs11_obj_list_import_url4(&plist, &plist_size, "pkcs11:type=cert;object=cert1", 0);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_obj_list_import_url4(&plist2, &plist2_size, "pkcs11:type=cert;object=cert1", GNUTLS_PKCS11_OBJ_FLAG_OVERWRITE_TRUSTMOD_EXT);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	if (plist2_size != 1 || plist_size != 1) {
	    fail("could not import certs %d, %d\n", plist_size, plist2_size);
	}

	ret = gnutls_x509_crt_import_pkcs11(crt, plist[0]);
	if (ret != 0) {
	    fail("could not import cert!\n");
	}

	ret = gnutls_x509_crt_import_pkcs11(ocrt, plist2[0]);
	if (ret != 0) {
	    fail("could not import cert!\n");
	}

	for (i=0;i<plist_size;i++)
		gnutls_pkcs11_obj_deinit(plist[i]);
	for (i=0;i<plist2_size;i++)
		gnutls_pkcs11_obj_deinit(plist2[i]);
	gnutls_free(plist);
	gnutls_free(plist2);

	ret = gnutls_x509_crt_equals(crt, ocrt);
	if (ret != 0) {
	    fail("exported certificates are equal!\n");
	}

	ret = gnutls_x509_crt_get_ca_status(ocrt, NULL);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	if (ret == 0) {
		fail("overriden cert is not a CA!\n");
		exit(1);
	}

	ret = gnutls_x509_crt_get_key_usage(ocrt, &keyusage, NULL);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	if (keyusage != (GNUTLS_KEY_KEY_ENCIPHERMENT|GNUTLS_KEY_ENCIPHER_ONLY|GNUTLS_KEY_KEY_CERT_SIGN)) {
		fail("Extension does not have the expected key usage!\n");
	}

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_crt_deinit(ocrt);
	if (debug)
		printf("done\n\n\n");

	gnutls_global_deinit();
}
