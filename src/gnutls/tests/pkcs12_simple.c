/*
 * Copyright (C) 2005-2012 Free Software Foundation, Inc.
 * Copyright (C) 2012 Nikos Mavrogiannopoulos
 *
 * Author: Simon Josefsson
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
#include <gnutls/pkcs12.h>
#include <gnutls/x509.h>
#include "utils.h"

#ifdef ENABLE_NON_SUITEB_CURVES
static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}
#endif

void doit(void)
{
#ifdef ENABLE_NON_SUITEB_CURVES
	const char *filename, *password = "1234";
	gnutls_pkcs12_t pkcs12;
	gnutls_datum_t data;
	gnutls_x509_crt_t *chain, *extras;
	unsigned int chain_size = 0, extras_size = 0, i;
	gnutls_x509_privkey_t pkey;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("global_init failed %d\n", ret);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	ret = gnutls_pkcs12_init(&pkcs12);
	if (ret < 0)
		fail("initialization failed: %s\n", gnutls_strerror(ret));

	filename = getenv("PKCS12_MANY_CERTS_FILE");

	if (!filename)
		filename = "pkcs12-decode/pkcs12_5certs.p12";

	if (debug)
		success
		    ("Reading PKCS#12 blob from `%s' using password `%s'.\n",
		     filename, password);

	ret = gnutls_load_file(filename, &data);
	if (ret < 0)
		fail("cannot open file");

	ret = gnutls_pkcs12_import(pkcs12, &data, GNUTLS_X509_FMT_DER, 0);
	if (ret < 0)
		fail("pkcs12_import failed %d: %s\n", ret,
		     gnutls_strerror(ret));

	if (debug)
		success("Read file OK\n");

	ret =
	    gnutls_pkcs12_simple_parse(pkcs12, password, &pkey, &chain,
					&chain_size, &extras, &extras_size,
					NULL, 0);
	if (ret < 0)
		fail("pkcs12_simple_parse failed %d: %s\n", ret,
		     gnutls_strerror(ret));

	if (chain_size != 1)
		fail("chain size (%u) should have been 1\n", chain_size);

	if (extras_size != 4)
		fail("extras size (%u) should have been 4\n", extras_size);

	if (debug) {
		char dn[512];
		size_t dn_size;

		dn_size = sizeof(dn);
		ret = gnutls_x509_crt_get_dn(chain[0], dn, &dn_size);
		if (ret < 0)
			fail("crt_get_dn failed %d: %s\n", ret,
			     gnutls_strerror(ret));

		success("dn: %s\n", dn);

		dn_size = sizeof(dn);
		ret =
		    gnutls_x509_crt_get_issuer_dn(chain[0], dn, &dn_size);
		if (ret < 0)
			fail("crt_get_dn failed %d: %s\n", ret,
			     gnutls_strerror(ret));

		success("issuer dn: %s\n", dn);
	}

	gnutls_pkcs12_deinit(pkcs12);
	gnutls_x509_privkey_deinit(pkey);

	for (i = 0; i < chain_size; i++)
		gnutls_x509_crt_deinit(chain[i]);
	gnutls_free(chain);

	for (i = 0; i < extras_size; i++)
		gnutls_x509_crt_deinit(extras[i]);
	gnutls_free(extras);

	/* Try gnutls_x509_privkey_import2() */
	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0)
		fail("gnutls_x509_privkey_init failed %d: %s\n", ret,
		     gnutls_strerror(ret));

	ret =
	    gnutls_x509_privkey_import2(pkey, &data, GNUTLS_X509_FMT_DER,
					password, 0);
	if (ret < 0)
		fail("gnutls_x509_privkey_import2 failed %d: %s\n", ret,
		     gnutls_strerror(ret));
	gnutls_x509_privkey_deinit(pkey);

	free(data.data);

	gnutls_global_deinit();
#else
	exit(77);
#endif
}
