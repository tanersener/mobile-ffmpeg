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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <assert.h>

#include "utils.h"
#include "cert-common.h"

static time_t mytime(time_t * t)
{
	time_t then = 1207000800;

	if (t)
		*t = then;

	return then;
}

static unsigned char saved_crl_pem[] =
	"-----BEGIN X509 CRL-----\n"
	"MIICXzCByAIBADANBgkqhkiG9w0BAQUFADAPMQ0wCwYDVQQDEwRDQS0zFw0wODAz\n"
	"MzEyMjAwMDBaFw0wODAzMzEyMjAxMDBaMFQwFAIDAQIDFw0wODAzMzEyMjAwMDBa\n"
	"MB0CDFejHTI2Wi75obBaUhcNMDgwMzMxMjIwMDAwWjAdAgxXox0yNbNP0Ln15zwX\n"
	"DTA4MDMzMTIyMDAwMFqgLzAtMB8GA1UdIwQYMBaAFPmohhljtqQUE2B2DwGaNTbv\n"
	"8bSvMAoGA1UdFAQDAgEBMA0GCSqGSIb3DQEBBQUAA4IBgQAcVsFF0HzAjAtD4Kwh\n"
	"pJwVl6BEC4lybSIVB0+ls/b23cEOfU1wE8Ls+26EjUHLOTCdQgKMFgbEuhAgUOb6\n"
	"kuatoWmi3R/42FJDvQxc+aYcEOX5ttbbB4KuS77zQ54Nv9RGyKcXqTDmax2MgqKg\n"
	"moIbYhemiUl4zCshPZvv0NsHFiDtToSIHZIbIy3u63/Mb/tXCm2Eyrl8za8ELGaJ\n"
	"5zjibO2wNRIwd7QbJJRkc6TrphfWxeU6tZi3rwOLoqf8x4EBWOcKXyUvIb+OxNVH\n"
	"aMXFxVCTmDAqxe9HrEzZsQIGS7CDlWCghIUW8AQkPJ/IL4kUvZhmRxyqI8DF4mLI\n"
	"XqCDF55CaQ5e2uMc3f5rvNTP1g1S7E/iZRTaATVhB6krha6X3MqEQ+VJnMklJPiI\n"
	"aZY5JS5apO9ewXykxuK0/A3BeHSdK4fj3Q1mt1NzX4G9cU2T3VdPRbAgchoU2YV3\n"
	"pBeFxTaJMEN+ajgixeXC69iE7aNBOFBLC38uPmMOpZ450q8=\n"
	"-----END X509 CRL-----\n";

static unsigned char saved_min_crl_pem[] =
	"-----BEGIN X509 CRL-----\n"
	"MIICUDCBuQIBADANBgkqhkiG9w0BAQUFADAPMQ0wCwYDVQQDEwRDQS0zFw0wODAz\n"
	"MzEyMjAwMDBaMFQwFAIDAQIDFw0wODAzMzEyMjAwMDBaMB0CDFejHTI2Wi75obBa\n"
	"UhcNMDgwMzMxMjIwMDAwWjAdAgxXox0yNbNP0Ln15zwXDTA4MDMzMTIyMDAwMFqg\n"
	"LzAtMB8GA1UdIwQYMBaAFPmohhljtqQUE2B2DwGaNTbv8bSvMAoGA1UdFAQDAgEB\n"
	"MA0GCSqGSIb3DQEBBQUAA4IBgQBwTFMCc5/y/rrVvv/rGD5BYF1rCk+Daln/aQvV\n"
	"UgFwbaYsnSUoHdivEF6rrtSJGdZj5JWk7Y4oICL6NLeiLiM+AeBuaGbB9EjIQH8d\n"
	"d4/QSR4VV/900xcWbSatycXq4k2nxnrFcC2TMD6ee0nQjs1YQcgBK5tEQBvtKa+w\n"
	"qemp7/WPuY1YcDTIJ1myjyM0yJpBope/9uYWxcYgHCwK+o1QqpDlnq21539QtdbC\n"
	"9isLxAohnvwmKJkRoYVUhi5jRjd4Yy/fiSAcQx+Gs+0kjRXqitAgofPUAyibMLZX\n"
	"EvTZvGDCBF8OqlF6WdBLgcYDVzX7GnYEYFSccQtPYdanilf9IGO0ToF0MfPliawb\n"
	"J/27rdbCDQXh3exSq4vGgdulmt+tmYsFwlivwvuCG/eV8KOLWv7q36jx4PzLJyiE\n"
	"JJimFkzuwEEaFSmIM9UDEKfmDC10jVQ4c7Y7CPI5rLnPDtEOTNWsjlw/rC2/XLem\n"
	"YdLVIwU0h1VJPvZsmbhU2baAhsM=\n"
	"-----END X509 CRL-----\n";

const gnutls_datum_t saved_crl = { saved_crl_pem, sizeof(saved_crl_pem)-1 };
const gnutls_datum_t saved_min_crl = { saved_min_crl_pem, sizeof(saved_min_crl_pem)-1 };

static void append_crt(gnutls_x509_crl_t crl, const gnutls_datum_t *pem)
{
	gnutls_x509_crt_t crt;
	int ret;

	assert(gnutls_x509_crt_init(&crt)>=0);
	assert(gnutls_x509_crt_import(crt, pem, GNUTLS_X509_FMT_PEM)>=0);
	ret = gnutls_x509_crl_set_crt(crl, crt, mytime(0));
	if (ret != 0)
		fail("gnutls_x509_crl_set_crt: %s\n", gnutls_strerror(ret));

	gnutls_x509_crt_deinit(crt);
}

static void append_aki(gnutls_x509_crl_t crl, const gnutls_datum_t *pem)
{
	gnutls_x509_crt_t crt;
	int ret;
	unsigned char aki[128];
	size_t aki_size;

	assert(gnutls_x509_crt_init(&crt)>=0);
	assert(gnutls_x509_crt_import(crt, pem, GNUTLS_X509_FMT_PEM)>=0);

	aki_size = sizeof(aki);
	assert(gnutls_x509_crt_get_subject_key_id(crt, aki, &aki_size, NULL) >= 0);

	ret = gnutls_x509_crl_set_authority_key_id(crl, aki, aki_size);
	if (ret != 0)
		fail("gnutls_x509_crl_set_authority_key_id: %s\n", gnutls_strerror(ret));

	gnutls_x509_crt_deinit(crt);
}

static void sign_crl(gnutls_x509_crl_t crl, const gnutls_datum_t *cert, const gnutls_datum_t *key)
{
	gnutls_x509_crt_t crt;
	gnutls_x509_privkey_t pkey;
	int ret;

	assert(gnutls_x509_crt_init(&crt)>=0);
	assert(gnutls_x509_privkey_init(&pkey)>=0);

	assert(gnutls_x509_crt_import(crt, cert, GNUTLS_X509_FMT_PEM)>=0);
	assert(gnutls_x509_privkey_import(pkey, key, GNUTLS_X509_FMT_PEM)>=0);

	ret = gnutls_x509_crl_sign(crl, crt, pkey);
	if (ret != 0)
		fail("gnutls_x509_crl_sign: %s\n", gnutls_strerror(ret));

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_privkey_deinit(pkey);
}

static gnutls_x509_crl_t generate_crl(unsigned skip_optional)
{
	gnutls_x509_crl_t crl;
	int ret;

	ret = gnutls_x509_crl_init(&crl);
	if (ret != 0)
		fail("gnutls_x509_crl_init\n");

	ret = gnutls_x509_crl_set_version(crl, 1);
	if (ret != 0)
		fail("gnutls_x509_crl_set_version\n");

	ret = gnutls_x509_crl_set_this_update(crl, mytime(0));
	if (ret != 0)
		fail("gnutls_x509_crl_set_this_update\n");

	if (!skip_optional) {
		ret = gnutls_x509_crl_set_next_update(crl, mytime(0)+60);
		if (ret != 0)
			fail("gnutls_x509_crl_set_next_update\n");
	}

	ret = gnutls_x509_crl_set_crt_serial(crl, "\x01\x02\x03", 3, mytime(0));
	if (ret != 0)
		fail("gnutls_x509_crl_set_serial %d\n", ret);

	append_crt(crl, &cli_ca3_cert);
	append_crt(crl, &subca3_cert);

	append_aki(crl, &ca3_cert);

	ret = gnutls_x509_crl_set_number(crl, "\x01", 1);
	if (ret != 0)
		fail("gnutls_x509_crl_set_number %d: %s\n",
		     ret, gnutls_strerror(ret));

	sign_crl(crl, &ca3_cert, &ca3_key);

	return crl;
}

void doit(void)
{
	gnutls_datum_t out;
	gnutls_x509_crl_t crl;

	gnutls_global_set_time_function(mytime);

	crl = generate_crl(0);

	assert(gnutls_x509_crl_export2(crl, GNUTLS_X509_FMT_PEM, &out) >= 0);

	fprintf(stdout, "%s", out.data);

	assert(out.size == saved_crl.size);
	assert(memcmp(out.data, saved_crl.data, out.size)==0);

	gnutls_free(out.data);
	gnutls_x509_crl_deinit(crl);

	/* skip optional parts */
	crl = generate_crl(1);

	assert(gnutls_x509_crl_export2(crl, GNUTLS_X509_FMT_PEM, &out) >= 0);

	fprintf(stdout, "%s", out.data);

	assert(out.size == saved_min_crl.size);
	assert(memcmp(out.data, saved_min_crl.data, out.size)==0);

	gnutls_free(out.data);
	gnutls_x509_crl_deinit(crl);
}
