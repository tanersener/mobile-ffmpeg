/*
 * Copyright (C) 2017 Tim Rühsen
 * Copyright (C) 2016, 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include "str.h"
#include <unistr.h>
#ifdef HAVE_LIBIDN2
# include <idn2.h>
#elif defined HAVE_LIBIDN
# include <idna.h>
# include <idn-free.h>
#endif

#if defined HAVE_LIBIDN2 || defined HAVE_LIBIDN

#ifdef HAVE_LIBIDN2
# define IDN_FREE idn2_free
#else
# define IDN_FREE idn_free
#endif

/**
 * gnutls_idna_map:
 * @input: contain the UTF-8 formatted domain name
 * @ilen: the length of the provided string
 * @out: the result in an null-terminated allocated string
 * @flags: should be zero
 *
 * This function will convert the provided UTF-8 domain name, to
 * its IDNA mapping in an allocated variable. Note that depending on the flags the used gnutls
 * library was compiled with, the output of this function may vary (i.e.,
 * may be IDNA2008, or IDNA2003).
 *
 * To force IDNA2008 specify the flag %GNUTLS_IDNA_FORCE_2008. In
 * the case GnuTLS is not compiled with the necessary dependencies,
 * %GNUTLS_E_UNIMPLEMENTED_FEATURE will be returned to indicate that
 * gnutls is unable to perform the requested conversion.
 *
 * Note also, that this function will return an empty string if an
 * empty string is provided as input.
 *
 * Returns: %GNUTLS_E_INVALID_UTF8_STRING on invalid UTF-8 data, or 0 on success.
 *
 * Since: 3.5.8
 **/
int gnutls_idna_map(const char *input, unsigned ilen, gnutls_datum_t *out, unsigned flags)
{
	char *idna = NULL;
	int rc, ret;
	gnutls_datum_t istr;

	if (ilen == 0) {
		out->data = (uint8_t*)gnutls_strdup("");
		out->size = 0;
		if (out->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		return 0;
	}

	if (_gnutls_str_is_print(input, ilen)) {
		return _gnutls_set_strdatum(out, input, ilen);
	}

#ifndef HAVE_LIBIDN2
	if (flags & GNUTLS_IDNA_FORCE_2008)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
#endif

	ret = _gnutls_set_strdatum(&istr, input, ilen);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

#ifdef HAVE_LIBIDN2
# if IDN2_VERSION_NUMBER >= 0x00140000
	/* IDN2_NONTRANSITIONAL automatically converts to lowercase
	 * IDN2_NFC_INPUT converts to NFC before toASCII conversion
	 *
	 * Since IDN2_NONTRANSITIONAL implicitely does NFC conversion, we don't need
	 * the additional IDN2_NFC_INPUT. But just for the unlikely case that the linked
	 * library is not matching the headers when building and it doesn't support TR46,
	 * we provide IDN2_NFC_INPUT. 
	 *
	 * The reason we fallback to transitional encoding on disallowed characters is
	 * to support domains which existed in IDNA2003, but were invalid with IDNA2008.
	 */

	rc = idn2_lookup_u8((uint8_t *)istr.data, (uint8_t **)&idna, IDN2_NFC_INPUT | IDN2_NONTRANSITIONAL);
	if (rc == IDN2_DISALLOWED && !(flags & GNUTLS_IDNA_FORCE_2008))
		rc = idn2_lookup_u8((uint8_t *)istr.data, (uint8_t **)&idna, IDN2_NFC_INPUT | IDN2_TRANSITIONAL);
# else
	rc = idn2_lookup_u8((uint8_t *)istr.data, (uint8_t **)&idna, IDN2_NFC_INPUT);
# endif
	if (rc != IDN2_OK) {
		gnutls_assert();
		idna = NULL; /* in case idn2_lookup_u8 modifies &idna */
		_gnutls_debug_log("unable to convert name '%s' to IDNA format: %s\n", istr.data, idn2_strerror(rc));
		ret = GNUTLS_E_INVALID_UTF8_STRING;
		goto fail;
	}
#else
	rc = idna_to_ascii_8z((char*)istr.data, &idna, 0);
	if (rc != IDNA_SUCCESS) {
		gnutls_assert();
		_gnutls_debug_log("unable to convert name '%s' to IDNA format: %s\n", istr.data, idna_strerror(rc));
		ret = GNUTLS_E_INVALID_UTF8_STRING;
		goto fail;
	}
#endif

	if (gnutls_free != IDN_FREE) {
		ret = _gnutls_set_strdatum(out, idna, strlen(idna));
	} else  {
		out->data = (unsigned char*)idna;
		out->size = strlen(idna);
		idna = NULL;
		ret = 0;
	}

 fail:
	IDN_FREE(idna);
	gnutls_free(istr.data);
	return ret;
}

#ifdef HAVE_LIBIDN2
int _idn2_punycode_decode(
	size_t input_length,
	const char input[],
	size_t *output_length,
	uint32_t output[],
	unsigned char case_flags[]);

static int _idn2_to_unicode_8z8z(const char *src, char **dst)
{
	int rc, run;
	size_t out_len = 0;
	const char *e, *s;
	char *p = NULL;

	for (run = 0; run < 2; run++) {
		if (run) {
			p = malloc(out_len + 1);
			if (!p)
				return IDN2_MALLOC;
			*dst = p;
		}

		out_len = 0;
		for (e = s = src; *e; s = e) {
			while (*e && *e != '.')
				e++;

			if (e - s > 4 && (s[0] == 'x' || s[0] == 'X') && (s[1] == 'n' || s[1] == 'N') && s[2] == '-' && s[3] == '-') {
				size_t u32len = IDN2_LABEL_MAX_LENGTH * 4;
				uint32_t u32[IDN2_LABEL_MAX_LENGTH * 4];
				uint8_t u8[IDN2_LABEL_MAX_LENGTH + 1];
				size_t u8len;

				rc = _idn2_punycode_decode(e - s - 4, s + 4, &u32len, u32, NULL);
				if (rc != IDN2_OK)
					return rc;

				u8len = sizeof(u8);
				if (u32_to_u8(u32, u32len, u8, &u8len) == NULL)
					return IDN2_ENCODING_ERROR;
				u8[u8len] = '\0';

				if (run)
					memcpy(*dst + out_len, u8, u8len);
				out_len += u8len;
			} else {
				if (run)
					memcpy(*dst + out_len, s, e - s);
				out_len += e - s;
			}

			if (*e) {
				e++;
				if (run)
					(*dst)[out_len] = '.';
				out_len++;
			}
		}
	}

	(*dst)[out_len] = 0;

	return IDN2_OK;
}
#endif

/**
 * gnutls_idna_reverse_map:
 * @input: contain the ACE (IDNA) formatted domain name
 * @ilen: the length of the provided string
 * @out: the result in an null-terminated allocated UTF-8 string
 * @flags: should be zero
 *
 * This function will convert an ACE (ASCII-encoded) domain name to a UTF-8 domain name.
 *
 * If GnuTLS is compiled without IDNA support, then this function
 * will return %GNUTLS_E_UNIMPLEMENTED_FEATURE.
 *
 * Note also, that this function will return an empty string if an
 * empty string is provided as input.
 *
 * Returns: A negative error code on error, or 0 on success.
 *
 * Since: 3.5.8
 **/
int gnutls_idna_reverse_map(const char *input, unsigned ilen, gnutls_datum_t *out, unsigned flags)
{
	char *u8 = NULL;
	int rc, ret;
	gnutls_datum_t istr;

	if (ilen == 0) {
		out->data = (uint8_t*)gnutls_strdup("");
		out->size = 0;
		if (out->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		return 0;
	}

	ret = _gnutls_set_strdatum(&istr, input, ilen);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

#ifdef HAVE_LIBIDN2
	/* currently libidn2 just converts single labels, thus a wrapper function */
	rc = _idn2_to_unicode_8z8z((char*)istr.data, &u8);
	if (rc != IDN2_OK) {
		gnutls_assert();
		_gnutls_debug_log("unable to convert ACE name '%s' to UTF-8 format: %s\n", istr.data, idn2_strerror(rc));
		ret = GNUTLS_E_INVALID_UTF8_STRING;
		goto fail;
	}
#else
	rc = idna_to_unicode_8z8z((char*)istr.data, &u8, IDNA_ALLOW_UNASSIGNED);
	if (rc != IDNA_SUCCESS) {
		gnutls_assert();
		_gnutls_debug_log("unable to convert ACE name '%s' to UTF-8 format: %s\n", istr.data, idna_strerror(rc));
		ret = GNUTLS_E_INVALID_UTF8_STRING;
		goto fail;
	}
#endif

	if (gnutls_malloc != malloc) {
		ret = _gnutls_set_strdatum(out, u8, strlen(u8));
	} else  {
		out->data = (unsigned char*)u8;
		out->size = strlen(u8);
		u8 = NULL;
		ret = 0;
	}
 fail:
#ifdef HAVE_LIBIDN2
	idn2_free(u8);
#else
	idn_free(u8);
#endif
	gnutls_free(istr.data);
	return ret;
}

#else

# undef gnutls_idna_map
int gnutls_idna_map(const char *input, unsigned ilen, gnutls_datum_t *out, unsigned flags)
{
	if (!_gnutls_str_is_print(input, ilen)) {
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}

	return _gnutls_set_strdatum(out, input, ilen);
}

int gnutls_idna_reverse_map(const char *input, unsigned ilen, gnutls_datum_t *out, unsigned flags)
{
	return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
}
#endif /* HAVE_LIBIDN2 */

int _gnutls_idna_email_map(const char *input, unsigned ilen, gnutls_datum_t *output)
{
	const char *p = input;

	while(*p != 0 && *p != '@') {
		if (!c_isprint(*p))
			return gnutls_assert_val(GNUTLS_E_INVALID_UTF8_EMAIL);
		p++;
	}

	if (_gnutls_str_is_print(input, ilen)) {
		return _gnutls_set_strdatum(output, input, ilen);
	}

	if (*p == '@') {
		unsigned name_part = p-input;
		int ret;
		gnutls_datum_t domain;

		ret = gnutls_idna_map(p+1, ilen-name_part-1, &domain, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);

		output->data = gnutls_malloc(name_part+1+domain.size+1);
		if (output->data == NULL) {
			gnutls_free(domain.data);
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}
		memcpy(output->data, input, name_part);
		output->data[name_part] = '@';
		memcpy(&output->data[name_part+1], domain.data, domain.size);
		output->data[name_part+domain.size+1] = 0;
		output->size = name_part+domain.size+1;
		gnutls_free(domain.data);
		return 0;
	} else {
		return gnutls_assert_val(GNUTLS_E_INVALID_UTF8_EMAIL);
	}
}
