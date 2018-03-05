/*
 * Copyright (C) 2014 Free Software Foundation, Inc.
 *
 * Authors: Nikos Mavrogiannopoulos, Martin Ukrop
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

/* Test for name constraints PKIX extension.
 */

static void check_for_error(int ret) {
	if (ret != GNUTLS_E_SUCCESS)
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
}

#define NAME_ACCEPTED 1
#define NAME_REJECTED 0

static void check_test_result(int ret, int expected_outcome, gnutls_datum_t *tested_data) {
	if (expected_outcome == NAME_ACCEPTED ? ret == 0 : ret != 0) {
		if (expected_outcome == NAME_ACCEPTED) {
			fail("Checking \"%.*s\" should have succeeded.\n", tested_data->size, tested_data->data);
		} else {
			fail("Checking \"%.*s\" should have failed.\n", tested_data->size, tested_data->data);
		}
	}
}

static void set_name(const char *name, gnutls_datum_t *datum) {
	datum->data = (unsigned char*) name;
	datum->size = strlen((char*) name);
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

static unsigned char cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEMTCCAxmgAwIBAgIBADANBgkqhkiG9w0BAQUFADCBlTELMAkGA1UEBhMCR1Ix\n"
    "RDBCBgNVBAoTO0hlbGxlbmljIEFjYWRlbWljIGFuZCBSZXNlYXJjaCBJbnN0aXR1\n"
    "dGlvbnMgQ2VydC4gQXV0aG9yaXR5MUAwPgYDVQQDEzdIZWxsZW5pYyBBY2FkZW1p\n"
    "YyBhbmQgUmVzZWFyY2ggSW5zdGl0dXRpb25zIFJvb3RDQSAyMDExMB4XDTExMTIw\n"
    "NjEzNDk1MloXDTMxMTIwMTEzNDk1MlowgZUxCzAJBgNVBAYTAkdSMUQwQgYDVQQK\n"
    "EztIZWxsZW5pYyBBY2FkZW1pYyBhbmQgUmVzZWFyY2ggSW5zdGl0dXRpb25zIENl\n"
    "cnQuIEF1dGhvcml0eTFAMD4GA1UEAxM3SGVsbGVuaWMgQWNhZGVtaWMgYW5kIFJl\n"
    "c2VhcmNoIEluc3RpdHV0aW9ucyBSb290Q0EgMjAxMTCCASIwDQYJKoZIhvcNAQEB\n"
    "BQADggEPADCCAQoCggEBAKlTAOMupvaO+mDYLZU++CwqVE7NuYRhlFhPjz2L5EPz\n"
    "dYmNUeTDN9KKiE15HrcS3UN4SoqS5tdI1Q+kOilENbgH9mgdVc04UfCMJDGFr4PJ\n"
    "fel3r+0ae50X+bOdOFAPplp5kYCvN66m0zH7tSYJnTxa71HFK9+WXesyHgLacEns\n"
    "bgzImjeN9/E2YEsmLIKe0HjzDQ9jpFEw4fkrJxIH2Oq9GGKYsFk3fb7u8yBRQlqD\n"
    "75O6aRXxYp2fmTmCobd0LovUxQt7L/DICto9eQqakxylKHJzkUOap9FNhYS5qXSP\n"
    "FEDH3N6sQWRstBmbAmNtJGSPRLIl6s5ddAxjMlyNh+UCAwEAAaOBiTCBhjAPBgNV\n"
    "HRMBAf8EBTADAQH/MAsGA1UdDwQEAwIBBjAdBgNVHQ4EFgQUppFC/RNhSiOeCKQp\n"
    "5dgTBCPuQSUwRwYDVR0eBEAwPqA8MAWCAy5ncjAFggMuZXUwBoIELmVkdTAGggQu\n"
    "b3JnMAWBAy5ncjAFgQMuZXUwBoEELmVkdTAGgQQub3JnMA0GCSqGSIb3DQEBBQUA\n"
    "A4IBAQAf73lB4XtuP7KMhjdCSk4cNx6NZrokgclPEg8hwAOXhiVtXdMiKahsog2p\n"
    "6z0GW5k6x8zDmjR/qw7IThzh+uTczQ2+vyT+bOdrwg3IBp5OjWEopmr95fZi6hg8\n"
    "TqBTnbI6nOulnJEWtk2C4AwFSKls9cz4y51JtPACpf1wA+2KIaWuE4ZJwzNzvoc7\n"
    "dIsXRSZMFpGD/md9zU1jZ/rzAxKWeAaNsWftjj++n08C9bMJL/NMh98qy5V8Acys\n"
    "Nnq/onN694/BtZqhFLKPM58N7yLcZnuEvUUXBj08yrl3NI/K6s8/MT7jiOOASSXI\n"
    "l7WdmplNsDz4SgCbZN2fOUvRJ9e4\n"
    "-----END CERTIFICATE-----\n";

const gnutls_datum_t cert = { cert_pem, sizeof(cert_pem) };

const gnutls_datum_t name1 = { (void*)"com", 3 };
const gnutls_datum_t name2 = { (void*)"example.com", sizeof("example.com")-1 };
const gnutls_datum_t name3 = { (void*)"another.example.com", sizeof("another.example.com")-1 };
const gnutls_datum_t name4 = { (void*)".gr", 3 };

const gnutls_datum_t mail1 = { (void*)"example.com", sizeof("example.com")-1 };
const gnutls_datum_t mail2 = { (void*)".example.net", sizeof(".example.net")-1 };
const gnutls_datum_t mail3 = { (void*)"nmav@redhat.com", sizeof("nmav@redhat.com")-1 };
const gnutls_datum_t mail4 = { (void*)"koko.example.net", sizeof("koko.example.net")-1 };

void doit(void)
{
	int ret;
	unsigned int crit, i, permitted, excluded;
	gnutls_x509_crt_t crt;
	gnutls_x509_name_constraints_t nc;
	unsigned type;
	gnutls_datum_t name;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* 0: test the reading of name constraints */

	ret = gnutls_x509_name_constraints_init(&nc);
	check_for_error(ret);

	ret = gnutls_x509_crt_init(&crt);
	check_for_error(ret);

	ret = gnutls_x509_crt_import(crt, &cert, GNUTLS_X509_FMT_PEM);
	check_for_error(ret);

	ret = gnutls_x509_crt_get_name_constraints(crt, nc, 0, &crit);
	check_for_error(ret);

	if (crit != 0) {
		fail("error reading criticality\n");
	}

	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &type, &name);

		if (ret >= 0 && i == 2) {
			if (name.size != 3 || memcmp(name.data, ".eu", 3) != 0) {
				fail("error reading 2nd constraint\n");
			}
		}
	} while(ret == 0);

	if (i-1 != 8) {
		fail("Could not read all contraints; read %d, expected %d\n", i-1, 8);
	}

	gnutls_x509_name_constraints_deinit(nc);
	gnutls_x509_crt_deinit(crt);

	/* 1: test the generation of name constraints */

	permitted = 0;
	excluded = 0;

	ret = gnutls_x509_name_constraints_init(&nc);
	check_for_error(ret);

	ret = gnutls_x509_crt_init(&crt);
	check_for_error(ret);

	ret = gnutls_x509_crt_import(crt, &cert, GNUTLS_X509_FMT_PEM);
	check_for_error(ret);

	permitted++;
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_DNSNAME, &name1);
	check_for_error(ret);

	excluded++;
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_DNSNAME, &name2);
	check_for_error(ret);

	excluded++;
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_DNSNAME, &name3);
	check_for_error(ret);

	permitted++;
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_DNSNAME, &name4);
	check_for_error(ret);

	excluded++;
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_URI, &name3);
	check_for_error(ret);

	permitted++;
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_RFC822NAME, &mail1);
	check_for_error(ret);

	permitted++;
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_RFC822NAME, &mail2);
	check_for_error(ret);

	permitted++;
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_RFC822NAME, &mail3);
	check_for_error(ret);

	excluded++;
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_RFC822NAME, &mail4);
	check_for_error(ret);

	ret = gnutls_x509_crt_set_name_constraints(crt, nc, 1);
	check_for_error(ret);

	/* 2: test the reading of the generated constraints */

	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &type, &name);

		if (ret >= 0 && i == 1) {
			if (name.size != name1.size || memcmp(name.data, name1.data, name1.size) != 0) {
				fail("%d: error reading 1st constraint\n", __LINE__);
			}
		}
	} while(ret == 0);

	if (i-1 != permitted) {
		fail("Could not read all contraints; read %d, expected %d\n", i-1, permitted);
	}

	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &type, &name);

		if (ret >= 0 && i == 1) {
			if (name.size != name2.size || memcmp(name.data, name2.data, name2.size) != 0) {
				fail("%d: error reading 1st excluded constraint\n", __LINE__);
			}
		}
		if (ret >= 0 && i == 2) {
			if (name.size != name3.size || memcmp(name.data, name3.data, name3.size) != 0) {
				fail("%d: error reading 1st excluded constraint\n", __LINE__);
			}
		}
	} while(ret == 0);

	if (i-1 != excluded) {
		fail("Could not read all excluded contraints; read %d, expected %d\n", i-1, excluded);
	}

	/* 3: test the name constraints check function */

	/* This name constraints structure doesn't have any excluded GNUTLS_SAN_DN so
	 * this test should succeed */
	set_name("ASFHAJHjhafjs", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DN, &name);
	check_test_result(ret, NAME_ACCEPTED, &name);

	/* Test e-mails */
	set_name("nmav@redhat.com", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(ret, NAME_ACCEPTED, &name);

	set_name("nmav@radhat.com", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(ret, NAME_REJECTED, &name);

	set_name("nmav@example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(ret, NAME_ACCEPTED, &name);

	set_name("nmav@test.example.net", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(ret, NAME_ACCEPTED, &name);

	set_name("nmav@example.net", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(ret, NAME_REJECTED, &name);

	set_name("nmav@koko.example.net", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(ret, NAME_REJECTED, &name);

	/* This name constraints structure does have an excluded URI so
	 * this test should fail */
	set_name("http://www.com", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_URI, &name);
	check_test_result(ret, NAME_REJECTED, &name);

	set_name("goodexample.com", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(ret, NAME_ACCEPTED, &name);

	set_name("good.com", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(ret, NAME_ACCEPTED, &name);

	set_name("www.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(ret, NAME_REJECTED, &name);

	set_name("www.example.net", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(ret, NAME_REJECTED, &name);

	set_name("www.example.gr", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(ret, NAME_ACCEPTED, &name);

	gnutls_x509_name_constraints_deinit(nc);
	gnutls_x509_crt_deinit(crt);

	/* 4: corner cases */

	/* 4a: empty excluded name (works as wildcard) */

	ret = gnutls_x509_name_constraints_init(&nc);
	check_for_error(ret);

	set_name("", &name);
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("example.net", &name);
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(ret, NAME_REJECTED, &name);

	gnutls_x509_name_constraints_deinit(nc);

	// Test suite end.

	if (debug)
		success("Test success.\n");
}
