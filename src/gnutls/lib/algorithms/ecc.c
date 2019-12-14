/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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
#include <algorithms.h>
#include "errors.h"
#include <x509/common.h>
#include <pk.h>
#include "c-strcase.h"

/* Supported ECC curves
 */

static SYSTEM_CONFIG_OR_CONST
gnutls_ecc_curve_entry_st ecc_curves[] = {
#ifdef ENABLE_NON_SUITEB_CURVES
	{
	 .name = "SECP192R1",
	 .oid = "1.2.840.10045.3.1.1",
	 .id = GNUTLS_ECC_CURVE_SECP192R1,
	 .group = GNUTLS_GROUP_SECP192R1,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 24,
	 .supported = 1,
	},
	{
	 .name = "SECP224R1",
	 .oid = "1.3.132.0.33",
	 .id = GNUTLS_ECC_CURVE_SECP224R1,
	 .group = GNUTLS_GROUP_SECP224R1,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 28,
	 .supported = 1,
	},
#endif
	{
	 .name = "SECP256R1",
	 .oid = "1.2.840.10045.3.1.7",
	 .id = GNUTLS_ECC_CURVE_SECP256R1,
	 .group = GNUTLS_GROUP_SECP256R1,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 32,
	 .supported = 1,
	},
	{
	 .name = "SECP384R1",
	 .oid = "1.3.132.0.34",
	 .id = GNUTLS_ECC_CURVE_SECP384R1,
	 .group = GNUTLS_GROUP_SECP384R1,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 48,
	 .supported = 1,
	},
	{
	 .name = "SECP521R1",
	 .oid = "1.3.132.0.35",
	 .id = GNUTLS_ECC_CURVE_SECP521R1,
	 .group = GNUTLS_GROUP_SECP521R1,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 66,
	 .supported = 1,
	},
	{
	 .name = "X25519",
	 .id = GNUTLS_ECC_CURVE_X25519,
	 .group = GNUTLS_GROUP_X25519,
	 .pk = GNUTLS_PK_ECDH_X25519,
	 .size = 32,
	 .supported = 1,
	},
	{
	 .name = "Ed25519",
	 .oid = SIG_EDDSA_SHA512_OID,
	 .id = GNUTLS_ECC_CURVE_ED25519,
	 .pk = GNUTLS_PK_EDDSA_ED25519,
	 .size = 32,
	 .sig_size = 64,
	 .supported = 1,
	},
#if ENABLE_GOST
	/* Curves for usage in GOST digital signature algorithm (GOST R
	 * 34.10-2001/-2012) and key agreement (VKO GOST R 34.10-2001/-2012).
	 *
	 * Historically CryptoPro has defined three 256-bit curves for use with
	 * digital signature algorithm (CryptoPro-A, -B, -C).
	 *
	 * Also it has reissues two of them with different OIDs for key
	 * exchange (CryptoPro-XchA = CryptoPro-A and CryptoPro-XchB =
	 * CryptoPro-C).
	 *
	 * Then TC26 (Standard comitee working on cryptographic standards) has
	 * defined one 256-bit curve (TC26-256-A) and three 512-bit curves
	 * (TC26-512-A, -B, -C).
	 *
	 * And finally TC26 has reissues original CryptoPro curves under their
	 * own OID namespace (TC26-256-B = CryptoPro-A, TC26-256-C =
	 * CryptoPro-B and TC26-256-D = CryptoPro-C).
	 *
	 * CryptoPro OIDs are usable for both GOST R 34.10-2001 and
	 * GOST R 34.10-2012 keys (thus they have GNUTLS_PK_UNKNOWN in this
	 * table).
	 * TC26 OIDs are usable only for GOST R 34.10-2012 keys.
	 */
	{
	 .name = "CryptoPro-A",
	 .oid = "1.2.643.2.2.35.1",
	 .id = GNUTLS_ECC_CURVE_GOST256CPA,
	 .group = GNUTLS_GROUP_GC256B,
	 .pk = GNUTLS_PK_UNKNOWN,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "CryptoPro-B",
	 .oid = "1.2.643.2.2.35.2",
	 .id = GNUTLS_ECC_CURVE_GOST256CPB,
	 .group = GNUTLS_GROUP_GC256C,
	 .pk = GNUTLS_PK_UNKNOWN,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "CryptoPro-C",
	 .oid = "1.2.643.2.2.35.3",
	 .id = GNUTLS_ECC_CURVE_GOST256CPC,
	 .group = GNUTLS_GROUP_GC256D,
	 .pk = GNUTLS_PK_UNKNOWN,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "CryptoPro-XchA",
	 .oid = "1.2.643.2.2.36.0",
	 .id = GNUTLS_ECC_CURVE_GOST256CPXA,
	 .group = GNUTLS_GROUP_GC256B,
	 .pk = GNUTLS_PK_UNKNOWN,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "CryptoPro-XchB",
	 .oid = "1.2.643.2.2.36.1",
	 .id = GNUTLS_ECC_CURVE_GOST256CPXB,
	 .group = GNUTLS_GROUP_GC256D,
	 .pk = GNUTLS_PK_UNKNOWN,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "TC26-256-A",
	 .oid = "1.2.643.7.1.2.1.1.1",
	 .id = GNUTLS_ECC_CURVE_GOST256A,
	 .group = GNUTLS_GROUP_GC256A,
	 .pk = GNUTLS_PK_GOST_12_256,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "TC26-256-B",
	 .oid = "1.2.643.7.1.2.1.1.2",
	 .id = GNUTLS_ECC_CURVE_GOST256B,
	 .group = GNUTLS_GROUP_GC256B,
	 .pk = GNUTLS_PK_GOST_12_256,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "TC26-256-C",
	 .oid = "1.2.643.7.1.2.1.1.3",
	 .id = GNUTLS_ECC_CURVE_GOST256C,
	 .group = GNUTLS_GROUP_GC256C,
	 .pk = GNUTLS_PK_GOST_12_256,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "TC26-256-D",
	 .oid = "1.2.643.7.1.2.1.1.4",
	 .id = GNUTLS_ECC_CURVE_GOST256D,
	 .group = GNUTLS_GROUP_GC256D,
	 .pk = GNUTLS_PK_GOST_12_256,
	 .size = 32,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "TC26-512-A",
	 .oid = "1.2.643.7.1.2.1.2.1",
	 .id = GNUTLS_ECC_CURVE_GOST512A,
	 .group = GNUTLS_GROUP_GC512A,
	 .pk = GNUTLS_PK_GOST_12_512,
	 .size = 64,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "TC26-512-B",
	 .oid = "1.2.643.7.1.2.1.2.2",
	 .id = GNUTLS_ECC_CURVE_GOST512B,
	 .group = GNUTLS_GROUP_GC512B,
	 .pk = GNUTLS_PK_GOST_12_512,
	 .size = 64,
	 .gost_curve = 1,
	 .supported = 1,
	},
	{
	 .name = "TC26-512-C",
	 .oid = "1.2.643.7.1.2.1.2.3",
	 .id = GNUTLS_ECC_CURVE_GOST512C,
	 .group = GNUTLS_GROUP_GC512C,
	 .pk = GNUTLS_PK_GOST_12_512,
	 .size = 64,
	 .gost_curve = 1,
	 .supported = 1,
	},
#endif
	{0, 0, 0}
};

#define GNUTLS_ECC_CURVE_LOOP(b) \
	{ const gnutls_ecc_curve_entry_st *p; \
		for(p = ecc_curves; p->name != NULL; p++) { b ; } }


/**
 * gnutls_ecc_curve_list:
 *
 * Get the list of supported elliptic curves.
 *
 * This function is not thread safe.
 *
 * Returns: Return a (0)-terminated list of #gnutls_ecc_curve_t
 *   integers indicating the available curves.
 **/
const gnutls_ecc_curve_t *gnutls_ecc_curve_list(void)
{
	static gnutls_ecc_curve_t supported_curves[MAX_ALGOS] = { 0 };

	if (supported_curves[0] == 0) {
		int i = 0;

		GNUTLS_ECC_CURVE_LOOP(
			if (p->supported && _gnutls_pk_curve_exists(p->id))
				supported_curves[i++] = p->id;
		);
		supported_curves[i++] = 0;
	}

	return supported_curves;
}

unsigned _gnutls_ecc_curve_is_supported(gnutls_ecc_curve_t curve)
{
	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == curve && p->supported && _gnutls_pk_curve_exists(p->id))
			return 1;
	);
	return 0;
}

/**
 * gnutls_oid_to_ecc_curve:
 * @oid: is a curve's OID
 *
 * Returns: return a #gnutls_ecc_curve_t value corresponding to
 *   the specified OID, or %GNUTLS_ECC_CURVE_INVALID on error.
 *
 * Since: 3.4.3
 **/
gnutls_ecc_curve_t gnutls_oid_to_ecc_curve(const char *oid)
{
	gnutls_ecc_curve_t ret = GNUTLS_ECC_CURVE_INVALID;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->oid != NULL && c_strcasecmp(p->oid, oid) == 0 && p->supported &&
		    _gnutls_pk_curve_exists(p->id)) {
			ret = p->id;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_ecc_curve_get_id:
 * @name: is a curve name
 *
 * The names are compared in a case insensitive way.
 *
 * Returns: return a #gnutls_ecc_curve_t value corresponding to
 *   the specified curve, or %GNUTLS_ECC_CURVE_INVALID on error.
 *
 * Since: 3.4.3
 **/
gnutls_ecc_curve_t gnutls_ecc_curve_get_id(const char *name)
{
	gnutls_ecc_curve_t ret = GNUTLS_ECC_CURVE_INVALID;

	GNUTLS_ECC_CURVE_LOOP(
		if (c_strcasecmp(p->name, name) == 0 && p->supported &&
		    _gnutls_pk_curve_exists(p->id)) {
			ret = p->id;
			break;
		}
	);

	return ret;
}

int _gnutls_ecc_curve_mark_disabled(const char *name)
{
	gnutls_ecc_curve_entry_st *p;

	for(p = ecc_curves; p->name != NULL; p++) {
		if (c_strcasecmp(p->name, name) == 0) {
			p->supported = 0;
			return 0;
		}
	}

	return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
}

static int _gnutls_ecc_pk_compatible(const gnutls_ecc_curve_entry_st *p,
				     gnutls_pk_algorithm_t pk)
{
	if (!p->supported || !_gnutls_pk_curve_exists(p->id))
		return 0;

	if (pk == GNUTLS_PK_GOST_01 ||
	    pk == GNUTLS_PK_GOST_12_256)
		return p->gost_curve && p->size == 32;

	return pk == p->pk;
}

/*-
 * _gnutls_ecc_bits_to_curve:
 * @bits: is a security parameter in bits
 *
 * Returns: return a #gnutls_ecc_curve_t value corresponding to
 *   the specified bit length, or %GNUTLS_ECC_CURVE_INVALID on error.
 -*/
gnutls_ecc_curve_t _gnutls_ecc_bits_to_curve(gnutls_pk_algorithm_t pk, int bits)
{
	gnutls_ecc_curve_t ret;

	if (pk == GNUTLS_PK_ECDSA)
		ret = GNUTLS_ECC_CURVE_SECP256R1;
	else if (pk == GNUTLS_PK_GOST_01 ||
		 pk == GNUTLS_PK_GOST_12_256)
		ret = GNUTLS_ECC_CURVE_GOST256CPA;
	else if (pk == GNUTLS_PK_GOST_12_512)
		ret = GNUTLS_ECC_CURVE_GOST512A;
	else
		ret = GNUTLS_ECC_CURVE_ED25519;

	GNUTLS_ECC_CURVE_LOOP(
		if (_gnutls_ecc_pk_compatible(p, pk) && 8 * p->size >= (unsigned)bits) {
			ret = p->id;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_ecc_curve_get_name:
 * @curve: is an ECC curve
 *
 * Convert a #gnutls_ecc_curve_t value to a string.
 *
 * Returns: a string that contains the name of the specified
 *   curve or %NULL.
 *
 * Since: 3.0
 **/
const char *gnutls_ecc_curve_get_name(gnutls_ecc_curve_t curve)
{
	const char *ret = NULL;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == curve) {
			ret = p->name;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_ecc_curve_get_oid:
 * @curve: is an ECC curve
 *
 * Convert a #gnutls_ecc_curve_t value to its object identifier.
 *
 * Returns: a string that contains the OID of the specified
 *   curve or %NULL.
 *
 * Since: 3.4.3
 **/
const char *gnutls_ecc_curve_get_oid(gnutls_ecc_curve_t curve)
{
	const char *ret = NULL;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == curve) {
			ret = p->oid;
			break;
		}
	);

	return ret;
}

/*-
 * _gnutls_ecc_curve_get_params:
 * @curve: is an ECC curve
 *
 * Returns the information on a curve.
 *
 * Returns: a pointer to #gnutls_ecc_curve_entry_st or %NULL.
 -*/
const gnutls_ecc_curve_entry_st
    *_gnutls_ecc_curve_get_params(gnutls_ecc_curve_t curve)
{
	const gnutls_ecc_curve_entry_st *ret = NULL;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == curve) {
			ret = p;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_ecc_curve_get_size:
 * @curve: is an ECC curve
 *
 * Returns: the size in bytes of the curve or 0 on failure.
 *
 * Since: 3.0
 **/
int gnutls_ecc_curve_get_size(gnutls_ecc_curve_t curve)
{
	int ret = 0;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == curve) {
			ret = p->size;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_ecc_curve_get_pk:
 * @curve: is an ECC curve
 *
 * Returns: the public key algorithm associated with the named curve or %GNUTLS_PK_UNKNOWN.
 *
 * Since: 3.5.0
 **/
gnutls_pk_algorithm_t gnutls_ecc_curve_get_pk(gnutls_ecc_curve_t curve)
{
	int ret = GNUTLS_PK_UNKNOWN;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == curve && p->supported) {
			ret = p->pk;
			break;
		}
	);

	return ret;
}

/**
 * _gnutls_ecc_curve_get_group:
 * @curve: is an ECC curve
 *
 * Returns: the group associated with the named curve or %GNUTLS_GROUP_INVALID.
 *
 * Since: 3.6.11
 */
gnutls_group_t _gnutls_ecc_curve_get_group(gnutls_ecc_curve_t curve)
{
	gnutls_group_t ret = GNUTLS_GROUP_INVALID;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == curve && p->supported && _gnutls_pk_curve_exists(p->id)) {
			ret = p->group;
			break;
		}
	);

	return ret;
}
