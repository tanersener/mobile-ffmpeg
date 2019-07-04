/*
 * Copyright (C) 2008-2016 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <assert.h>

#include "utils.h"

#include "cert-common.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static time_t mytime(time_t * t)
{
	time_t then = 1207000800;

	if (t)
		*t = then;

	return then;
}

/* write V1 cert with extensions */
static void do_crt_with_exts(unsigned version)
{
	gnutls_x509_privkey_t pkey;
	gnutls_x509_crt_t crt;
	const char *err = NULL;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("global_init\n");

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = gnutls_x509_crt_init(&crt);
	if (ret != 0)
		fail("gnutls_x509_crt_init\n");

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret != 0)
		fail("gnutls_x509_privkey_init\n");

	ret = gnutls_x509_privkey_import(pkey, &key_dat, GNUTLS_X509_FMT_PEM);
	if (ret != 0)
		fail("gnutls_x509_privkey_import\n");

	/* Setup CRT */

	ret = gnutls_x509_crt_set_version(crt, version);
	if (ret != 0)
		fail("gnutls_x509_crt_set_version\n");

	ret = gnutls_x509_crt_set_serial(crt, "\x0a\x11\x00", 3);
	if (ret != 0)
		fail("gnutls_x509_crt_set_serial\n");

	ret = gnutls_x509_crt_set_expiration_time(crt, -1);
	if (ret != 0)
		fail("error\n");

	ret = gnutls_x509_crt_set_activation_time(crt, mytime(0));
	if (ret != 0)
		fail("error\n");

	ret = gnutls_x509_crt_set_key(crt, pkey);
	if (ret != 0)
		fail("gnutls_x509_crt_set_key\n");

	ret = gnutls_x509_crt_set_basic_constraints(crt, 0, -1); /* invalid for V1 */
	if (ret < 0) {
		fail("error\n");
	}

	ret = gnutls_x509_crt_set_key_usage(crt, GNUTLS_KEY_DIGITAL_SIGNATURE); /* inv for V1 */
	if (ret != 0)
		fail("gnutls_x509_crt_set_key_usage %d\n", ret);

	ret = gnutls_x509_crt_set_dn(crt, "o = none to\\, mention,cn = nikos", &err);
	if (ret < 0) {
		fail("gnutls_x509_crt_set_dn: %s, %s\n", gnutls_strerror(ret), err);
	}

	ret = gnutls_x509_crt_sign2(crt, crt, pkey, GNUTLS_DIG_SHA256, 0);
	if (ret != GNUTLS_E_X509_CERTIFICATE_ERROR) {
		gnutls_datum_t out;
		assert(gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_PEM, &out) >= 0);
		printf("%s\n\n", out.data);

		fail("gnutls_x509_crt_sign2: %s\n", gnutls_strerror(ret));
	}

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_privkey_deinit(pkey);

	gnutls_global_deinit();
}

/* write V1 cert with unique id */
static void do_v1_invalid_crt(void)
{
	gnutls_x509_privkey_t pkey;
	gnutls_x509_crt_t crt;
	const char *err = NULL;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("global_init\n");

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = gnutls_x509_crt_init(&crt);
	if (ret != 0)
		fail("gnutls_x509_crt_init\n");

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret != 0)
		fail("gnutls_x509_privkey_init\n");

	ret = gnutls_x509_privkey_import(pkey, &key_dat, GNUTLS_X509_FMT_PEM);
	if (ret != 0)
		fail("gnutls_x509_privkey_import\n");

	/* Setup CRT */

	ret = gnutls_x509_crt_set_version(crt, 1);
	if (ret != 0)
		fail("gnutls_x509_crt_set_version\n");

	ret = gnutls_x509_crt_set_serial(crt, "\x0a\x11\x00", 3);
	if (ret != 0)
		fail("gnutls_x509_crt_set_serial\n");

	ret = gnutls_x509_crt_set_expiration_time(crt, -1);
	if (ret != 0)
		fail("error\n");

	ret = gnutls_x509_crt_set_activation_time(crt, mytime(0));
	if (ret != 0)
		fail("error\n");

	ret = gnutls_x509_crt_set_key(crt, pkey);
	if (ret != 0)
		fail("gnutls_x509_crt_set_key\n");

	ret = gnutls_x509_crt_set_issuer_unique_id(crt, "\x00\x01\x03", 3);
	if (ret < 0) {
		fail("error\n");
	}

	ret = gnutls_x509_crt_set_dn(crt, "o = none to\\, mention,cn = nikos", &err);
	if (ret < 0) {
		fail("gnutls_x509_crt_set_dn: %s, %s\n", gnutls_strerror(ret), err);
	}

	ret = gnutls_x509_crt_sign2(crt, crt, pkey, GNUTLS_DIG_SHA256, 0);
	if (ret != GNUTLS_E_X509_CERTIFICATE_ERROR) {
		gnutls_datum_t out;
		assert(gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_PEM, &out) >= 0);
		printf("%s\n\n", out.data);

		fail("gnutls_x509_crt_sign2: %s\n", gnutls_strerror(ret));
	}

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_privkey_deinit(pkey);

	gnutls_global_deinit();
}

void doit(void)
{
	do_crt_with_exts(1);
	do_crt_with_exts(2);
	do_v1_invalid_crt();
}
