/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Daiki Ueno
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "ecdsa-compute-k.h"

#include "dsa-compute-k.h"
#include "gnutls_int.h"

static inline int
_gnutls_ecc_curve_to_dsa_q(mpz_t *q, gnutls_ecc_curve_t curve)
{
	switch (curve) {
#ifdef ENABLE_NON_SUITEB_CURVES
	case GNUTLS_ECC_CURVE_SECP192R1:
		mpz_init_set_str(*q,
				 "FFFFFFFFFFFFFFFFFFFFFFFF99DEF836"
				 "146BC9B1B4D22831",
				 16);
		return 0;
	case GNUTLS_ECC_CURVE_SECP224R1:
		mpz_init_set_str(*q,
				 "FFFFFFFFFFFFFFFFFFFFFFFFFFFF16A2"
				 "E0B8F03E13DD29455C5C2A3D",
				 16);
		return 0;
#endif
	case GNUTLS_ECC_CURVE_SECP256R1:
		mpz_init_set_str(*q,
				 "FFFFFFFF00000000FFFFFFFFFFFFFFFF"
				 "BCE6FAADA7179E84F3B9CAC2FC632551",
				 16);
		return 0;
	case GNUTLS_ECC_CURVE_SECP384R1:
		mpz_init_set_str(*q,
				 "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
				 "FFFFFFFFFFFFFFFFC7634D81F4372DDF"
				 "581A0DB248B0A77AECEC196ACCC52973",
				 16);
		return 0;
	case GNUTLS_ECC_CURVE_SECP521R1:
		mpz_init_set_str(*q,
				 "1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
				 "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
				 "FFA51868783BF2F966B7FCC0148F709A"
				 "5D03BB5C9B8899C47AEBB6FB71E91386"
				 "409",
				 16);
		return 0;
	default:
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM);
	}
}

int
_gnutls_ecdsa_compute_k (mpz_t k,
			 gnutls_ecc_curve_t curve,
			 const mpz_t x,
			 gnutls_mac_algorithm_t mac,
			 const uint8_t *digest,
			 size_t length)
{
	mpz_t q;
	int ret;

	ret = _gnutls_ecc_curve_to_dsa_q(&q, curve);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_dsa_compute_k (k, q, x, mac, digest, length);
	mpz_clear(q);
	return ret;
}
