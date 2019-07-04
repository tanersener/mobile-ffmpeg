/*
 * Copyright (C) 2008-2016 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <assert.h>

#include "utils.h"

#include "cert-common.h"

static unsigned char saved_crt_pem[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIICIzCCAYygAwIBAgIDChEAMA0GCSqGSIb3DQEBCwUAMCsxDjAMBgNVBAMTBW5p\n"
	"a29zMRkwFwYDVQQKExBub25lIHRvLCBtZW50aW9uMCAXDTA4MDMzMTIyMDAwMFoY\n"
	"Dzk5OTkxMjMxMjM1OTU5WjArMQ4wDAYDVQQDEwVuaWtvczEZMBcGA1UEChMQbm9u\n"
	"ZSB0bywgbWVudGlvbjCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAu2ZD9fLF\n"
	"17aMzMXf9Yg7sclLag6hrSBQQAiAoU9co9D4bM/mPPfsBHYTF4tkiSJbwN1TfDvt\n"
	"fAS7gLkovo6bxo6gpRLL9Vceoue7tzNJn+O7Sq5qTWj/yRHiMo3OPYALjXXv2ACB\n"
	"jygEA6AijWEEB/q2N30hB0nSCWFpmJCjWKkCAwEAAaNTMFEwDAYDVR0TAQH/BAIw\n"
	"ADAPBgNVHQ8BAf8EBQMDB4AAMA4GA1UdEQQHMAWCA2FwYTAgBgNVHSUBAf8EFjAU\n"
	"BggrBgEFBQcDAQYIKwYBBQUHAwIwDQYJKoZIhvcNAQELBQADgYEAs98nzI0+UM8n\n"
	"Xy5Nu9iAJG+d1Ukh8lXheoVjYs3Rd8DZcBPZLcMr8SS3unjpsAhLkvn0qm0+PYBT\n"
	"9gasQVZWs+PDSrn+Z/8L/WKRj+Nhli1Eq2ARnRzUY3Qq+ff1Tvll2a3DM0KoKm0F\n"
	"d3TOPG5tr3bBVXaDQ4K7bQCiTC6Skew=\n"
	"-----END CERTIFICATE-----\n";

const gnutls_datum_t saved_crt = { saved_crt_pem, sizeof(saved_crt_pem)-1 };

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

void doit(void)
{
	gnutls_x509_privkey_t pkey;
	gnutls_x509_crt_t crt;
	gnutls_x509_crt_t crt2;
	const char *err = NULL;
	gnutls_datum_t out;
	size_t s = 0;
	unsigned int status;
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

	ret = gnutls_x509_crt_init(&crt2);
	if (ret != 0)
		fail("gnutls_x509_crt_init\n");

	ret = gnutls_x509_crt_import(crt2, &server_ecc_cert, GNUTLS_X509_FMT_PEM);
	if (ret != 0)
		fail("gnutls_x509_crt_import\n");

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret != 0)
		fail("gnutls_x509_privkey_init\n");

	ret = gnutls_x509_privkey_import(pkey, &key_dat, GNUTLS_X509_FMT_PEM);
	if (ret != 0)
		fail("gnutls_x509_privkey_import\n");

	/* Setup CRT */

	ret = gnutls_x509_crt_set_version(crt, 3);
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

	ret = gnutls_x509_crt_set_basic_constraints(crt, 0, -1);
	if (ret < 0) {
		fail("error\n");
	}

	ret = gnutls_x509_crt_set_key_usage(crt, GNUTLS_KEY_DIGITAL_SIGNATURE);
	if (ret != 0)
		fail("gnutls_x509_crt_set_key_usage %d\n", ret);

	ret = gnutls_x509_crt_set_dn(crt, "o = none to\\, mention,cn = nikos", &err);
	if (ret < 0) {
		fail("gnutls_x509_crt_set_dn: %s, %s\n", gnutls_strerror(ret), err);
	}


	ret = gnutls_x509_crt_set_subject_alt_name(crt, GNUTLS_SAN_DNSNAME,
						   "foo", 3, 1);
	if (ret != 0)
		fail("gnutls_x509_crt_set_subject_alt_name\n");

	ret = gnutls_x509_crt_set_subject_alt_name(crt, GNUTLS_SAN_RFC822NAME,
						   "foo@bar.org", strlen("foo@bar.org"), 1);
	if (ret != 0)
		fail("gnutls_x509_crt_set_subject_alt_name\n");


	ret = gnutls_x509_crt_set_subject_alt_name(crt, GNUTLS_SAN_IPADDRESS,
						   "\xc1\x5c\x96\x3", 4, 1);
	if (ret != 0)
		fail("gnutls_x509_crt_set_subject_alt_name\n");

	ret = gnutls_x509_crt_set_subject_alt_name(crt, GNUTLS_SAN_IPADDRESS,
						   "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 16, 1);
	if (ret != 0)
		fail("gnutls_x509_crt_set_subject_alt_name\n");

	ret = gnutls_x509_crt_set_subject_alt_name(crt, GNUTLS_SAN_DNSNAME,
						   "apa", 3, 0);
	if (ret != 0)
		fail("gnutls_x509_crt_set_subject_alt_name\n");

	s = 0;
	ret = gnutls_x509_crt_get_key_purpose_oid(crt, 0, NULL, &s, NULL);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
		fail("gnutls_x509_crt_get_key_purpose_oid %d\n", ret);

	s = 0;
	ret =
	    gnutls_x509_crt_set_key_purpose_oid(crt,
						GNUTLS_KP_TLS_WWW_SERVER,
						0);
	if (ret != 0)
		fail("gnutls_x509_crt_set_key_purpose_oid %d\n", ret);

	s = 0;
	ret = gnutls_x509_crt_get_key_purpose_oid(crt, 0, NULL, &s, NULL);
	if (ret != GNUTLS_E_SHORT_MEMORY_BUFFER)
		fail("gnutls_x509_crt_get_key_purpose_oid %d\n", ret);

	s = 0;
	ret =
	    gnutls_x509_crt_set_key_purpose_oid(crt,
						GNUTLS_KP_TLS_WWW_CLIENT,
						1);
	if (ret != 0)
		fail("gnutls_x509_crt_set_key_purpose_oid2 %d\n", ret);

	/* in the end this will be ignored as the issuer will be set
	 * by gnutls_x509_crt_sign2() */
	ret = gnutls_x509_crt_set_issuer_dn(crt, "cn = my CA, o = big\\, and one", &err);
	if (ret < 0) {
		fail("gnutls_x509_crt_set_issuer_dn: %s, %s\n", gnutls_strerror(ret), err);
	}

	/* Sign and finalize the certificate */
	ret = gnutls_x509_crt_sign(crt, crt, pkey);
	if (ret < 0)
		fail("gnutls_x509_crt_sign2: %s\n", gnutls_strerror(ret));


	ret = gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_FULL, &out);
	if (ret != 0)
		fail("gnutls_x509_crt_print\n");
	if (debug)
		printf("crt: %.*s\n", out.size, out.data);
	gnutls_free(out.data);

	ret = gnutls_x509_crt_get_raw_dn(crt, &out);
	if (ret < 0 || out.size == 0)
		fail("gnutls_x509_crt_get_raw_dn: %s\n", gnutls_strerror(ret));

	if (out.size != 45 ||
	    memcmp(out.data, "\x30\x2b\x31\x0e\x30\x0c\x06\x03\x55\x04\x03\x13\x05\x6e\x69\x6b\x6f\x73\x31\x19\x30\x17\x06\x03\x55\x04\x0a\x13\x10\x6e\x6f\x6e\x65\x20\x74\x6f\x2c\x20\x6d\x65\x6e\x74\x69\x6f\x6e", 45) != 0) {
		fail("DN comparison failed\n");
	}
	gnutls_free(out.data);

	ret = gnutls_x509_crt_equals(crt, crt);
	if (ret == 0) {
		fail("equality test failed\n");
	}

	ret = gnutls_x509_crt_equals(crt, crt2);
	if (ret != 0) {
		fail("equality test failed\n");
	}

	ret = gnutls_x509_crt_verify(crt, &crt, 1, 0, &status);
	if (ret < 0) {
		fail("verification failed\n");
	}

	if (status != 0) {
		fail("verification status failed\n");
	}

	assert(gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_PEM, &out) >= 0);

	if (debug)
		fprintf(stderr, "%s\n", out.data);
	assert(out.size == saved_crt.size);
	assert(memcmp(out.data, saved_crt.data, out.size)==0);

	gnutls_free(out.data);

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_crt_deinit(crt2);
	gnutls_x509_privkey_deinit(pkey);

	gnutls_global_deinit();
}
