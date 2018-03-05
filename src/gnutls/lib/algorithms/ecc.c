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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include <algorithms.h>
#include "errors.h"
#include <x509/common.h>
#include <pk.h>

/* Supported ECC curves
 */

static const gnutls_ecc_curve_entry_st ecc_curves[] = {
	{
	 .name = "SECP192R1",
	 .oid = "1.2.840.10045.3.1.1",
	 .id = GNUTLS_ECC_CURVE_SECP192R1,
	 .tls_id = 19,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 24,
	},
	{
	 .name = "SECP224R1",
	 .oid = "1.3.132.0.33",
	 .id = GNUTLS_ECC_CURVE_SECP224R1,
	 .tls_id = 21,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 28,
	},
	{
	 .name = "SECP256R1",
	 .oid = "1.2.840.10045.3.1.7",
	 .id = GNUTLS_ECC_CURVE_SECP256R1,
	 .tls_id = 23,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 32,
	},
	{
	 .name = "SECP384R1",
	 .oid = "1.3.132.0.34",
	 .id = GNUTLS_ECC_CURVE_SECP384R1,
	 .tls_id = 24,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 48,
	},
	{
	 .name = "SECP521R1",
	 .oid = "1.3.132.0.35",
	 .id = GNUTLS_ECC_CURVE_SECP521R1,
	 .tls_id = 25,
	 .pk = GNUTLS_PK_ECDSA,
	 .size = 66,
	},
	{
	 .name = "X25519",
	 .id = GNUTLS_ECC_CURVE_X25519,
	 .tls_id = 29,
	 .pk = GNUTLS_PK_ECDHX,
	 .size = 32,
	},
	{0, 0, 0}
};

#define GNUTLS_ECC_CURVE_LOOP(b) \
	{ const gnutls_ecc_curve_entry_st *p; \
		for(p = ecc_curves; p->name != NULL; p++) { b ; } }


/* Returns the TLS id of the given curve
 */
int _gnutls_tls_id_to_ecc_curve(int num)
{
	gnutls_ecc_curve_t ret = GNUTLS_ECC_CURVE_INVALID;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->tls_id == num && _gnutls_pk_curve_exists(p->id)) {
			ret = p->id; 
			break;
		}
	);

	return ret;
}

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
			if (_gnutls_pk_curve_exists(p->id)) 
				supported_curves[i++] = p->id;
		);
		supported_curves[i++] = 0;
	}

	return supported_curves;
}

/* Maps numbers to TLS NamedCurve IDs (RFC4492).
 * Returns a negative number on error.
 */
int _gnutls_ecc_curve_get_tls_id(gnutls_ecc_curve_t supported_ecc)
{
	int ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;

	GNUTLS_ECC_CURVE_LOOP(
		if (p->id == supported_ecc) {
			ret = p->tls_id; 
			break;
		}
	);

	return ret;
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
		if (p->oid && strcasecmp(p->oid, oid) == 0 && _gnutls_pk_curve_exists(p->id)) {
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
		if (strcasecmp(p->name, name) == 0 && _gnutls_pk_curve_exists(p->id)) {
			ret = p->id;
			break;
		}
	);

	return ret;
}

/*-
 * _gnutls_ecc_bits_to_curve:
 * @bits: is a security parameter in bits
 *
 * Returns: return a #gnutls_ecc_curve_t value corresponding to
 *   the specified bit length, or %GNUTLS_ECC_CURVE_INVALID on error.
 -*/
gnutls_ecc_curve_t _gnutls_ecc_bits_to_curve(int bits)
{
	gnutls_ecc_curve_t ret = GNUTLS_ECC_CURVE_SECP256R1;

	GNUTLS_ECC_CURVE_LOOP(
		if (8 * p->size >= bits && _gnutls_pk_curve_exists(p->id)) {
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
		if (p->id == curve) {
			ret = p->pk;
			break;
		}
	);

	return ret;
}

