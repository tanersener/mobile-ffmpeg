/*
 * Copyright (C) 2016 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Martin Ukrop
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
#include <sys/types.h>
#include <unistd.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include "../lib/x509/ip.h"

typedef struct test_vars_t {
	gnutls_x509_name_constraints_t nc;
	gnutls_x509_name_constraints_t nc2;
	gnutls_datum_t ip;
} test_vars_t;

/* just declaration: function is exported privately
   from lib/x509/name_constraints.c (declared in lib/x509/x509_int.h)
   but including the header breaks includes */
extern int _gnutls_x509_name_constraints_merge(
					gnutls_x509_name_constraints_t nc,
					gnutls_x509_name_constraints_t nc2);

static void check_for_error(int ret) {
	if (ret != GNUTLS_E_SUCCESS)
		fail_msg("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
}

#define IP_ACCEPTED 1
#define IP_REJECTED 0

static void check_test_result(int ret, int expected_outcome,
							  gnutls_datum_t *tested_ip) {
	if (expected_outcome == IP_ACCEPTED ? ret == 0 : ret != 0) {
		char ip_out[48];
		_gnutls_ip_to_string(tested_ip->data, tested_ip->size, ip_out, sizeof(ip_out));
		if (expected_outcome == IP_ACCEPTED) {
			fail_msg("Checking %.*s should have succeeded.\n",
				 (int) sizeof(ip_out), ip_out);
		} else {
			fail_msg("Checking %.*s should have failed.\n",
				 (int) sizeof(ip_out), ip_out);
		}
	}
}

static void parse_cidr(const char* cidr, gnutls_datum_t *datum) {
	if (datum->data != NULL) {
		gnutls_free(datum->data);
	}
	int ret = gnutls_x509_cidr_to_rfc5280(cidr, datum);
	check_for_error(ret);
}

static void tls_log_func(int level, const char *str) {
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

// CIDRs mostly use prefix for documentation purposes (RFC5737, RFC3849)

static void check_generation_reading_basic_checking(void **glob_state)
{
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	unsigned int i, num_permitted, num_excluded, type;
	gnutls_x509_crt_t crt;
	gnutls_datum_t name;

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(2);

	/* 1: test the generation of name constraints */

	ret = gnutls_x509_crt_init(&crt);
	check_for_error(ret);

	ret = gnutls_x509_crt_import(crt, &cert, GNUTLS_X509_FMT_PEM);
	check_for_error(ret);

	num_permitted = num_excluded = 0;

	parse_cidr("203.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	num_permitted++;
	check_for_error(ret);

	parse_cidr("2001:DB8::/32", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	num_permitted++;
	check_for_error(ret);

	parse_cidr("203.0.113.0/26", ip);
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_IPADDRESS, ip);
	num_excluded++;
	check_for_error(ret);

	parse_cidr("2001:DB8::/34", ip);
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_IPADDRESS, ip);
	num_excluded++;
	check_for_error(ret);

	// Try to add invalid name constraints

	parse_cidr("2001:DB8::/34", ip);
	ip->data[30] = 2;
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_IPADDRESS, ip);
	if (ret == 0)
		fail_msg("Checking invalid network mask should have failed.");

	parse_cidr("2001:DB8::/34", ip);
	ip->size = 31;
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	if (ret == 0)
		fail_msg("Checking invalid IP size should have failed.");

	ret = gnutls_x509_crt_set_name_constraints(crt, nc, 1);
	check_for_error(ret);

	/* 2: test the reading of the generated constraints */

	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_permitted(nc, i++, &type, &name);
#ifdef DEBUG
			_gnutls_cidr_to_string(name.data, name.size, ip_out, sizeof(ip_out));
			printf("Loaded name constraint: %s\n",ip_out);
#endif
	} while(ret == 0);

	if (i-1 != num_permitted) {
		fail_msg("Could not read all contraints; read %d, expected %d\n", i-1, num_permitted);
	}

	i = 0;
	do {
		ret = gnutls_x509_name_constraints_get_excluded(nc, i++, &type, &name);
#ifdef DEBUG
		_gnutls_cidr_to_string(name.data, name.size, ip_out, sizeof(ip_out));
		printf("Loaded name constraint: %s\n",ip_out);
#endif
	} while(ret == 0);

	if (i-1 != num_excluded) {
		fail_msg("Could not read all excluded contraints; read %d, expected %d\n", i-1, num_excluded);
	}

	/* 3: test the name constraints check function */

	parse_cidr("203.0.113.250/32", ip);
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("203.0.114.0/32", ip);
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("203.0.113.10/32", ip);
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);


	parse_cidr("2001:DB8:4000::/128", ip);
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("2001:DB9::/128", ip);
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("2001:DB8:10::/128", ip);
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	gnutls_x509_crt_deinit(crt);
}

static void check_universal_constraint_checking(void **glob_state)
{
	/* 3b setting universal constraint */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("2001:DB8::/0", ip);
	ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);

	parse_cidr("2001:DB8:10::/128", ip);
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("::/128", ip);
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_simple_intersection(void **glob_state)
{
	/* 4: simple intersection
	 * --------P:203.0.113.0/24--------
	 * --P:203.0.113.0/26--
	 *      A		   B	  C
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("203.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("203.0.113.0/26", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("203.0.113.2/32", ip); // A
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("203.0.113.250/32", ip); // B
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("203.0.114.0/32", ip); // C
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_empty_intersection(void **glob_state)
{
	/* 5: empty intersection
	 * --P:127.0.113.0/24--
	 *			    --P:255.0.113.0/24--
	 *      A		 B	  C
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("127.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("255.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("127.0.113.2/32", ip); // A
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.0.2/32", ip); // B
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.113.2/32", ip); // C
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_mediocre_intersection(void **glob_state)
{
	/* 6: mediocre intersection
	 * --------P:127.0.113.0/24--------
	 * --P:127.0.113.0/26--		    --P:255.0.113.0/24--
	 *      A		 B	  C	    D
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("127.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("127.0.113.0/26", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("255.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("127.0.113.2/32", ip); // A
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("127.0.113.250/32", ip); // B
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.0.2/32", ip); // C
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.113.2/32", ip); // D
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_difficult_intersection(void **glob_state)
{
	/* 7: difficult intersection
	 * --------P:0.0.0.0/3---------------     --P:88.0.0.0/5--
	 * --P:0.0.0.0/5-- --P:16.0.0.0/5--   ----P:64.0.0.0/3----
	 *      A	 B	C	D E  F	 G	H
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("0.0.0.0/3", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("88.0.0.0/5", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("0.0.0.0/5", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("16.0.0.0/5", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("64.0.0.0/3", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("0.0.113.2/32", ip); // A
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("15.255.255.255/32", ip); // B
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("16.0.0.0/32", ip); // C
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("31.12.25.2/32", ip); // D
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("63.255.255.255/32", ip); // E
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("64.0.0.0/32", ip); // F
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("89.125.7.187/32", ip); // G
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("96.0.0.0/32", ip); // H
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_ipv6_intersection(void **glob_state)
{
	/* 8: IPv6 intersection
	 *   --------P:affb::/16-----   --P:affd:0000::/20--
	 *     --P:affb:aa00::/24--
	 * A  B	C	   D  E     F		G
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("affb::/16", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("affd:0000::/20", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("affb:aa00::/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("affa:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128", ip); // A
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("affb:a500::/128", ip); // B
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("affb:aa00::/128", ip); // C
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_ACCEPTED, ip);

	parse_cidr("affb:ab01::/128", ip); // D
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("affc::/128", ip); // E
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("affd:0fff::/128", ip); // F
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("affd:1000::/128", ip); // G
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_empty_ipv4_intersection_ipv6_remains(void **glob_state)
{
	/* 9: IPv4 and IPv6 in a common test case
	 *    IPv4 with empty intersection, but IPv6 gets restricted as well
	 * --P:127.0.113.0/24--
	 *			    --P:255.0.113.0/24--
	 *      A		 B	  C
	 *
	 * --P:bfa6::/16--
	 *    D	   E
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("127.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("bfa6::/16", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("255.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("127.0.113.2/32", ip); // A
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.0.2/32", ip); // B
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.113.2/32", ip); // C
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("bfa6:ab01::/128", ip); // D
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("bfa7::/128", ip); // E
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_empty_ipv4v6_intersections(void **glob_state)
{
	/* 10: IPv4 and IPv6 in a common test case
	 *     both IPv4 and IPv6 have empty intersection
	 * --P:127.0.113.0/24--
	 *			    --P:255.0.113.0/24--
	 *      A		 B	  C
	 *
	 * --P:bfa6::/16--
	 *			  --P:cfa6::/16--
	 *    D	   E	     F
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("127.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("bfa6::/16", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("255.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("cfa6::/16", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("127.0.113.2/32", ip); // A
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.0.2/32", ip); // B
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.113.2/32", ip); // C
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("bfa6:ab01::/128", ip); // D
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("bfa7::/128", ip); // E
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("cfa7:00cc::/128", ip); // F
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static void check_ipv4v6_single_constraint_each(void **glob_state)
{
	/* 11: 1 IPv4 range and 1 IPv6 range in a common test case
	 *     (no overlap)
	 * --P:127.0.113.0/24--
	 *      A		B
	 *
	 * --P:bfa6::/16--
	 *    C	   D
	 */
	int ret;
	gnutls_x509_name_constraints_t nc =  ((test_vars_t*)*glob_state)->nc;
	gnutls_x509_name_constraints_t nc2 = ((test_vars_t*)*glob_state)->nc2;
	gnutls_datum_t *ip = &(((test_vars_t*)*glob_state)->ip);

	parse_cidr("127.0.113.0/24", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	parse_cidr("bfa6::/16", ip);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_IPADDRESS, ip);
	check_for_error(ret);
	ret = _gnutls_x509_name_constraints_merge(nc, nc2);
	check_for_error(ret);

	parse_cidr("127.0.113.2/32", ip); // A
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("255.0.0.2/32", ip); // B
	ip->size = 4; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("bfa6:ab01::/128", ip); // C
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);

	parse_cidr("bfa7::/128", ip); // D
	ip->size = 16; // strip network mask
	ret = gnutls_x509_name_constraints_check(nc, GNUTLS_SAN_IPADDRESS, ip);
	check_test_result(ret, IP_REJECTED, ip);
}

static int setup(void **state) {
	test_vars_t* test_vars = gnutls_malloc(sizeof(test_vars_t));
	if (test_vars == NULL)
		return -1;
	test_vars->ip.size = 0;
	test_vars->ip.data = NULL;

	int ret;
	ret = gnutls_x509_name_constraints_init(&(test_vars->nc));
	check_for_error(ret);
	ret = gnutls_x509_name_constraints_init(&(test_vars->nc2));
	check_for_error(ret);

	*state = test_vars;
	return 0;
}

static int teardown(void **state) {
	test_vars_t* test_vars = *state;
	gnutls_free(test_vars->ip.data);
	gnutls_x509_name_constraints_deinit(test_vars->nc);
	gnutls_x509_name_constraints_deinit(test_vars->nc2);
	gnutls_free(*state);
	return 0;
}

int main(int argc, char **argv)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(check_generation_reading_basic_checking, setup, teardown),
		cmocka_unit_test_setup_teardown(check_universal_constraint_checking, setup, teardown),
		cmocka_unit_test_setup_teardown(check_simple_intersection, setup, teardown),
		cmocka_unit_test_setup_teardown(check_empty_intersection, setup, teardown),
		cmocka_unit_test_setup_teardown(check_mediocre_intersection, setup, teardown),
		cmocka_unit_test_setup_teardown(check_difficult_intersection, setup, teardown),
		cmocka_unit_test_setup_teardown(check_ipv6_intersection, setup, teardown),
		cmocka_unit_test_setup_teardown(check_empty_ipv4_intersection_ipv6_remains, setup, teardown),
		cmocka_unit_test_setup_teardown(check_empty_ipv4v6_intersections, setup, teardown),
		cmocka_unit_test_setup_teardown(check_ipv4v6_single_constraint_each, setup, teardown)
	};
	cmocka_run_group_tests(tests, NULL, NULL);
}
