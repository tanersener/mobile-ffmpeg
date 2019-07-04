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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
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

static time_t then = 1207000800;

static time_t mytime(time_t * t)
{
	if (t)
		*t = then;

	return then;
}

static unsigned char saved_crl_pem[] =
	"-----BEGIN X509 CRL-----\n"
	"MIICXzCByAIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0zFw0wODAz\n"
	"MzEyMjAwMDBaFw0wODAzMzEyMjAyMDBaMFQwFAIDAQIDFw0wODAzMzEyMjAwMDBa\n"
	"MB0CDFejHTI2Wi75obBaUhcNMDgwMzMxMjIwMDAwWjAdAgxXox0yNbNP0Ln15zwX\n"
	"DTA4MDMzMTIyMDAwMFqgLzAtMB8GA1UdIwQYMBaAFPmohhljtqQUE2B2DwGaNTbv\n"
	"8bSvMAoGA1UdFAQDAgEBMA0GCSqGSIb3DQEBCwUAA4IBgQAFpyifa5AJclRpJfjh\n"
	"QOcSoiCJz5QsrGaK5I/UYHcY958hhFjnE2c9g3wYEEt13M2gkgOTXapImPbLXHv+\n"
	"cHWGoTqX6+crs7xcC6mFc6JfY7q9O2eP1x386dzCxhsXMti5ml0iOeBpNrMO46Pr\n"
	"PuvNaY7OE1UgN0Ha3YjmhP8HtWJSQCMmqIo6vP1/HBSzaXP/cjS7f0WBZemj0eE7\n"
	"wwA1GUoUx9wHipvNkCSKy/eQz4fpOJExrvHeb1/N3po9hfZaZJAqR+rsC0j9J+wd\n"
	"ZGAdVFKCJUZs0IgsWQqagg0tXGJ8ejdt4yE8zvhhcpf4pcGoYUqtoUPT+Fjnsw7C\n"
	"P1GCVZQ2ciGxixljTJFdifhqPshgC1Ytd75MkDYH2RRir/JwypQK9CcqIAOjBzTl\n"
	"uk4SkKL2xAIduw6Dz5kAC7G2EM94uODoI/RO5b6eN6Kb/592JrKAfB96jh2wwqW+\n"
	"swaA4JPFqNQaiMWW1IXM3VJwXBt8DRSRo46JV5OktvvFRwI=\n"
	"-----END X509 CRL-----\n";

static unsigned char saved_min_crl_pem[] =
	"-----BEGIN X509 CRL-----\n"
	"MIICUDCBuQIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0zFw0wODAz\n"
	"MzEyMjAwMTBaMFQwFAIDAQIDFw0wODAzMzEyMjAwMTBaMB0CDFejHTI2Wi75obBa\n"
	"UhcNMDgwMzMxMjIwMDEwWjAdAgxXox0yNbNP0Ln15zwXDTA4MDMzMTIyMDAxMFqg\n"
	"LzAtMB8GA1UdIwQYMBaAFPmohhljtqQUE2B2DwGaNTbv8bSvMAoGA1UdFAQDAgEB\n"
	"MA0GCSqGSIb3DQEBCwUAA4IBgQB/Y7MxKf7HpYBoi7N5lNCe7nSd0epQiNPOford\n"
	"hGb1ZirZk9m67zg146Cwc0W4ipPzW/OjwgUoVQTm21I7oZj/GPItAABlILd6eRQe\n"
	"jYJap0fxiXV7aMRfu2o3qCRGAITQf306H5zJmpdeNxbxzlr3t6IAHBDbLI1WYXiC\n"
	"pTHo3wlpwFJEPw5NQ0j6rCAzSH81FHTrEiIOar17uRqeMjbGN6Eo4zjezEx2+ewg\n"
	"unsdzx4OWx3KgzsQnyV9EoU6l9jREe519mICx7La6DZkhO4dSPJv6R5jEFitWDNB\n"
	"lxZMA5ePrYXuE/3b+Li89R53O+xZxShLQYwBRSHDue44xUv6hh6YNIKDgt4ycIs8\n"
	"9JAWsOYJDYUEbAUo+S4sWCU6LzloEvmg7EdJtvJWsScUKK4qbwkDfkBTKjbeBdFj\n"
	"w6naZIjzbjMPEe8/T+hmu/txFj3fGj/GzOM1GaJNZ4vMWA4Y6LhB+H1Zf3xK+hV0\n"
	"sc0eYw7RpIzEyc9PPz/uM+XabsI=\n"
	"-----END X509 CRL-----\n";

const gnutls_datum_t saved_crl = { saved_crl_pem, sizeof(saved_crl_pem) - 1 };
const gnutls_datum_t saved_min_crl =
    { saved_min_crl_pem, sizeof(saved_min_crl_pem) - 1 };

static void append_crt(gnutls_x509_crl_t crl, const gnutls_datum_t * pem)
{
	gnutls_x509_crt_t crt;
	int ret;

	assert(gnutls_x509_crt_init(&crt) >= 0);
	assert(gnutls_x509_crt_import(crt, pem, GNUTLS_X509_FMT_PEM) >= 0);
	ret = gnutls_x509_crl_set_crt(crl, crt, mytime(0));
	if (ret != 0)
		fail("gnutls_x509_crl_set_crt: %s\n", gnutls_strerror(ret));

	gnutls_x509_crt_deinit(crt);
}

static void append_aki(gnutls_x509_crl_t crl, const gnutls_datum_t * pem)
{
	gnutls_x509_crt_t crt;
	int ret;
	unsigned char aki[128];
	size_t aki_size;

	assert(gnutls_x509_crt_init(&crt) >= 0);
	assert(gnutls_x509_crt_import(crt, pem, GNUTLS_X509_FMT_PEM) >= 0);

	aki_size = sizeof(aki);
	assert(gnutls_x509_crt_get_subject_key_id(crt, aki, &aki_size, NULL) >=
	       0);

	ret = gnutls_x509_crl_set_authority_key_id(crl, aki, aki_size);
	if (ret != 0)
		fail("gnutls_x509_crl_set_authority_key_id: %s\n",
		     gnutls_strerror(ret));

	gnutls_x509_crt_deinit(crt);
}

static void verify_crl(gnutls_x509_crl_t _crl, gnutls_x509_crt_t crt)
{
	int ret;
	gnutls_x509_crl_t crl;
	unsigned status;
	gnutls_datum_t out;

	assert(gnutls_x509_crl_export2(_crl, GNUTLS_X509_FMT_DER, &out) >= 0);

	assert(gnutls_x509_crl_init(&crl) >= 0);
	assert(gnutls_x509_crl_import(crl, &out, GNUTLS_X509_FMT_DER) >= 0);

	gnutls_free(out.data);

	ret = gnutls_x509_crl_verify(crl, &crt, 1, 0, &status);
	if (ret < 0)
		fail("gnutls_x509_crl_verify: %s\n", gnutls_strerror(ret));

	if (status != 0)
		fail("gnutls_x509_crl_verify status: %x\n", status);
	gnutls_x509_crl_deinit(crl);
}

static void sign_crl(gnutls_x509_crl_t crl, const gnutls_datum_t * cert,
		     const gnutls_datum_t * key)
{
	gnutls_x509_crt_t crt;
	gnutls_x509_privkey_t pkey;
	int ret;

	assert(gnutls_x509_crt_init(&crt) >= 0);
	assert(gnutls_x509_privkey_init(&pkey) >= 0);

	assert(gnutls_x509_crt_import(crt, cert, GNUTLS_X509_FMT_PEM) >= 0);
	assert(gnutls_x509_privkey_import(pkey, key, GNUTLS_X509_FMT_PEM) >= 0);

	ret = gnutls_x509_crl_sign(crl, crt, pkey);
	if (ret != 0)
		fail("gnutls_x509_crl_sign: %s\n", gnutls_strerror(ret));

	then+=10;

	verify_crl(crl, crt);

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_privkey_deinit(pkey);
}

static gnutls_x509_crl_t generate_crl(unsigned skip_optional)
{
	gnutls_x509_crl_t crl;
	int ret;

	success("Generating CRL (%d)\n", skip_optional);

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
		ret = gnutls_x509_crl_set_next_update(crl, mytime(0) + 120);
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
	assert(memcmp(out.data, saved_crl.data, out.size) == 0);

	gnutls_free(out.data);
	gnutls_x509_crl_deinit(crl);

	/* skip optional parts */
	crl = generate_crl(1);

	assert(gnutls_x509_crl_export2(crl, GNUTLS_X509_FMT_PEM, &out) >= 0);

	fprintf(stdout, "%s", out.data);

	assert(out.size == saved_min_crl.size);
	assert(memcmp(out.data, saved_min_crl.data, out.size) == 0);

	gnutls_free(out.data);
	gnutls_x509_crl_deinit(crl);

}
