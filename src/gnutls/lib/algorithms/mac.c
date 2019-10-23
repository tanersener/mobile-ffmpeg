/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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
#include "c-strcase.h"

#define MAC_OID_SHA1 "1.2.840.113549.2.7"
#define MAC_OID_SHA224 "1.2.840.113549.2.8"
#define MAC_OID_SHA256 "1.2.840.113549.2.9"
#define MAC_OID_SHA384 "1.2.840.113549.2.10"
#define MAC_OID_SHA512 "1.2.840.113549.2.11"
#define MAC_OID_GOST_R_3411_94 "1.2.643.2.2.10"
#define MAC_OID_STREEBOG_256 "1.2.643.7.1.1.4.1"
#define MAC_OID_STREEBOG_512 "1.2.643.7.1.1.4.2"

static SYSTEM_CONFIG_OR_CONST
mac_entry_st hash_algorithms[] = {
	{.name = "SHA1",
	 .oid = HASH_OID_SHA1,
	 .mac_oid = MAC_OID_SHA1,
	 .id = GNUTLS_MAC_SHA1,
	 .output_size = 20,
	 .key_size = 20,
	 .block_size = 64},
	{.name = "MD5+SHA1",
	 .id = GNUTLS_MAC_MD5_SHA1,
	 .output_size = 36,
	 .key_size = 36,
	 .preimage_insecure = 1,
	 .block_size = 64},
	{.name = "SHA256",
	 .oid = HASH_OID_SHA256,
	 .mac_oid = MAC_OID_SHA256,
	 .id = GNUTLS_MAC_SHA256,
	 .output_size = 32,
	 .key_size = 32,
	 .block_size = 64},
	{.name = "SHA384",
	 .oid = HASH_OID_SHA384,
	 .mac_oid = MAC_OID_SHA384,
	 .id = GNUTLS_MAC_SHA384,
	 .output_size = 48,
	 .key_size = 48,
	 .block_size = 128},
	{.name = "SHA512",
	 .oid = HASH_OID_SHA512,
	 .mac_oid = MAC_OID_SHA512,
	 .id = GNUTLS_MAC_SHA512,
	 .output_size = 64,
	 .key_size = 64,
	 .block_size = 128},
	{.name = "SHA224",
	 .oid = HASH_OID_SHA224,
	 .mac_oid = MAC_OID_SHA224,
	 .id = GNUTLS_MAC_SHA224,
	 .output_size = 28,
	 .key_size = 28,
	 .block_size = 64},
	{.name = "SHA3-256",
	 .oid = HASH_OID_SHA3_256,
	 .id = GNUTLS_MAC_SHA3_256,
	 .output_size = 32,
	 .key_size = 32,
	 .block_size = 136},
	{.name = "SHA3-384",
	 .oid = HASH_OID_SHA3_384,
	 .id = GNUTLS_MAC_SHA3_384,
	 .output_size = 48,
	 .key_size = 48,
	 .block_size = 104},
	{.name = "SHA3-512",
	 .oid = HASH_OID_SHA3_512,
	 .id = GNUTLS_MAC_SHA3_512,
	 .output_size = 64,
	 .key_size = 64,
	 .block_size = 72},
	{.name = "SHA3-224",
	 .oid = HASH_OID_SHA3_224,
	 .id = GNUTLS_MAC_SHA3_224,
	 .output_size = 28,
	 .key_size = 28,
	 .block_size = 144},
	{.name = "UMAC-96",
	 .id = GNUTLS_MAC_UMAC_96,
	 .output_size = 12,
	 .key_size = 16,
	 .nonce_size = 8},
	{.name = "UMAC-128",
	 .id = GNUTLS_MAC_UMAC_128,
	 .output_size = 16,
	 .key_size = 16,
	 .nonce_size = 8},
	{.name = "AEAD",
	 .id = GNUTLS_MAC_AEAD,
	 .placeholder = 1},
	{.name = "MD5",
	 .oid = HASH_OID_MD5,
	 .id = GNUTLS_MAC_MD5,
	 .output_size = 16,
	 .key_size = 16,
	 .preimage_insecure = 1,
	 .block_size = 64},
	{.name = "MD2",
	 .oid = HASH_OID_MD2,
	 .preimage_insecure = 1,
	 .id = GNUTLS_MAC_MD2},
	{.name = "RIPEMD160",
	 .oid = HASH_OID_RMD160,
	 .id = GNUTLS_MAC_RMD160,
	 .output_size = 20,
	 .key_size = 20,
	 .block_size = 64},
	{.name = "GOSTR341194",
	 .oid = HASH_OID_GOST_R_3411_94,
	 .mac_oid = MAC_OID_GOST_R_3411_94,
	 .id = GNUTLS_MAC_GOSTR_94,
	 .output_size = 32,
	 .key_size = 32,
	 .block_size = 32},
	{.name = "STREEBOG-256",
	 .oid = HASH_OID_STREEBOG_256,
	 .mac_oid = MAC_OID_STREEBOG_256,
	 .id = GNUTLS_MAC_STREEBOG_256,
	 .output_size = 32,
	 .key_size = 32,
	 .block_size = 64},
	{.name = "STREEBOG-512",
	 .oid = HASH_OID_STREEBOG_512,
	 .mac_oid = MAC_OID_STREEBOG_512,
	 .id = GNUTLS_MAC_STREEBOG_512,
	 .output_size = 64,
	 .key_size = 64,
	 .block_size = 64},
	{.name = "AES-CMAC-128",
	 .id = GNUTLS_MAC_AES_CMAC_128,
	 .output_size = 16,
	 .key_size = 16,},
	{.name = "AES-CMAC-256",
	 .id = GNUTLS_MAC_AES_CMAC_256,
	 .output_size = 16,
	 .key_size = 32},
	{.name = "AES-GMAC-128",
	 .id = GNUTLS_MAC_AES_GMAC_128,
	 .output_size = 16,
	 .key_size = 16,
	 .nonce_size = 12},
	{.name = "AES-GMAC-192",
	 .id = GNUTLS_MAC_AES_GMAC_192,
	 .output_size = 16,
	 .key_size = 24,
	 .nonce_size = 12},
	{.name = "AES-GMAC-256",
	 .id = GNUTLS_MAC_AES_GMAC_256,
	 .output_size = 16,
	 .key_size = 32,
	 .nonce_size = 12},
	{.name = "GOST28147-TC26Z-IMIT",
	 .id = GNUTLS_MAC_GOST28147_TC26Z_IMIT,
	 .output_size = 4,
	 .key_size = 32,
	 .block_size = 8},
	{.name = "MAC-NULL",
	 .id = GNUTLS_MAC_NULL},
	{0, 0, 0, 0, 0, 0, 0, 0, 0}
};


#define GNUTLS_HASH_LOOP(b) \
	const mac_entry_st *p; \
		for(p = hash_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_HASH_ALG_LOOP(a) \
			GNUTLS_HASH_LOOP( if(p->id == algorithm) { a; break; } )

const mac_entry_st *_gnutls_mac_to_entry(gnutls_mac_algorithm_t c)
{
	GNUTLS_HASH_LOOP(if (c == p->id) return p);

	return NULL;
}

/**
 * gnutls_mac_get_name:
 * @algorithm: is a MAC algorithm
 *
 * Convert a #gnutls_mac_algorithm_t value to a string.
 *
 * Returns: a string that contains the name of the specified MAC
 *   algorithm, or %NULL.
 **/
const char *gnutls_mac_get_name(gnutls_mac_algorithm_t algorithm)
{
	const char *ret = NULL;

	/* avoid prefix */
	GNUTLS_HASH_ALG_LOOP(ret = p->name);

	return ret;
}

/**
 * gnutls_digest_get_name:
 * @algorithm: is a digest algorithm
 *
 * Convert a #gnutls_digest_algorithm_t value to a string.
 *
 * Returns: a string that contains the name of the specified digest
 *   algorithm, or %NULL.
 **/
const char *gnutls_digest_get_name(gnutls_digest_algorithm_t algorithm)
{
	const char *ret = NULL;

	GNUTLS_HASH_LOOP(
		if (algorithm == (unsigned) p->id && p->oid != NULL) {
			ret = p->name;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_digest_get_id:
 * @name: is a digest algorithm name
 *
 * Convert a string to a #gnutls_digest_algorithm_t value.  The names are
 * compared in a case insensitive way.
 *
 * Returns: a #gnutls_digest_algorithm_t id of the specified MAC
 *   algorithm string, or %GNUTLS_DIG_UNKNOWN on failure.
 **/
gnutls_digest_algorithm_t gnutls_digest_get_id(const char *name)
{
	gnutls_digest_algorithm_t ret = GNUTLS_DIG_UNKNOWN;

	GNUTLS_HASH_LOOP(
		if (p->oid != NULL && c_strcasecmp(p->name, name) == 0) {
			if (_gnutls_digest_exists((gnutls_digest_algorithm_t)p->id))
				ret = (gnutls_digest_algorithm_t)p->id;
			break;
		}
	);

	return ret;
}

int _gnutls_digest_mark_insecure(const char *name)
{
#ifndef DISABLE_SYSTEM_CONFIG
	mac_entry_st *p;

	for(p = hash_algorithms; p->name != NULL; p++) {
		if (p->oid != NULL && c_strcasecmp(p->name, name) == 0) {
			p->preimage_insecure = 1;
			return 0;
		}
	}

#endif
	return GNUTLS_E_INVALID_REQUEST;
}

unsigned _gnutls_digest_is_insecure(gnutls_digest_algorithm_t dig)
{
	const mac_entry_st *p;

	for(p = hash_algorithms; p->name != NULL; p++) {
		if (p->oid != NULL && p->id == (gnutls_mac_algorithm_t)dig) {
			return p->preimage_insecure;
		}
	}

	return 1;
}

/**
 * gnutls_mac_get_id:
 * @name: is a MAC algorithm name
 *
 * Convert a string to a #gnutls_mac_algorithm_t value.  The names are
 * compared in a case insensitive way.
 *
 * Returns: a #gnutls_mac_algorithm_t id of the specified MAC
 *   algorithm string, or %GNUTLS_MAC_UNKNOWN on failure.
 **/
gnutls_mac_algorithm_t gnutls_mac_get_id(const char *name)
{
	gnutls_mac_algorithm_t ret = GNUTLS_MAC_UNKNOWN;

	GNUTLS_HASH_LOOP(
		if (c_strcasecmp(p->name, name) == 0) {
			if (p->placeholder != 0 || _gnutls_mac_exists(p->id))
				ret = p->id;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_mac_get_key_size:
 * @algorithm: is an encryption algorithm
 *
 * Returns the size of the MAC key used in TLS.
 *
 * Returns: length (in bytes) of the given MAC key size, or 0 if the
 *   given MAC algorithm is invalid.
 **/
size_t gnutls_mac_get_key_size(gnutls_mac_algorithm_t algorithm)
{
	size_t ret = 0;

	/* avoid prefix */
	GNUTLS_HASH_ALG_LOOP(ret = p->key_size);

	return ret;
}

/**
 * gnutls_mac_get_nonce_size:
 * @algorithm: is an encryption algorithm
 *
 * Returns the size of the nonce used by the MAC in TLS.
 *
 * Returns: length (in bytes) of the given MAC nonce size, or 0.
 *
 * Since: 3.2.0
 **/
size_t gnutls_mac_get_nonce_size(gnutls_mac_algorithm_t algorithm)
{
	size_t ret = 0;

	/* avoid prefix */
	GNUTLS_HASH_ALG_LOOP(ret = p->nonce_size);

	return ret;
}

/**
 * gnutls_mac_list:
 *
 * Get a list of hash algorithms for use as MACs.  Note that not
 * necessarily all MACs are supported in TLS cipher suites.
 * This function is not thread safe.
 *
 * Returns: Return a (0)-terminated list of #gnutls_mac_algorithm_t
 *   integers indicating the available MACs.
 **/
const gnutls_mac_algorithm_t *gnutls_mac_list(void)
{
	static gnutls_mac_algorithm_t supported_macs[MAX_ALGOS] = { 0 };

	if (supported_macs[0] == 0) {
		int i = 0;

		GNUTLS_HASH_LOOP(
			if (p->placeholder != 0 || _gnutls_mac_exists(p->id))
				 supported_macs[i++] = p->id;
		);
		supported_macs[i++] = 0;
	}

	return supported_macs;
}

/**
 * gnutls_digest_list:
 *
 * Get a list of hash (digest) algorithms supported by GnuTLS.
 *
 * This function is not thread safe.
 *
 * Returns: Return a (0)-terminated list of #gnutls_digest_algorithm_t
 *   integers indicating the available digests.
 **/
const gnutls_digest_algorithm_t *gnutls_digest_list(void)
{
	static gnutls_digest_algorithm_t supported_digests[MAX_ALGOS] =
	    { 0 };

	if (supported_digests[0] == 0) {
		int i = 0;

		GNUTLS_HASH_LOOP(
			if (p->oid != NULL && (p->placeholder != 0 ||
				_gnutls_mac_exists(p->id))) {

				supported_digests[i++] = (gnutls_digest_algorithm_t)p->id;
			}
		);
		supported_digests[i++] = 0;
	}

	return supported_digests;
}

/**
 * gnutls_oid_to_digest:
 * @oid: is an object identifier
 *
 * Converts a textual object identifier to a #gnutls_digest_algorithm_t value.
 *
 * Returns: a #gnutls_digest_algorithm_t id of the specified digest
 *   algorithm, or %GNUTLS_DIG_UNKNOWN on failure.
 *
 * Since: 3.4.3
 **/
gnutls_digest_algorithm_t gnutls_oid_to_digest(const char *oid)
{
	GNUTLS_HASH_LOOP(
		if (p->oid && strcmp(oid, p->oid) == 0) {
			if (_gnutls_digest_exists((gnutls_digest_algorithm_t)p->id)) {
				return (gnutls_digest_algorithm_t) p->id;
			}
			break;
		}
	);

	return GNUTLS_DIG_UNKNOWN;
}

/**
 * gnutls_oid_to_mac:
 * @oid: is an object identifier
 *
 * Converts a textual object identifier typically from PKCS#5 values to a #gnutls_mac_algorithm_t value.
 *
 * Returns: a #gnutls_mac_algorithm_t id of the specified digest
 *   algorithm, or %GNUTLS_MAC_UNKNOWN on failure.
 *
 * Since: 3.5.4
 **/
gnutls_mac_algorithm_t gnutls_oid_to_mac(const char *oid)
{
	GNUTLS_HASH_LOOP(
		if (p->mac_oid && strcmp(oid, p->mac_oid) == 0) {
			if (_gnutls_mac_exists(p->id)) {
				return p->id;
			}
			break;
		}
	);

	return GNUTLS_MAC_UNKNOWN;
}

/**
 * gnutls_digest_get_oid:
 * @algorithm: is a digest algorithm
 *
 * Convert a #gnutls_digest_algorithm_t value to its object identifier.
 *
 * Returns: a string that contains the object identifier of the specified digest
 *   algorithm, or %NULL.
 *
 * Since: 3.4.3
 **/
const char *gnutls_digest_get_oid(gnutls_digest_algorithm_t algorithm)
{
	GNUTLS_HASH_LOOP(
		if (algorithm == (unsigned) p->id && p->oid != NULL) {
			return p->oid;
		}
	);

	return NULL;
}

gnutls_digest_algorithm_t _gnutls_hash_size_to_sha_hash(unsigned int size)
{
	if (size == 20)
		return GNUTLS_DIG_SHA1;
	else if (size == 28)
		return GNUTLS_DIG_SHA224;
	else if (size == 32)
		return GNUTLS_DIG_SHA256;
	else if (size == 48)
		return GNUTLS_DIG_SHA384;
	else if (size == 64)
		return GNUTLS_DIG_SHA512;

	return GNUTLS_DIG_UNKNOWN;
}
