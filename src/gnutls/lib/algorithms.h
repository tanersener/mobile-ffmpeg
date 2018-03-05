/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "auth.h"

#define GNUTLS_RENEGO_PROTECTION_REQUEST_MAJOR 0x00
#define GNUTLS_RENEGO_PROTECTION_REQUEST_MINOR 0xFF

#define GNUTLS_FALLBACK_SCSV_MAJOR 0x56
#define GNUTLS_FALLBACK_SCSV_MINOR 0x00

/* would allow for 256 ciphersuites */
#define MAX_CIPHERSUITE_SIZE 512

#define IS_EC(x) (((x)==GNUTLS_PK_ECDSA)||((x)==GNUTLS_PK_ECDHX))

/* Functions for version handling. */
const version_entry_st *version_to_entry(gnutls_protocol_t c);
const version_entry_st *_gnutls_version_lowest(gnutls_session_t session);
gnutls_protocol_t _gnutls_version_max(gnutls_session_t session);
int _gnutls_version_priority(gnutls_session_t session,
			     gnutls_protocol_t version);
int _gnutls_version_is_supported(gnutls_session_t session,
				 const gnutls_protocol_t version);
gnutls_protocol_t _gnutls_version_get(uint8_t major, uint8_t minor);
unsigned _gnutls_version_is_too_high(gnutls_session_t session, uint8_t major, uint8_t minor);

/* Functions for feature checks */
inline static int
_gnutls_version_has_selectable_prf(const version_entry_st * ver)
{
	if (unlikely(ver == NULL))
		return 0;
	return ver->selectable_prf;
}

inline static int
_gnutls_version_has_selectable_sighash(const version_entry_st * ver)
{
	if (unlikely(ver == NULL))
		return 0;
	return ver->selectable_sighash;
}

inline static
int _gnutls_version_has_extensions(const version_entry_st * ver)
{
	if (unlikely(ver == NULL))
		return 0;
	return ver->extensions;
}

inline static
int _gnutls_version_has_explicit_iv(const version_entry_st * ver)
{
	if (unlikely(ver == NULL))
		return 0;
	return ver->explicit_iv;
}

/* Functions for MACs. */
const mac_entry_st *_gnutls_mac_to_entry(gnutls_mac_algorithm_t c);
#define mac_to_entry(x) _gnutls_mac_to_entry(x)
#define hash_to_entry(x) mac_to_entry((gnutls_mac_algorithm_t)(x))

inline static int _gnutls_mac_is_ok(const mac_entry_st * e)
{
	if (unlikely(e == NULL) || e->id == 0)
		return 0;
	else
		return 1;
}

/*-
 * _gnutls_mac_get_algo_len:
 * @algorithm: is an encryption algorithm
 *
 * Get size of MAC key.
 *
 * Returns: length (in bytes) of the MAC output size, or 0 if the
 *   given MAC algorithm is invalid.
 -*/
inline static size_t _gnutls_mac_get_algo_len(const mac_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	else
		return e->output_size;
}

inline static const char *_gnutls_x509_mac_to_oid(const mac_entry_st * e)
{
	if (unlikely(e == NULL))
		return NULL;
	else
		return e->oid;
}

inline static const char *_gnutls_mac_get_name(const mac_entry_st * e)
{
	if (unlikely(e == NULL))
		return NULL;
	else
		return e->name;
}

inline static int _gnutls_mac_block_size(const mac_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	else
		return e->block_size;
}

inline static int _gnutls_mac_get_key_size(const mac_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	else
		return e->key_size;
}

/* Functions for digests. */
#define _gnutls_x509_digest_to_oid _gnutls_x509_mac_to_oid
#define _gnutls_digest_get_name _gnutls_mac_get_name
#define _gnutls_hash_get_algo_len _gnutls_mac_get_algo_len

inline static int _gnutls_digest_is_secure(const mac_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	else
		return e->secure;
}

/* Functions for cipher suites. */
int _gnutls_supported_ciphersuites(gnutls_session_t session,
				   uint8_t * cipher_suites,
				   unsigned int max_cipher_suite_size);
int
_gnutls_remove_unwanted_ciphersuites(gnutls_session_t session,
			     uint8_t * cipher_suites,
			     int cipher_suites_size,
			     gnutls_pk_algorithm_t * pk_algos,
			     size_t pk_algos_size);

const char *_gnutls_cipher_suite_get_name(const uint8_t suite[2]);
gnutls_mac_algorithm_t _gnutls_cipher_suite_get_prf(const uint8_t
						    suite[2]);
const cipher_entry_st *_gnutls_cipher_suite_get_cipher_algo(const uint8_t
							    suite[2]);
gnutls_kx_algorithm_t _gnutls_cipher_suite_get_kx_algo(const uint8_t
						       suite[2]);
const mac_entry_st *_gnutls_cipher_suite_get_mac_algo(const uint8_t
						      suite[2]);

int
_gnutls_cipher_suite_get_id(gnutls_kx_algorithm_t kx_algorithm,
			    gnutls_cipher_algorithm_t cipher_algorithm,
			    gnutls_mac_algorithm_t mac_algorithm,
			    uint8_t suite[2]);

const gnutls_cipher_suite_entry_st *ciphersuite_to_entry(const uint8_t suite[2]);

/* Functions for ciphers. */
const cipher_entry_st *cipher_to_entry(gnutls_cipher_algorithm_t c);
const cipher_entry_st *cipher_name_to_entry(const char *name);

inline static cipher_type_t _gnutls_cipher_type(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	return e->type;
}

inline static int _gnutls_cipher_get_block_size(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	return e->blocksize;
}

inline static int
_gnutls_cipher_get_implicit_iv_size(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	return e->implicit_iv;
}

inline static int
_gnutls_cipher_get_iv_size(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	return e->cipher_iv;
}

inline static int
_gnutls_cipher_get_explicit_iv_size(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	return e->explicit_iv;
}

inline static int _gnutls_cipher_get_key_size(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	return e->keysize;
}

inline static const char *_gnutls_cipher_get_name(const cipher_entry_st *
						  e)
{
	if (unlikely(e == NULL))
		return NULL;
	return e->name;
}

inline static int _gnutls_cipher_algo_is_aead(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	return (e->type == CIPHER_AEAD)?1:0;
}

inline static int _gnutls_cipher_is_ok(const cipher_entry_st * e)
{
	if (unlikely(e == NULL) || e->id == 0)
		return 0;
	else
		return 1;
}

inline static int _gnutls_cipher_get_tag_size(const cipher_entry_st * e)
{
	size_t ret = 0;

	if (unlikely(e == NULL))
		return ret;

	/* non-AEAD have 0 as tag size */
	return e->tagsize;
}

/* Functions for key exchange. */
bool _gnutls_kx_needs_dh_params(gnutls_kx_algorithm_t algorithm);
bool _gnutls_kx_allows_false_start(gnutls_session_t session);
int _gnutls_kx_cert_pk_params(gnutls_kx_algorithm_t algorithm);
mod_auth_st *_gnutls_kx_auth_struct(gnutls_kx_algorithm_t algorithm);
int _gnutls_kx_is_ok(gnutls_kx_algorithm_t algorithm);

int _gnutls_kx_get_id(const char *name);

/* Type to KX mappings. */
gnutls_kx_algorithm_t _gnutls_map_kx_get_kx(gnutls_credentials_type_t type,
					    int server);
gnutls_credentials_type_t _gnutls_map_kx_get_cred(gnutls_kx_algorithm_t
						  algorithm, int server);

/* KX to PK mapping. */

/* DSA + RSA + ECC */
#define GNUTLS_DISTINCT_PK_ALGORITHMS 3
gnutls_pk_algorithm_t _gnutls_map_kx_get_pk(gnutls_kx_algorithm_t
					    kx_algorithm);

enum encipher_type { CIPHER_ENCRYPT = 0, CIPHER_SIGN = 1, CIPHER_IGN };

enum encipher_type _gnutls_kx_encipher_type(gnutls_kx_algorithm_t
					    algorithm);

/* Functions for sign algorithms. */
gnutls_pk_algorithm_t _gnutls_x509_sign_to_pk(gnutls_sign_algorithm_t
					      sign);
const char *_gnutls_x509_sign_to_oid(gnutls_pk_algorithm_t,
				     gnutls_digest_algorithm_t mac);
gnutls_sign_algorithm_t _gnutls_tls_aid_to_sign(const sign_algorithm_st *
						aid);
const sign_algorithm_st *_gnutls_sign_to_tls_aid(gnutls_sign_algorithm_t
						 sign);

int _gnutls_mac_priority(gnutls_session_t session,
			 gnutls_mac_algorithm_t algorithm);
int _gnutls_cipher_priority(gnutls_session_t session,
			    gnutls_cipher_algorithm_t a);
int _gnutls_kx_priority(gnutls_session_t session,
			gnutls_kx_algorithm_t algorithm);

unsigned int _gnutls_pk_bits_to_subgroup_bits(unsigned int pk_bits);

/* ECC */
struct gnutls_ecc_curve_entry_st {
	const char *name;
	const char *oid;
	gnutls_ecc_curve_t id;
	gnutls_pk_algorithm_t pk;
	int tls_id;		/* The RFC4492 namedCurve ID */
	int size;		/* the size in bytes */
};
typedef struct gnutls_ecc_curve_entry_st gnutls_ecc_curve_entry_st;

const gnutls_ecc_curve_entry_st
    *_gnutls_ecc_curve_get_params(gnutls_ecc_curve_t curve);
gnutls_ecc_curve_t gnutls_ecc_curve_get_id(const char *name);
int _gnutls_tls_id_to_ecc_curve(int num);
int _gnutls_ecc_curve_get_tls_id(gnutls_ecc_curve_t supported_ecc);
gnutls_ecc_curve_t _gnutls_ecc_bits_to_curve(int bits);
#define MAX_ECC_CURVE_SIZE 66

static inline int _gnutls_kx_is_ecc(gnutls_kx_algorithm_t kx)
{
	if (kx == GNUTLS_KX_ECDHE_RSA || kx == GNUTLS_KX_ECDHE_ECDSA ||
	    kx == GNUTLS_KX_ANON_ECDH || kx == GNUTLS_KX_ECDHE_PSK)
		return 1;

	return 0;
}

static inline int _sig_is_ecdsa(gnutls_sign_algorithm_t sig)
{
	if (sig == GNUTLS_SIGN_ECDSA_SHA1 || sig == GNUTLS_SIGN_ECDSA_SHA224 ||
	    sig == GNUTLS_SIGN_ECDSA_SHA256 || sig == GNUTLS_SIGN_ECDSA_SHA384 ||
	    sig == GNUTLS_SIGN_ECDSA_SHA512)
		return 1;

	return 0;
}

#endif
