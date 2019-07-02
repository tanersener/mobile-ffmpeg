/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Authors: Nikos Mavrogiannopoulos
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
#include <stdio.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include <cmocka.h>

int _gnutls_utf8_to_ucs2(const void *data, size_t size,
			 gnutls_datum_t * output, unsigned be);

int _gnutls_ucs2_to_utf8(const void *data, size_t size,
			 gnutls_datum_t * output, unsigned be);

#define DEBUG

#ifdef DEBUG
static void PRINT(const char *str, unsigned char *val, unsigned int size)
{
	unsigned i;
	printf("%s", str);
	for (i=0;i<size;i++) {
		printf("%.2x", val[i]);
	}
	printf("\n");
}
#else
#define PRINT(x, y, x)
#endif

#define UTF8_MATCH(fname, utf8, utf16) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t out; \
	int ret = _gnutls_utf8_to_ucs2(utf8, strlen(utf8), &out, 1); \
	assert_int_equal(ret, 0); \
	if (out.size != sizeof(utf16)-1 || memcmp(utf16, out.data, out.size) != 0) { PRINT("got:      ", out.data, out.size); \
	PRINT("expected: ", (unsigned char*)utf16, sizeof(utf16)-1); } \
	assert_int_equal(out.size, sizeof(utf16)-1); \
	assert_memory_equal(utf16, out.data, out.size); \
	gnutls_free(out.data); \
}

#define UTF16_MATCH(fname, utf8, utf16) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t out; \
	int ret = _gnutls_ucs2_to_utf8(utf16, sizeof(utf16)-1, &out, 1); \
	assert_int_equal(ret, 0); \
	if (out.size != strlen(utf8) || memcmp(utf8, out.data, out.size) != 0) { PRINT("got:      ", out.data, out.size); \
	PRINT("expected: ", (unsigned char*)utf8, strlen(utf8)); } \
	assert_int_equal(out.size, strlen(utf8)); \
	assert_memory_equal(utf8, out.data, out.size); \
	gnutls_free(out.data); \
}

#define UTF8_FAIL(fname, utf8, utf8_size) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t out; \
	int ret = _gnutls_utf8_to_ucs2(utf8, utf8_size, &out, 1); \
	assert_int_not_equal(ret, 0); \
}

#define UTF16_FAIL(fname, utf16, utf16_size) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t out; \
	int ret = _gnutls_ucs2_to_utf8(utf16, utf16_size, &out, 1); \
	assert_int_not_equal(ret, 0); \
}

UTF8_MATCH(check_utf8_ok1, "abcd", "\x00\x61\x00\x62\x00\x63\x00\x64");
UTF8_MATCH(check_utf8_ok2, "ユーザー別サイト", "\x30\xE6\x30\xFC\x30\xB6\x30\xFC\x52\x25\x30\xB5\x30\xA4\x30\xC8");
UTF8_MATCH(check_utf8_ok3, "简体中文", "\x7B\x80\x4F\x53\x4E\x2D\x65\x87");
UTF8_MATCH(check_utf8_ok4, "Σὲ γνωρίζω ἀπὸ", "\x03\xA3\x1F\x72\x00\x20\x03\xB3\x03\xBD\x03\xC9\x03\xC1\x03\xAF\x03\xB6\x03\xC9\x00\x20\x1F\x00\x03\xC0\x1F\x78");

UTF16_MATCH(check_utf16_ok1, "abcd", "\x00\x61\x00\x62\x00\x63\x00\x64");
UTF16_MATCH(check_utf16_ok2, "ユーザー別サイト", "\x30\xE6\x30\xFC\x30\xB6\x30\xFC\x52\x25\x30\xB5\x30\xA4\x30\xC8");
UTF16_MATCH(check_utf16_ok3, "简体中文", "\x7B\x80\x4F\x53\x4E\x2D\x65\x87");
UTF16_MATCH(check_utf16_ok4, "Σὲ γνωρίζω ἀπὸ", "\x03\xA3\x1F\x72\x00\x20\x03\xB3\x03\xBD\x03\xC9\x03\xC1\x03\xAF\x03\xB6\x03\xC9\x00\x20\x1F\x00\x03\xC0\x1F\x78");

UTF8_FAIL(check_utf8_fail1, "\xfe\xff\xaa\x80\xff", 5);
UTF8_FAIL(check_utf8_fail2, "\x64\x00\x62\xf3\x64\x65", 6);
UTF16_FAIL(check_utf16_fail1, "\xd8\x00\xdb\xff\x00\x63\x00\x04", 8);

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(check_utf8_fail1),
		cmocka_unit_test(check_utf8_fail2),
		cmocka_unit_test(check_utf16_fail1),
		cmocka_unit_test(check_utf8_ok1),
		cmocka_unit_test(check_utf8_ok2),
		cmocka_unit_test(check_utf8_ok3),
		cmocka_unit_test(check_utf8_ok4),
		cmocka_unit_test(check_utf16_ok1),
		cmocka_unit_test(check_utf16_ok2),
		cmocka_unit_test(check_utf16_ok3),
		cmocka_unit_test(check_utf16_ok4)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
