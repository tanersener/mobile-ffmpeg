/*
 * Copyright (C) 2008-2014 Free Software Foundation, Inc.
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
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

/* Test for revocation in verification.
 * In that test server2 is included in the CRL.
 */


static const char _ca[] = {
/* CRL */
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBfTCCASSgAwIBAgIBATAKBggqhkjOPQQDAjAkMQ8wDQYDVQQDEwZWUE4gQ0Ex\n"
    "ETAPBgNVBAoTCEJpZyBDb3JwMCIYDzIwMTQxMTMwMjA1NDQ1WhgPOTk5OTEyMzEy\n"
    "MzU5NTlaMCQxDzANBgNVBAMTBlZQTiBDQTERMA8GA1UEChMIQmlnIENvcnAwWTAT\n"
    "BgcqhkjOPQIBBggqhkjOPQMBBwNCAASvDJl26Hzb47Xi+Wx6uJY0NUD+Bij+PJ9l\n"
    "mmS2wbLaLNyga5aRvf+s7HKq9o+7+CE6E0t8fuCe0j8nLN64iAZlo0MwQTAPBgNV\n"
    "HRMBAf8EBTADAQH/MA8GA1UdDwEB/wQFAwMHBgAwHQYDVR0OBBYEFFJATAcyatKW\n"
    "ionSww8obkh7JKCYMAoGCCqGSM49BAMCA0cAMEQCIDPmWRvQAUbnSrnh79DM46/l\n"
    "My88UjFi2+ZhmIwufLP7AiBB9eeXKUmtWXuXAar0vHNH6edgEcggHgfOOHekukOr\n"
    "hw==\n"
    "-----END CERTIFICATE-----\n"
};

gnutls_datum_t ca = {(void*)_ca, sizeof(_ca)-1};

static const char _server1[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBvzCCAWWgAwIBAgIMVHuEbjXPSvP+nSDXMAoGCCqGSM49BAMCMCQxDzANBgNV\n"
    "BAMTBlZQTiBDQTERMA8GA1UEChMIQmlnIENvcnAwIhgPMjAxNDExMzAyMDU2MTRa\n"
    "GA85OTk5MTIzMTIzNTk1OVowJzERMA8GA1UEAwwIc2VydmVyMQ0xEjAQBgNVBAoT\n"
    "CU15Q29tcGFueTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABLViTN5K7scNWH0u\n"
    "wLvlDj6nJdZ76sP+oZoev+gYMyV42JqG/60S2VizrAIcmQA9QFfGlZz2GpE641Gd\n"
    "HiH09dajdjB0MAwGA1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDwYD\n"
    "VR0PAQH/BAUDAweAADAdBgNVHQ4EFgQUNWE8WZGVgvhyw/56sMSCuyXhBjMwHwYD\n"
    "VR0jBBgwFoAUUkBMBzJq0paKidLDDyhuSHskoJgwCgYIKoZIzj0EAwIDSAAwRQIh\n"
    "AKk+TA7XgvPwo6oDcAWUYgQbnKWEh5xO55nvNf6TVgMrAiAEI+w6IVJbXgtmskIJ\n"
    "gedi4kA4sDjRKtTzfxlIdaZhuA==\n"
    "-----END CERTIFICATE-----\n"
};

gnutls_datum_t server1 = {(void*)_server1, sizeof(_server1)-1};

static const char _server2[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBvzCCAWWgAwIBAgIMVHuEfQn9F35YK44BMAoGCCqGSM49BAMCMCQxDzANBgNV\n"
    "BAMTBlZQTiBDQTERMA8GA1UEChMIQmlnIENvcnAwIhgPMjAxNDExMzAyMDU2Mjla\n"
    "GA85OTk5MTIzMTIzNTk1OVowJzERMA8GA1UEAwwIc2VydmVyMg0xEjAQBgNVBAoT\n"
    "CU15Q29tcGFueTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABBfy/l8rtthQYHRA\n"
    "sWoY3E7HHM2eP0RyNrusfh6Okl4TN8D1jlmx3yc+9h4RqIvC6dHhSS/mio8fjZpU\n"
    "aXzv7dujdjB0MAwGA1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDwYD\n"
    "VR0PAQH/BAUDAweAADAdBgNVHQ4EFgQUee5izg6T1FxiNtJbWBz90d20GVYwHwYD\n"
    "VR0jBBgwFoAUUkBMBzJq0paKidLDDyhuSHskoJgwCgYIKoZIzj0EAwIDSAAwRQIh\n"
    "AKMgl86d4ENyrpqkXR7pN8FN/Pd1Hji6Usnm536zuFjIAiA9RRxtPQXjrk3Sx8QR\n"
    "c0NrnBYRCM24FXMHSWOL1YUb7w==\n"
    "-----END CERTIFICATE-----\n"
};

gnutls_datum_t server2 = {(void*)_server2, sizeof(_server2)-1};

static const char _server3[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBvjCCAWWgAwIBAgIMVHuEhyM4BCuvifY3MAoGCCqGSM49BAMCMCQxDzANBgNV\n"
    "BAMTBlZQTiBDQTERMA8GA1UEChMIQmlnIENvcnAwIhgPMjAxNDExMzAyMDU2Mzla\n"
    "GA85OTk5MTIzMTIzNTk1OVowJzERMA8GA1UEAwwIc2VydmVyMw0xEjAQBgNVBAoT\n"
    "CU15Q29tcGFueTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABLKjVDVHPM7sK8Gr\n"
    "+eUTmT1sQSVhUr4znBEkiccPlGTN30m5KoaM1cflRxb+p/pYk6xFfAZW/33XmWON\n"
    "IjwygACjdjB0MAwGA1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDwYD\n"
    "VR0PAQH/BAUDAweAADAdBgNVHQ4EFgQU3TmVO7uyA1t4+tbbmTbKoXiHP1QwHwYD\n"
    "VR0jBBgwFoAUUkBMBzJq0paKidLDDyhuSHskoJgwCgYIKoZIzj0EAwIDRwAwRAIg\n"
    "RI1GVQ/ol9Es0niE3Ex/X+2a5tEVBOECLO3+Vr6rPs0CIHSxEksboGo8qJzESmjY\n"
    "If7aJsOFgpBmGKWGf+dVDjjg\n"
    "-----END CERTIFICATE-----\n"
};

gnutls_datum_t server3 = {(void*)_server3, sizeof(_server3)-1};

static const char _crl[] = {
    "-----BEGIN X509 CRL-----\n"
    "MIIBJTCBzAIBATAKBggqhkjOPQQDAjAkMQ8wDQYDVQQDEwZWUE4gQ0ExETAPBgNV\n"
    "BAoTCEJpZyBDb3JwGA8yMDE0MTEzMDIxMTkwNFoYDzk5OTkxMjMxMjM1OTU5WjBC\n"
    "MB8CDFR7hnMaGdABn3iWABgPMjAxNDExMzAyMTE5MDRaMB8CDFR7hH0J/Rd+WCuO\n"
    "ARgPMjAxNDExMzAyMTE5MDRaoC8wLTAfBgNVHSMEGDAWgBRSQEwHMmrSloqJ0sMP\n"
    "KG5IeySgmDAKBgNVHRQEAwIBATAKBggqhkjOPQQDAgNIADBFAiEAt3Ks2JNhxuuT\n"
    "nzok7rYbi+p6dWiPj7mWNawba2+xjYwCIGpTiTU1ssn5Fa70j7S+PjmnN4fuyjXh\n"
    "AuXYcsNpjsPz\n"
    "-----END X509 CRL-----\n"
};

gnutls_datum_t crl = {(void*)_crl, sizeof(_crl)-1};

/* GnuTLS internally calls time() to find out the current time when
   verifying certificates.  To avoid a time bomb, we hard code the
   current time.  This should work fine on systems where the library
   call to time is resolved at run-time.  */
static time_t mytime(time_t * t)
{
	time_t then = 1417381345;

	if (t)
		*t = then;

	return then;
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

void doit(void)
{
	int ret;
	gnutls_x509_trust_list_t tl;
	gnutls_x509_crt_t s1, s2, s3;
	unsigned status;

	/* The overloading of time() seems to work in linux (ELF?)
	 * systems only. Disable it on windows.
	 */
#ifdef _WIN32
	exit(77);
#endif

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	gnutls_x509_crt_init(&s1);
	gnutls_x509_crt_init(&s2);
	gnutls_x509_crt_init(&s3);
	gnutls_x509_trust_list_init(&tl, 0);

	ret = gnutls_x509_crt_import(s1, &server1, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_import(s2, &server2, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_import(s3, &server3, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_trust_list_add_trust_mem(tl, &ca, NULL, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret != 1) {
		fail("error in %d: (%d) %s\n", __LINE__, ret, gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_trust_list_add_trust_mem(tl, NULL, &crl, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0) {
		fail("error in %d: (%d) %s\n", __LINE__, ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_trust_list_verify_crt2(tl, &s1, 1, NULL, 0, 0, &status, NULL);
	if (ret < 0 || status != 0) {
		fail("error in %d: (status: 0x%x) %s\n", __LINE__, status, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_trust_list_verify_crt2(tl, &s2, 1, NULL, 0, 0, &status, NULL);
	if (ret < 0 || status != (GNUTLS_CERT_INVALID|GNUTLS_CERT_REVOKED)) {
		fail("error in %d: (status: 0x%x) %s\n", __LINE__, status, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_trust_list_verify_crt2(tl, &s3, 1, NULL, 0, 0, &status, NULL);
	if (ret < 0 || status != 0) {
		fail("error in %d: (status: 0x%x) %s\n", __LINE__, status, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_trust_list_deinit(tl, 1);
	gnutls_x509_crt_deinit(s1);
	gnutls_x509_crt_deinit(s2);
	gnutls_x509_crt_deinit(s3);

	if (debug)
		printf("done\n\n\n");

	gnutls_global_deinit();

	exit(0);
}
