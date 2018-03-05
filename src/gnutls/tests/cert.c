/*
 * Copyright (C) 2006-2012 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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
#include "config.h"
#endif

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>

#include "utils.h"

/* This program will load certificates from CERT_DIR and try to print
 * them. If CERT_DIR/certname.err is available, it should contain the
 * error code that gnutls_x509_crt_import() should return.
 */

#define CERT_DIR "certs-interesting"

static int getnextcert(DIR **dirp, gnutls_datum_t *der, int *exp_ret)
{
	struct dirent *d;
	char path[256];
	char cert_dir[256];
	const char *src;
	int ret;
	gnutls_datum_t local;

	src = getenv("srcdir");
	if (src == NULL)
		src = ".";

	snprintf(cert_dir, sizeof(cert_dir), "%s/%s", src, CERT_DIR);

	if (*dirp == NULL) {
		*dirp = opendir(cert_dir);
		if (*dirp == NULL)
			return -1;
	}

	do {
		d = readdir(*dirp);
		if (d != NULL
#ifdef _DIRENT_HAVE_D_TYPE
			&& d->d_type == DT_REG
#endif
			) {
			if (strstr(d->d_name, ".der") == 0)
				continue;
			if (strstr(d->d_name, ".err") != 0)
				continue;
			snprintf(path, sizeof(path), "%s/%s", cert_dir, d->d_name);

			success("Loading %s\n", path);
			ret = gnutls_load_file(path, der);
			if (ret < 0) {
				return -1;
			}

			snprintf(path, sizeof(path), "%s/%s.err", cert_dir, d->d_name);
			success("Loading errfile %s\n", path);
			ret = gnutls_load_file(path, &local);
			if (ret < 0) { /* not found assume success */
				*exp_ret = 0;
			} else {
				*exp_ret = atoi((char*)local.data);
				success("expecting error code %d\n", *exp_ret);
				gnutls_free(local.data);
				local.data = NULL;
			}

			return 0;
		}
	} while(d != NULL);

	closedir(*dirp);
	return -1; /* finished */
}

void doit(void)
{
	int ret, exp_ret;
	gnutls_x509_crt_t cert;
	gnutls_datum_t der;
	DIR *dirp = NULL;

	ret = global_init();
	if (ret < 0)
		fail("init %d\n", ret);

	while (getnextcert(&dirp, &der, &exp_ret)==0) {
		ret = gnutls_x509_crt_init(&cert);
		if (ret < 0)
			fail("crt_init %d\n", ret);

		ret = gnutls_x509_crt_import(cert, &der, GNUTLS_X509_FMT_DER);
		if (ret != exp_ret) {
			fail("crt_import %s\n", gnutls_strerror(ret));
		}

		if (ret == 0) {
			/* attempt to fully decode */
			gnutls_datum_t out;
			ret = gnutls_x509_crt_print(cert, GNUTLS_CRT_PRINT_FULL, &out);
			if (ret < 0) {
				fail("print: %s\n", gnutls_strerror(ret));
			}
			gnutls_free(out.data);
		}

		gnutls_x509_crt_deinit(cert);
		gnutls_free(der.data);
		der.data = NULL;
		der.size = 0;
		exp_ret = -1;
	}

	gnutls_global_deinit();
}
