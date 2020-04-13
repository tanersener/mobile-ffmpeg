/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_ALGORITHMS_H
#define GNUTLS_LIB_ALGORITHMS_H

#include "auth.h"

#ifdef DISABLE_SYSTEM_CONFIG
# define SYSTEM_CONFIG_OR_CONST const
#else
# define SYSTEM_CONFIG_OR_CONST
#endif

#define version_to_entry _gnutls_version_to_entry

#define GNUTLS_RENEGO_PROTECTION_REQUEST_MAJOR 0x00
#define GNUTLS_RENEGO_PROTECTION_REQUEST_MINOR 0xFF

#define GNUTLS_FALLBACK_SCSV_MAJOR 0x56
#define GNUTLS_FALLBACK_SCSV_MINOR 0x00

#define IS_GOSTEC(x) (((x)==GNUTLS_PK_GOST_01)	|| \
		      ((x)==GNUTLS_PK_GOST_12_256)|| \
		      ((x)==GNUTLS_PK_GOST_12_512))

#define IS_EC(x) (((x)==GNUTLS_PK_ECDSA)|| \
		  ((x)==GNUTLS_PK_ECDH_X25519)||((x)==GNUTLS_PK_EDDSA_ED25519)|| \
		  ((x)==GNUTLS_PK_ECDH_X448)||((x)==GNUTLS_PK_EDDSA_ED448))

#define SIG_SEM_PRE_TLS12 (1<<1)
#define SIG_SEM_TLS13 (1<<2)
#define SIG_SEM_DEFAULT (SIG_SEM_PRE_TLS12|SIG_SEM_TLS13)

#define TLS_SIGN_AID_UNKNOWN {{255, 255}, 0}
#define HAVE_UNKNOWN_SIGAID(aid) ((aid)->id[0] == 255 && (aid)->id[1] == 255)

#define CS_INVALID_MAJOR 0x00
#define CS_INVALID_MINOR 0x00

/* Functions for version handling. */
const version_entry_st *version_to_entry(gnutls_protocol_t version);
const version_entry_st *nversion_to_entry(uint8_t major, uint8_t minor);
const version_entry_st *_gnutls_version_lowest(gnutls_session_t session);

const version_entry_st *_gnutls_legacy_version_max(gnutls_session_t session);
const version_entry_st *_gnutls_version_max(gnutls_session_t session);
int _gnutls_version_priority(gnutls_session_t session,
			     gnutls_protocol_t version);
int _gnutls_nversion_is_supported(gnutls_session_t session,
				  unsigned char major, unsigned char minor);
gnutls_protocol_t _gnutls_version_get(uint8_t major, uint8_t minor);
unsigned _gnutls_version_is_too_high(gnutls_session_t session, uint8_t major, uint8_t minor);

int _gnutls_write_supported_versions(gnutls_session_t session, uint8_t *buffer, ssize_t buffer_size);

/* Functions for feature checks */
int
_gnutls_figure_common_ciphersuite(gnutls_session_t session,
				  const ciphersuite_list_st *peer_clist,
				  const gnutls_cipher_suite_entry_st **ce);

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

/* Security against pre-image attacks */
inline static int _gnutls_digest_is_secure(const mac_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	else
		return !(e->flags & GNUTLS_MAC_FLAG_PREIMAGE_INSECURE);
}

/* Functions for cipher suites. */
int _gnutls_get_client_ciphersuites(gnutls_session_t session,
	gnutls_buffer_st * cdata, const version_entry_st *minver,
	unsigned add_scsv);

int _gnutls_supported_ciphersuites(gnutls_session_t session,
				   uint8_t * cipher_suites,
				   unsigned int max_cipher_suite_size);

const gnutls_cipher_suite_entry_st
    *cipher_suite_get(gnutls_kx_algorithm_t kx_algorithm,
		      gnutls_cipher_algorithm_t cipher_algorithm,
		      gnutls_mac_algorithm_t mac_algorithm);

const char *_gnutls_cipher_suite_get_name(const uint8_t suite[2]);
gnutls_kx_algorithm_t _gnutls_cipher_suite_get_kx_algo(const uint8_t
						       suite[2]);

int
_gnutls_cipher_suite_get_id(gnutls_kx_algorithm_t kx_algorithm,
			    gnutls_cipher_algorithm_t cipher_algorithm,
			    gnutls_mac_algorithm_t mac_algorithm,
			    uint8_t suite[2]);

const gnutls_cipher_suite_entry_st *ciphersuite_to_entry(const uint8_t suite[2]);

/* Functions for ciphers. */
const cipher_entry_st *_gnutls_cipher_to_entry(gnutls_cipher_algorithm_t c);
#define cipher_to_entry(x) _gnutls_cipher_to_entry(x)
const cipher_entry_st *cipher_name_to_entry(const char *name);

inline static cipher_type_t _gnutls_cipher_type(const cipher_entry_st * e)
{
	if (unlikely(e == NULL))
		return CIPHER_AEAD; /* doesn't matter */
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
mod_auth_st *_gnutls_kx_auth_struct(gnutls_kx_algorithm_t algorithm);
int _gnutls_kx_is_ok(gnutls_kx_algorithm_t algorithm);

int _gnutls_kx_get_id(const char *name);

gnutls_credentials_type_t _gnutls_map_kx_get_cred(gnutls_kx_algorithm_t
						  algorithm, int server);

/* KX to PK mapping. */
unsigned
_gnutls_kx_supports_pk(gnutls_kx_algorithm_t kx_algorithm,
		       gnutls_pk_algorithm_t pk_algorithm);

unsigned
_gnutls_kx_supports_pk_usage(gnutls_kx_algorithm_t kx_algorithm,
		       gnutls_pk_algorithm_t pk_algorithm,
		       unsigned key_usage);

enum encipher_type { CIPHER_ENCRYPT = 0, CIPHER_SIGN = 1, CIPHER_IGN };

enum encipher_type _gnutls_kx_encipher_type(gnutls_kx_algorithm_t
					    kx_algorithm);

/* Functions for sign algorithms. */

typedef enum hash_security_level_t {
	_SECURE,
	_INSECURE_FOR_CERTS,
	_INSECURE
} hash_security_level_t;

int _gnutls_ecc_curve_mark_disabled(const char *name);
int _gnutls_sign_mark_insecure(const char *name, hash_security_level_t);
int _gnutls_digest_mark_insecure(const char *name);
unsigned _gnutls_digest_is_insecure(gnutls_digest_algorithm_t dig);
int _gnutls_version_mark_disabled(const char *name);
gnutls_protocol_t _gnutls_protocol_get_id_if_supported(const char *name);

#define GNUTLS_SIGN_FLAG_TLS13_OK	1 /* if it is ok to use under TLS1.3 */
#define GNUTLS_SIGN_FLAG_CRT_VRFY_REVERSE (1 << 1) /* reverse order of bytes in CrtVrfy signature */
struct gnutls_sign_entry_st {
	const char *name;
	const char *oid;
	gnutls_sign_algorithm_t id;
	gnutls_pk_algorithm_t pk;
	gnutls_digest_algorithm_t hash;

	/* if non-zero it must be the algorithm of the
	 * private key used or certificate. This is for algorithms
	 * which can have a different public key type than the
	 * private key (e.g., RSA PKCS#1 1.5 certificate, but
	 * an RSA-PSS private key, or an RSA private key and
	 * an RSA-PSS certificate). */
	gnutls_pk_algorithm_t priv_pk;
	gnutls_pk_algorithm_t cert_pk;

	unsigned flags;

	/* if this signature algorithm is restricted to a curve
	 * under TLS 1.3. */
	gnutls_ecc_curve_t curve;

	/* See RFC 5246 HashAlgorithm and SignatureAlgorithm
	   for values to use in aid struct. */
	const sign_algorithm_st aid;
	hash_security_level_t slevel;	/* contains values of hash_security_level_t */

	/* 0 if it matches the predefined hash output size, otherwise
	 * it is truncated or expanded (with XOF) */
	unsigned hash_output_size;
};
typedef struct gnutls_sign_entry_st gnutls_sign_entry_st;

const gnutls_sign_entry_st *_gnutls_sign_to_entry(gnutls_sign_algorithm_t sign);
const gnutls_sign_entry_st *_gnutls_pk_to_sign_entry(gnutls_pk_algorithm_t pk, gnutls_digest_algorithm_t hash);
const gnutls_sign_entry_st *_gnutls_oid_to_sign_entry(const char *oid);

/* returns true if that signature can be generated
 * from the given private key algorithm. */
inline static unsigned
sign_supports_priv_pk_algorithm(const gnutls_sign_entry_st *se, gnutls_pk_algorithm_t pk)
{
	if (pk == se->pk || (se->priv_pk && se->priv_pk == pk))
		return 1;

	return 0;
}

/* returns true if that signature can be verified with
 * the given public key algorithm. */
inline static unsigned
sign_supports_cert_pk_algorithm(const gnutls_sign_entry_st *se, gnutls_pk_algorithm_t pk)
{
	if ((!se->cert_pk && pk == se->pk) || (se->cert_pk && se->cert_pk == pk))
		return 1;

	return 0;
}

bool _gnutls_sign_is_secure2(const gnutls_sign_entry_st *se, unsigned int flags);

gnutls_pk_algorithm_t _gnutls_x509_sign_to_pk(gnutls_sign_algorithm_t
					      sign);
const char *_gnutls_x509_sign_to_oid(gnutls_pk_algorithm_t,
				     gnutls_digest_algorithm_t mac);

const gnutls_sign_entry_st *
_gnutls_tls_aid_to_sign_entry(uint8_t id0, uint8_t id1, const version_entry_st *ver);

gnutls_sign_algorithm_t
_gnutls_tls_aid_to_sign(uint8_t id0, uint8_t id1, const version_entry_st *ver);
const sign_algorithm_st *_gnutls_sign_to_tls_aid(gnutls_sign_algorithm_t
						 sign);

const gnutls_sign_entry_st *
_gnutls13_sign_get_compatible_with_privkey(gnutls_privkey_t privkey);

unsigned int _gnutls_pk_bits_to_subgroup_bits(unsigned int pk_bits);
gnutls_digest_algorithm_t _gnutls_pk_bits_to_sha_hash(unsigned int pk_bits);

gnutls_digest_algorithm_t _gnutls_hash_size_to_sha_hash(unsigned int size);

bool _gnutls_pk_is_not_prehashed(gnutls_pk_algorithm_t algorithm);

/* ECC */
typedef struct gnutls_ecc_curve_entry_st {
	const char *name;
	const char *oid;
	gnutls_ecc_curve_t id;
	gnutls_pk_algorithm_t pk;
	unsigned size;		/* the size in bytes */
	unsigned sig_size;	/* the size of curve signatures in bytes (EdDSA) */
	unsigned gost_curve;
	bool supported;
	gnutls_group_t group;
} gnutls_ecc_curve_entry_st;

const gnutls_ecc_curve_entry_st
    *_gnutls_ecc_curve_get_params(gnutls_ecc_curve_t curve);

unsigned _gnutls_ecc_curve_is_supported(gnutls_ecc_curve_t);

gnutls_group_t _gnutls_ecc_curve_get_group(gnutls_ecc_curve_t);
const gnutls_group_entry_st *_gnutls_tls_id_to_group(unsigned num);
const gnutls_group_entry_st * _gnutls_id_to_group(unsigned id);

gnutls_ecc_curve_t _gnutls_ecc_bits_to_curve(gnutls_pk_algorithm_t pk, int bits);
#define MAX_ECC_CURVE_SIZE 66

gnutls_pk_algorithm_t _gnutls_oid_to_pk_and_curve(const char *oid, gnutls_ecc_curve_t *curve);

inline static int _curve_is_eddsa(const gnutls_ecc_curve_entry_st * e)
{
	if (unlikely(e == NULL))
		return 0;
	if (e->pk == GNUTLS_PK_EDDSA_ED25519 ||
	    e->pk == GNUTLS_PK_EDDSA_ED448)
		return 1;
	return 0;
}

inline static int curve_is_eddsa(gnutls_ecc_curve_t id)
{
	const gnutls_ecc_curve_entry_st *e = _gnutls_ecc_curve_get_params(id);
	return _curve_is_eddsa(e);
}

static inline int _gnutls_kx_is_ecc(gnutls_kx_algorithm_t kx)
{
	if (kx == GNUTLS_KX_ECDHE_RSA || kx == GNUTLS_KX_ECDHE_ECDSA ||
	    kx == GNUTLS_KX_ANON_ECDH || kx == GNUTLS_KX_ECDHE_PSK)
		return 1;

	return 0;
}

static inline int _gnutls_kx_is_psk(gnutls_kx_algorithm_t kx)
{
	if (kx == GNUTLS_KX_PSK || kx == GNUTLS_KX_DHE_PSK ||
	    kx == GNUTLS_KX_ECDHE_PSK || kx == GNUTLS_KX_RSA_PSK)
		return 1;

	return 0;
}

static inline int _gnutls_kx_is_dhe(gnutls_kx_algorithm_t kx)
{
	if (kx == GNUTLS_KX_DHE_RSA || kx == GNUTLS_KX_DHE_DSS ||
	    kx == GNUTLS_KX_ANON_DH || kx == GNUTLS_KX_DHE_PSK)
		return 1;

	return 0;
}

static inline unsigned _gnutls_kx_is_vko_gost(gnutls_kx_algorithm_t kx)
{
	if (kx == GNUTLS_KX_VKO_GOST_12)
		return 1;

	return 0;
}

static inline bool
_sign_is_gost(const gnutls_sign_entry_st *se)
{
	gnutls_pk_algorithm_t pk = se->pk;

	return  (pk == GNUTLS_PK_GOST_01) ||
		(pk == GNUTLS_PK_GOST_12_256) ||
		(pk == GNUTLS_PK_GOST_12_512);
}

static inline int _sig_is_ecdsa(gnutls_sign_algorithm_t sig)
{
	if (sig == GNUTLS_SIGN_ECDSA_SHA1 || sig == GNUTLS_SIGN_ECDSA_SHA224 ||
	    sig == GNUTLS_SIGN_ECDSA_SHA256 || sig == GNUTLS_SIGN_ECDSA_SHA384 ||
	    sig == GNUTLS_SIGN_ECDSA_SHA512)
		return 1;

	return 0;
}

bool _gnutls_pk_are_compat(gnutls_pk_algorithm_t pk1, gnutls_pk_algorithm_t pk2);

unsigned _gnutls_sign_get_hash_strength(gnutls_sign_algorithm_t sign);

#endif /* GNUTLS_LIB_ALGORITHMS_H */
