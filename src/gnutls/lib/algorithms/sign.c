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
#include <assert.h>
#include "c-strcase.h"

/* signature algorithms;
 */

#ifdef ALLOW_SHA1
# define SHA1_SECURE_VAL _SECURE
#else
# define SHA1_SECURE_VAL _INSECURE_FOR_CERTS
#endif

static SYSTEM_CONFIG_OR_CONST
gnutls_sign_entry_st sign_algorithms[] = {
	 /* RSA-PKCS#1 1.5: must be before PSS,
	  * so that gnutls_pk_to_sign() will return
	  * these first for backwards compatibility. */
	{.name = "RSA-SHA256",
	 .oid = SIG_RSA_SHA256_OID,
	 .id = GNUTLS_SIGN_RSA_SHA256,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA256,
	 .aid = {{4, 1}, SIG_SEM_DEFAULT}},
	{.name = "RSA-SHA384",
	 .oid = SIG_RSA_SHA384_OID,
	 .id = GNUTLS_SIGN_RSA_SHA384,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA384,
	 .aid = {{5, 1}, SIG_SEM_DEFAULT}},
	{.name = "RSA-SHA512",
	 .oid = SIG_RSA_SHA512_OID,
	 .id = GNUTLS_SIGN_RSA_SHA512,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA512,
	 .aid = {{6, 1}, SIG_SEM_DEFAULT}},

	/* RSA-PSS */
	{.name = "RSA-PSS-SHA256",
	 .oid = PK_PKIX1_RSA_PSS_OID,
	 .id = GNUTLS_SIGN_RSA_PSS_SHA256,
	 .pk = GNUTLS_PK_RSA_PSS,
	 .priv_pk = GNUTLS_PK_RSA, /* PKCS#11 doesn't separate RSA from RSA-PSS privkeys */
	 .hash = GNUTLS_DIG_SHA256,
	 .tls13_ok = 1,
	 .aid = {{8, 9}, SIG_SEM_DEFAULT}},
	{.name = "RSA-PSS-RSAE-SHA256",
	 .oid = PK_PKIX1_RSA_PSS_OID,
	 .id = GNUTLS_SIGN_RSA_PSS_RSAE_SHA256,
	 .pk = GNUTLS_PK_RSA_PSS,
	 .cert_pk = GNUTLS_PK_RSA,
	 .priv_pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA256,
	 .tls13_ok = 1,
	 .aid = {{8, 4}, SIG_SEM_DEFAULT}},
	{.name = "RSA-PSS-SHA384",
	 .oid = PK_PKIX1_RSA_PSS_OID,
	 .id = GNUTLS_SIGN_RSA_PSS_SHA384,
	 .pk = GNUTLS_PK_RSA_PSS,
	 .priv_pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA384,
	 .tls13_ok = 1,
	 .aid = {{8, 0x0A}, SIG_SEM_DEFAULT}},
	{.name = "RSA-PSS-RSAE-SHA384",
	 .oid = PK_PKIX1_RSA_PSS_OID,
	 .id = GNUTLS_SIGN_RSA_PSS_RSAE_SHA384,
	 .pk = GNUTLS_PK_RSA_PSS,
	 .cert_pk = GNUTLS_PK_RSA,
	 .priv_pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA384,
	 .tls13_ok = 1,
	 .aid = {{8, 5}, SIG_SEM_DEFAULT}},
	{.name = "RSA-PSS-SHA512",
	 .oid = PK_PKIX1_RSA_PSS_OID,
	 .id = GNUTLS_SIGN_RSA_PSS_SHA512,
	 .pk = GNUTLS_PK_RSA_PSS,
	 .priv_pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA512,
	 .tls13_ok = 1,
	 .aid = {{8, 0x0B}, SIG_SEM_DEFAULT}},
	{.name = "RSA-PSS-RSAE-SHA512",
	 .oid = PK_PKIX1_RSA_PSS_OID,
	 .id = GNUTLS_SIGN_RSA_PSS_RSAE_SHA512,
	 .pk = GNUTLS_PK_RSA_PSS,
	 .cert_pk = GNUTLS_PK_RSA,
	 .priv_pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA512,
	 .tls13_ok = 1,
	 .aid = {{8, 6}, SIG_SEM_DEFAULT}},

	 /* Ed25519: The hash algorithm here is set to be SHA512, although that is
	  * an internal detail of Ed25519; we set it, because CMS/PKCS#7 requires
	  * that mapping. */
	 {.name = "EdDSA-Ed25519",
	 .oid = SIG_EDDSA_SHA512_OID,
	 .id = GNUTLS_SIGN_EDDSA_ED25519,
	 .pk = GNUTLS_PK_EDDSA_ED25519,
	 .hash = GNUTLS_DIG_SHA512,
	 .tls13_ok = 1,
	 .aid = {{8, 7}, SIG_SEM_DEFAULT}},

	 /* ECDSA */
	 /* The following three signature algorithms
	  * have different semantics when used under TLS 1.2
	  * or TLS 1.3. Under the former they behave as the
	  * as ECDSA signed by SHAXXX by any curve, but under the
	  * latter they are restricted to a single curve.
	  * For this reason the ECDSA-SHAXXX algorithms act
	  * as an alias to them. */
	 /* we have intentionally the ECDSA-SHAXXX algorithms first
	  * so that gnutls_pk_to_sign() will return these. */
	{.name = "ECDSA-SHA256",
	 .oid = "1.2.840.10045.4.3.2",
	 .id = GNUTLS_SIGN_ECDSA_SHA256,
	 .pk = GNUTLS_PK_ECDSA,
	 .hash = GNUTLS_DIG_SHA256,
	 .aid = {{4, 3}, SIG_SEM_PRE_TLS12}},
	{.name = "ECDSA-SHA384",
	 .oid = "1.2.840.10045.4.3.3",
	 .id = GNUTLS_SIGN_ECDSA_SHA384,
	 .pk = GNUTLS_PK_ECDSA,
	 .hash = GNUTLS_DIG_SHA384,
	 .aid = {{5, 3}, SIG_SEM_PRE_TLS12}},
	{.name = "ECDSA-SHA512",
	 .oid = "1.2.840.10045.4.3.4",
	 .id = GNUTLS_SIGN_ECDSA_SHA512,
	 .pk = GNUTLS_PK_ECDSA,
	 .hash = GNUTLS_DIG_SHA512,
	 .aid = {{6, 3}, SIG_SEM_PRE_TLS12}},

	{.name = "ECDSA-SECP256R1-SHA256",
	 .id = GNUTLS_SIGN_ECDSA_SECP256R1_SHA256,
	 .pk = GNUTLS_PK_ECDSA,
	 .curve = GNUTLS_ECC_CURVE_SECP256R1,
	 .hash = GNUTLS_DIG_SHA256,
	 .tls13_ok = 1,
	 .aid = {{4, 3}, SIG_SEM_TLS13}},
	{.name = "ECDSA-SECP384R1-SHA384",
	 .id = GNUTLS_SIGN_ECDSA_SECP384R1_SHA384,
	 .pk = GNUTLS_PK_ECDSA,
	 .curve = GNUTLS_ECC_CURVE_SECP384R1,
	 .hash = GNUTLS_DIG_SHA384,
	 .tls13_ok = 1,
	 .aid = {{5, 3}, SIG_SEM_TLS13}},
	{.name = "ECDSA-SECP521R1-SHA512",
	 .id = GNUTLS_SIGN_ECDSA_SECP521R1_SHA512,
	 .pk = GNUTLS_PK_ECDSA,
	 .curve = GNUTLS_ECC_CURVE_SECP521R1,
	 .hash = GNUTLS_DIG_SHA512,
	 .tls13_ok = 1,
	 .aid = {{6, 3}, SIG_SEM_TLS13}},

	 /* ECDSA-SHA3 */
	{.name = "ECDSA-SHA3-224",
	 .oid = SIG_ECDSA_SHA3_224_OID,
	 .id = GNUTLS_SIGN_ECDSA_SHA3_224,
	 .pk = GNUTLS_PK_EC,
	 .hash = GNUTLS_DIG_SHA3_224,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "ECDSA-SHA3-256",
	 .oid = SIG_ECDSA_SHA3_256_OID,
	 .id = GNUTLS_SIGN_ECDSA_SHA3_256,
	 .pk = GNUTLS_PK_EC,
	 .hash = GNUTLS_DIG_SHA3_256,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "ECDSA-SHA3-384",
	 .oid = SIG_ECDSA_SHA3_384_OID,
	 .id = GNUTLS_SIGN_ECDSA_SHA3_384,
	 .pk = GNUTLS_PK_EC,
	 .hash = GNUTLS_DIG_SHA3_384,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "ECDSA-SHA3-512",
	 .oid = SIG_ECDSA_SHA3_512_OID,
	 .id = GNUTLS_SIGN_ECDSA_SHA3_512,
	 .pk = GNUTLS_PK_EC,
	 .hash = GNUTLS_DIG_SHA3_512,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-SHA3-224",
	 .oid = SIG_RSA_SHA3_224_OID,
	 .id = GNUTLS_SIGN_RSA_SHA3_224,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA3_224,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-SHA3-256",
	 .oid = SIG_RSA_SHA3_256_OID,
	 .id = GNUTLS_SIGN_RSA_SHA3_256,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA3_256,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-SHA3-384",
	 .oid = SIG_RSA_SHA3_384_OID,
	 .id = GNUTLS_SIGN_RSA_SHA3_384,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA3_384,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-SHA3-512",
	 .oid = SIG_RSA_SHA3_512_OID,
	 .id = GNUTLS_SIGN_RSA_SHA3_512,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA3_512,
	 .aid = TLS_SIGN_AID_UNKNOWN},

	 /* DSA-SHA3 */
	{.name = "DSA-SHA3-224",
	 .oid = SIG_DSA_SHA3_224_OID,
	 .id = GNUTLS_SIGN_DSA_SHA3_224,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA3_224,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "DSA-SHA3-256",
	 .oid = SIG_DSA_SHA3_256_OID,
	 .id = GNUTLS_SIGN_DSA_SHA3_256,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA3_256,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "DSA-SHA3-384",
	 .oid = SIG_DSA_SHA3_384_OID,
	 .id = GNUTLS_SIGN_DSA_SHA3_384,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA3_384,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "DSA-SHA3-512",
	 .oid = SIG_DSA_SHA3_512_OID,
	 .id = GNUTLS_SIGN_DSA_SHA3_512,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA3_512,
	 .aid = TLS_SIGN_AID_UNKNOWN},

	 /* legacy */
	{.name = "RSA-RAW",
	 .oid = NULL,
	 .id = GNUTLS_SIGN_RSA_RAW,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_UNKNOWN,
	 .aid = TLS_SIGN_AID_UNKNOWN
	},
	{.name = "RSA-SHA1",
	 .oid = SIG_RSA_SHA1_OID,
	 .id = GNUTLS_SIGN_RSA_SHA1,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA1,
	 .slevel = SHA1_SECURE_VAL,
	 .aid = {{2, 1}, SIG_SEM_DEFAULT}},
	{.name = "RSA-SHA1",
	 .oid = ISO_SIG_RSA_SHA1_OID,
	 .id = GNUTLS_SIGN_RSA_SHA1,
	 .pk = GNUTLS_PK_RSA,
	 .slevel = SHA1_SECURE_VAL,
	 .hash = GNUTLS_DIG_SHA1,
	 .aid = {{2, 1}, SIG_SEM_DEFAULT}},
	{.name = "RSA-SHA224",
	 .oid = SIG_RSA_SHA224_OID,
	 .id = GNUTLS_SIGN_RSA_SHA224,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_SHA224,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-RMD160",
	 .oid = SIG_RSA_RMD160_OID,
	 .id = GNUTLS_SIGN_RSA_RMD160,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_RMD160,
	 .slevel = _INSECURE_FOR_CERTS,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "DSA-SHA1",
	 .oid = SIG_DSA_SHA1_OID,
	 .id = GNUTLS_SIGN_DSA_SHA1,
	 .pk = GNUTLS_PK_DSA,
	 .slevel = SHA1_SECURE_VAL,
	 .hash = GNUTLS_DIG_SHA1,
	 .aid = {{2, 2}, SIG_SEM_PRE_TLS12}},
	{.name = "DSA-SHA1",
	 .oid = "1.3.14.3.2.27",
	 .id = GNUTLS_SIGN_DSA_SHA1,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA1,
	 .slevel = SHA1_SECURE_VAL,
	 .aid = {{2, 2}, SIG_SEM_PRE_TLS12}},
	{.name = "DSA-SHA224",
	 .oid = SIG_DSA_SHA224_OID,
	 .id = GNUTLS_SIGN_DSA_SHA224,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA224,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "DSA-SHA256",
	 .oid = SIG_DSA_SHA256_OID,
	 .id = GNUTLS_SIGN_DSA_SHA256,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA256,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-MD5",
	 .oid = SIG_RSA_MD5_OID,
	 .id = GNUTLS_SIGN_RSA_MD5,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_MD5,
	 .slevel = _INSECURE,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-MD5",
	 .oid = "1.3.14.3.2.25",
	 .id = GNUTLS_SIGN_RSA_MD5,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_MD5,
	 .slevel = _INSECURE,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "RSA-MD2",
	 .oid = SIG_RSA_MD2_OID,
	 .id = GNUTLS_SIGN_RSA_MD2,
	 .pk = GNUTLS_PK_RSA,
	 .hash = GNUTLS_DIG_MD2,
	 .slevel = _INSECURE,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "ECDSA-SHA1",
	 .oid = "1.2.840.10045.4.1",
	 .id = GNUTLS_SIGN_ECDSA_SHA1,
	 .pk = GNUTLS_PK_EC,
	 .slevel = SHA1_SECURE_VAL,
	 .hash = GNUTLS_DIG_SHA1,
	 .aid = {{2, 3}, SIG_SEM_DEFAULT}},
	{.name = "ECDSA-SHA224",
	 .oid = "1.2.840.10045.4.3.1",
	 .id = GNUTLS_SIGN_ECDSA_SHA224,
	 .pk = GNUTLS_PK_EC,
	 .hash = GNUTLS_DIG_SHA224,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	/* GOST R 34.10-2012-512 */
	{.name = "GOSTR341012-512",
	 .oid = SIG_GOST_R3410_2012_512_OID,
	 .id = GNUTLS_SIGN_GOST_512,
	 .pk = GNUTLS_PK_GOST_12_512,
	 .hash = GNUTLS_DIG_STREEBOG_512,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	/* GOST R 34.10-2012-256 */
	{.name = "GOSTR341012-256",
	 .oid = SIG_GOST_R3410_2012_256_OID,
	 .id = GNUTLS_SIGN_GOST_256,
	 .pk = GNUTLS_PK_GOST_12_256,
	 .hash = GNUTLS_DIG_STREEBOG_256,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	/* GOST R 34.10-2001 */
	{.name = "GOSTR341001",
	 .oid = SIG_GOST_R3410_2001_OID,
	 .id = GNUTLS_SIGN_GOST_94,
	 .pk = GNUTLS_PK_GOST_01,
	 .hash = GNUTLS_DIG_GOSTR_94,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	/* GOST R 34.10-94 */
	{.name = "GOSTR341094",
	 .oid = SIG_GOST_R3410_94_OID,
	 .id = 0,
	 .pk = 0,
	 .hash = 0,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "DSA-SHA384",
	 .oid = SIG_DSA_SHA384_OID,
	 .id = GNUTLS_SIGN_DSA_SHA384,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA384,
	 .aid = TLS_SIGN_AID_UNKNOWN},
	{.name = "DSA-SHA512",
	 .oid = SIG_DSA_SHA512_OID,
	 .id = GNUTLS_SIGN_DSA_SHA512,
	 .pk = GNUTLS_PK_DSA,
	 .hash = GNUTLS_DIG_SHA512,
	 .aid = TLS_SIGN_AID_UNKNOWN},

	{.name = 0,
	 .oid = 0,
	 .id = 0,
	 .pk = 0,
	 .hash = 0,
	 .aid = TLS_SIGN_AID_UNKNOWN}
};

#define GNUTLS_SIGN_LOOP(b) \
  do {								       \
    const gnutls_sign_entry_st *p;					       \
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
unsigned gnutls_sign_is_secure(gnutls_sign_algorithm_t algorithm)
{
	return gnutls_sign_is_secure2(algorithm, 0);
}

bool _gnutls_sign_is_secure2(const gnutls_sign_entry_st *se, unsigned int flags)
{
	if (se->hash != GNUTLS_DIG_UNKNOWN && _gnutls_digest_is_insecure(se->hash))
		return gnutls_assert_val(0);

	if (flags & GNUTLS_SIGN_FLAG_SECURE_FOR_CERTS)
		return (se->slevel==_SECURE)?1:0;
	else
		return (se->slevel==_SECURE || se->slevel == _INSECURE_FOR_CERTS)?1:0;
}

int _gnutls_sign_mark_insecure(const char *name, hash_security_level_t level)
{
#ifndef DISABLE_SYSTEM_CONFIG
	gnutls_sign_entry_st *p;

	if (unlikely(level == _SECURE))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	for(p = sign_algorithms; p->name != NULL; p++) {
		if (c_strcasecmp(p->name, name) == 0) {
				p->slevel = level;
			return 0;
		}
	}
#endif
	return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
}

/**
 * gnutls_sign_is_secure2:
 * @algorithm: is a sign algorithm
 * @flags: zero or %GNUTLS_SIGN_FLAG_SECURE_FOR_CERTS
 *
 * Returns: Non-zero if the provided signature algorithm is considered to be secure.
 **/
unsigned gnutls_sign_is_secure2(gnutls_sign_algorithm_t algorithm, unsigned int flags)
{
	const gnutls_sign_entry_st *se;

	se = _gnutls_sign_to_entry(algorithm);
	if (se == NULL)
		return 0;

	return _gnutls_sign_is_secure2(se, flags);
}

/**
 * gnutls_sign_list:
 *
 * Get a list of supported public key signature algorithms.
 * This function is not thread safe.
 *
 * Returns: a (0)-terminated list of #gnutls_sign_algorithm_t
 *   integers indicating the available ciphers.
 *
 **/
const gnutls_sign_algorithm_t *gnutls_sign_list(void)
{
	static gnutls_sign_algorithm_t supported_sign[MAX_ALGOS+1] = { 0 };

	if (supported_sign[0] == 0) {
		int i = 0;

		GNUTLS_SIGN_LOOP(
			/* list all algorithms, but not duplicates */
			if (supported_sign[i] != p->id) {
				assert(i+1 < MAX_ALGOS);
				supported_sign[i++] = p->id;
				supported_sign[i+1] = 0;
			}
		);
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
		if (c_strcasecmp(p->name, name) == 0) {
			ret = p->id;
			break;
		}
	);

	return ret;

}

const gnutls_sign_entry_st *_gnutls_oid_to_sign_entry(const char *oid)
{
	GNUTLS_SIGN_LOOP(
		if (p->oid && strcmp(oid, p->oid) == 0) {
			return p;
		}
	);
	return NULL;
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
	const gnutls_sign_entry_st *se;

	se = _gnutls_oid_to_sign_entry(oid);
	if (se == NULL) {
		_gnutls_debug_log("Unknown SIGN OID: '%s'\n", oid);
		return GNUTLS_SIGN_UNKNOWN;
	}
	return se->id;
}

const gnutls_sign_entry_st *_gnutls_pk_to_sign_entry(gnutls_pk_algorithm_t pk, gnutls_digest_algorithm_t hash)
{
	GNUTLS_SIGN_LOOP(
		if (pk == p->pk && hash == p->hash) {
			return p;
		}
	);

	return NULL;
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
	const gnutls_sign_entry_st *e;

	e = _gnutls_pk_to_sign_entry(pk, hash);
	if (e == NULL)
		return GNUTLS_SIGN_UNKNOWN;
	return e->id;
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

	GNUTLS_SIGN_ALG_LOOP(ret = p->hash);

	return ret;
}

/**
 * gnutls_sign_get_pk_algorithm:
 * @sign: is a signature algorithm
 *
 * This function returns the public key algorithm corresponding to
 * the given signature algorithms. Note that there may be multiple
 * public key algorithms supporting a particular signature type;
 * when dealing with such algorithms use instead gnutls_sign_supports_pk_algorithm().
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

/**
 * gnutls_sign_supports_pk_algorithm:
 * @sign: is a signature algorithm
 * @pk: is a public key algorithm
 *
 * This function returns non-zero if the public key algorithm corresponds to
 * the given signature algorithm. That is, if that signature can be generated
 * from the given private key algorithm.
 *
 * Since: 3.6.0
 *
 * Returns: return non-zero when the provided algorithms are compatible.
 **/
unsigned
gnutls_sign_supports_pk_algorithm(gnutls_sign_algorithm_t sign, gnutls_pk_algorithm_t pk)
{
	const gnutls_sign_entry_st *p;
	unsigned r;

	for(p = sign_algorithms; p->name != NULL; p++) {
		if (p->id && p->id == sign) {
			r = sign_supports_priv_pk_algorithm(p, pk);
			if (r != 0)
				return r;
		}
	}

	return 0;
}

gnutls_sign_algorithm_t
_gnutls_tls_aid_to_sign(uint8_t id0, uint8_t id1, const version_entry_st *ver)
{
	gnutls_sign_algorithm_t ret = GNUTLS_SIGN_UNKNOWN;

	if (id0 == 255 && id1 == 255)
		return ret;

	GNUTLS_SIGN_LOOP(
		if (p->aid.id[0] == id0 &&
		     p->aid.id[1] == id1 &&
		     ((p->aid.tls_sem & ver->tls_sig_sem) != 0)) {

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

	if (ret != NULL && HAVE_UNKNOWN_SIGAID(ret))
		return NULL;

	return ret;
}

const gnutls_sign_entry_st *_gnutls_sign_to_entry(gnutls_sign_algorithm_t sign)
{
	const gnutls_sign_entry_st *ret = NULL;

	GNUTLS_SIGN_ALG_LOOP(ret = p);

	return ret;
}

const gnutls_sign_entry_st *
_gnutls_tls_aid_to_sign_entry(uint8_t id0, uint8_t id1, const version_entry_st *ver)
{
	if (id0 == 255 && id1 == 255)
		return NULL;

	GNUTLS_SIGN_LOOP(
		if (p->aid.id[0] == id0 &&
		     p->aid.id[1] == id1 &&
		     ((p->aid.tls_sem & ver->tls_sig_sem) != 0)) {

			return p;
		}
	);

	return NULL;
}

const gnutls_sign_entry_st *
_gnutls13_sign_get_compatible_with_privkey(gnutls_privkey_t privkey)
{
	GNUTLS_SIGN_LOOP(
		if (p->tls13_ok &&
		    _gnutls_privkey_compatible_with_sig(privkey, p->id)) {
			return p;
		}
	);

	return NULL;
}
