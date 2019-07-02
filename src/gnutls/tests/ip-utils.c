/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#include <config.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#ifndef _WIN32
# include <cmocka.h>
# include <sys/socket.h>

#define BUILD_IN_TESTS
#include "../lib/x509/ip-in-cidr.h"

#define _MATCH_FUNC(fname, CIDR, IP, status) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t dcidr; \
	const char cidr[] = CIDR; \
	const char ip[] = IP; \
	char xip[4]; \
	gnutls_datum_t dip = {(unsigned char*)xip, sizeof(xip)}; \
	assert_int_equal(gnutls_x509_cidr_to_rfc5280(cidr, &dcidr), 0); \
	assert_int_equal(inet_pton(AF_INET, ip, xip), 1); \
	assert_int_equal(ip_in_cidr(&dip, &dcidr), status); \
	gnutls_free(dcidr.data); \
}

#define MATCH_FUNC_OK(fname, CIDR, IP) _MATCH_FUNC(fname, CIDR, IP, 1)
#define MATCH_FUNC_NOT_OK(fname, CIDR, IP) _MATCH_FUNC(fname, CIDR, IP, 0)

MATCH_FUNC_OK(check_ip1_match, "192.168.1.0/24", "192.168.1.128");
MATCH_FUNC_OK(check_ip2_match, "192.168.1.0/24", "192.168.1.1");
MATCH_FUNC_OK(check_ip3_match, "192.168.1.0/24", "192.168.1.0");
MATCH_FUNC_OK(check_ip4_match, "192.168.1.0/28", "192.168.1.0");
MATCH_FUNC_OK(check_ip5_match, "192.168.1.0/28", "192.168.1.14");

MATCH_FUNC_NOT_OK(check_ip1_not_match, "192.168.1.0/24", "192.168.2.128");
MATCH_FUNC_NOT_OK(check_ip2_not_match, "192.168.1.0/24", "192.168.128.1");
MATCH_FUNC_NOT_OK(check_ip3_not_match, "192.168.1.0/24", "193.168.1.0");
MATCH_FUNC_NOT_OK(check_ip4_not_match, "192.168.1.0/28", "192.168.1.16");
MATCH_FUNC_NOT_OK(check_ip5_not_match, "192.168.1.0/28", "192.168.1.64");
MATCH_FUNC_NOT_OK(check_ip6_not_match, "192.168.1.0/24", "10.0.0.0");
MATCH_FUNC_NOT_OK(check_ip7_not_match, "192.168.1.0/24", "192.169.1.0");

#define CIDR_MATCH(fname, CIDR, EXPECTED) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t dcidr; \
	const char cidr[] = CIDR; \
	assert_int_equal(gnutls_x509_cidr_to_rfc5280(cidr, &dcidr), 0); \
	assert_memory_equal(EXPECTED, dcidr.data, dcidr.size); \
	gnutls_free(dcidr.data); \
}

#define CIDR_FAIL(fname, CIDR) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t dcidr; \
	const char cidr[] = CIDR; \
	assert_int_not_equal(gnutls_x509_cidr_to_rfc5280(cidr, &dcidr), 0); \
}

CIDR_MATCH(check_cidr_ok1, "0.0.0.0/32","\x00\x00\x00\x00\xff\xff\xff\xff");
CIDR_MATCH(check_cidr_ok2, "192.168.1.1/12", "\xc0\xa0\x00\x00\xff\xf0\x00\x00");
CIDR_MATCH(check_cidr_ok3, "192.168.1.1/0", "\x00\x00\x00\x00\x00\x00\x00\x00");
CIDR_MATCH(check_cidr_ok4, "::/19", "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xe0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
CIDR_MATCH(check_cidr_ok5, "::1/128", "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff");
CIDR_MATCH(check_cidr_ok6, "2001:db8::/48", "\x20\x01\x0d\xb8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");

CIDR_FAIL(check_cidr_fail1, "0.0.0.0/100");
CIDR_FAIL(check_cidr_fail2, "1.2.3.4/-1");
CIDR_FAIL(check_cidr_fail3, "1.300.3.4/-1");
CIDR_FAIL(check_cidr_fail4, "1.2.3/-1");
CIDR_FAIL(check_cidr_fail5, "1.2.3.4.5/-1");
CIDR_FAIL(check_cidr_fail6, "1.2.3.4");
CIDR_FAIL(check_cidr_fail7, ":://128");
CIDR_FAIL(check_cidr_fail8, "192.168.1.1/");
CIDR_FAIL(check_cidr_fail9, "192.168.1.1/33");
CIDR_FAIL(check_cidr_fail10, "::/");
CIDR_FAIL(check_cidr_fail11, "::/129");

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(check_ip1_match),
		cmocka_unit_test(check_ip2_match),
		cmocka_unit_test(check_ip3_match),
		cmocka_unit_test(check_ip4_match),
		cmocka_unit_test(check_ip5_match),
		cmocka_unit_test(check_ip1_not_match),
		cmocka_unit_test(check_ip2_not_match),
		cmocka_unit_test(check_ip3_not_match),
		cmocka_unit_test(check_ip4_not_match),
		cmocka_unit_test(check_ip5_not_match),
		cmocka_unit_test(check_ip6_not_match),
		cmocka_unit_test(check_ip7_not_match),

		cmocka_unit_test(check_cidr_ok1),
		cmocka_unit_test(check_cidr_ok2),
		cmocka_unit_test(check_cidr_ok3),
		cmocka_unit_test(check_cidr_ok4),
		cmocka_unit_test(check_cidr_ok5),
		cmocka_unit_test(check_cidr_ok6),

		cmocka_unit_test(check_cidr_fail1),
		cmocka_unit_test(check_cidr_fail2),
		cmocka_unit_test(check_cidr_fail3),
		cmocka_unit_test(check_cidr_fail4),
		cmocka_unit_test(check_cidr_fail5),
		cmocka_unit_test(check_cidr_fail6),
		cmocka_unit_test(check_cidr_fail7),
		cmocka_unit_test(check_cidr_fail8),
		cmocka_unit_test(check_cidr_fail9),
		cmocka_unit_test(check_cidr_fail10),
		cmocka_unit_test(check_cidr_fail11),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
#else
int main(void)
{
	exit(77);
}
#endif
