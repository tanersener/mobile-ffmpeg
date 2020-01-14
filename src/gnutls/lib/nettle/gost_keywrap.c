/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2016 Dmitry Eremin-Solenikov
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
 */

#include "gnutls_int.h"
#include "gost/gost28147.h"

static const struct gost28147_param *
_gnutls_gost_get_param(gnutls_gost_paramset_t param)
{
	if (param == GNUTLS_GOST_PARAMSET_TC26_Z)
		return &gost28147_param_TC26_Z;
	else if (param == GNUTLS_GOST_PARAMSET_CP_A)
		return &gost28147_param_CryptoPro_A;
	else if (param == GNUTLS_GOST_PARAMSET_CP_B)
		return &gost28147_param_CryptoPro_B;
	else if (param == GNUTLS_GOST_PARAMSET_CP_C)
		return &gost28147_param_CryptoPro_C;
	else if (param == GNUTLS_GOST_PARAMSET_CP_D)
		return &gost28147_param_CryptoPro_D;

	gnutls_assert();

	return NULL;
}

int _gnutls_gost_key_wrap(gnutls_gost_paramset_t gost_params,
			  const gnutls_datum_t *kek,
			  const gnutls_datum_t *ukm,
			  const gnutls_datum_t *cek,
			  gnutls_datum_t *enc,
			  gnutls_datum_t *imit)
{
	const struct gost28147_param *gp;

	gp = _gnutls_gost_get_param(gost_params);
	if (gp == NULL) {
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
	}

	if (kek->size != GOST28147_KEY_SIZE ||
	    cek->size != GOST28147_KEY_SIZE ||
	    ukm->size < GOST28147_IMIT_BLOCK_SIZE) {
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
	}

	enc->size = GOST28147_KEY_SIZE;
	enc->data = gnutls_malloc(enc->size);
	if (enc->data == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	imit->size = GOST28147_IMIT_DIGEST_SIZE;
	imit->data = gnutls_malloc(imit->size);
	if (imit->data == NULL) {
		_gnutls_free_datum(enc);
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	gost28147_key_wrap_cryptopro(gp, kek->data, ukm->data, ukm->size,
				     cek->data, enc->data, imit->data);

	return 0;
}

int _gnutls_gost_key_unwrap(gnutls_gost_paramset_t gost_params,
			    const gnutls_datum_t *kek,
			    const gnutls_datum_t *ukm,
			    const gnutls_datum_t *enc,
			    const gnutls_datum_t *imit,
			    gnutls_datum_t *cek)
{
	const struct gost28147_param *gp;
	int ret;

	gp = _gnutls_gost_get_param(gost_params);
	if (gp == NULL) {
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
	}

	if (kek->size != GOST28147_KEY_SIZE ||
	    enc->size != GOST28147_KEY_SIZE ||
	    imit->size != GOST28147_IMIT_DIGEST_SIZE ||
	    ukm->size < GOST28147_IMIT_BLOCK_SIZE) {
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
	}

	cek->size = GOST28147_KEY_SIZE;
	cek->data = gnutls_malloc(cek->size);
	if (cek->data == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	ret = gost28147_key_unwrap_cryptopro(gp, kek->data,
					     ukm->data, ukm->size,
					     enc->data, imit->data,
					     cek->data);
	if (ret == 0) {
		gnutls_assert();
		_gnutls_free_temp_key_datum(cek);
		return GNUTLS_E_DECRYPTION_FAILED;
	}

	return 0;
}
