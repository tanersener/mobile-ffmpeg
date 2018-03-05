/*
 * Copyright (C) 2004-2015 Free Software Foundation, Inc.
 * Copyright (C) 2015 Red Hat, Inc.
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

/* Here lies the code of the gnutls_*_set_priority() functions.
 */

#include "gnutls_int.h"
#include "algorithms.h"
#include "errors.h"
#include <num.h>
#include <gnutls/x509.h>
#include <c-ctype.h>
#include <extensions.h>
#include "fips.h"
#include "errno.h"

#define MAX_ELEMENTS 64

/* This function is used by the test suite */
char *_gnutls_resolve_priorities(const char* priorities);

static void prio_remove(priority_st * priority_list, unsigned int algo);
static void prio_add(priority_st * priority_list, unsigned int algo);
static void
break_list(char *etag,
		 char *broken_etag[MAX_ELEMENTS], int *size);

typedef void (bulk_rmadd_func) (priority_st * priority_list, const int *);

inline static void _set_priority(priority_st * st, const int *list)
{
	int num = 0, i;

	while (list[num] != 0)
		num++;
	if (num > MAX_ALGOS)
		num = MAX_ALGOS;
	st->algorithms = num;

	for (i = 0; i < num; i++) {
		st->priority[i] = list[i];
	}

	return;
}

inline static void _add_priority(priority_st * st, const int *list)
{
	int num, i, j, init;

	init = i = st->algorithms;

	for (num = 0; list[num] != 0; ++num) {
		if (i + 1 > MAX_ALGOS) {
			return;
		}

		for (j = 0; j < init; j++) {
			if (st->priority[j] == (unsigned) list[num]) {
				break;
			}
		}

		if (j == init) {
			st->priority[i++] = list[num];
			st->algorithms++;
		}
	}

	return;
}

static void _clear_priorities(priority_st * st, const int *list)
{
	memset(st, 0, sizeof(*st));
}

static void _clear_given_priorities(priority_st * st, const int *list)
{
	unsigned i;

	for (i=0;list[i]!=0;i++) {
		prio_remove(st, list[i]);
	}
}

static const int _supported_ecc_normal[] = {
	GNUTLS_ECC_CURVE_SECP256R1,
	GNUTLS_ECC_CURVE_SECP384R1,
	GNUTLS_ECC_CURVE_SECP521R1,
#ifdef ENABLE_NON_SUITEB_CURVES
	GNUTLS_ECC_CURVE_SECP224R1,
	GNUTLS_ECC_CURVE_SECP192R1,
#endif
	0
};
static const int* supported_ecc_normal = _supported_ecc_normal;

static const int _supported_ecc_secure128[] = {
	GNUTLS_ECC_CURVE_SECP256R1,
	GNUTLS_ECC_CURVE_SECP384R1,
	GNUTLS_ECC_CURVE_SECP521R1,
	0
};
static const int* supported_ecc_secure128 = _supported_ecc_secure128;

static const int _supported_ecc_suiteb128[] = {
	GNUTLS_ECC_CURVE_SECP256R1,
	GNUTLS_ECC_CURVE_SECP384R1,
	0
};
static const int* supported_ecc_suiteb128 = _supported_ecc_suiteb128;

static const int _supported_ecc_suiteb192[] = {
	GNUTLS_ECC_CURVE_SECP384R1,
	0
};
static const int* supported_ecc_suiteb192 = _supported_ecc_suiteb192;

static const int _supported_ecc_secure192[] = {
	GNUTLS_ECC_CURVE_SECP384R1,
	GNUTLS_ECC_CURVE_SECP521R1,
	0
};
static const int* supported_ecc_secure192 = _supported_ecc_secure192;

static const int protocol_priority[] = {
	GNUTLS_TLS1_2,
	GNUTLS_TLS1_1,
	GNUTLS_TLS1_0,
	GNUTLS_DTLS1_2,
	GNUTLS_DTLS1_0,
	0
};

static const int stream_protocol_priority[] = {
	GNUTLS_TLS1_2,
	GNUTLS_TLS1_1,
	GNUTLS_TLS1_0,
	0
};

static const int dgram_protocol_priority[] = {
	GNUTLS_DTLS1_2,
	GNUTLS_DTLS1_0,
	GNUTLS_DTLS0_9,
	0
};

static const int dtls_protocol_priority[] = {
	GNUTLS_DTLS1_2,
	GNUTLS_DTLS1_0,
	0
};

static const int _protocol_priority_suiteb[] = {
	GNUTLS_TLS1_2,
	0
};
static const int* protocol_priority_suiteb = _protocol_priority_suiteb;

static const int _kx_priority_performance[] = {
	GNUTLS_KX_RSA,
#ifdef ENABLE_ECDHE
	GNUTLS_KX_ECDHE_ECDSA,
	GNUTLS_KX_ECDHE_RSA,
#endif
#ifdef ENABLE_DHE
	GNUTLS_KX_DHE_RSA,
#endif
	0
};
static const int* kx_priority_performance = _kx_priority_performance;

static const int _kx_priority_pfs[] = {
#ifdef ENABLE_ECDHE
	GNUTLS_KX_ECDHE_ECDSA,
	GNUTLS_KX_ECDHE_RSA,
#endif
#ifdef ENABLE_DHE
	GNUTLS_KX_DHE_RSA,
#endif
	0
};
static const int* kx_priority_pfs = _kx_priority_pfs;

static const int _kx_priority_suiteb[] = {
	GNUTLS_KX_ECDHE_ECDSA,
	0
};
static const int* kx_priority_suiteb = _kx_priority_suiteb;

static const int _kx_priority_secure[] = {
	/* The ciphersuites that offer forward secrecy take
	 * precedence
	 */
#ifdef ENABLE_ECDHE
	GNUTLS_KX_ECDHE_ECDSA,
	GNUTLS_KX_ECDHE_RSA,
#endif
	GNUTLS_KX_RSA,
	/* KX-RSA is now ahead of DHE-RSA and DHE-DSS due to the compatibility
	 * issues the DHE ciphersuites have. That is, one cannot enforce a specific
	 * security level without dropping the connection. 
	 */
#ifdef ENABLE_DHE
	GNUTLS_KX_DHE_RSA,
#endif
	/* GNUTLS_KX_ANON_DH: Man-in-the-middle prone, don't add!
	 */
	0
};
static const int* kx_priority_secure = _kx_priority_secure;

static const int _cipher_priority_performance_default[] = {
	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_128_CCM,
	GNUTLS_CIPHER_AES_256_CCM,
	GNUTLS_CIPHER_CAMELLIA_128_GCM,
	GNUTLS_CIPHER_CAMELLIA_256_GCM,
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_CAMELLIA_128_CBC,
	GNUTLS_CIPHER_CAMELLIA_256_CBC,
	GNUTLS_CIPHER_3DES_CBC,
	0
};

static const int _cipher_priority_performance_no_aesni[] = {
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_AES_128_CCM,
	GNUTLS_CIPHER_AES_256_CCM,
	GNUTLS_CIPHER_CAMELLIA_128_GCM,
	GNUTLS_CIPHER_CAMELLIA_256_GCM,
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_CAMELLIA_128_CBC,
	GNUTLS_CIPHER_CAMELLIA_256_CBC,
	GNUTLS_CIPHER_3DES_CBC,
	0
};

/* If GCM and AES acceleration is available then prefer
 * them over anything else. Overall we prioritise AEAD
 * over legacy ciphers, and 256-bit over 128 (for future
 * proof).
 */
static const int _cipher_priority_normal_default[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_CAMELLIA_256_GCM,
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_256_CCM,

	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_CAMELLIA_256_CBC,

	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_CAMELLIA_128_GCM,
	GNUTLS_CIPHER_AES_128_CCM,

	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_CAMELLIA_128_CBC,

	GNUTLS_CIPHER_3DES_CBC,
	0
};

static const int cipher_priority_performance_fips[] = {
	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_128_CCM,
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_AES_256_CCM,

	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_3DES_CBC,
	0
};

static const int cipher_priority_normal_fips[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_AES_256_CCM,
	GNUTLS_CIPHER_AES_256_CBC,

	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_128_CCM,
	GNUTLS_CIPHER_3DES_CBC,
	0
};


static const int _cipher_priority_suiteb128[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_AES_128_GCM,
	0
};
static const int* cipher_priority_suiteb128 = _cipher_priority_suiteb128;

static const int _cipher_priority_suiteb192[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	0
};
static const int* cipher_priority_suiteb192 = _cipher_priority_suiteb192;


static const int _cipher_priority_secure128[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_CAMELLIA_256_GCM,
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_CAMELLIA_256_CBC,
	GNUTLS_CIPHER_AES_256_CCM,

	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_CAMELLIA_128_GCM,
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_CAMELLIA_128_CBC,
	GNUTLS_CIPHER_AES_128_CCM,
	0
};
static const int *cipher_priority_secure128 = _cipher_priority_secure128;


static const int _cipher_priority_secure192[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_CAMELLIA_256_GCM,
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_CAMELLIA_256_CBC,
	GNUTLS_CIPHER_AES_256_CCM,
	0
};
static const int* cipher_priority_secure192 = _cipher_priority_secure192;

static const int comp_priority[] = {
	/* compression should be explicitly requested to be enabled */
	GNUTLS_COMP_NULL,
	0
};

static const int _sign_priority_default[] = {
	GNUTLS_SIGN_RSA_SHA256,
	GNUTLS_SIGN_ECDSA_SHA256,

	GNUTLS_SIGN_RSA_SHA384,
	GNUTLS_SIGN_ECDSA_SHA384,

	GNUTLS_SIGN_RSA_SHA512,
	GNUTLS_SIGN_ECDSA_SHA512,

	GNUTLS_SIGN_RSA_SHA224,
	GNUTLS_SIGN_ECDSA_SHA224,

	GNUTLS_SIGN_RSA_SHA1,
	GNUTLS_SIGN_ECDSA_SHA1,
	0
};
static const int* sign_priority_default = _sign_priority_default;

static const int _sign_priority_suiteb128[] = {
	GNUTLS_SIGN_ECDSA_SHA256,
	GNUTLS_SIGN_ECDSA_SHA384,
	0
};
static const int* sign_priority_suiteb128 = _sign_priority_suiteb128;

static const int _sign_priority_suiteb192[] = {
	GNUTLS_SIGN_ECDSA_SHA384,
	0
};
static const int* sign_priority_suiteb192 = _sign_priority_suiteb192;

static const int _sign_priority_secure128[] = {
	GNUTLS_SIGN_RSA_SHA256,
	GNUTLS_SIGN_ECDSA_SHA256,
	GNUTLS_SIGN_RSA_SHA384,
	GNUTLS_SIGN_ECDSA_SHA384,
	GNUTLS_SIGN_RSA_SHA512,
	GNUTLS_SIGN_ECDSA_SHA512,
	0
};
static const int* sign_priority_secure128 = _sign_priority_secure128;

static const int _sign_priority_secure192[] = {
	GNUTLS_SIGN_RSA_SHA384,
	GNUTLS_SIGN_ECDSA_SHA384,
	GNUTLS_SIGN_RSA_SHA512,
	GNUTLS_SIGN_ECDSA_SHA512,
	0
};
static const int* sign_priority_secure192 = _sign_priority_secure192;

static const int mac_priority_normal_default[] = {
	GNUTLS_MAC_SHA1,
	GNUTLS_MAC_SHA256,
	GNUTLS_MAC_SHA384,
	GNUTLS_MAC_AEAD,
	GNUTLS_MAC_MD5,
	0
};

static const int mac_priority_normal_fips[] = {
	GNUTLS_MAC_SHA1,
	GNUTLS_MAC_SHA256,
	GNUTLS_MAC_SHA384,
	GNUTLS_MAC_AEAD,
	0
};

static const int *cipher_priority_performance = _cipher_priority_performance_default;
static const int *cipher_priority_normal = _cipher_priority_normal_default;
static const int *mac_priority_normal = mac_priority_normal_default;

/* if called with replace the default priorities with the FIPS140 ones */
void _gnutls_priority_update_fips(void)
{
	cipher_priority_performance = cipher_priority_performance_fips;
	cipher_priority_normal = cipher_priority_normal_fips;
	mac_priority_normal = mac_priority_normal_fips;
}

void _gnutls_priority_update_non_aesni(void)
{
	/* if we have no AES acceleration in performance mode
	 * prefer fast stream ciphers */
	if (_gnutls_fips_mode_enabled() == 0) {
		cipher_priority_performance = _cipher_priority_performance_no_aesni;
	}
}

static const int _mac_priority_suiteb[] = {
	GNUTLS_MAC_AEAD,
	0
};
static const int* mac_priority_suiteb = _mac_priority_suiteb;

static const int _mac_priority_secure128[] = {
	GNUTLS_MAC_SHA1,
	GNUTLS_MAC_SHA256,
	GNUTLS_MAC_SHA384,
	GNUTLS_MAC_AEAD,
	0
};
static const int* mac_priority_secure128 = _mac_priority_secure128;

static const int _mac_priority_secure192[] = {
	GNUTLS_MAC_SHA256,
	GNUTLS_MAC_SHA384,
	GNUTLS_MAC_AEAD,
	0
};
static const int* mac_priority_secure192 = _mac_priority_secure192;

static const int cert_type_priority_default[] = {
	GNUTLS_CRT_X509,
	0
};

static const int cert_type_priority_all[] = {
	GNUTLS_CRT_X509,
	GNUTLS_CRT_OPENPGP,
	0
};

typedef void (rmadd_func) (priority_st * priority_list, unsigned int alg);

static void prio_remove(priority_st * priority_list, unsigned int algo)
{
	unsigned int i;

	for (i = 0; i < priority_list->algorithms; i++) {
		if (priority_list->priority[i] == algo) {
			priority_list->algorithms--;
			if ((priority_list->algorithms - i) > 0)
				memmove(&priority_list->priority[i],
					&priority_list->priority[i + 1],
					(priority_list->algorithms -
					 i) *
					sizeof(priority_list->
					       priority[0]));
			priority_list->priority[priority_list->
						algorithms] = 0;
			break;
		}
	}

	return;
}

static void prio_add(priority_st * priority_list, unsigned int algo)
{
	unsigned int i, l = priority_list->algorithms;

	if (l >= MAX_ALGOS)
		return;		/* can't add it anyway */

	for (i = 0; i < l; ++i) {
		if (algo == priority_list->priority[i])
			return;	/* if it exists */
	}

	priority_list->priority[l] = algo;
	priority_list->algorithms++;

	return;
}


/**
 * gnutls_priority_set:
 * @session: is a #gnutls_session_t type.
 * @priority: is a #gnutls_priority_t type.
 *
 * Sets the priorities to use on the ciphers, key exchange methods,
 * macs and compression methods.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_priority_set(gnutls_session_t session, gnutls_priority_t priority)
{
	if (priority == NULL) {
		gnutls_assert();
		return GNUTLS_E_NO_CIPHER_SUITES;
	}

	memcpy(&session->internals.priorities, priority,
	       sizeof(struct gnutls_priority_st));

	/* set the current version to the first in the chain.
	 * This will be overridden later.
	 */
	if (session->internals.priorities.protocol.algorithms > 0) {
		if (_gnutls_set_current_version(session,
					    session->internals.priorities.
					    protocol.priority[0]) < 0) {
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);
		}
	}

	if (priority->no_tickets != 0) {
		/* when PFS is explicitly requested, disable session tickets */
		_gnutls_ext_unset_session_data(session, GNUTLS_EXTENSION_SESSION_TICKET);
	}

	if (session->internals.priorities.protocol.algorithms == 0 ||
	    session->internals.priorities.cipher.algorithms == 0 ||
	    session->internals.priorities.mac.algorithms == 0 ||
	    session->internals.priorities.kx.algorithms == 0 ||
	    session->internals.priorities.compression.algorithms == 0)
		return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);

	ADD_PROFILE_VFLAGS(session, priority->additional_verify_flags);

	return 0;
}


#define LEVEL_NONE "NONE"
#define LEVEL_NORMAL "NORMAL"
#define LEVEL_PFS "PFS"
#define LEVEL_PERFORMANCE "PERFORMANCE"
#define LEVEL_SECURE128 "SECURE128"
#define LEVEL_SECURE192 "SECURE192"
#define LEVEL_SECURE256 "SECURE256"
#define LEVEL_SUITEB128 "SUITEB128"
#define LEVEL_SUITEB192 "SUITEB192"
#define LEVEL_LEGACY "LEGACY"

struct priority_groups_st {
	const char *name;
	const char *alias;
	const int **proto_list;
	const int **cipher_list;
	const int **mac_list;
	const int **kx_list;
	const int **sign_list;
	const int **ecc_list;
	unsigned profile;
	int sec_param;
	bool no_tickets;
};

static const struct priority_groups_st pgroups[] = 
{
	{.name = LEVEL_NORMAL,
	 .cipher_list = &cipher_priority_normal,
	 .mac_list = &mac_priority_normal,
	 .kx_list = &kx_priority_secure,
	 .sign_list = &sign_priority_default,
	 .ecc_list = &supported_ecc_normal,
	 .profile = GNUTLS_PROFILE_LOW,
	 .sec_param = GNUTLS_SEC_PARAM_WEAK
	},
	{.name = LEVEL_PFS,
	 .cipher_list = &cipher_priority_normal,
	 .mac_list = &mac_priority_secure128,
	 .kx_list = &kx_priority_pfs,
	 .sign_list = &sign_priority_default,
	 .ecc_list = &supported_ecc_normal,
	 .profile = GNUTLS_PROFILE_LOW,
	 .sec_param = GNUTLS_SEC_PARAM_WEAK,
	 .no_tickets = 1
	},
	{.name = LEVEL_SECURE128,
	 .alias = "SECURE",
	 .cipher_list = &cipher_priority_secure128,
	 .mac_list = &mac_priority_secure128,
	 .kx_list = &kx_priority_secure,
	 .sign_list = &sign_priority_secure128,
	 .ecc_list = &supported_ecc_secure128,
		/* The profile should have been HIGH but if we don't allow
		 * SHA-1 (80-bits) as signature algorithm we are not able
		 * to connect anywhere with this level */
	 .profile = GNUTLS_PROFILE_LOW,
	 .sec_param = GNUTLS_SEC_PARAM_LOW
	},
	{.name = LEVEL_SECURE192,
	 .alias = LEVEL_SECURE256,
	 .cipher_list = &cipher_priority_secure192,
	 .mac_list = &mac_priority_secure192,
	 .kx_list = &kx_priority_secure,
	 .sign_list = &sign_priority_secure192,
	 .ecc_list = &supported_ecc_secure192,
	 .profile = GNUTLS_PROFILE_HIGH,
	 .sec_param = GNUTLS_SEC_PARAM_HIGH
	},
	{.name = LEVEL_SUITEB128,
	 .proto_list = &protocol_priority_suiteb,
	 .cipher_list = &cipher_priority_suiteb128,
	 .mac_list = &mac_priority_suiteb,
	 .kx_list = &kx_priority_suiteb,
	 .sign_list = &sign_priority_suiteb128,
	 .ecc_list = &supported_ecc_suiteb128,
	 .profile = GNUTLS_PROFILE_SUITEB128,
	 .sec_param = GNUTLS_SEC_PARAM_HIGH
	},
	{.name = LEVEL_SUITEB192,
	 .proto_list = &protocol_priority_suiteb,
	 .cipher_list = &cipher_priority_suiteb192,
	 .mac_list = &mac_priority_suiteb,
	 .kx_list = &kx_priority_suiteb,
	 .sign_list = &sign_priority_suiteb192,
	 .ecc_list = &supported_ecc_suiteb192,
	 .profile = GNUTLS_PROFILE_SUITEB192,
	 .sec_param = GNUTLS_SEC_PARAM_ULTRA
	},
	{.name = LEVEL_LEGACY,
	 .cipher_list = &cipher_priority_normal,
	 .mac_list = &mac_priority_normal,
	 .kx_list = &kx_priority_secure,
	 .sign_list = &sign_priority_default,
	 .ecc_list = &supported_ecc_normal,
	 .sec_param = GNUTLS_SEC_PARAM_VERY_WEAK
	},
	{.name = LEVEL_PERFORMANCE,
	 .cipher_list = &cipher_priority_performance,
	 .mac_list = &mac_priority_normal,
	 .kx_list = &kx_priority_performance,
	 .sign_list = &sign_priority_default,
	 .ecc_list = &supported_ecc_normal,
	 .profile = GNUTLS_PROFILE_LOW,
	 .sec_param = GNUTLS_SEC_PARAM_WEAK
	},
	{
	 .name = NULL,
	}
};

#define SET_PROFILE(to_set) \
	profile = GNUTLS_VFLAGS_TO_PROFILE(priority_cache->additional_verify_flags); \
	if (profile == 0 || profile > to_set) { \
		priority_cache->additional_verify_flags &= ~GNUTLS_VFLAGS_PROFILE_MASK; \
		priority_cache->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(to_set); \
	}

#define SET_LEVEL(to_set) \
		if (priority_cache->level == 0 || (unsigned)priority_cache->level > (unsigned)to_set) \
			priority_cache->level = to_set

static
int check_level(const char *level, gnutls_priority_t priority_cache,
		int add)
{
	bulk_rmadd_func *func;
	unsigned profile = 0;
	unsigned i;
	int j;
	const cipher_entry_st *centry;

	if (add)
		func = _add_priority;
	else
		func = _set_priority;

	for (i=0;;i++) {
		if (pgroups[i].name == NULL)
			return 0;

		if (strcasecmp(level, pgroups[i].name) == 0 ||
			(pgroups[i].alias != NULL && strcasecmp(level, pgroups[i].alias) == 0)) {
			if (pgroups[i].proto_list != NULL)
				func(&priority_cache->protocol, *pgroups[i].proto_list);
			func(&priority_cache->cipher, *pgroups[i].cipher_list);
			func(&priority_cache->kx, *pgroups[i].kx_list);
			func(&priority_cache->mac, *pgroups[i].mac_list);
			func(&priority_cache->sign_algo, *pgroups[i].sign_list);
			func(&priority_cache->supported_ecc, *pgroups[i].ecc_list);

			if (pgroups[i].profile != 0) {
				SET_PROFILE(pgroups[i].profile); /* set certificate level */
			}
			SET_LEVEL(pgroups[i].sec_param); /* set DH params level */
			priority_cache->no_tickets = pgroups[i].no_tickets;
			if (priority_cache->have_cbc == 0) {
				for (j=0;(*pgroups[i].cipher_list)[j]!=0;j++) {
					centry = cipher_to_entry((*pgroups[i].cipher_list)[j]);
					if (centry != NULL && centry->type == CIPHER_BLOCK) {
						priority_cache->have_cbc = 1;
						break;
					}
				}
			}
			return 1;
		}
	}
}

static void enable_compat(gnutls_priority_t c)
{
	ENABLE_COMPAT(c);
}
static void enable_server_key_usage_violations(gnutls_priority_t c)
{
	c->allow_server_key_usage_violation = 1;
}
static void enable_dumbfw(gnutls_priority_t c)
{
	c->dumbfw = 1;
}
static void enable_no_extensions(gnutls_priority_t c)
{
	c->no_extensions = 1;
}
static void enable_no_ext_master_secret(gnutls_priority_t c)
{
	c->no_ext_master_secret = 1;
}
static void enable_no_etm(gnutls_priority_t c)
{
	c->no_etm = 1;
}
static void enable_no_tickets(gnutls_priority_t c)
{
	c->no_tickets = 1;
}
static void enable_stateless_compression(gnutls_priority_t c)
{
	c->stateless_compression = 1;
}
static void disable_wildcards(gnutls_priority_t c)
{
	c->additional_verify_flags |= GNUTLS_VERIFY_DO_NOT_ALLOW_WILDCARDS;
}
static void enable_profile_very_weak(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_VERY_WEAK);
	c->level = GNUTLS_SEC_PARAM_VERY_WEAK;
}
static void enable_profile_low(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_LOW);
	c->level = GNUTLS_SEC_PARAM_LOW;
}
static void enable_profile_legacy(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_LEGACY);
	c->level = GNUTLS_SEC_PARAM_LEGACY;
}
static void enable_profile_high(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_HIGH);
	c->level = GNUTLS_SEC_PARAM_HIGH;
}
static void enable_profile_ultra(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_ULTRA);
	c->level = GNUTLS_SEC_PARAM_ULTRA;
}
static void enable_profile_medium(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_MEDIUM);
	c->level = GNUTLS_SEC_PARAM_MEDIUM;
}
static void enable_profile_suiteb128(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_SUITEB128);
	c->level = GNUTLS_SEC_PARAM_HIGH;
}
static void enable_profile_suiteb192(gnutls_priority_t c)
{
	c->additional_verify_flags &= 0x00ffffff;
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(GNUTLS_PROFILE_SUITEB192);
	c->level = GNUTLS_SEC_PARAM_ULTRA;
}
static void enable_safe_renegotiation(gnutls_priority_t c)
{
	c->sr = SR_SAFE;

}
static void enable_unsafe_renegotiation(gnutls_priority_t c)
{
	c->sr = SR_UNSAFE;
}
static void enable_partial_safe_renegotiation(gnutls_priority_t c)
{
	c->sr = SR_PARTIAL;
}
static void disable_safe_renegotiation(gnutls_priority_t c)
{
	c->sr = SR_DISABLED;
}
static void enable_fallback_scsv(gnutls_priority_t c)
{
	c->fallback = 1;
}
static void enable_latest_record_version(gnutls_priority_t c)
{
	c->min_record_version = 0;
}
static void enable_ssl3_record_version(gnutls_priority_t c)
{
	c->min_record_version = 1;
}
static void enable_verify_allow_rsa_md5(gnutls_priority_t c)
{
	c->additional_verify_flags |=
	    GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD5;
}
static void disable_crl_checks(gnutls_priority_t c)
{
	c->additional_verify_flags |=
		GNUTLS_VERIFY_DISABLE_CRL_CHECKS;
}
static void enable_server_precedence(gnutls_priority_t c)
{
	c->server_precedence = 1;
}
static void dummy_func(gnutls_priority_t c)
{
}

#include <priority_options.h>

static char *check_str(char *line, size_t line_size, const char *needle, size_t needle_size)
{
	char *p;
	unsigned n;

	while (c_isspace(*line)) {
		line++;
		line_size--;
	}

	if (line[0] == '#' || needle_size >= line_size)
		return NULL;

	if (memcmp(line, needle, needle_size) == 0) {
		p = &line[needle_size];
		while (c_isspace(*p)) {
			p++;
		}
		if (*p != '=') {
			return NULL;
		} else
			p++;

		while (c_isspace(*p)) {
			p++;
		}

		n = strlen(p);

		if (n > 1 && p[n-1] == '\n') {
			n--;
			p[n] = 0;
		}

		if (n > 1 && p[n-1] == '\r') {
			n--;
			p[n] = 0;
		}
		return p;
	}

	return NULL;
}

static const char *system_priority_file = SYSTEM_PRIORITY_FILE;
static char *system_priority_buf = NULL;
static size_t system_priority_buf_size = 0;
static time_t system_priority_last_mod = 0;


static void _gnutls_update_system_priorities(void)
{
#ifdef HAVE_FMEMOPEN
	gnutls_datum_t data;
	int ret;
	struct stat sb;

	if (stat(system_priority_file, &sb) < 0) {
		_gnutls_debug_log("unable to access: %s: %d\n",
				  system_priority_file, errno);
		return;
	}

	if (system_priority_buf != NULL &&
	    sb.st_mtime == system_priority_last_mod) {
		_gnutls_debug_log("system priority %s has not changed\n",
				  system_priority_file);
		return;
	}

	ret = gnutls_load_file(system_priority_file, &data);
	if (ret < 0) {
		_gnutls_debug_log("unable to load: %s: %d\n",
				  system_priority_file, ret);
		return;
	}

	_gnutls_debug_log("cached system priority %s mtime %lld\n",
			  system_priority_file,
			  (unsigned long long)sb.st_mtime);
	gnutls_free(system_priority_buf);
	system_priority_buf = (char*)data.data;
	system_priority_buf_size = data.size;
	system_priority_last_mod = sb.st_mtime;
#endif
}

void _gnutls_load_system_priorities(void)
{
	const char *p;

	p = secure_getenv("GNUTLS_SYSTEM_PRIORITY_FILE");
	if (p != NULL)
		system_priority_file = p;

	_gnutls_update_system_priorities();
}

void _gnutls_unload_system_priorities(void)
{
#ifdef HAVE_FMEMOPEN
	gnutls_free(system_priority_buf);
#endif
	system_priority_buf = NULL;
	system_priority_buf_size = 0;
	system_priority_last_mod = 0;
}

/* Returns the new priorities if a priority string prefixed
 * with '@' is provided, or just a copy of the provided
 * priorities, appended with any additional present in
 * the priorities string.
 *
 * The returned string must be released using free().
 */
char *_gnutls_resolve_priorities(const char* priorities)
{
char *p = (char*)priorities;
char *additional = NULL;
char *ret = NULL;
char *ss, *ss_next, *line = NULL;
unsigned ss_len, ss_next_len;
int l;
FILE* fp = NULL;
size_t n, n2 = 0, line_size;

	while (c_isspace(*p))
		p++;

	if (*p == '@') {
		ss = p+1;
		additional = strchr(ss, ':');
		if (additional != NULL) {
			additional++;
		}

		do {
			ss_next = strchr(ss, ',');
			if (ss_next != NULL) {
				if (additional && ss_next > additional)
					ss_next = NULL;
				else
					ss_next++;
			}

			if (ss_next) {
				ss_len = ss_next - ss - 1;
				ss_next_len = additional - ss_next - 1;
			} else if (additional) {
				ss_len = additional - ss - 1;
				ss_next_len = 0;
			} else {
				ss_len = strlen(ss);
				ss_next_len = 0;
			}

#ifdef HAVE_FMEMOPEN
			/* Always try to refresh the cached data, to
			 * allow it to be updated without restarting
			 * all applications
			 */
			_gnutls_update_system_priorities();
			fp = fmemopen(system_priority_buf, system_priority_buf_size, "r");
#else
			fp = fopen(system_priority_file, "r");
#endif
			if (fp == NULL) {/* fail */
				ret = NULL;
				goto finish;
			}

			do {
				l = getline(&line, &line_size, fp);
				if (l > 0) {
					p = check_str(line, line_size, ss, ss_len);
					if (p != NULL)
						break;
				}
			} while (l>0);

			_gnutls_debug_log("resolved '%.*s' to '%s', next '%.*s'\n",
					  ss_len, ss, p ? : "", ss_next_len, ss_next ? : "");
			ss = ss_next;
			fclose(fp);
			fp = NULL;
		} while (ss && p == NULL);

		if (p == NULL) {
			_gnutls_debug_log("unable to resolve %s\n", priorities);
			ret = NULL;
			goto finish;
		}

		n = strlen(p);
		if (additional)
			n2 = strlen(additional);

		ret = malloc(n+n2+1+1);
		if (ret == NULL) {
			goto finish;
		}

		memcpy(ret, p, n);
		if (additional != NULL) {
			ret[n] = ':';
			memcpy(&ret[n+1], additional, n2);
			ret[n+n2+1] = 0;
		} else {
			ret[n] = 0;
		}
	} else {
		return strdup(p);
	}

finish:
	if (ret != NULL) {
		_gnutls_debug_log("selected priority string: %s\n", ret);
	}
	free(line);
	if (fp != NULL)
		fclose(fp);

	return ret;
}

/**
 * gnutls_priority_init:
 * @priority_cache: is a #gnutls_prioritity_t type.
 * @priorities: is a string describing priorities (may be %NULL)
 * @err_pos: In case of an error this will have the position in the string the error occurred
 *
 * Sets priorities for the ciphers, key exchange methods, macs and
 * compression methods. The @priority_cache should be deinitialized
 * using gnutls_priority_deinit().
 *
 * The #priorities option allows you to specify a colon
 * separated list of the cipher priorities to enable.
 * Some keywords are defined to provide quick access
 * to common preferences.
 *
 * Unless there is a special need, use the "NORMAL" keyword to
 * apply a reasonable security level, or "NORMAL:%%COMPAT" for compatibility.
 *
 * "PERFORMANCE" means all the "secure" ciphersuites are enabled,
 * limited to 128 bit ciphers and sorted by terms of speed
 * performance.
 *
 * "LEGACY" the NORMAL settings for GnuTLS 3.2.x or earlier. There is
 * no verification profile set, and the allowed DH primes are considered
 * weak today.
 *
 * "NORMAL" means all "secure" ciphersuites. The 256-bit ciphers are
 * included as a fallback only.  The ciphers are sorted by security
 * margin.
 *
 * "PFS" means all "secure" ciphersuites that support perfect forward secrecy. 
 * The 256-bit ciphers are included as a fallback only.  
 * The ciphers are sorted by security margin.
 *
 * "SECURE128" means all "secure" ciphersuites of security level 128-bit
 * or more.
 *
 * "SECURE192" means all "secure" ciphersuites of security level 192-bit
 * or more.
 *
 * "SUITEB128" means all the NSA SuiteB ciphersuites with security level
 * of 128.
 *
 * "SUITEB192" means all the NSA SuiteB ciphersuites with security level
 * of 192.
 *
 * "NONE" means nothing is enabled.  This disables even protocols and
 * compression methods.
 *
 * "@@KEYWORD1,KEYWORD2,..." The system administrator imposed settings.
 * The provided keyword(s) will be expanded from a configuration-time
 * provided file - default is: /etc/gnutls/default-priorities.
 * Any attributes that follow it, will be appended to the expanded
 * string. If multiple keywords are provided, separated by commas,
 * then the first keyword that exists in the configuration file
 * will be used. At least one of the keywords must exist, or this
 * function will return an error. Typical usage would be to specify
 * an application specified keyword first, followed by "SYSTEM" as
 * a default fallback. e.g., "@LIBVIRT,SYSTEM:!-VERS-SSL3.0" will
 * first try to find a config file entry matching "LIBVIRT", but if
 * that does not exist will use the entry for "SYSTEM". If "SYSTEM"
 * does not exist either, an error will be returned. In all cases,
 * the SSL3.0 protocol will be disabled. The system priority file
 * entries should be formatted as "KEYWORD=VALUE", e.g.,
 * "SYSTEM=NORMAL:+ARCFOUR-128".
 *
 * Special keywords are "!", "-" and "+".
 * "!" or "-" appended with an algorithm will remove this algorithm.
 * "+" appended with an algorithm will add this algorithm.
 *
 * Check the GnuTLS manual section "Priority strings" for detailed
 * information.
 *
 * Examples:
 *
 * "NONE:+VERS-TLS-ALL:+MAC-ALL:+RSA:+AES-128-CBC:+SIGN-ALL:+COMP-NULL"
 *
 * "NORMAL:+ARCFOUR-128" means normal ciphers plus ARCFOUR-128.
 *
 * "SECURE128:-VERS-SSL3.0:+COMP-DEFLATE" means that only secure ciphers are
 * enabled, SSL3.0 is disabled, and libz compression enabled.
 *
 * "NONE:+VERS-TLS-ALL:+AES-128-CBC:+RSA:+SHA1:+COMP-NULL:+SIGN-RSA-SHA1", 
 *
 * "NONE:+VERS-TLS-ALL:+AES-128-CBC:+ECDHE-RSA:+SHA1:+COMP-NULL:+SIGN-RSA-SHA1:+CURVE-SECP256R1", 
 *
 * "SECURE256:+SECURE128",
 *
 * Note that "NORMAL:%%COMPAT" is the most compatible mode.
 *
 * A %NULL @priorities string indicates the default priorities to be
 * used (this is available since GnuTLS 3.3.0).
 *
 * Returns: On syntax error %GNUTLS_E_INVALID_REQUEST is returned,
 * %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_priority_init(gnutls_priority_t * priority_cache,
		     const char *priorities, const char **err_pos)
{
	char *broken_list[MAX_ELEMENTS];
	int broken_list_size = 0, i = 0, j;
	char *darg = NULL;
	unsigned ikeyword_set = 0;
	int algo;
	rmadd_func *fn;
	bulk_rmadd_func *bulk_fn;
	bulk_rmadd_func *bulk_given_fn;
	const cipher_entry_st *centry;

	if (err_pos)
		*err_pos = priorities;

	*priority_cache =
	    gnutls_calloc(1, sizeof(struct gnutls_priority_st));
	if (*priority_cache == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	/* for now unsafe renegotiation is default on everyone. To be removed
	 * when we make it the default.
	 */
	(*priority_cache)->sr = SR_PARTIAL;
	(*priority_cache)->min_record_version = 1;

	if (priorities == NULL)
		priorities = DEFAULT_PRIORITY_STRING;

	darg = _gnutls_resolve_priorities(priorities);
	if (darg == NULL) {
		gnutls_assert();
		goto error;
	}

	break_list(darg, broken_list, &broken_list_size);
	/* This is our default set of protocol version, certificate types and
	 * compression methods.
	 */
	if (strcasecmp(broken_list[0], LEVEL_NONE) != 0) {
		_set_priority(&(*priority_cache)->protocol,
			      protocol_priority);
		_set_priority(&(*priority_cache)->compression,
			      comp_priority);
		_set_priority(&(*priority_cache)->cert_type,
			      cert_type_priority_default);
		_set_priority(&(*priority_cache)->sign_algo,
			      sign_priority_default);
		_set_priority(&(*priority_cache)->supported_ecc,
			      supported_ecc_normal);
		i = 0;
	} else {
		ikeyword_set = 1;
		i = 1;
	}

	for (; i < broken_list_size; i++) {
		if (check_level(broken_list[i], *priority_cache, ikeyword_set) != 0) {
			ikeyword_set = 1;
			continue;
		} else if (broken_list[i][0] == '!'
			   || broken_list[i][0] == '+'
			   || broken_list[i][0] == '-') {
			if (broken_list[i][0] == '+') {
				fn = prio_add;
				bulk_fn = _add_priority;
				bulk_given_fn = _add_priority;
			} else {
				fn = prio_remove;
				bulk_fn = _clear_priorities;
				bulk_given_fn = _clear_given_priorities;
			}

			if (broken_list[i][0] == '+'
			    && check_level(&broken_list[i][1],
					   *priority_cache, 1) != 0) {
				continue;
			} else if ((algo =
				    gnutls_mac_get_id(&broken_list[i][1]))
				   != GNUTLS_MAC_UNKNOWN) {
				fn(&(*priority_cache)->mac, algo);
			} else if ((centry = cipher_name_to_entry(&broken_list[i][1])) != NULL) {
				if (_gnutls_cipher_exists(centry->id)) {
					fn(&(*priority_cache)->cipher, centry->id);
					if (centry->type == CIPHER_BLOCK)
						(*priority_cache)->have_cbc = 1;
				}
			} else if ((algo =
				  _gnutls_kx_get_id(&broken_list[i][1])) !=
				 GNUTLS_KX_UNKNOWN) {
				if (algo != GNUTLS_KX_INVALID)
					fn(&(*priority_cache)->kx, algo);
			} else if (strncasecmp
				 (&broken_list[i][1], "VERS-", 5) == 0) {
				if (strncasecmp
				    (&broken_list[i][1], "VERS-TLS-ALL",
				     12) == 0) {
					bulk_given_fn(&(*priority_cache)->
						protocol,
						stream_protocol_priority);
				} else if (strncasecmp
					(&broken_list[i][1],
					 "VERS-DTLS-ALL", 13) == 0) {
					bulk_given_fn(&(*priority_cache)->
						protocol,
						(bulk_given_fn==_add_priority)?dtls_protocol_priority:dgram_protocol_priority);
				} else if (strncasecmp
					(&broken_list[i][1],
					 "VERS-ALL", 8) == 0) {
					bulk_fn(&(*priority_cache)->
						protocol,
						protocol_priority);
				} else {
					if ((algo =
					     gnutls_protocol_get_id
					     (&broken_list[i][6])) !=
					    GNUTLS_VERSION_UNKNOWN)
						fn(&(*priority_cache)->
						   protocol, algo);
					else
						goto error;

				}
			} /* now check if the element is something like -ALGO */
			else if (strncasecmp
				 (&broken_list[i][1], "COMP-", 5) == 0) {
				if (strncasecmp
				    (&broken_list[i][1], "COMP-ALL",
				     8) == 0) {
					bulk_fn(&(*priority_cache)->
						compression,
						comp_priority);
				} else {
					if ((algo =
					     gnutls_compression_get_id
					     (&broken_list[i][6])) !=
					    GNUTLS_COMP_UNKNOWN)
						fn(&(*priority_cache)->
						   compression, algo);
					else
						goto error;
				}
			} /* now check if the element is something like -ALGO */
			else if (strncasecmp
				 (&broken_list[i][1], "CURVE-", 6) == 0) {
				if (strncasecmp
				    (&broken_list[i][1], "CURVE-ALL",
				     9) == 0) {
					bulk_fn(&(*priority_cache)->
						supported_ecc,
						supported_ecc_normal);
				} else {
					if ((algo =
					     gnutls_ecc_curve_get_id
					     (&broken_list[i][7])) !=
					    GNUTLS_ECC_CURVE_INVALID)
						fn(&(*priority_cache)->
						   supported_ecc, algo);
					else
						goto error;
				}
			} /* now check if the element is something like -ALGO */
			else if (strncasecmp
				 (&broken_list[i][1], "CTYPE-", 6) == 0) {
				if (strncasecmp
				    (&broken_list[i][1], "CTYPE-ALL",
				     9) == 0) {
					bulk_fn(&(*priority_cache)->
						cert_type,
						cert_type_priority_all);
				} else {
					if ((algo =
					     gnutls_certificate_type_get_id
					     (&broken_list[i][7])) !=
					    GNUTLS_CRT_UNKNOWN)
						fn(&(*priority_cache)->
						   cert_type, algo);
					else
						goto error;
				}
			} /* now check if the element is something like -ALGO */
			else if (strncasecmp
				 (&broken_list[i][1], "SIGN-", 5) == 0) {
				if (strncasecmp
				    (&broken_list[i][1], "SIGN-ALL",
				     8) == 0) {
					bulk_fn(&(*priority_cache)->
						sign_algo,
						sign_priority_default);
				} else {
					if ((algo =
					     gnutls_sign_get_id
					     (&broken_list[i][6])) !=
					    GNUTLS_SIGN_UNKNOWN)
						fn(&(*priority_cache)->
						   sign_algo, algo);
					else
						goto error;
				}
			} else
			    if (strncasecmp
				(&broken_list[i][1], "MAC-ALL", 7) == 0) {
				bulk_fn(&(*priority_cache)->mac,
					mac_priority_normal);
			} else
			    if (strncasecmp
				(&broken_list[i][1], "CIPHER-ALL",
				 10) == 0) {
				bulk_fn(&(*priority_cache)->cipher,
					cipher_priority_normal);
			} else
			    if (strncasecmp
				(&broken_list[i][1], "KX-ALL", 6) == 0) {
				bulk_fn(&(*priority_cache)->kx,
					kx_priority_secure);
			} else
				goto error;
		} else if (broken_list[i][0] == '%') {
			const struct priority_options_st * o;
			/* to add a new option modify
			 * priority_options.gperf */
			o = in_word_set(&broken_list[i][1], strlen(&broken_list[i][1]));
			if (o == NULL) {
				goto error;
			}
			o->func(*priority_cache);
		} else
			goto error;
	}

	free(darg);
	return 0;

      error:
	if (err_pos != NULL && i < broken_list_size) {
		*err_pos = priorities;
		for (j = 0; j < i; j++) {
			(*err_pos) += strlen(broken_list[j]) + 1;
		}
	}
	free(darg);
	gnutls_free(*priority_cache);
	*priority_cache = NULL;

	return GNUTLS_E_INVALID_REQUEST;

}

/**
 * gnutls_priority_deinit:
 * @priority_cache: is a #gnutls_prioritity_t type.
 *
 * Deinitializes the priority cache.
 **/
void gnutls_priority_deinit(gnutls_priority_t priority_cache)
{
	gnutls_free(priority_cache);
}


/**
 * gnutls_priority_set_direct:
 * @session: is a #gnutls_session_t type.
 * @priorities: is a string describing priorities
 * @err_pos: In case of an error this will have the position in the string the error occurred
 *
 * Sets the priorities to use on the ciphers, key exchange methods,
 * macs and compression methods.  This function avoids keeping a
 * priority cache and is used to directly set string priorities to a
 * TLS session.  For documentation check the gnutls_priority_init().
 *
 * To simply use a reasonable default, consider using gnutls_set_default_priority().
 *
 * Returns: On syntax error %GNUTLS_E_INVALID_REQUEST is returned,
 * %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_priority_set_direct(gnutls_session_t session,
			   const char *priorities, const char **err_pos)
{
	gnutls_priority_t prio;
	int ret;

	ret = gnutls_priority_init(&prio, priorities, err_pos);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_priority_set(session, prio);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	gnutls_priority_deinit(prio);

	return 0;
}

/* Breaks a list of "xxx", "yyy", to a character array, of
 * MAX_COMMA_SEP_ELEMENTS size; Note that the given string is modified.
  */
static void
break_list(char *list,
		 char *broken_list[MAX_ELEMENTS], int *size)
{
	char *p = list;

	*size = 0;

	do {
		broken_list[*size] = p;

		(*size)++;

		p = strchr(p, ':');
		if (p) {
			*p = 0;
			p++;	/* move to next entry and skip white
				 * space.
				 */
			while (*p == ' ')
				p++;
		}
	}
	while (p != NULL && *size < MAX_ELEMENTS);
}

/**
 * gnutls_set_default_priority:
 * @session: is a #gnutls_session_t type.
 *
 * Sets the default priority on the ciphers, key exchange methods,
 * macs and compression methods. For more fine-tuning you could
 * use gnutls_priority_set_direct() or gnutls_priority_set() instead.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int gnutls_set_default_priority(gnutls_session_t session)
{
	return gnutls_priority_set_direct(session, NULL, NULL);
}

/**
 * gnutls_priority_ecc_curve_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available elliptic curves in the priority
 * structure. 
 *
 * Returns: the number of curves, or an error code.
 * Since: 3.0
 **/
int
gnutls_priority_ecc_curve_list(gnutls_priority_t pcache,
			       const unsigned int **list)
{
	if (pcache->supported_ecc.algorithms == 0)
		return 0;

	*list = pcache->supported_ecc.priority;
	return pcache->supported_ecc.algorithms;
}

/**
 * gnutls_priority_kx_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available key exchange methods in the priority
 * structure. 
 *
 * Returns: the number of curves, or an error code.
 * Since: 3.2.3
 **/
int
gnutls_priority_kx_list(gnutls_priority_t pcache,
			const unsigned int **list)
{
	if (pcache->kx.algorithms == 0)
		return 0;

	*list = pcache->kx.priority;
	return pcache->kx.algorithms;
}

/**
 * gnutls_priority_cipher_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available ciphers in the priority
 * structure. 
 *
 * Returns: the number of curves, or an error code.
 * Since: 3.2.3
 **/
int
gnutls_priority_cipher_list(gnutls_priority_t pcache,
			    const unsigned int **list)
{
	if (pcache->cipher.algorithms == 0)
		return 0;

	*list = pcache->cipher.priority;
	return pcache->cipher.algorithms;
}

/**
 * gnutls_priority_mac_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available MAC algorithms in the priority
 * structure. 
 *
 * Returns: the number of curves, or an error code.
 * Since: 3.2.3
 **/
int
gnutls_priority_mac_list(gnutls_priority_t pcache,
			 const unsigned int **list)
{
	if (pcache->mac.algorithms == 0)
		return 0;

	*list = pcache->mac.priority;
	return pcache->mac.algorithms;
}

/**
 * gnutls_priority_compression_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available compression method in the priority
 * structure. 
 *
 * Returns: the number of methods, or an error code.
 * Since: 3.0
 **/
int
gnutls_priority_compression_list(gnutls_priority_t pcache,
				 const unsigned int **list)
{
	if (pcache->compression.algorithms == 0)
		return 0;

	*list = pcache->compression.priority;
	return pcache->compression.algorithms;
}

/**
 * gnutls_priority_protocol_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available TLS version numbers in the priority
 * structure. 
 *
 * Returns: the number of protocols, or an error code.
 * Since: 3.0
 **/
int
gnutls_priority_protocol_list(gnutls_priority_t pcache,
			      const unsigned int **list)
{
	if (pcache->protocol.algorithms == 0)
		return 0;

	*list = pcache->protocol.priority;
	return pcache->protocol.algorithms;
}

/**
 * gnutls_priority_sign_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available signature algorithms in the priority
 * structure. 
 *
 * Returns: the number of algorithms, or an error code.
 * Since: 3.0
 **/
int
gnutls_priority_sign_list(gnutls_priority_t pcache,
			  const unsigned int **list)
{
	if (pcache->sign_algo.algorithms == 0)
		return 0;

	*list = pcache->sign_algo.priority;
	return pcache->sign_algo.algorithms;
}

/**
 * gnutls_priority_certificate_type_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available certificate types in the priority
 * structure. 
 *
 * Returns: the number of certificate types, or an error code.
 * Since: 3.0
 **/
int
gnutls_priority_certificate_type_list(gnutls_priority_t pcache,
				      const unsigned int **list)
{
	if (pcache->cert_type.algorithms == 0)
		return 0;

	*list = pcache->cert_type.priority;
	return pcache->cert_type.algorithms;
}

/**
 * gnutls_priority_string_list:
 * @iter: an integer counter starting from zero
 * @flags: one of %GNUTLS_PRIORITY_LIST_INIT_KEYWORDS, %GNUTLS_PRIORITY_LIST_SPECIAL
 *
 * Can be used to iterate all available priority strings.
 * Due to internal implementation details, there are cases where this
 * function can return the empty string. In that case that string should be ignored.
 * When no strings are available it returns %NULL.
 *
 * Returns: a priority string
 * Since: 3.4.0
 **/
const char *
gnutls_priority_string_list(unsigned iter, unsigned int flags)
{
	if (flags & GNUTLS_PRIORITY_LIST_INIT_KEYWORDS) {
		if (iter >= (sizeof(pgroups)/sizeof(pgroups[0]))-1)
			return NULL;
		return pgroups[iter].name;
	} else if (flags & GNUTLS_PRIORITY_LIST_SPECIAL) {
		if (iter >= (sizeof(wordlist)/sizeof(wordlist[0]))-1)
			return NULL;
		return wordlist[iter].name;
	}
	return NULL;
}
