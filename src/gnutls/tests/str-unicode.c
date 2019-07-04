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
#include <gnutls/gnutls.h>
#include <cmocka.h>

#define MATCH_FUNC(fname, password, normalized) \
static void fname(void **glob_state) \
{ \
	const char *pwd_normalized = normalized; \
	gnutls_datum_t out; \
	int ret = gnutls_utf8_password_normalize((uint8_t*)password, strlen(password), &out, 0); \
	if (pwd_normalized == NULL) { /* expect failure */ \
		assert_int_not_equal(ret, 0); \
	} else { \
		assert_int_equal(ret, 0); \
		assert_int_equal(strcmp((char*)out.data, (char*)pwd_normalized), 0); \
		gnutls_free(out.data); \
	} \
}

#define INVALID_MATCH_FUNC(fname, password, normalized) \
static void inv_##fname(void **glob_state) \
{ \
	const char *pwd_normalized = normalized; \
	gnutls_datum_t out; \
	int ret = gnutls_utf8_password_normalize((uint8_t*)password, strlen(password), &out, GNUTLS_UTF8_IGNORE_ERRS); \
	if (pwd_normalized == NULL) { \
		assert_int_not_equal(ret, 0); \
	} else { \
		assert_int_equal(ret, 0); \
		assert_int_equal(strcmp((char*)out.data, (char*)pwd_normalized), 0); \
		gnutls_free(out.data); \
	} \
}

MATCH_FUNC(test_ascii, "correct horse battery staple", "correct horse battery staple");
MATCH_FUNC(test_capitals, "Correct Horse Battery Staple", "Correct Horse Battery Staple");
MATCH_FUNC(test_multilang, "\xCF\x80\xC3\x9F\xC3\xA5", "πßå");
MATCH_FUNC(test_special_char, "\x4A\x61\x63\x6B\x20\x6F\x66\x20\xE2\x99\xA6\x73", "Jack of ♦s");
MATCH_FUNC(test_space_replacement, "foo bar", "foo bar");
MATCH_FUNC(test_invalid, "my cat is a \x09 by", NULL);
MATCH_FUNC(test_normalization1, "char \x49\xCC\x87", "char \xC4\xB0");
MATCH_FUNC(test_other_chars, "char \xc2\xbc", "char \xC2\xbc");
MATCH_FUNC(test_spaces, "char \xe2\x80\x89\xe2\x80\x88 ", "char    ");
MATCH_FUNC(test_symbols, "char \xe2\x98\xa3 \xe2\x99\xa3", "char \xe2\x98\xa3 \xe2\x99\xa3");
MATCH_FUNC(test_compatibility, "char \xcf\x90\xe2\x84\xb5", "char \xcf\x90\xe2\x84\xb5");
MATCH_FUNC(test_invalid_ignorable1, "my ignorable char is \xe2\x80\x8f", NULL);
MATCH_FUNC(test_invalid_ignorable2, "my ignorable char is \xe1\x85\x9f", NULL);
MATCH_FUNC(test_invalid_ignorable3, "my ignorable char is \xef\xbf\xbf", NULL);
MATCH_FUNC(test_invalid_exception1, "my exception is \xc2\xb7", NULL); /* CONTEXTO - disallowed */
MATCH_FUNC(test_invalid_exception2, "my exception is \xcf\x82", "my exception is ς"); /* PVALID */
MATCH_FUNC(test_invalid_exception3, "my exception is \xd9\xa2", NULL); /* CONTEXT0/PVALID */
MATCH_FUNC(test_invalid_exception4, "my exception is \xe3\x80\xae", NULL); /* CONTEXT0/DISALLOWED */
MATCH_FUNC(test_invalid_join_control, "my exception is \xe2\x80\x8d", NULL);

INVALID_MATCH_FUNC(test_ascii, "correct horse battery staple", "correct horse battery staple");
INVALID_MATCH_FUNC(test_special_char, "\x4A\x61\x63\x6B\x20\x6F\x66\x20\xE2\x99\xA6\x73", "Jack of ♦s");
INVALID_MATCH_FUNC(test_invalid, "my cat is a \x09 by", "my cat is a \x09 by");
INVALID_MATCH_FUNC(test_invalid_exception1, "my exception is \xc2\xb7", "my exception is ·");
INVALID_MATCH_FUNC(test_invalid_exception3, "my exception is \xd9\xa2", "my exception is \xd9\xa2");
INVALID_MATCH_FUNC(test_invalid_exception4, "my exception is \xe3\x80\xae", "my exception is \xe3\x80\xae"); /* CONTEXT0/DISALLOWED */
INVALID_MATCH_FUNC(test_invalid_join_control, "my exception is \xe2\x80\x8d", "my exception is \xe2\x80\x8d");

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_ascii),
		cmocka_unit_test(test_capitals),
		cmocka_unit_test(test_multilang),
		cmocka_unit_test(test_special_char),
		cmocka_unit_test(test_space_replacement),
		cmocka_unit_test(test_invalid),
		cmocka_unit_test(test_normalization1),
		cmocka_unit_test(inv_test_ascii),
		cmocka_unit_test(inv_test_special_char),
		cmocka_unit_test(inv_test_invalid),
		cmocka_unit_test(test_other_chars),
		cmocka_unit_test(test_spaces),
		cmocka_unit_test(test_compatibility),
		cmocka_unit_test(test_symbols),
		cmocka_unit_test(test_invalid_ignorable1),
		cmocka_unit_test(test_invalid_ignorable2),
		cmocka_unit_test(test_invalid_ignorable3),
		cmocka_unit_test(test_invalid_exception1),
		cmocka_unit_test(test_invalid_exception2),
		cmocka_unit_test(test_invalid_exception3),
		cmocka_unit_test(test_invalid_exception4),
		cmocka_unit_test(test_invalid_join_control),
		cmocka_unit_test(inv_test_invalid_exception1),
		cmocka_unit_test(inv_test_invalid_exception3),
		cmocka_unit_test(inv_test_invalid_exception4),
		cmocka_unit_test(inv_test_invalid_join_control)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
