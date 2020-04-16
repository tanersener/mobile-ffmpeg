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


/* KX mappings to PK algorithms */
typedef struct {
	gnutls_kx_algorithm_t kx_algorithm;
	gnutls_pk_algorithm_t pk_algorithm;
	enum encipher_type encipher_type;	/* CIPHER_ENCRYPT if this algorithm is to be used
						 * for encryption, CIPHER_SIGN if signature only,
						 * CIPHER_IGN if this does not apply at all.
						 *
						 * This is useful to certificate cipher suites, which check
						 * against the certificate key usage bits.
						 */
} gnutls_pk_map;

/* This table maps the Key exchange algorithms to
 * the certificate algorithms. Eg. if we have
 * RSA algorithm in the certificate then we can
 * use GNUTLS_KX_RSA or GNUTLS_KX_DHE_RSA.
 */
static const gnutls_pk_map pk_mappings[] = {
	{GNUTLS_KX_RSA, GNUTLS_PK_RSA, CIPHER_ENCRYPT},
	{GNUTLS_KX_DHE_RSA, GNUTLS_PK_RSA, CIPHER_SIGN},
	{GNUTLS_KX_SRP_RSA, GNUTLS_PK_RSA, CIPHER_SIGN},
	{GNUTLS_KX_ECDHE_RSA, GNUTLS_PK_RSA, CIPHER_SIGN},
	{GNUTLS_KX_ECDHE_ECDSA, GNUTLS_PK_EC, CIPHER_SIGN},
	{GNUTLS_KX_ECDHE_ECDSA, GNUTLS_PK_EDDSA_ED25519, CIPHER_SIGN},
	{GNUTLS_KX_ECDHE_ECDSA, GNUTLS_PK_EDDSA_ED448, CIPHER_SIGN},
	{GNUTLS_KX_DHE_DSS, GNUTLS_PK_DSA, CIPHER_SIGN},
	{GNUTLS_KX_DHE_RSA, GNUTLS_PK_RSA_PSS, CIPHER_SIGN},
	{GNUTLS_KX_ECDHE_RSA, GNUTLS_PK_RSA_PSS, CIPHER_SIGN},
	{GNUTLS_KX_SRP_DSS, GNUTLS_PK_DSA, CIPHER_SIGN},
	{GNUTLS_KX_RSA_PSK, GNUTLS_PK_RSA, CIPHER_ENCRYPT},
	{GNUTLS_KX_VKO_GOST_12, GNUTLS_PK_GOST_01, CIPHER_SIGN},
	{GNUTLS_KX_VKO_GOST_12, GNUTLS_PK_GOST_12_256, CIPHER_SIGN},
	{GNUTLS_KX_VKO_GOST_12, GNUTLS_PK_GOST_12_512, CIPHER_SIGN},
	{0, 0, 0}
};

#define GNUTLS_PK_MAP_LOOP(b) \
	const gnutls_pk_map *p; \
		for(p = pk_mappings; p->kx_algorithm != 0; p++) { b }

#define GNUTLS_PK_MAP_ALG_LOOP(a) \
			GNUTLS_PK_MAP_LOOP( if(p->kx_algorithm == kx_algorithm) { a; break; })


unsigned
_gnutls_kx_supports_pk(gnutls_kx_algorithm_t kx_algorithm,
		       gnutls_pk_algorithm_t pk_algorithm)
{
	GNUTLS_PK_MAP_LOOP(if (p->kx_algorithm == kx_algorithm && p->pk_algorithm == pk_algorithm) { return 1; })
	return 0;
}

unsigned
_gnutls_kx_supports_pk_usage(gnutls_kx_algorithm_t kx_algorithm,
			     gnutls_pk_algorithm_t pk_algorithm,
			     unsigned int key_usage)
{
	const gnutls_pk_map *p;

	for(p = pk_mappings; p->kx_algorithm != 0; p++) {
		if (p->kx_algorithm == kx_algorithm && p->pk_algorithm == pk_algorithm) {
			if (key_usage == 0)
				return 1;
			else if (p->encipher_type == CIPHER_SIGN && (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE))
				return 1;
			else if (p->encipher_type == CIPHER_ENCRYPT && (key_usage & GNUTLS_KEY_KEY_ENCIPHERMENT))
				return 1;
			else
				return 0;
		}
	}

	return 0;
}

/* pk algorithms;
 */
struct gnutls_pk_entry {
	const char *name;
	const char *oid;
	gnutls_pk_algorithm_t id;
	gnutls_ecc_curve_t curve; /* to map PK to specific OID, we need to know the curve for EdDSA */
	bool no_prehashed; /* non-zero if the algorithm cannot sign pre-hashed data */
};
typedef struct gnutls_pk_entry gnutls_pk_entry;

static const gnutls_pk_entry pk_algorithms[] = {
	/* having duplicate entries is ok, as long as the one
	 * we want to return OID from is first */
	{ .name = "RSA", .oid = PK_PKIX1_RSA_OID, .id = GNUTLS_PK_RSA,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "RSA-PSS", .oid = PK_PKIX1_RSA_PSS_OID, .id = GNUTLS_PK_RSA_PSS,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "RSA (X.509)", .oid = PK_X509_RSA_OID, .id = GNUTLS_PK_RSA,
	   .curve = GNUTLS_ECC_CURVE_INVALID },	/* some certificates use this OID for RSA */
	{ .name = "RSA-MD5", .oid = SIG_RSA_MD5_OID, .id = GNUTLS_PK_RSA,
	   .curve = GNUTLS_ECC_CURVE_INVALID },	/* some other broken certificates set RSA with MD5 as an indicator of RSA */
	{ .name = "RSA-SHA1", .oid = SIG_RSA_SHA1_OID, .id = GNUTLS_PK_RSA,
	   .curve = GNUTLS_ECC_CURVE_INVALID },	/* some other broken certificates set RSA with SHA1 as an indicator of RSA */
	{ .name = "RSA-SHA1", .oid = ISO_SIG_RSA_SHA1_OID, .id = GNUTLS_PK_RSA,
	   .curve = GNUTLS_ECC_CURVE_INVALID },	/* some other broken certificates set RSA with SHA1 as an indicator of RSA */
	{ .name = "DSA", .oid = PK_DSA_OID, .id = GNUTLS_PK_DSA,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "GOST R 34.10-2012-512", .oid = PK_GOST_R3410_2012_512_OID, .id = GNUTLS_PK_GOST_12_512,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "GOST R 34.10-2012-256", .oid = PK_GOST_R3410_2012_256_OID, .id = GNUTLS_PK_GOST_12_256,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "GOST R 34.10-2001", .oid = PK_GOST_R3410_2001_OID, .id = GNUTLS_PK_GOST_01,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "GOST R 34.10-94", .oid = PK_GOST_R3410_94_OID, .id = GNUTLS_PK_UNKNOWN,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "EC/ECDSA", .oid = "1.2.840.10045.2.1", .id = GNUTLS_PK_ECDSA,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "EdDSA (Ed25519)", .oid = SIG_EDDSA_SHA512_OID, .id = GNUTLS_PK_EDDSA_ED25519, 
	  .curve = GNUTLS_ECC_CURVE_ED25519, .no_prehashed = 1 },
	{ .name = "EdDSA (Ed448)", .oid = SIG_ED448_OID, .id = GNUTLS_PK_EDDSA_ED448,
	  .curve = GNUTLS_ECC_CURVE_ED448, .no_prehashed = 1 },
	{ .name = "DH", .oid = NULL, .id = GNUTLS_PK_DH,
	   .curve = GNUTLS_ECC_CURVE_INVALID },
	{ .name = "ECDH (X25519)", .oid = "1.3.101.110", .id = GNUTLS_PK_ECDH_X25519,
	  .curve = GNUTLS_ECC_CURVE_X25519 },
	{ .name = "ECDH (X448)", .oid = "1.3.101.111", .id = GNUTLS_PK_ECDH_X448,
	  .curve = GNUTLS_ECC_CURVE_X448 },
	{ .name = "UNKNOWN", .oid = NULL, .id = GNUTLS_PK_UNKNOWN, 
	  .curve = GNUTLS_ECC_CURVE_INVALID },
	{0, 0, 0, 0}
};

#define GNUTLS_PK_LOOP(b) \
	{ const gnutls_pk_entry *p; \
		for(p = pk_algorithms; p->name != NULL; p++) { b ; } }


/**
 * gnutls_pk_algorithm_get_name:
 * @algorithm: is a pk algorithm
 *
 * Convert a #gnutls_pk_algorithm_t value to a string.
 *
 * Returns: a string that contains the name of the specified public
 *   key algorithm, or %NULL.
 **/
const char *gnutls_pk_algorithm_get_name(gnutls_pk_algorithm_t algorithm)
{
	const char *ret = NULL;

	GNUTLS_PK_LOOP(
		if (p->id == algorithm) {
			ret = p->name;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_pk_list:
 *
 * Get a list of supported public key algorithms.
 *
 * This function is not thread safe.
 *
 * Returns: a (0)-terminated list of #gnutls_pk_algorithm_t integers
 *   indicating the available ciphers.
 *
 * Since: 2.6.0
 **/
const gnutls_pk_algorithm_t *gnutls_pk_list(void)
{
	static gnutls_pk_algorithm_t supported_pks[MAX_ALGOS] = { 0 };

	if (supported_pks[0] == 0) {
		int i = 0;

		GNUTLS_PK_LOOP(
			if (p->id != GNUTLS_PK_UNKNOWN && supported_pks[i > 0 ? (i - 1) : 0] != p->id)
				supported_pks[i++] = p->id
		);
		supported_pks[i++] = 0;
	}

	return supported_pks;
}

/**
 * gnutls_pk_get_id:
 * @name: is a string containing a public key algorithm name.
 *
 * Convert a string to a #gnutls_pk_algorithm_t value.  The names are
 * compared in a case insensitive way.  For example,
 * gnutls_pk_get_id("RSA") will return %GNUTLS_PK_RSA.
 *
 * Returns: a #gnutls_pk_algorithm_t id of the specified public key
 *   algorithm string, or %GNUTLS_PK_UNKNOWN on failures.
 *
 * Since: 2.6.0
 **/
gnutls_pk_algorithm_t gnutls_pk_get_id(const char *name)
{
	gnutls_pk_algorithm_t ret = GNUTLS_PK_UNKNOWN;
	const gnutls_pk_entry *p;

	for (p = pk_algorithms; p->name != NULL; p++)
		if (name && strcmp(p->name, name) == 0) {
			ret = p->id;
			break;
		}

	return ret;
}

/**
 * gnutls_pk_get_name:
 * @algorithm: is a public key algorithm
 *
 * Convert a #gnutls_pk_algorithm_t value to a string.
 *
 * Returns: a pointer to a string that contains the name of the
 *   specified public key algorithm, or %NULL.
 *
 * Since: 2.6.0
 **/
const char *gnutls_pk_get_name(gnutls_pk_algorithm_t algorithm)
{
	const char *ret = "Unknown";
	const gnutls_pk_entry *p;

	for (p = pk_algorithms; p->name != NULL; p++)
		if (algorithm == p->id) {
			ret = p->name;
			break;
		}

	return ret;
}

/*-
 * _gnutls_pk_is_not_prehashed:
 * @algorithm: is a public key algorithm
 *
 * Returns non-zero when the public key algorithm does not support pre-hashed
 * data.
 *
 * Since: 3.6.0
 **/
bool _gnutls_pk_is_not_prehashed(gnutls_pk_algorithm_t algorithm)
{
	const gnutls_pk_entry *p;

	for (p = pk_algorithms; p->name != NULL; p++)
		if (algorithm == p->id) {
			return p->no_prehashed;
		}

	return 0;
}

/**
 * gnutls_oid_to_pk:
 * @oid: is an object identifier
 *
 * Converts a textual object identifier to a #gnutls_pk_algorithm_t value.
 *
 * Returns: a #gnutls_pk_algorithm_t id of the specified digest
 *   algorithm, or %GNUTLS_PK_UNKNOWN on failure.
 *
 * Since: 3.4.3
 **/
gnutls_pk_algorithm_t gnutls_oid_to_pk(const char *oid)
{
	gnutls_pk_algorithm_t ret = GNUTLS_PK_UNKNOWN;
	const gnutls_pk_entry *p;

	for (p = pk_algorithms; p->name != NULL; p++)
		if (p->oid && strcmp(p->oid, oid) == 0) {
			ret = p->id;
			break;
		}

	return ret;
}

/**
 * gnutls_pk_get_oid:
 * @algorithm: is a public key algorithm
 *
 * Convert a #gnutls_pk_algorithm_t value to its object identifier string.
 *
 * Returns: a pointer to a string that contains the object identifier of the
 *   specified public key algorithm, or %NULL.
 *
 * Since: 3.4.3
 **/
const char *gnutls_pk_get_oid(gnutls_pk_algorithm_t algorithm)
{
	const char *ret = NULL;
	const gnutls_pk_entry *p;

	if (algorithm == 0)
		return NULL;

	for (p = pk_algorithms; p->name != NULL; p++)
		if (p->id == algorithm) {
			ret = p->oid;
			break;
		}

	return ret;
}

/*-
 * _gnutls_oid_to_pk_and_curve:
 * @oid: is an object identifier
 *
 * Convert an OID to a #gnutls_pk_algorithm_t and curve values. If no curve
 * is applicable, curve will be set GNUTLS_ECC_CURVE_INVALID.
 *
 * Returns: a #gnutls_pk_algorithm_t id of the specified digest
 *   algorithm, or %GNUTLS_PK_UNKNOWN on failure.
 *
 * Since: 3.6.0
 -*/
gnutls_pk_algorithm_t _gnutls_oid_to_pk_and_curve(const char *oid, gnutls_ecc_curve_t *curve)
{
	gnutls_pk_algorithm_t ret = GNUTLS_PK_UNKNOWN;
	const gnutls_pk_entry *p;

	for (p = pk_algorithms; p->name != NULL; p++)
		if (p->oid && strcmp(p->oid, oid) == 0) {
			ret = p->id;
			if (curve)
				*curve = p->curve;
			break;
		}

	if (ret == GNUTLS_PK_UNKNOWN && curve)
		*curve = GNUTLS_ECC_CURVE_INVALID;

	return ret;
}

/* Returns the encipher type for the given key exchange algorithm.
 * That one of CIPHER_ENCRYPT, CIPHER_SIGN, CIPHER_IGN.
 *
 * ex. GNUTLS_KX_RSA requires a certificate able to encrypt... so returns CIPHER_ENCRYPT.
 */
enum encipher_type
_gnutls_kx_encipher_type(gnutls_kx_algorithm_t kx_algorithm)
{
	int ret = CIPHER_IGN;
	GNUTLS_PK_MAP_ALG_LOOP(ret = p->encipher_type)

	return ret;

}

bool _gnutls_pk_are_compat(gnutls_pk_algorithm_t pk1, gnutls_pk_algorithm_t pk2)
{
	if (pk1 == pk2)
		return 1;

	if (GNUTLS_PK_IS_RSA(pk1) && GNUTLS_PK_IS_RSA(pk2))
		return 1;

	return 0;
}
