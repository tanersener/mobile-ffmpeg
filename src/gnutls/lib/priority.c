/*
 * Copyright (C) 2004-2015 Free Software Foundation, Inc.
 * Copyright (C) 2015-2019 Red Hat, Inc.
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

/* Here lies the code of the gnutls_*_set_priority() functions.
 */

#include "gnutls_int.h"
#include "algorithms.h"
#include "errors.h"
#include <num.h>
#include <gnutls/x509.h>
#include <c-ctype.h>
#include <hello_ext.h>
#include <c-strcase.h>
#include "fips.h"
#include "errno.h"
#include "ext/srp.h"
#include <gnutls/gnutls.h>
#include "profiles.h"
#include "c-strcase.h"

#define MAX_ELEMENTS 64

#define ENABLE_PROFILE(c, profile) do { \
	c->additional_verify_flags &= 0x00ffffff; \
	c->additional_verify_flags |= GNUTLS_PROFILE_TO_VFLAGS(profile); \
	c->level = _gnutls_profile_to_sec_level(profile); \
	} while(0)

/* This function is used by the test suite */
char *_gnutls_resolve_priorities(const char* priorities);
const char *_gnutls_default_priority_string = DEFAULT_PRIORITY_STRING;

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
	st->num_priorities = num;

	for (i = 0; i < num; i++) {
		st->priorities[i] = list[i];
	}

	return;
}

inline static void _add_priority(priority_st * st, const int *list)
{
	int num, i, j, init;

	init = i = st->num_priorities;

	for (num = 0; list[num] != 0; ++num) {
		if (i + 1 > MAX_ALGOS) {
			return;
		}

		for (j = 0; j < init; j++) {
			if (st->priorities[j] == (unsigned) list[num]) {
				break;
			}
		}

		if (j == init) {
			st->priorities[i++] = list[num];
			st->num_priorities++;
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

static const int _supported_groups_dh[] = {
	GNUTLS_GROUP_FFDHE2048,
	GNUTLS_GROUP_FFDHE3072,
	GNUTLS_GROUP_FFDHE4096,
	GNUTLS_GROUP_FFDHE6144,
	GNUTLS_GROUP_FFDHE8192,
	0
};

static const int _supported_groups_ecdh[] = {
	GNUTLS_GROUP_SECP256R1,
	GNUTLS_GROUP_SECP384R1,
	GNUTLS_GROUP_SECP521R1,
	GNUTLS_GROUP_X25519, /* draft-ietf-tls-rfc4492bis */
	0
};

static const int _supported_groups_normal[] = {
	GNUTLS_GROUP_SECP256R1,
	GNUTLS_GROUP_SECP384R1,
	GNUTLS_GROUP_SECP521R1,
	GNUTLS_GROUP_X25519, /* draft-ietf-tls-rfc4492bis */

	/* These should stay last as our default behavior
	 * is to send key shares for two top types (GNUTLS_KEY_SHARE_TOP2)
	 * and we wouldn't want to have these sent by all clients
	 * by default as they are quite expensive CPU-wise. */
	GNUTLS_GROUP_FFDHE2048,
	GNUTLS_GROUP_FFDHE3072,
	GNUTLS_GROUP_FFDHE4096,
	GNUTLS_GROUP_FFDHE6144,
	GNUTLS_GROUP_FFDHE8192,
	0
};
static const int* supported_groups_normal = _supported_groups_normal;

static const int _supported_groups_secure128[] = {
	GNUTLS_GROUP_SECP256R1,
	GNUTLS_GROUP_SECP384R1,
	GNUTLS_GROUP_SECP521R1,
	GNUTLS_GROUP_X25519, /* draft-ietf-tls-rfc4492bis */
	GNUTLS_GROUP_FFDHE2048,
	GNUTLS_GROUP_FFDHE3072,
	GNUTLS_GROUP_FFDHE4096,
	GNUTLS_GROUP_FFDHE6144,
	GNUTLS_GROUP_FFDHE8192,
	0
};
static const int* supported_groups_secure128 = _supported_groups_secure128;

static const int _supported_groups_suiteb128[] = {
	GNUTLS_GROUP_SECP256R1,
	GNUTLS_GROUP_SECP384R1,
	0
};
static const int* supported_groups_suiteb128 = _supported_groups_suiteb128;

static const int _supported_groups_suiteb192[] = {
	GNUTLS_GROUP_SECP384R1,
	0
};
static const int* supported_groups_suiteb192 = _supported_groups_suiteb192;

static const int _supported_groups_secure192[] = {
	GNUTLS_GROUP_SECP384R1,
	GNUTLS_GROUP_SECP521R1,
	GNUTLS_GROUP_FFDHE8192,
	0
};
static const int* supported_groups_secure192 = _supported_groups_secure192;

static const int protocol_priority[] = {
	GNUTLS_TLS1_3,
	GNUTLS_TLS1_2,
	GNUTLS_TLS1_1,
	GNUTLS_TLS1_0,
	GNUTLS_DTLS1_2,
	GNUTLS_DTLS1_0,
	0
};

/* contains all the supported TLS protocols, intended to be used for eliminating them
 */
static const int stream_protocol_priority[] = {
	GNUTLS_TLS1_3,
	GNUTLS_TLS1_2,
	GNUTLS_TLS1_1,
	GNUTLS_TLS1_0,
	0
};

/* contains all the supported DTLS protocols, intended to be used for eliminating them
 */
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
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_256_CBC,
	0
};

static const int _cipher_priority_performance_no_aesni[] = {
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_AES_128_CCM,
	GNUTLS_CIPHER_AES_256_CCM,
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_256_CBC,
	0
};

/* If GCM and AES acceleration is available then prefer
 * them over anything else. Overall we prioritise AEAD
 * over legacy ciphers, and 256-bit over 128 (for future
 * proof).
 */
static const int _cipher_priority_normal_default[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_256_CCM,

	GNUTLS_CIPHER_AES_256_CBC,

	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_128_CCM,

	GNUTLS_CIPHER_AES_128_CBC,
	0
};

static const int cipher_priority_performance_fips[] = {
	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_128_CCM,
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_AES_256_CCM,

	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_256_CBC,
	0
};

static const int cipher_priority_normal_fips[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_AES_256_CCM,
	GNUTLS_CIPHER_AES_256_CBC,

	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_128_CCM,
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
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_AES_256_CCM,

	GNUTLS_CIPHER_AES_128_GCM,
	GNUTLS_CIPHER_AES_128_CBC,
	GNUTLS_CIPHER_AES_128_CCM,
	0
};
static const int *cipher_priority_secure128 = _cipher_priority_secure128;


static const int _cipher_priority_secure192[] = {
	GNUTLS_CIPHER_AES_256_GCM,
	GNUTLS_CIPHER_CHACHA20_POLY1305,
	GNUTLS_CIPHER_AES_256_CBC,
	GNUTLS_CIPHER_AES_256_CCM,
	0
};
static const int* cipher_priority_secure192 = _cipher_priority_secure192;

static const int _sign_priority_default[] = {
	GNUTLS_SIGN_RSA_SHA256,
	GNUTLS_SIGN_RSA_PSS_SHA256,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA256,
	GNUTLS_SIGN_ECDSA_SHA256,
	GNUTLS_SIGN_ECDSA_SECP256R1_SHA256,

	GNUTLS_SIGN_EDDSA_ED25519,

	GNUTLS_SIGN_RSA_SHA384,
	GNUTLS_SIGN_RSA_PSS_SHA384,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA384,
	GNUTLS_SIGN_ECDSA_SHA384,
	GNUTLS_SIGN_ECDSA_SECP384R1_SHA384,

	GNUTLS_SIGN_RSA_SHA512,
	GNUTLS_SIGN_RSA_PSS_SHA512,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA512,

	GNUTLS_SIGN_ECDSA_SHA512,
	GNUTLS_SIGN_ECDSA_SECP521R1_SHA512,

	GNUTLS_SIGN_RSA_SHA1,
	GNUTLS_SIGN_ECDSA_SHA1,

	0
};
static const int* sign_priority_default = _sign_priority_default;

static const int _sign_priority_suiteb128[] = {
	GNUTLS_SIGN_ECDSA_SHA256,
	GNUTLS_SIGN_ECDSA_SECP256R1_SHA256,
	GNUTLS_SIGN_ECDSA_SHA384,
	GNUTLS_SIGN_ECDSA_SECP384R1_SHA384,
	0
};
static const int* sign_priority_suiteb128 = _sign_priority_suiteb128;

static const int _sign_priority_suiteb192[] = {
	GNUTLS_SIGN_ECDSA_SHA384,
	GNUTLS_SIGN_ECDSA_SECP384R1_SHA384,
	0
};
static const int* sign_priority_suiteb192 = _sign_priority_suiteb192;

static const int _sign_priority_secure128[] = {
	GNUTLS_SIGN_RSA_SHA256,
	GNUTLS_SIGN_RSA_PSS_SHA256,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA256,
	GNUTLS_SIGN_ECDSA_SHA256,
	GNUTLS_SIGN_ECDSA_SECP256R1_SHA256,
	GNUTLS_SIGN_EDDSA_ED25519,

	GNUTLS_SIGN_RSA_SHA384,
	GNUTLS_SIGN_RSA_PSS_SHA384,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA384,
	GNUTLS_SIGN_ECDSA_SHA384,
	GNUTLS_SIGN_ECDSA_SECP384R1_SHA384,

	GNUTLS_SIGN_RSA_SHA512,
	GNUTLS_SIGN_RSA_PSS_SHA512,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA512,
	GNUTLS_SIGN_ECDSA_SHA512,
	GNUTLS_SIGN_ECDSA_SECP521R1_SHA512,

	0
};
static const int* sign_priority_secure128 = _sign_priority_secure128;

static const int _sign_priority_secure192[] = {
	GNUTLS_SIGN_RSA_SHA384,
	GNUTLS_SIGN_RSA_PSS_SHA384,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA384,
	GNUTLS_SIGN_ECDSA_SHA384,
	GNUTLS_SIGN_ECDSA_SECP384R1_SHA384,
	GNUTLS_SIGN_RSA_SHA512,
	GNUTLS_SIGN_RSA_PSS_SHA512,
	GNUTLS_SIGN_RSA_PSS_RSAE_SHA512,
	GNUTLS_SIGN_ECDSA_SHA512,
	GNUTLS_SIGN_ECDSA_SECP521R1_SHA512,

	0
};
static const int* sign_priority_secure192 = _sign_priority_secure192;

static const int mac_priority_normal_default[] = {
	GNUTLS_MAC_SHA1,
	GNUTLS_MAC_AEAD,
	0
};

static const int mac_priority_normal_fips[] = {
	GNUTLS_MAC_SHA1,
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
	GNUTLS_MAC_AEAD,
	0
};
static const int* mac_priority_secure128 = _mac_priority_secure128;

static const int _mac_priority_secure192[] = {
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
	GNUTLS_CRT_RAWPK,
	0
};

typedef void (rmadd_func) (priority_st * priority_list, unsigned int alg);

static void prio_remove(priority_st * priority_list, unsigned int algo)
{
	unsigned int i;

	for (i = 0; i < priority_list->num_priorities; i++) {
		if (priority_list->priorities[i] == algo) {
			priority_list->num_priorities--;
			if ((priority_list->num_priorities - i) > 0)
				memmove(&priority_list->priorities[i],
					&priority_list->priorities[i + 1],
					(priority_list->num_priorities -
					 i) *
					sizeof(priority_list->
					       priorities[0]));
			priority_list->priorities[priority_list->
						num_priorities] = 0;
			break;
		}
	}

	return;
}

static void prio_add(priority_st * priority_list, unsigned int algo)
{
	unsigned int i, l = priority_list->num_priorities;

	if (l >= MAX_ALGOS)
		return;		/* can't add it anyway */

	for (i = 0; i < l; ++i) {
		if (algo == priority_list->priorities[i])
			return;	/* if it exists */
	}

	priority_list->priorities[l] = algo;
	priority_list->num_priorities++;

	return;
}


/**
 * gnutls_priority_set:
 * @session: is a #gnutls_session_t type.
 * @priority: is a #gnutls_priority_t type.
 *
 * Sets the priorities to use on the ciphers, key exchange methods,
 * and macs. Note that this function is expected to be called once
 * per session; when called multiple times (e.g., before a re-handshake,
 * the caller should make sure that any new settings are not incompatible
 * with the original session).
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code on error.
 **/
int
gnutls_priority_set(gnutls_session_t session, gnutls_priority_t priority)
{
	int ret;

	if (priority == NULL || priority->protocol.num_priorities == 0 ||
	    priority->cs.size == 0)
		return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);

	/* set the current version to the first in the chain, if this is
	 * the call before the initial handshake. During a re-handshake
	 * we do not set the version to avoid overriding the currently
	 * negotiated version. */
	if (!session->internals.handshake_in_progress &&
	    !session->internals.initial_negotiation_completed) {
		ret = _gnutls_set_current_version(session,
						  priority->protocol.priorities[0]);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	/* At this point the provided priorities passed the sanity tests */

	if (session->internals.priorities)
		gnutls_priority_deinit(session->internals.priorities);

	gnutls_atomic_increment(&priority->usage_cnt);
	session->internals.priorities = priority;

	if (priority->no_tickets != 0) {
		/* when PFS is explicitly requested, disable session tickets */
		session->internals.flags |= GNUTLS_NO_TICKETS;
	}

	ADD_PROFILE_VFLAGS(session, priority->additional_verify_flags);

	/* mirror variables */
#undef COPY_TO_INTERNALS
#define COPY_TO_INTERNALS(xx) session->internals.xx = priority->_##xx
	COPY_TO_INTERNALS(allow_large_records);
	COPY_TO_INTERNALS(allow_small_records);
	COPY_TO_INTERNALS(no_etm);
	COPY_TO_INTERNALS(no_ext_master_secret);
	COPY_TO_INTERNALS(allow_key_usage_violation);
	COPY_TO_INTERNALS(allow_wrong_pms);
	COPY_TO_INTERNALS(dumbfw);
	COPY_TO_INTERNALS(dh_prime_bits);

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
	const int **group_list;
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
	 .group_list = &supported_groups_normal,
	 .profile = GNUTLS_PROFILE_LOW,
	 .sec_param = GNUTLS_SEC_PARAM_WEAK
	},
	{.name = LEVEL_PFS,
	 .cipher_list = &cipher_priority_normal,
	 .mac_list = &mac_priority_secure128,
	 .kx_list = &kx_priority_pfs,
	 .sign_list = &sign_priority_default,
	 .group_list = &supported_groups_normal,
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
	 .group_list = &supported_groups_secure128,
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
	 .group_list = &supported_groups_secure192,
	 .profile = GNUTLS_PROFILE_HIGH,
	 .sec_param = GNUTLS_SEC_PARAM_HIGH
	},
	{.name = LEVEL_SUITEB128,
	 .proto_list = &protocol_priority_suiteb,
	 .cipher_list = &cipher_priority_suiteb128,
	 .mac_list = &mac_priority_suiteb,
	 .kx_list = &kx_priority_suiteb,
	 .sign_list = &sign_priority_suiteb128,
	 .group_list = &supported_groups_suiteb128,
	 .profile = GNUTLS_PROFILE_SUITEB128,
	 .sec_param = GNUTLS_SEC_PARAM_HIGH
	},
	{.name = LEVEL_SUITEB192,
	 .proto_list = &protocol_priority_suiteb,
	 .cipher_list = &cipher_priority_suiteb192,
	 .mac_list = &mac_priority_suiteb,
	 .kx_list = &kx_priority_suiteb,
	 .sign_list = &sign_priority_suiteb192,
	 .group_list = &supported_groups_suiteb192,
	 .profile = GNUTLS_PROFILE_SUITEB192,
	 .sec_param = GNUTLS_SEC_PARAM_ULTRA
	},
	{.name = LEVEL_LEGACY,
	 .cipher_list = &cipher_priority_normal,
	 .mac_list = &mac_priority_normal,
	 .kx_list = &kx_priority_secure,
	 .sign_list = &sign_priority_default,
	 .group_list = &supported_groups_normal,
	 .sec_param = GNUTLS_SEC_PARAM_VERY_WEAK
	},
	{.name = LEVEL_PERFORMANCE,
	 .cipher_list = &cipher_priority_performance,
	 .mac_list = &mac_priority_normal,
	 .kx_list = &kx_priority_performance,
	 .sign_list = &sign_priority_default,
	 .group_list = &supported_groups_normal,
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

		if (c_strcasecmp(level, pgroups[i].name) == 0 ||
			(pgroups[i].alias != NULL && c_strcasecmp(level, pgroups[i].alias) == 0)) {
			if (pgroups[i].proto_list != NULL)
				func(&priority_cache->protocol, *pgroups[i].proto_list);
			func(&priority_cache->_cipher, *pgroups[i].cipher_list);
			func(&priority_cache->_kx, *pgroups[i].kx_list);
			func(&priority_cache->_mac, *pgroups[i].mac_list);
			func(&priority_cache->_sign_algo, *pgroups[i].sign_list);
			func(&priority_cache->_supported_ecc, *pgroups[i].group_list);

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
	ENABLE_PRIO_COMPAT(c);
}
static void enable_server_key_usage_violations(gnutls_priority_t c)
{
	c->allow_server_key_usage_violation = 1;
}
static void enable_allow_small_records(gnutls_priority_t c)
{
	c->_allow_small_records = 1;
}
static void enable_dumbfw(gnutls_priority_t c)
{
	c->_dumbfw = 1;
}
static void enable_no_extensions(gnutls_priority_t c)
{
	c->no_extensions = 1;
}
static void enable_no_ext_master_secret(gnutls_priority_t c)
{
	c->_no_ext_master_secret = 1;
}
static void enable_no_etm(gnutls_priority_t c)
{
	c->_no_etm = 1;
}
static void enable_force_etm(gnutls_priority_t c)
{
	c->force_etm = 1;
}
static void enable_no_tickets(gnutls_priority_t c)
{
	c->no_tickets = 1;
}
static void disable_wildcards(gnutls_priority_t c)
{
	c->additional_verify_flags |= GNUTLS_VERIFY_DO_NOT_ALLOW_WILDCARDS;
}
static void enable_profile_very_weak(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_VERY_WEAK);
}
static void enable_profile_low(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_LOW);
}
static void enable_profile_legacy(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_LEGACY);
}
static void enable_profile_medium(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_MEDIUM);
}
static void enable_profile_high(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_HIGH);
}
static void enable_profile_ultra(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_ULTRA);
}
static void enable_profile_future(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_FUTURE);
}
static void enable_profile_suiteb128(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_SUITEB128);
}
static void enable_profile_suiteb192(gnutls_priority_t c)
{
	ENABLE_PROFILE(c, GNUTLS_PROFILE_SUITEB128);
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
static void enable_verify_allow_sha1(gnutls_priority_t c)
{
	c->additional_verify_flags |=
	    GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1;
}
static void enable_verify_allow_broken(gnutls_priority_t c)
{
	c->additional_verify_flags |=
	    GNUTLS_VERIFY_ALLOW_BROKEN;
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

#define S(str) ((str!=NULL)?str:"")

/* Returns the new priorities if a priority string prefixed
 * with '@' is provided, or just a copy of the provided
 * priorities, appended with any additional present in
 * the priorities string.
 *
 * The returned string must be released using gnutls_free().
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
					  ss_len, ss, S(p), ss_next_len, S(ss_next));
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

		ret = gnutls_malloc(n+n2+1+1);
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
		return gnutls_strdup(p);
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

static void add_ec(gnutls_priority_t priority_cache)
{
	const gnutls_group_entry_st *ge;
	unsigned i;

	for (i = 0; i < priority_cache->_supported_ecc.num_priorities; i++) {
		ge = _gnutls_id_to_group(priority_cache->_supported_ecc.priorities[i]);
		if (ge != NULL && priority_cache->groups.size < sizeof(priority_cache->groups.entry)/sizeof(priority_cache->groups.entry[0])) {
			/* do not add groups which do not correspond to enabled ciphersuites */
			if (!ge->curve)
				continue;
			priority_cache->groups.entry[priority_cache->groups.size++] = ge;
		}
	}
}

static void add_dh(gnutls_priority_t priority_cache)
{
	const gnutls_group_entry_st *ge;
	unsigned i;

	for (i = 0; i < priority_cache->_supported_ecc.num_priorities; i++) {
		ge = _gnutls_id_to_group(priority_cache->_supported_ecc.priorities[i]);
		if (ge != NULL && priority_cache->groups.size < sizeof(priority_cache->groups.entry)/sizeof(priority_cache->groups.entry[0])) {
			/* do not add groups which do not correspond to enabled ciphersuites */
			if (!ge->prime)
				continue;
			priority_cache->groups.entry[priority_cache->groups.size++] = ge;
			priority_cache->groups.have_ffdhe = 1;
		}
	}
}

static int set_ciphersuite_list(gnutls_priority_t priority_cache)
{
	unsigned i, j, z;
	const gnutls_cipher_suite_entry_st *ce;
	const gnutls_sign_entry_st *se;
	unsigned have_ec = 0;
	unsigned have_dh = 0;
	unsigned tls_sig_sem = 0;
	const version_entry_st *tlsmax = NULL, *vers;
	const version_entry_st *dtlsmax = NULL;
	const version_entry_st *tlsmin = NULL;
	const version_entry_st *dtlsmin = NULL;
	unsigned have_tls13 = 0, have_srp = 0;
	unsigned have_pre_tls12 = 0, have_tls12 = 0;
	unsigned have_psk = 0, have_null = 0, have_rsa_psk = 0;

	/* have_psk indicates that a PSK key exchange compatible
	 * with TLS1.3 is enabled. */

	priority_cache->cs.size = 0;
	priority_cache->sigalg.size = 0;
	priority_cache->groups.size = 0;
	priority_cache->groups.have_ffdhe = 0;

	for (j=0;j<priority_cache->_cipher.num_priorities;j++) {
		if (priority_cache->_cipher.priorities[j] == GNUTLS_CIPHER_NULL) {
			have_null = 1;
			break;
		}
	}

	for (i = 0; i < priority_cache->_kx.num_priorities; i++) {
		if (IS_SRP_KX(priority_cache->_kx.priorities[i])) {
			have_srp = 1;
		} else if (_gnutls_kx_is_psk(priority_cache->_kx.priorities[i])) {
			if (priority_cache->_kx.priorities[i] == GNUTLS_KX_RSA_PSK)
				have_rsa_psk = 1;
			else
				have_psk = 1;
		}
	}

	/* if we have NULL ciphersuites, SRP, or RSA-PSK enabled remove TLS1.3+
	 * protocol versions; they cannot be negotiated under TLS1.3. */
	if (have_null || have_srp || have_rsa_psk || priority_cache->no_extensions) {
		for (i = j = 0; i < priority_cache->protocol.num_priorities; i++) {
			vers = version_to_entry(priority_cache->protocol.priorities[i]);
			if (!vers || !vers->tls13_sem)
				priority_cache->protocol.priorities[j++] = priority_cache->protocol.priorities[i];
		}
		priority_cache->protocol.num_priorities = j;
	}

	for (i = 0; i < priority_cache->protocol.num_priorities; i++) {
		vers = version_to_entry(priority_cache->protocol.priorities[i]);
		if (!vers)
			continue;

		if (vers->transport == GNUTLS_STREAM) { /* TLS */
			tls_sig_sem |= vers->tls_sig_sem;
			if (vers->tls13_sem)
				have_tls13 = 1;

			if (vers->id == GNUTLS_TLS1_2)
				have_tls12 = 1;
			else if (vers->id < GNUTLS_TLS1_2)
				have_pre_tls12 = 1;

			if (tlsmax == NULL || vers->age > tlsmax->age)
				tlsmax = vers;
			if (tlsmin == NULL || vers->age < tlsmin->age)
				tlsmin = vers;
		} else { /* dtls */
			tls_sig_sem |= vers->tls_sig_sem;

			/* we need to introduce similar handling to above
			 * when DTLS1.3 is supported */

			if (dtlsmax == NULL || vers->age > dtlsmax->age)
				dtlsmax = vers;
			if (dtlsmin == NULL || vers->age < dtlsmin->age)
				dtlsmin = vers;
		}
	}

	/* DTLS or TLS protocols must be present */
	if ((!tlsmax || !tlsmin) && (!dtlsmax || !dtlsmin))
		return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);


	priority_cache->have_psk = have_psk;

	/* if we are have TLS1.3+ do not enable any key exchange algorithms,
	 * the protocol doesn't require any. */
	if (tlsmin && tlsmin->tls13_sem && !have_psk) {
		if (!dtlsmin || (dtlsmin && dtlsmin->tls13_sem))
			priority_cache->_kx.num_priorities = 0;
	}

	/* Add TLS 1.3 ciphersuites (no KX) */
	for (j=0;j<priority_cache->_cipher.num_priorities;j++) {
		for (z=0;z<priority_cache->_mac.num_priorities;z++) {
			ce = cipher_suite_get(
				0, priority_cache->_cipher.priorities[j],
				priority_cache->_mac.priorities[z]);

			if (ce != NULL && priority_cache->cs.size < MAX_CIPHERSUITE_SIZE) {
				priority_cache->cs.entry[priority_cache->cs.size++] = ce;
			}
		}
	}

	for (i = 0; i < priority_cache->_kx.num_priorities; i++) {
		for (j=0;j<priority_cache->_cipher.num_priorities;j++) {
			for (z=0;z<priority_cache->_mac.num_priorities;z++) {
				ce = cipher_suite_get(
					priority_cache->_kx.priorities[i],
					priority_cache->_cipher.priorities[j],
					priority_cache->_mac.priorities[z]);

				if (ce != NULL && priority_cache->cs.size < MAX_CIPHERSUITE_SIZE) {
					priority_cache->cs.entry[priority_cache->cs.size++] = ce;
					if (!have_ec && _gnutls_kx_is_ecc(ce->kx_algorithm)) {
						have_ec = 1;
						add_ec(priority_cache);
					}
					if (!have_dh && _gnutls_kx_is_dhe(ce->kx_algorithm)) {
						have_dh = 1;
						add_dh(priority_cache);
					}
				}
			}
		}
	}

	if (have_tls13 && (!have_ec || !have_dh)) {
		/* scan groups to determine have_ec and have_dh */
		for (i=0; i < priority_cache->_supported_ecc.num_priorities; i++) {
			const gnutls_group_entry_st *ge;
			ge = _gnutls_id_to_group(priority_cache->_supported_ecc.priorities[i]);
			if (ge) {
				if (ge->curve && !have_ec) {
					add_ec(priority_cache);
					have_ec = 1;
				} else if (ge->prime && !have_dh) {
					add_dh(priority_cache);
					have_dh = 1;
				}

				if (have_dh && have_ec)
					break;
			}
		}

	}

	for (i = 0; i < priority_cache->_sign_algo.num_priorities; i++) {
		se = _gnutls_sign_to_entry(priority_cache->_sign_algo.priorities[i]);
		if (se != NULL && priority_cache->sigalg.size < sizeof(priority_cache->sigalg.entry)/sizeof(priority_cache->sigalg.entry[0])) {
			/* if the signature algorithm semantics are not compatible with
			 * the protocol's, then skip. */
			if ((se->aid.tls_sem & tls_sig_sem) == 0)
				continue;
			priority_cache->sigalg.entry[priority_cache->sigalg.size++] = se;
		}
	}

	_gnutls_debug_log("added %d protocols, %d ciphersuites, %d sig algos and %d groups into priority list\n",
			  priority_cache->protocol.num_priorities,
			  priority_cache->cs.size, priority_cache->sigalg.size,
			  priority_cache->groups.size);

	if (priority_cache->sigalg.size == 0) {
		/* no signature algorithms; eliminate TLS 1.2 or DTLS 1.2 and later */
		priority_st newp;
		newp.num_priorities = 0;

		/* we need to eliminate TLS 1.2 or DTLS 1.2 and later protocols */
		for (i = 0; i < priority_cache->protocol.num_priorities; i++) {
			if (priority_cache->protocol.priorities[i] < GNUTLS_TLS1_2) {
				newp.priorities[newp.num_priorities++] = priority_cache->protocol.priorities[i];
			} else if (priority_cache->protocol.priorities[i] >= GNUTLS_DTLS_VERSION_MIN &&
				   priority_cache->protocol.priorities[i] < GNUTLS_DTLS1_2) {
				newp.priorities[newp.num_priorities++] = priority_cache->protocol.priorities[i];
			}
		}
		memcpy(&priority_cache->protocol, &newp, sizeof(newp));
	}

	if (unlikely(priority_cache->protocol.num_priorities == 0))
		return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);
#ifndef ENABLE_SSL3
	else if (unlikely(priority_cache->protocol.num_priorities == 1 && priority_cache->protocol.priorities[0] == GNUTLS_SSL3))
		return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);
#endif

	if (unlikely(priority_cache->cs.size == 0))
		return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);

	/* when TLS 1.3 is available we must have groups set; additionally
	 * we require TLS1.2 to be enabled if TLS1.3 is asked for, and
	 * a pre-TLS1.2 protocol is there; that is because servers which
	 * do not support TLS1.3 will negotiate TLS1.2 if seen a TLS1.3 handshake */
	if (unlikely((!have_psk && tlsmax && tlsmax->id >= GNUTLS_TLS1_3 && priority_cache->groups.size == 0)) ||
	    (!have_tls12 && have_pre_tls12 && have_tls13)) {
		for (i = j = 0; i < priority_cache->protocol.num_priorities; i++) {
			vers = version_to_entry(priority_cache->protocol.priorities[i]);
			if (!vers || vers->transport != GNUTLS_STREAM || !vers->tls13_sem)
				priority_cache->protocol.priorities[j++] = priority_cache->protocol.priorities[i];
		}
		priority_cache->protocol.num_priorities = j;
	}

	return 0;
}

/**
 * gnutls_priority_init2:
 * @priority_cache: is a #gnutls_prioritity_t type.
 * @priorities: is a string describing priorities (may be %NULL)
 * @err_pos: In case of an error this will have the position in the string the error occurred
 * @flags: zero or %GNUTLS_PRIORITY_INIT_DEF_APPEND
 *
 * Sets priorities for the ciphers, key exchange methods, and macs.
 * The @priority_cache should be deinitialized
 * using gnutls_priority_deinit().
 *
 * The #priorities option allows you to specify a colon
 * separated list of the cipher priorities to enable.
 * Some keywords are defined to provide quick access
 * to common preferences.
 *
 * When @flags is set to %GNUTLS_PRIORITY_INIT_DEF_APPEND then the @priorities
 * specified will be appended to the default options.
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
 * "NONE" means nothing is enabled.  This disables everything, including protocols.
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
 * "SECURE128:-VERS-SSL3.0" means that only secure ciphers are
 * and enabled, SSL3.0 is disabled.
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
 *
 * Since: 3.6.3
 **/
int
gnutls_priority_init2(gnutls_priority_t * priority_cache,
		      const char *priorities, const char **err_pos,
		      unsigned flags)
{
	gnutls_buffer_st buf;
	const char *ep;
	int ret;

	if (flags & GNUTLS_PRIORITY_INIT_DEF_APPEND) {
		if (priorities == NULL)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		if (err_pos)
			*err_pos = priorities;

		_gnutls_buffer_init(&buf);

		ret = _gnutls_buffer_append_str(&buf, _gnutls_default_priority_string);
		if (ret < 0) {
			_gnutls_buffer_clear(&buf);
			return gnutls_assert_val(ret);
		}

		ret = _gnutls_buffer_append_str(&buf, ":");
		if (ret < 0) {
			_gnutls_buffer_clear(&buf);
			return gnutls_assert_val(ret);
		}

		ret = _gnutls_buffer_append_str(&buf, priorities);
		if (ret < 0) {
			_gnutls_buffer_clear(&buf);
			return gnutls_assert_val(ret);
		}

		ret = gnutls_priority_init(priority_cache, (const char*)buf.data, &ep);
		if (ret < 0 && ep != (const char*)buf.data && ep != NULL) {
			ptrdiff_t diff = (ptrdiff_t)ep-(ptrdiff_t)buf.data;
			unsigned hlen = strlen(_gnutls_default_priority_string)+1;

			if (err_pos && diff > hlen) {
				*err_pos = priorities + diff - hlen;
			}
		}
		_gnutls_buffer_clear(&buf);
		return ret;
	} else {
		return gnutls_priority_init(priority_cache, priorities, err_pos);
	}
}

/**
 * gnutls_priority_init:
 * @priority_cache: is a #gnutls_prioritity_t type.
 * @priorities: is a string describing priorities (may be %NULL)
 * @err_pos: In case of an error this will have the position in the string the error occurred
 *
 * For applications that do not modify their crypto settings per release, consider
 * using gnutls_priority_init2() with %GNUTLS_PRIORITY_INIT_DEF_APPEND flag
 * instead. We suggest to use centralized crypto settings handled by the GnuTLS
 * library, and applications modifying the default settings to their needs.
 *
 * This function is identical to gnutls_priority_init2() with zero
 * flags.
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
	int ret;
	rmadd_func *fn;
	bulk_rmadd_func *bulk_fn;
	bulk_rmadd_func *bulk_given_fn;
	const cipher_entry_st *centry;
	unsigned resolved_match = 1;

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
	gnutls_atomic_init(&(*priority_cache)->usage_cnt);

	if (priorities == NULL) {
		priorities = _gnutls_default_priority_string;
		resolved_match = 0;
	}

	darg = _gnutls_resolve_priorities(priorities);
	if (darg == NULL) {
		gnutls_assert();
		goto error;
	}

	if (strcmp(darg, priorities) != 0)
		resolved_match = 0;

	break_list(darg, broken_list, &broken_list_size);
	/* This is our default set of protocol version, certificate types.
	 */
	if (c_strcasecmp(broken_list[0], LEVEL_NONE) != 0) {
		_set_priority(&(*priority_cache)->protocol,
			      protocol_priority);
		_set_priority(&(*priority_cache)->client_ctype,
			      cert_type_priority_default);
		_set_priority(&(*priority_cache)->server_ctype,
			      cert_type_priority_default);
		_set_priority(&(*priority_cache)->_sign_algo,
			      sign_priority_default);
		_set_priority(&(*priority_cache)->_supported_ecc,
			      supported_groups_normal);
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
				fn(&(*priority_cache)->_mac, algo);
			} else if ((centry = cipher_name_to_entry(&broken_list[i][1])) != NULL) {
				if (_gnutls_cipher_exists(centry->id)) {
					fn(&(*priority_cache)->_cipher, centry->id);
					if (centry->type == CIPHER_BLOCK)
						(*priority_cache)->have_cbc = 1;
				}
			} else if ((algo =
				  _gnutls_kx_get_id(&broken_list[i][1])) !=
				 GNUTLS_KX_UNKNOWN) {
				if (algo != GNUTLS_KX_INVALID)
					fn(&(*priority_cache)->_kx, algo);
			} else if (c_strncasecmp
				 (&broken_list[i][1], "VERS-", 5) == 0) {
				if (c_strncasecmp
				    (&broken_list[i][1], "VERS-TLS-ALL",
				     12) == 0) {
					bulk_given_fn(&(*priority_cache)->
						protocol,
						stream_protocol_priority);
				} else if (c_strncasecmp
					(&broken_list[i][1],
					 "VERS-DTLS-ALL", 13) == 0) {
					bulk_given_fn(&(*priority_cache)->
						protocol,
						(bulk_given_fn==_add_priority)?dtls_protocol_priority:dgram_protocol_priority);
				} else if (c_strncasecmp
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
			else if (c_strncasecmp
				 (&broken_list[i][1], "COMP-", 5) == 0) {
				/* ignore all compression methods */
				continue;
			} /* now check if the element is something like -ALGO */
			else if (c_strncasecmp
				 (&broken_list[i][1], "CURVE-", 6) == 0) {
				if (c_strncasecmp
				    (&broken_list[i][1], "CURVE-ALL",
				     9) == 0) {
					bulk_fn(&(*priority_cache)->
						_supported_ecc,
						supported_groups_normal);
				} else {
					if ((algo =
					     gnutls_ecc_curve_get_id
					     (&broken_list[i][7])) !=
					    GNUTLS_ECC_CURVE_INVALID)
						fn(&(*priority_cache)->
						   _supported_ecc, algo);
					else
						goto error;
				}
			} else if (c_strncasecmp
				 (&broken_list[i][1], "GROUP-", 6) == 0) {
				if (c_strncasecmp
				    (&broken_list[i][1], "GROUP-ALL",
				     9) == 0) {
					bulk_fn(&(*priority_cache)->
						_supported_ecc,
						supported_groups_normal);
				} else if (strncasecmp
				    (&broken_list[i][1], "GROUP-DH-ALL",
				     12) == 0) {
					bulk_given_fn(&(*priority_cache)->
						_supported_ecc,
						_supported_groups_dh);
				} else if (strncasecmp
				    (&broken_list[i][1], "GROUP-EC-ALL",
				     12) == 0) {
					bulk_given_fn(&(*priority_cache)->
						_supported_ecc,
						_supported_groups_ecdh);
				} else {
					if ((algo =
					     gnutls_group_get_id
					     (&broken_list[i][7])) !=
					    GNUTLS_GROUP_INVALID)
						fn(&(*priority_cache)->
						   _supported_ecc, algo);
					else
						goto error;
				}
			} else if (strncasecmp(&broken_list[i][1], "CTYPE-", 6) == 0) {
				// Certificate types
				if (strncasecmp(&broken_list[i][1], "CTYPE-ALL", 9) == 0) {
					// Symmetric cert types, all types allowed
					bulk_fn(&(*priority_cache)->client_ctype,
						cert_type_priority_all);
					bulk_fn(&(*priority_cache)->server_ctype,
						cert_type_priority_all);
				} else if (strncasecmp(&broken_list[i][1], "CTYPE-CLI-", 10) == 0) {
					// Client certificate types
					if (strncasecmp(&broken_list[i][1], "CTYPE-CLI-ALL", 13) == 0) {
						// All client cert types allowed
						bulk_fn(&(*priority_cache)->client_ctype,
							cert_type_priority_all);
					} else if ((algo = gnutls_certificate_type_get_id
							(&broken_list[i][11])) != GNUTLS_CRT_UNKNOWN) {
						// Specific client cert type allowed
						fn(&(*priority_cache)->client_ctype, algo);
					} else goto error;
				} else if (strncasecmp(&broken_list[i][1], "CTYPE-SRV-", 10) == 0) {
					// Server certificate types
					if (strncasecmp(&broken_list[i][1], "CTYPE-SRV-ALL", 13) == 0) {
						// All server cert types allowed
						bulk_fn(&(*priority_cache)->server_ctype,
							cert_type_priority_all);
					} else if ((algo = gnutls_certificate_type_get_id
							(&broken_list[i][11])) != GNUTLS_CRT_UNKNOWN) {
							// Specific server cert type allowed
						fn(&(*priority_cache)->server_ctype, algo);
					} else goto error;
				} else { // Symmetric certificate type
					if ((algo = gnutls_certificate_type_get_id
					     (&broken_list[i][7])) != GNUTLS_CRT_UNKNOWN) {
						fn(&(*priority_cache)->client_ctype, algo);
						fn(&(*priority_cache)->server_ctype, algo);
					} else if (strncasecmp(&broken_list[i][1], "CTYPE-OPENPGP", 13) == 0) {
						/* legacy openpgp option - ignore */
						continue;
					} else goto error;
				}
			} else if (strncasecmp
				 (&broken_list[i][1], "SIGN-", 5) == 0) {
				if (strncasecmp
				    (&broken_list[i][1], "SIGN-ALL",
				     8) == 0) {
					bulk_fn(&(*priority_cache)->
						_sign_algo,
						sign_priority_default);
				} else {
					if ((algo =
					     gnutls_sign_get_id
					     (&broken_list[i][6])) !=
					    GNUTLS_SIGN_UNKNOWN)
						fn(&(*priority_cache)->
						   _sign_algo, algo);
					else
						goto error;
				}
			} else if (c_strncasecmp
				(&broken_list[i][1], "MAC-ALL", 7) == 0) {
				bulk_fn(&(*priority_cache)->_mac,
					mac_priority_normal);
			} else if (c_strncasecmp
				(&broken_list[i][1], "CIPHER-ALL",
				 10) == 0) {
				bulk_fn(&(*priority_cache)->_cipher,
					cipher_priority_normal);
			} else if (c_strncasecmp
				(&broken_list[i][1], "KX-ALL", 6) == 0) {
				bulk_fn(&(*priority_cache)->_kx,
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

	ret = set_ciphersuite_list(*priority_cache);
	if (ret < 0) {
		if (err_pos)
			*err_pos = priorities;
		goto error_cleanup;
	}

	gnutls_free(darg);

	return 0;

 error:
	if (err_pos != NULL && i < broken_list_size && resolved_match) {
		*err_pos = priorities;
		for (j = 0; j < i; j++) {
			(*err_pos) += strlen(broken_list[j]) + 1;
		}
	}
	ret = GNUTLS_E_INVALID_REQUEST;

 error_cleanup:
	free(darg);
	gnutls_priority_deinit(*priority_cache);
	*priority_cache = NULL;

	return ret;
}

/**
 * gnutls_priority_deinit:
 * @priority_cache: is a #gnutls_prioritity_t type.
 *
 * Deinitializes the priority cache.
 **/
void gnutls_priority_deinit(gnutls_priority_t priority_cache)
{
	if (priority_cache == NULL)
		return;

	/* Note that here we care about the following two cases:
	 * 1. Multiple sessions or different threads holding a reference + a global reference
	 * 2. One session holding a reference with a possible global reference
	 *
	 * As such, it will never be that two threads reach the
	 * zero state at the same time, unless the global reference
	 * is cleared too, which is invalid state.
	 */
	if (gnutls_atomic_val(&priority_cache->usage_cnt) == 0) {
		gnutls_atomic_deinit(&priority_cache->usage_cnt);
		gnutls_free(priority_cache);
		return;
	} else {
		gnutls_atomic_decrement(&priority_cache->usage_cnt);
	}
}


/**
 * gnutls_priority_set_direct:
 * @session: is a #gnutls_session_t type.
 * @priorities: is a string describing priorities
 * @err_pos: In case of an error this will have the position in the string the error occurred
 *
 * Sets the priorities to use on the ciphers, key exchange methods,
 * and macs.  This function avoids keeping a
 * priority cache and is used to directly set string priorities to a
 * TLS session.  For documentation check the gnutls_priority_init().
 *
 * To use a reasonable default, consider using gnutls_set_default_priority(),
 * or gnutls_set_default_priority_append() instead of this function.
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

	/* ensure that the session holds the only reference for the struct */
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
 * and macs. This is the recommended method of
 * setting the defaults, in order to promote consistency between applications
 * using GnuTLS, and to allow GnuTLS using applications to update settings
 * in par with the library. For client applications which require
 * maximum compatibility consider calling gnutls_session_enable_compatibility_mode()
 * after this function.
 *
 * For an application to specify additional options to priority string
 * consider using gnutls_set_default_priority_append().
 *
 * To allow a user to override the defaults (e.g., when a user interface
 * or configuration file is available), the functions
 * gnutls_priority_set_direct() or gnutls_priority_set() can
 * be used.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 *
 * Since: 2.1.4
 **/
int gnutls_set_default_priority(gnutls_session_t session)
{
	return gnutls_priority_set_direct(session, NULL, NULL);
}

/**
 * gnutls_set_default_priority_append:
 * @session: is a #gnutls_session_t type.
 * @add_prio: is a string describing priorities to be appended to default
 * @err_pos: In case of an error this will have the position in the string the error occurred
 * @flags: must be zero
 *
 * Sets the default priority on the ciphers, key exchange methods,
 * and macs with the additional options in @add_prio. This is the recommended method of
 * setting the defaults when only few additional options are to be added. This promotes
 * consistency between applications using GnuTLS, and allows GnuTLS using applications
 * to update settings in par with the library.
 *
 * The @add_prio string should start as a normal priority string, e.g.,
 * '-VERS-TLS-ALL:+VERS-TLS1.3:%%COMPAT' or '%%FORCE_ETM'. That is, it must not start
 * with ':'.
 *
 * To allow a user to override the defaults (e.g., when a user interface
 * or configuration file is available), the functions
 * gnutls_priority_set_direct() or gnutls_priority_set() can
 * be used.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 *
 * Since: 3.6.3
 **/
int gnutls_set_default_priority_append(gnutls_session_t session,
				       const char *add_prio,
				       const char **err_pos,
				       unsigned flags)
{
	gnutls_priority_t prio;
	int ret;

	ret = gnutls_priority_init2(&prio, add_prio, err_pos, GNUTLS_PRIORITY_INIT_DEF_APPEND);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_priority_set(session, prio);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* ensure that the session holds the only reference for the struct */
	gnutls_priority_deinit(prio);

	return 0;
}

/**
 * gnutls_priority_ecc_curve_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available elliptic curves in the priority
 * structure.
 *
 * Deprecated: This function has been replaced by
 * gnutls_priority_group_list() since 3.6.0.
 *
 * Returns: the number of items, or an error code.
 *
 * Since: 3.0
 **/
int
gnutls_priority_ecc_curve_list(gnutls_priority_t pcache,
			       const unsigned int **list)
{
	unsigned i;

	if (pcache->_supported_ecc.num_priorities == 0)
		return 0;

	*list = pcache->_supported_ecc.priorities;

	/* to ensure we don't confuse the caller, we do not include
	 * any FFDHE groups. This may return an incomplete list. */
	for (i=0;i<pcache->_supported_ecc.num_priorities;i++)
		if (pcache->_supported_ecc.priorities[i] > GNUTLS_ECC_CURVE_MAX)
			return i;

	return pcache->_supported_ecc.num_priorities;
}

/**
 * gnutls_priority_group_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available groups in the priority
 * structure.
 *
 * Returns: the number of items, or an error code.
 *
 * Since: 3.6.0
 **/
int
gnutls_priority_group_list(gnutls_priority_t pcache,
			       const unsigned int **list)
{
	if (pcache->_supported_ecc.num_priorities == 0)
		return 0;

	*list = pcache->_supported_ecc.priorities;
	return pcache->_supported_ecc.num_priorities;
}

/**
 * gnutls_priority_kx_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available key exchange methods in the priority
 * structure.
 *
 * Returns: the number of items, or an error code.
 * Since: 3.2.3
 **/
int
gnutls_priority_kx_list(gnutls_priority_t pcache,
			const unsigned int **list)
{
	if (pcache->_kx.num_priorities == 0)
		return 0;

	*list = pcache->_kx.priorities;
	return pcache->_kx.num_priorities;
}

/**
 * gnutls_priority_cipher_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available ciphers in the priority
 * structure.
 *
 * Returns: the number of items, or an error code.
 * Since: 3.2.3
 **/
int
gnutls_priority_cipher_list(gnutls_priority_t pcache,
			    const unsigned int **list)
{
	if (pcache->_cipher.num_priorities == 0)
		return 0;

	*list = pcache->_cipher.priorities;
	return pcache->_cipher.num_priorities;
}

/**
 * gnutls_priority_mac_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available MAC algorithms in the priority
 * structure.
 *
 * Returns: the number of items, or an error code.
 * Since: 3.2.3
 **/
int
gnutls_priority_mac_list(gnutls_priority_t pcache,
			 const unsigned int **list)
{
	if (pcache->_mac.num_priorities == 0)
		return 0;

	*list = pcache->_mac.priorities;
	return pcache->_mac.num_priorities;
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
	static const unsigned int priority[1] = {GNUTLS_COMP_NULL};

	*list = priority;
	return 1;
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
	if (pcache->protocol.num_priorities == 0)
		return 0;

	*list = pcache->protocol.priorities;
	return pcache->protocol.num_priorities;
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
	if (pcache->_sign_algo.num_priorities == 0)
		return 0;

	*list = pcache->_sign_algo.priorities;
	return pcache->_sign_algo.num_priorities;
}

/**
 * gnutls_priority_certificate_type_list:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list
 *
 * Get a list of available certificate types in the priority
 * structure.
 *
 * As of version 3.6.4 this function is an alias for
 * gnutls_priority_certificate_type_list2 with the target parameter
 * set to:
 * - GNUTLS_CTYPE_SERVER, if the %SERVER_PRECEDENCE option is set
 * - GNUTLS_CTYPE_CLIENT, otherwise.
 *
 * Returns: the number of certificate types, or an error code.
 * Since: 3.0
 **/
int
gnutls_priority_certificate_type_list(gnutls_priority_t pcache,
				      const unsigned int **list)
{
	gnutls_ctype_target_t target =
		pcache->server_precedence ? GNUTLS_CTYPE_SERVER : GNUTLS_CTYPE_CLIENT;

	return gnutls_priority_certificate_type_list2(pcache, list, target);
}

/**
 * gnutls_priority_certificate_type_list2:
 * @pcache: is a #gnutls_prioritity_t type.
 * @list: will point to an integer list.
 * @target: is a #gnutls_ctype_target_t type. Valid arguments are
 *   GNUTLS_CTYPE_CLIENT and GNUTLS_CTYPE_SERVER
 *
 * Get a list of available certificate types for the given target
 * in the priority structure.
 *
 * Returns: the number of certificate types, or an error code.
 *
 * Since: 3.6.4
 **/
int
gnutls_priority_certificate_type_list2(gnutls_priority_t pcache,
				      const unsigned int **list, gnutls_ctype_target_t target)
{
	switch (target)	{
		case GNUTLS_CTYPE_CLIENT:
			if(pcache->client_ctype.num_priorities > 0) {
				*list = pcache->client_ctype.priorities;
				return pcache->client_ctype.num_priorities;
			}
			break;
		case GNUTLS_CTYPE_SERVER:
			if(pcache->server_ctype.num_priorities > 0)	{
				*list = pcache->server_ctype.priorities;
				return pcache->server_ctype.num_priorities;
			}
			break;
		default:
			// Invalid target given
			gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	// Found a matching target but non of them had any ctypes set
	return 0;
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
