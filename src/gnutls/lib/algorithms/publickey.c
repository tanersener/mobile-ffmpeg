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
	{GNUTLS_KX_DHE_DSS, GNUTLS_PK_DSA, CIPHER_SIGN},
	{GNUTLS_KX_SRP_DSS, GNUTLS_PK_DSA, CIPHER_SIGN},
	{GNUTLS_KX_RSA_PSK, GNUTLS_PK_RSA, CIPHER_ENCRYPT},
	{0, 0, 0}
};

#define GNUTLS_PK_MAP_LOOP(b) \
	const gnutls_pk_map *p; \
		for(p = pk_mappings; p->kx_algorithm != 0; p++) { b }

#define GNUTLS_PK_MAP_ALG_LOOP(a) \
			GNUTLS_PK_MAP_LOOP( if(p->kx_algorithm == kx_algorithm) { a; break; })


/* returns the gnutls_pk_algorithm_t which is compatible with
 * the given gnutls_kx_algorithm_t.
 */
gnutls_pk_algorithm_t
_gnutls_map_kx_get_pk(gnutls_kx_algorithm_t kx_algorithm)
{
	gnutls_pk_algorithm_t ret = -1;

	GNUTLS_PK_MAP_ALG_LOOP(ret = p->pk_algorithm) return ret;
}

/* pk algorithms;
 */
struct gnutls_pk_entry {
	const char *name;
	const char *oid;
	gnutls_pk_algorithm_t id;
};
typedef struct gnutls_pk_entry gnutls_pk_entry;

static const gnutls_pk_entry pk_algorithms[] = {
	/* having duplicate entries is ok, as long as the one
	 * we want to return OID from is first */
	{"UNKNOWN", NULL, GNUTLS_PK_UNKNOWN},
	{"RSA", PK_PKIX1_RSA_OID, GNUTLS_PK_RSA},
	{"RSA (X.509)", PK_X509_RSA_OID, GNUTLS_PK_RSA},	/* some certificates use this OID for RSA */
	{"RSA-MD5", SIG_RSA_MD5_OID, GNUTLS_PK_RSA},	/* some other broken certificates set RSA with MD5 as an indicator of RSA */
	{"RSA-SHA1", SIG_RSA_SHA1_OID, GNUTLS_PK_RSA},	/* some other broken certificates set RSA with SHA1 as an indicator of RSA */
	{"RSA-SHA1", ISO_SIG_RSA_SHA1_OID, GNUTLS_PK_RSA},	/* some other broken certificates set RSA with SHA1 as an indicator of RSA */
	{"DSA", PK_DSA_OID, GNUTLS_PK_DSA},
	{"GOST R 34.10-2001", PK_GOST_R3410_2001_OID, GNUTLS_PK_UNKNOWN},
	{"GOST R 34.10-94", PK_GOST_R3410_94_OID, GNUTLS_PK_UNKNOWN},
	{"EC/ECDSA", "1.2.840.10045.2.1", GNUTLS_PK_ECDSA},
	{"DH", NULL, GNUTLS_PK_DH},
	{"ECDHX", NULL, GNUTLS_PK_ECDHX},
	{0, 0, 0}
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

	for (p = pk_algorithms; p->name != NULL; p++)
		if (p->id == algorithm) {
			ret = p->oid;
			break;
		}

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
