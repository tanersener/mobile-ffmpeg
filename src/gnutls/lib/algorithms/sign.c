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

/* signature algorithms;
 */
struct gnutls_sign_entry {
	const char *name;
	const char *oid;
	gnutls_sign_algorithm_t id;
	gnutls_pk_algorithm_t pk;
	gnutls_digest_algorithm_t mac;
	/* See RFC 5246 HashAlgorithm and SignatureAlgorithm
	   for values to use in aid struct. */
	const sign_algorithm_st aid;
};
typedef struct gnutls_sign_entry gnutls_sign_entry;

#define TLS_SIGN_AID_UNKNOWN {255, 255}
static const sign_algorithm_st unknown_tls_aid = TLS_SIGN_AID_UNKNOWN;

static const gnutls_sign_entry sign_algorithms[] = {
	{"RSA-SHA1", SIG_RSA_SHA1_OID, GNUTLS_SIGN_RSA_SHA1, GNUTLS_PK_RSA,
	 GNUTLS_DIG_SHA1, {2, 1}},
	{"RSA-SHA1", ISO_SIG_RSA_SHA1_OID, GNUTLS_SIGN_RSA_SHA1,
	 GNUTLS_PK_RSA,
	 GNUTLS_DIG_SHA1, {2, 1}},
	{"RSA-SHA224", SIG_RSA_SHA224_OID, GNUTLS_SIGN_RSA_SHA224,
	 GNUTLS_PK_RSA,
	 GNUTLS_DIG_SHA224, {3, 1}},
	{"RSA-SHA256", SIG_RSA_SHA256_OID, GNUTLS_SIGN_RSA_SHA256,
	 GNUTLS_PK_RSA,
	 GNUTLS_DIG_SHA256, {4, 1}},
	{"RSA-SHA384", SIG_RSA_SHA384_OID, GNUTLS_SIGN_RSA_SHA384,
	 GNUTLS_PK_RSA,
	 GNUTLS_DIG_SHA384, {5, 1}},
	{"RSA-SHA512", SIG_RSA_SHA512_OID, GNUTLS_SIGN_RSA_SHA512,
	 GNUTLS_PK_RSA,
	 GNUTLS_DIG_SHA512, {6, 1}},
	{"RSA-RMD160", SIG_RSA_RMD160_OID, GNUTLS_SIGN_RSA_RMD160,
	 GNUTLS_PK_RSA,
	 GNUTLS_DIG_RMD160, TLS_SIGN_AID_UNKNOWN},
	{"DSA-SHA1", SIG_DSA_SHA1_OID, GNUTLS_SIGN_DSA_SHA1, GNUTLS_PK_DSA,
	 GNUTLS_DIG_SHA1, {2, 2}},
	{"DSA-SHA1", "1.3.14.3.2.27", GNUTLS_SIGN_DSA_SHA1, GNUTLS_PK_DSA,
	 GNUTLS_DIG_SHA1, {2, 2}},
	{"DSA-SHA224", SIG_DSA_SHA224_OID, GNUTLS_SIGN_DSA_SHA224,
	 GNUTLS_PK_DSA,
	 GNUTLS_DIG_SHA224, {3, 2}},
	{"DSA-SHA256", SIG_DSA_SHA256_OID, GNUTLS_SIGN_DSA_SHA256,
	 GNUTLS_PK_DSA, GNUTLS_DIG_SHA256, {4, 2}},
	{"RSA-MD5", SIG_RSA_MD5_OID, GNUTLS_SIGN_RSA_MD5, GNUTLS_PK_RSA,
	 GNUTLS_DIG_MD5, {1, 1}},
	{"RSA-MD5", "1.3.14.3.2.25", GNUTLS_SIGN_RSA_MD5, GNUTLS_PK_RSA,
	 GNUTLS_DIG_MD5, {1, 1}},
	{"RSA-MD2", SIG_RSA_MD2_OID, GNUTLS_SIGN_RSA_MD2, GNUTLS_PK_RSA,
	 GNUTLS_DIG_MD2, TLS_SIGN_AID_UNKNOWN},
	{"ECDSA-SHA1", "1.2.840.10045.4.1", GNUTLS_SIGN_ECDSA_SHA1,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA1, {2, 3}},
	{"ECDSA-SHA224", "1.2.840.10045.4.3.1", GNUTLS_SIGN_ECDSA_SHA224,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA224, {3, 3}},
	{"ECDSA-SHA256", "1.2.840.10045.4.3.2", GNUTLS_SIGN_ECDSA_SHA256,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA256, {4, 3}},
	{"ECDSA-SHA384", "1.2.840.10045.4.3.3", GNUTLS_SIGN_ECDSA_SHA384,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA384, {5, 3}},
	{"ECDSA-SHA512", "1.2.840.10045.4.3.4", GNUTLS_SIGN_ECDSA_SHA512,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA512, {6, 3}},
	{"GOST R 34.10-2001", SIG_GOST_R3410_2001_OID, 0, 0, 0,
	 TLS_SIGN_AID_UNKNOWN},
	{"GOST R 34.10-94", SIG_GOST_R3410_94_OID, 0, 0, 0,
	 TLS_SIGN_AID_UNKNOWN},
	{"DSA-SHA384", SIG_DSA_SHA384_OID, GNUTLS_SIGN_DSA_SHA384,
	 GNUTLS_PK_DSA, GNUTLS_DIG_SHA384, {5, 2}},
	{"DSA-SHA512", SIG_DSA_SHA512_OID, GNUTLS_SIGN_DSA_SHA512,
	 GNUTLS_PK_DSA, GNUTLS_DIG_SHA512, {6, 2}},

	{"ECDSA-SHA3-224", SIG_ECDSA_SHA3_224_OID, GNUTLS_SIGN_ECDSA_SHA3_224,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA3_224, TLS_SIGN_AID_UNKNOWN},
	{"ECDSA-SHA3-256", SIG_ECDSA_SHA3_256_OID, GNUTLS_SIGN_ECDSA_SHA3_256,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA3_256, TLS_SIGN_AID_UNKNOWN},
	{"ECDSA-SHA3-384", SIG_ECDSA_SHA3_384_OID, GNUTLS_SIGN_ECDSA_SHA3_384,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA3_384, TLS_SIGN_AID_UNKNOWN},
	{"ECDSA-SHA3-512", SIG_ECDSA_SHA3_512_OID, GNUTLS_SIGN_ECDSA_SHA3_512,
	 GNUTLS_PK_EC, GNUTLS_DIG_SHA3_512, TLS_SIGN_AID_UNKNOWN},

	{"RSA-SHA3-224", SIG_RSA_SHA3_224_OID, GNUTLS_SIGN_RSA_SHA3_224,
	 GNUTLS_PK_RSA, GNUTLS_DIG_SHA3_224, TLS_SIGN_AID_UNKNOWN},
	{"RSA-SHA3-256", SIG_RSA_SHA3_256_OID, GNUTLS_SIGN_RSA_SHA3_256,
	 GNUTLS_PK_RSA, GNUTLS_DIG_SHA3_256, TLS_SIGN_AID_UNKNOWN},
	{"RSA-SHA3-384", SIG_RSA_SHA3_384_OID, GNUTLS_SIGN_RSA_SHA3_384,
	 GNUTLS_PK_RSA, GNUTLS_DIG_SHA3_384, TLS_SIGN_AID_UNKNOWN},
	{"RSA-SHA3-512", SIG_RSA_SHA3_512_OID, GNUTLS_SIGN_RSA_SHA3_512,
	 GNUTLS_PK_RSA, GNUTLS_DIG_SHA3_512, TLS_SIGN_AID_UNKNOWN},

	{"DSA-SHA3-224", SIG_DSA_SHA3_224_OID, GNUTLS_SIGN_DSA_SHA3_224,
	 GNUTLS_PK_DSA, GNUTLS_DIG_SHA3_224, TLS_SIGN_AID_UNKNOWN},
	{"DSA-SHA3-256", SIG_DSA_SHA3_256_OID, GNUTLS_SIGN_DSA_SHA3_256,
	 GNUTLS_PK_DSA, GNUTLS_DIG_SHA3_256, TLS_SIGN_AID_UNKNOWN},
	{"DSA-SHA3-384", SIG_DSA_SHA3_384_OID, GNUTLS_SIGN_DSA_SHA3_384,
	 GNUTLS_PK_DSA, GNUTLS_DIG_SHA3_384, TLS_SIGN_AID_UNKNOWN},
	{"DSA-SHA3-512", SIG_DSA_SHA3_512_OID, GNUTLS_SIGN_DSA_SHA3_512,
	 GNUTLS_PK_DSA, GNUTLS_DIG_SHA3_512, TLS_SIGN_AID_UNKNOWN},
	{0, 0, 0, 0, 0, TLS_SIGN_AID_UNKNOWN}
};

#define GNUTLS_SIGN_LOOP(b) \
  do {								       \
    const gnutls_sign_entry *p;					       \
    for(p = sign_algorithms; p->name != NULL; p++) { b ; }	       \
  } while (0)

#define GNUTLS_SIGN_ALG_LOOP(a) \
  GNUTLS_SIGN_LOOP( if(p->id && p->id == sign) { a; break; } )

/**
 * gnutls_sign_get_name:
 * @algorithm: is a sign algorithm
 *
 * Convert a #gnutls_sign_algorithm_t value to a string.
 *
 * Returns: a string that contains the name of the specified sign
 *   algorithm, or %NULL.
 **/
const char *gnutls_sign_get_name(gnutls_sign_algorithm_t algorithm)
{
	gnutls_sign_algorithm_t sign = algorithm;
	const char *ret = NULL;

	/* avoid prefix */
	GNUTLS_SIGN_ALG_LOOP(ret = p->name);

	return ret;
}

/**
 * gnutls_sign_is_secure:
 * @algorithm: is a sign algorithm
 *
 * Returns: Non-zero if the provided signature algorithm is considered to be secure.
 **/
int gnutls_sign_is_secure(gnutls_sign_algorithm_t algorithm)
{
	gnutls_sign_algorithm_t sign = algorithm;
	gnutls_digest_algorithm_t dig = GNUTLS_DIG_UNKNOWN;

	/* avoid prefix */
	GNUTLS_SIGN_ALG_LOOP(dig = p->mac);

	if (dig != GNUTLS_DIG_UNKNOWN)
		return _gnutls_digest_is_secure(hash_to_entry(dig));

	return 0;
}

/**
 * gnutls_sign_list:
 *
 * Get a list of supported public key signature algorithms.
 *
 * Returns: a (0)-terminated list of #gnutls_sign_algorithm_t
 *   integers indicating the available ciphers.
 *
 **/
const gnutls_sign_algorithm_t *gnutls_sign_list(void)
{
	static gnutls_sign_algorithm_t supported_sign[MAX_ALGOS] = { 0 };

	if (supported_sign[0] == 0) {
		int i = 0;

		GNUTLS_SIGN_LOOP(supported_sign[i++] = p->id);
		supported_sign[i++] = 0;
	}

	return supported_sign;
}

/**
 * gnutls_sign_get_id:
 * @name: is a sign algorithm name
 *
 * The names are compared in a case insensitive way.
 *
 * Returns: return a #gnutls_sign_algorithm_t value corresponding to
 *   the specified algorithm, or %GNUTLS_SIGN_UNKNOWN on error.
 **/
gnutls_sign_algorithm_t gnutls_sign_get_id(const char *name)
{
	gnutls_sign_algorithm_t ret = GNUTLS_SIGN_UNKNOWN;

	GNUTLS_SIGN_LOOP(
		if (strcasecmp(p->name, name) == 0) {
			ret = p->id;
			break;
		}
	);

	return ret;

}

/**
 * gnutls_oid_to_sign:
 * @oid: is an object identifier
 *
 * Converts a textual object identifier to a #gnutls_sign_algorithm_t value.
 *
 * Returns: a #gnutls_sign_algorithm_t id of the specified digest
 *   algorithm, or %GNUTLS_SIGN_UNKNOWN on failure.
 *
 * Since: 3.4.3
 **/
gnutls_sign_algorithm_t gnutls_oid_to_sign(const char *oid)
{
	gnutls_sign_algorithm_t ret = 0;

	GNUTLS_SIGN_LOOP(
		if (p->oid && strcmp(oid, p->oid) == 0) {
			ret = p->id; break;}
	);

	if (ret == 0) {
		_gnutls_debug_log("Unknown SIGN OID: '%s'\n", oid);
		return GNUTLS_SIGN_UNKNOWN;
	}
	return ret;
}

/**
 * gnutls_pk_to_sign:
 * @pk: is a public key algorithm
 * @hash: a hash algorithm
 *
 * This function maps public key and hash algorithms combinations
 * to signature algorithms.
 *
 * Returns: return a #gnutls_sign_algorithm_t value, or %GNUTLS_SIGN_UNKNOWN on error.
 **/
gnutls_sign_algorithm_t
gnutls_pk_to_sign(gnutls_pk_algorithm_t pk, gnutls_digest_algorithm_t hash)
{
	gnutls_sign_algorithm_t ret = 0;

	GNUTLS_SIGN_LOOP(
		if (pk == p->pk && hash == p->mac) {
			ret = p->id;
			break;
		}
	);

	if (ret == 0)
		return GNUTLS_SIGN_UNKNOWN;
	return ret;
}

/**
 * gnutls_sign_get_oid:
 * @sign: is a sign algorithm
 *
 * Convert a #gnutls_sign_algorithm_t value to its object identifier.
 *
 * Returns: a string that contains the object identifier of the specified sign
 *   algorithm, or %NULL.
 *
 * Since: 3.4.3
 **/
const char *gnutls_sign_get_oid(gnutls_sign_algorithm_t sign)
{
	const char *ret = NULL;

	GNUTLS_SIGN_ALG_LOOP(ret = p->oid);

	return ret;
}

/**
 * gnutls_sign_get_hash_algorithm:
 * @sign: is a signature algorithm
 *
 * This function returns the digest algorithm corresponding to
 * the given signature algorithms.
 *
 * Since: 3.1.1
 *
 * Returns: return a #gnutls_digest_algorithm_t value, or %GNUTLS_DIG_UNKNOWN on error.
 **/
gnutls_digest_algorithm_t
gnutls_sign_get_hash_algorithm(gnutls_sign_algorithm_t sign)
{
	gnutls_digest_algorithm_t ret = GNUTLS_DIG_UNKNOWN;

	GNUTLS_SIGN_ALG_LOOP(ret = p->mac);

	return ret;
}

/**
 * gnutls_sign_get_pk_algorithm:
 * @sign: is a signature algorithm
 *
 * This function returns the public key algorithm corresponding to
 * the given signature algorithms.
 *
 * Since: 3.1.1
 *
 * Returns: return a #gnutls_pk_algorithm_t value, or %GNUTLS_PK_UNKNOWN on error.
 **/
gnutls_pk_algorithm_t
gnutls_sign_get_pk_algorithm(gnutls_sign_algorithm_t sign)
{
	gnutls_pk_algorithm_t ret = GNUTLS_PK_UNKNOWN;

	GNUTLS_SIGN_ALG_LOOP(ret = p->pk);

	return ret;
}

gnutls_sign_algorithm_t
_gnutls_tls_aid_to_sign(const sign_algorithm_st * aid)
{
	gnutls_sign_algorithm_t ret = GNUTLS_SIGN_UNKNOWN;

	if (aid->hash_algorithm == unknown_tls_aid.hash_algorithm &&
		aid->sign_algorithm == unknown_tls_aid.sign_algorithm)
		return ret;

	GNUTLS_SIGN_LOOP(
		if (p->aid.hash_algorithm == aid->hash_algorithm && 
			p->aid.sign_algorithm == aid->sign_algorithm) {

			ret = p->id;
			break;
		}
	);


	return ret;
}

/* Returns NULL if a valid AID is not found
 */
const sign_algorithm_st *_gnutls_sign_to_tls_aid(gnutls_sign_algorithm_t
						 sign)
{
	const sign_algorithm_st *ret = NULL;

	GNUTLS_SIGN_ALG_LOOP(ret = &p->aid);

	if (ret != NULL &&
	    ret->hash_algorithm == unknown_tls_aid.hash_algorithm &&
	    ret->sign_algorithm == unknown_tls_aid.sign_algorithm)
		return NULL;

	return ret;
}
