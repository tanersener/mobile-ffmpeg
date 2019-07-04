/*
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include "str.h"
#include <uninorm.h>
#include <unistr.h>
#include <unictype.h>

/* rfc5892#section-2.6 exceptions
 */
inline static int is_allowed_exception(uint32_t ch)
{
	switch (ch) {
		case 0xB7:
		case 0x0375:
		case 0x05F3:
		case 0x05F4:
		case 0x30FB:
		case 0x0660:
		case 0x0661:
		case 0x0662:
		case 0x0663:
		case 0x0664:
		case 0x0665:
		case 0x0666:
		case 0x0667:
		case 0x0668:
		case 0x0669:
		case 0x06F0:
		case 0x06F1:
		case 0x06F2:
		case 0x06F3:
		case 0x06F4:
		case 0x06F5:
		case 0x06F6:
		case 0x06F7:
		case 0x06F8:
		case 0x06F9:
		case 0x0640:
		case 0x07FA:
		case 0x302E:
		case 0x302F:
		case 0x3031:
		case 0x3032:
		case 0x3033:
		case 0x3034:
		case 0x3035:
		case 0x303B:
			return 0; /* disallowed */
		case 0xDF:
		case 0x03C2:
		case 0x06FD:
		case 0x06FE:
		case 0x0F0B:
		case 0x3007:
			return 1; /* allowed */
		default:
			return -1; /* not exception */
	}
}

/* Checks whether the provided string is in the valid set of FreeFormClass (RFC7564
 * as an RFC7613 requirement), and converts all spaces to the ASCII-space. */
static int check_for_valid_freeformclass(uint32_t *ucs4, unsigned ucs4_size)
{
	unsigned i;
	int rc;
	uint32_t tmp[4];
	size_t tmp_size;
	uint32_t *nrm;
	uc_general_category_t cat;
	unsigned is_invalid;

	/* make the union of Valid categories, excluding any invalid (i.e., control) */
	cat = uc_general_category_or(UC_CATEGORY_Ll, UC_CATEGORY_Lu); /* LetterDigits */
	cat = uc_general_category_or(cat, UC_CATEGORY_Lo);
	cat = uc_general_category_or(cat, UC_CATEGORY_Nd);
	cat = uc_general_category_or(cat, UC_CATEGORY_Lm);
	cat = uc_general_category_or(cat, UC_CATEGORY_Mn);
	cat = uc_general_category_or(cat, UC_CATEGORY_Mc);
	cat = uc_general_category_or(cat, UC_CATEGORY_Lt); /* OtherLetterDigits */
	cat = uc_general_category_or(cat, UC_CATEGORY_Nl);
	cat = uc_general_category_or(cat, UC_CATEGORY_No);
	cat = uc_general_category_or(cat, UC_CATEGORY_Me);
	cat = uc_general_category_or(cat, UC_CATEGORY_Sm); /* Symbols */
	cat = uc_general_category_or(cat, UC_CATEGORY_Sc);
	cat = uc_general_category_or(cat, UC_CATEGORY_So);
	cat = uc_general_category_or(cat, UC_CATEGORY_Sk);
	cat = uc_general_category_or(cat, UC_CATEGORY_Pc); /* Punctuation */
	cat = uc_general_category_or(cat, UC_CATEGORY_Pd);
	cat = uc_general_category_or(cat, UC_CATEGORY_Ps);
	cat = uc_general_category_or(cat, UC_CATEGORY_Pe);
	cat = uc_general_category_or(cat, UC_CATEGORY_Pi);
	cat = uc_general_category_or(cat, UC_CATEGORY_Pf);
	cat = uc_general_category_or(cat, UC_CATEGORY_Po);
	cat = uc_general_category_or(cat, UC_CATEGORY_Zs); /* Spaces */
	cat = uc_general_category_and_not(cat, UC_CATEGORY_Cc); /* Not in Control */

	/* check for being in the allowed sets in rfc7564#section-4.3 */
	for (i=0;i<ucs4_size;i++) {
		is_invalid = 0;

		/* Disallowed 
		   o  Old Hangul Jamo characters, i.e., the OldHangulJamo ("I") category
		      (not handled in this code)

		   o  Control characters, i.e., the Controls ("L") category

		   o  Ignorable characters, i.e., the PrecisIgnorableProperties ("M")
		 */
		if (uc_is_property_default_ignorable_code_point(ucs4[i]) ||
		    uc_is_property_not_a_character(ucs4[i])) {
			return gnutls_assert_val(GNUTLS_E_INVALID_UTF8_STRING);
		}


		/* Contextual rules - we do not implement them / we reject chars from these sets
		   o  A number of characters from the Exceptions ("F") category defined

		   o  Joining characters, i.e., the JoinControl ("H") category defined
		 */
		rc = is_allowed_exception(ucs4[i]);
		if (rc == 0 || uc_is_property_join_control(ucs4[i]))
			return gnutls_assert_val(GNUTLS_E_INVALID_UTF8_STRING);

		if (rc == 1) /* exceptionally allowed, continue */
			continue;


		/* Replace all spaces; an RFC7613 requirement
		 */
		if (uc_is_general_category(ucs4[i], UC_CATEGORY_Zs)) /* replace */
			ucs4[i] = 0x20;

		/* Valid */
		if ((ucs4[i] < 0x21 || ucs4[i] > 0x7E) && !uc_is_general_category(ucs4[i], cat))
			is_invalid = 1;

		/* HasCompat */
		if (is_invalid) {
			tmp_size = sizeof(tmp)/sizeof(tmp[0]);
			nrm = u32_normalize(UNINORM_NFKC, &ucs4[i], 1, tmp, &tmp_size);
			if (nrm == NULL || (tmp_size == 1 && nrm[0] == ucs4[i]))
				return gnutls_assert_val(GNUTLS_E_INVALID_UTF8_STRING);
		}
	}

	return 0;
}


/**
 * gnutls_utf8_password_normalize:
 * @password: contain the UTF-8 formatted password
 * @plen: the length of the provided password
 * @out: the result in an null-terminated allocated string
 * @flags: should be zero
 *
 * This function will convert the provided UTF-8 password according
 * to the normalization rules in RFC7613.
 *
 * If the flag %GNUTLS_UTF8_IGNORE_ERRS is specified, any UTF-8 encoding
 * errors will be ignored, and in that case the output will be a copy of the input.
 *
 * Returns: %GNUTLS_E_INVALID_UTF8_STRING on invalid UTF-8 data, or 0 on success.
 *
 * Since: 3.5.7
 **/
int gnutls_utf8_password_normalize(const unsigned char *password, unsigned plen,
				   gnutls_datum_t *out, unsigned flags)
{
	size_t ucs4_size = 0, nrm_size = 0;
	size_t final_size = 0;
	uint8_t *final = NULL;
	uint32_t *ucs4 = NULL;
	uint32_t *nrm = NULL;
	uint8_t *nrmu8 = NULL;
	int ret;

	if (plen == 0) {
		out->data = (uint8_t*)gnutls_strdup("");
		out->size = 0;
		if (out->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		return 0;
	}

	/* check for invalid UTF-8 */
	if (u8_check((uint8_t*)password, plen) != NULL) {
		gnutls_assert();
		if (flags & GNUTLS_UTF8_IGNORE_ERRS) {
 raw_copy:
			out->data = gnutls_malloc(plen+1);
			if (out->data == NULL)
				return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
			out->size = plen;
			memcpy(out->data, password, plen);
			out->data[plen] = 0;
			return 0;
		} else {
			return GNUTLS_E_INVALID_UTF8_STRING;
		}
	}

	/* convert to UTF-32 */
	ucs4 = u8_to_u32((uint8_t*)password, plen, NULL, &ucs4_size);
	if (ucs4 == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_PARSING_ERROR;
		goto fail;
	}

	ret = check_for_valid_freeformclass(ucs4, ucs4_size);
	if (ret < 0) {
		gnutls_assert();
		if (flags & GNUTLS_UTF8_IGNORE_ERRS) {
			free(ucs4);
			goto raw_copy;
		}
		if (ret == GNUTLS_E_INVALID_UTF8_STRING)
			ret = GNUTLS_E_INVALID_PASSWORD_STRING;
		goto fail;
	}

	/* normalize to NFC */
	nrm = u32_normalize(UNINORM_NFC, ucs4, ucs4_size, NULL, &nrm_size);
	if (nrm == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_PASSWORD_STRING;
		goto fail;
	}

	/* convert back to UTF-8 */
	final_size = 0;
	nrmu8 = u32_to_u8(nrm, nrm_size, NULL, &final_size);
	if (nrmu8 == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_PASSWORD_STRING;
		goto fail;
	}

	/* copy to output with null terminator */
	final = gnutls_malloc(final_size+1);
	if (final == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto fail;
	}

	memcpy(final, nrmu8, final_size);
	final[final_size] = 0;

	free(ucs4);
	free(nrm);
	free(nrmu8);

	out->data = final;
	out->size = final_size;

	return 0;

 fail:
	gnutls_free(final);
	free(ucs4);
	free(nrm);
	free(nrmu8);
	return ret;
}

