/*
 * Copyright (C) 2005-2012 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

typedef struct {
	const char *file;
	const char *pass;
} files_st;

files_st files[] = {
	{"client.p12", "foobar"},
	{"cert-ca.p12", "1234"}, /* 2 certs, one is a CA */
	{"pkcs12_2certs.p12", ""}, /* 2 certs, on is unrelated */
	{NULL, NULL}
};

void doit(void)
{
	gnutls_certificate_credentials_t x509cred;
	const char *path;
	unsigned int i;
	char file[512];
	int ret;

	if (gnutls_fips140_mode_enabled()) {
		exit(77);
	}

	ret = global_init();
	if (ret < 0)
		fail("global_init failed %d\n", ret);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	for (i = 0; files[i].file != NULL; i++) {

		ret = gnutls_certificate_allocate_credentials(&x509cred);
		if (ret < 0)
			fail("gnutls_certificate_allocate_credentials failed %d\n", ret);

		path = getenv("PKCS12PATH");
		if (!path)
			path = "cert-tests/data/";

		snprintf(file, sizeof(file), "%s/%s", path, files[i].file);

		if (debug)
			success
			    ("Reading PKCS#12 blob from `%s' using password `%s'.\n",
			     file, files[i].pass);
		ret =
		    gnutls_certificate_set_x509_simple_pkcs12_file(x509cred,
								   file,
								   GNUTLS_X509_FMT_DER,
								   files[i].
								   pass);
		if (ret < 0)
			fail("x509_pkcs12 failed %d: %s\n", ret,
			     gnutls_strerror(ret));

		if (debug)
			success("Read file OK\n");

		gnutls_certificate_free_credentials(x509cred);
	}

	gnutls_global_deinit();
}
