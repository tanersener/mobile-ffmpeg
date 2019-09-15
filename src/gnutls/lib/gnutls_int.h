/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2018 Red Hat, Inc.
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

#ifndef GNUTLS_LIB_GNUTLS_INT_H
#define GNUTLS_LIB_GNUTLS_INT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifdef NO_SSIZE_T
#define HAVE_SSIZE_T
typedef int ssize_t;
#endif

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#include <time.h>

#include <nettle/memxor.h>

#define ENABLE_ALIGN16

#ifdef __clang_major
# define _GNUTLS_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#else
# define _GNUTLS_CLANG_VERSION 0
#endif

/* clang also defines __GNUC__. It promotes a GCC version of 4.2.1. */
#ifdef __GNUC__
# define _GNUTLS_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if _GNUTLS_GCC_VERSION >= 30100
# define likely(x)      __builtin_expect((x), 1)
# define unlikely(x)    __builtin_expect((x), 0)
#else
# define likely
# define unlikely
#endif

#if _GNUTLS_GCC_VERSION >= 30300
# define attr_nonnull_all __attribute__ ((nonnull))
# define attr_nonnull(a)  __attribute__ ((nonnull a))
#else
# define attr_nonnull_all
# define attr_nonnull(a)
#endif

#if _GNUTLS_GCC_VERSION >= 30400 && (_GNUTLS_CLANG_VERSION == 0 || _GNUTLS_CLANG_VERSION >= 40000)
# define attr_warn_unused_result __attribute__((warn_unused_result))
#else
# define attr_warn_unused_result
#endif

#if _GNUTLS_GCC_VERSION >= 70100
# define FALLTHROUGH __attribute__ ((fallthrough))
#else
# define FALLTHROUGH
#endif


/* some systems had problems with long long int, thus,
 * it is not used.
 */
typedef struct {
	unsigned char i[8];
} gnutls_uint64;

#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <gnutls/abstract.h>
#include <gnutls/socket.h>
#include <system.h>

/* in case we compile with system headers taking priority, we
 * make sure that some new attributes are still available.
 */
#ifndef __GNUTLS_CONST__
# define __GNUTLS_CONST__
#endif

/* The size of a handshake message should not
 * be larger than this value.
 */
#define MAX_HANDSHAKE_PACKET_SIZE 128*1024

#define GNUTLS_DEF_SESSION_ID_SIZE 32

/* The maximum digest size of hash algorithms.
 */
#define MAX_FILENAME 512
#define MAX_HASH_SIZE 64

#define MAX_MAC_KEY_SIZE 64

#define MAX_CIPHER_BLOCK_SIZE 64 /* CHACHA20 */
#define MAX_CIPHER_KEY_SIZE 32

#define MAX_CIPHER_IV_SIZE 16

#define MAX_USERNAME_SIZE 128
#define MAX_SERVER_NAME_SIZE 256

#define AEAD_EXPLICIT_DATA_SIZE 8
#define AEAD_IMPLICIT_DATA_SIZE 4

#define GNUTLS_MASTER_SIZE 48
#define GNUTLS_RANDOM_SIZE 32

/* Under TLS1.3 a hello retry request is sent as server hello */
#define REAL_HSK_TYPE(t) ((t)==GNUTLS_HANDSHAKE_HELLO_RETRY_REQUEST?GNUTLS_HANDSHAKE_SERVER_HELLO:t)

/* DTLS */
#define DTLS_RETRANS_TIMEOUT 1000

/* TLS Extensions */
/* we can receive up to MAX_EXT_TYPES extensions.
 */
#define MAX_EXT_TYPES 32

/* TLS-internal extension (will be parsed after a ciphersuite is selected).
 * This amends the gnutls_ext_parse_type_t. Not exported yet to allow more refining
 * prior to finalizing an API. */
#define _GNUTLS_EXT_TLS_POST_CS 177

/* expire time for resuming sessions */
#define DEFAULT_EXPIRE_TIME 21600
#define STEK_ROTATION_PERIOD_PRODUCT 3
#define DEFAULT_HANDSHAKE_TIMEOUT_MS 40*1000

/* The EC group to be used when the extension
 * supported groups/curves is not present */
#define DEFAULT_EC_GROUP GNUTLS_GROUP_SECP256R1

typedef enum transport_t {
	GNUTLS_STREAM,
	GNUTLS_DGRAM
} transport_t;

/* The TLS 1.3 stage of handshake */
typedef enum hs_stage_t {
	STAGE_HS,
	STAGE_APP,
	STAGE_UPD_OURS,
	STAGE_UPD_PEERS,
	STAGE_EARLY
} hs_stage_t;

typedef enum record_send_state_t {
	RECORD_SEND_NORMAL = 0,
	RECORD_SEND_CORKED, /* corked and transition to NORMAL afterwards */
	RECORD_SEND_CORKED_TO_KU, /* corked but must transition to RECORD_SEND_KEY_UPDATE_1 */
	RECORD_SEND_KEY_UPDATE_1,
	RECORD_SEND_KEY_UPDATE_2,
	RECORD_SEND_KEY_UPDATE_3
} record_send_state_t;

/* The mode check occurs a lot throughout GnuTLS and can be replaced by
 * the following shorter macro. Also easier to update one macro
 * in the future when the internal structure changes than all the conditionals
 * itself.
 */
#define IS_SERVER(session) (session->security_parameters.entity == GNUTLS_SERVER)

/* To check whether we have a DTLS session */
#define IS_DTLS(session) (session->internals.transport == GNUTLS_DGRAM)

/* the maximum size of encrypted packets */
#define DEFAULT_MAX_RECORD_SIZE 16384
#define DEFAULT_MAX_EARLY_DATA_SIZE 16384
#define TLS_RECORD_HEADER_SIZE 5
#define DTLS_RECORD_HEADER_SIZE (TLS_RECORD_HEADER_SIZE+8)
#define RECORD_HEADER_SIZE(session) (IS_DTLS(session) ? DTLS_RECORD_HEADER_SIZE : TLS_RECORD_HEADER_SIZE)
#define MAX_RECORD_HEADER_SIZE DTLS_RECORD_HEADER_SIZE

#define MIN_RECORD_SIZE 512
#define MIN_RECORD_SIZE_SMALL 64

/* The following macro is used to calculate the overhead when sending.
 * when receiving we use a different way as there are implementations that
 * store more data than allowed.
 */
#define MAX_RECORD_SEND_OVERHEAD(session) (MAX_CIPHER_BLOCK_SIZE/*iv*/+MAX_PAD_SIZE+MAX_HASH_SIZE/*MAC*/)
#define MAX_RECORD_SEND_SIZE(session) (IS_DTLS(session)? \
	(MIN((size_t)gnutls_dtls_get_mtu(session), (size_t)session->security_parameters.max_record_send_size+MAX_RECORD_SEND_OVERHEAD(session))): \
	((size_t)session->security_parameters.max_record_send_size+MAX_RECORD_SEND_OVERHEAD(session)))
#define MAX_PAD_SIZE 255
#define EXTRA_COMP_SIZE 2048

#define TLS_HANDSHAKE_HEADER_SIZE 4
#define DTLS_HANDSHAKE_HEADER_SIZE (TLS_HANDSHAKE_HEADER_SIZE+8)
#define HANDSHAKE_HEADER_SIZE(session) (IS_DTLS(session) ? DTLS_HANDSHAKE_HEADER_SIZE : TLS_HANDSHAKE_HEADER_SIZE)
#define MAX_HANDSHAKE_HEADER_SIZE DTLS_HANDSHAKE_HEADER_SIZE

/* Maximum seed size for provable parameters */
#define MAX_PVP_SEED_SIZE 256

/* This is the maximum handshake message size we send without
   fragmentation. This currently ignores record layer overhead. */
#define DTLS_DEFAULT_MTU 1200

/* the maximum size of the DTLS cookie */
#define DTLS_MAX_COOKIE_SIZE 32

/* The maximum number of HELLO_VERIFY_REQUEST messages the client
   processes before aborting. */
#define MAX_HANDSHAKE_HELLO_VERIFY_REQUESTS 5

#define MAX_PK_PARAM_SIZE 2048

/* defaults for verification functions
 */
#define DEFAULT_MAX_VERIFY_DEPTH 16
#define DEFAULT_MAX_VERIFY_BITS (MAX_PK_PARAM_SIZE*8)
#define MAX_VERIFY_DEPTH 4096

#include <mem.h>

#define MEMSUB(x,y) ((ssize_t)((ptrdiff_t)x-(ptrdiff_t)y))

#define DECR_LEN(len, x) do { len-=x; if (len<0) {gnutls_assert(); return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;} } while (0)
#define DECR_LEN_FINAL(len, x) do { \
	len-=x; \
	if (len != 0) \
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH); \
	} while (0)
#define DECR_LENGTH_RET(len, x, RET) do { len-=x; if (len<0) {gnutls_assert(); return RET;} } while (0)
#define DECR_LENGTH_COM(len, x, COM) do { len-=x; if (len<0) {gnutls_assert(); COM;} } while (0)

#define GNUTLS_POINTER_TO_INT(_) ((int) GNUTLS_POINTER_TO_INT_CAST (_))
#define GNUTLS_INT_TO_POINTER(_) ((void*) GNUTLS_POINTER_TO_INT_CAST (_))

#define GNUTLS_KX_INVALID (-1)

typedef struct {
	uint8_t pint[3];
} uint24;

#include <mpi.h>

typedef enum handshake_state_t { STATE0 = 0, STATE1, STATE2,
	STATE3, STATE4, STATE5, STATE6, STATE7, STATE8,
	STATE9, STATE10, STATE11, STATE12, STATE13, STATE14,
	STATE15, STATE16, STATE17, STATE18, STATE19,
	STATE20 = 20, STATE21, STATE22,
	STATE30 = 30, STATE31, STATE40 = 40, STATE41, STATE50 = 50,
	STATE90=90, STATE91, STATE92, STATE93, STATE94, STATE99=99,
	STATE100=100, STATE101, STATE102, STATE103, STATE104,
	STATE105, STATE106, STATE107, STATE108, STATE109, STATE110,
	STATE111, STATE112, STATE113, STATE114, STATE115,
	STATE150 /* key update */
} handshake_state_t;

typedef enum bye_state_t {
	BYE_STATE0 = 0, BYE_STATE1, BYE_STATE2
} bye_state_t;

typedef enum send_ticket_state_t {
	TICKET_STATE0 = 0, TICKET_STATE1
} send_ticket_state_t;

typedef enum reauth_state_t {
	REAUTH_STATE0 = 0, REAUTH_STATE1, REAUTH_STATE2, REAUTH_STATE3,
	REAUTH_STATE4, REAUTH_STATE5
} reauth_state_t;

#define TICKET_STATE session->internals.ticket_state
#define BYE_STATE session->internals.bye_state
#define REAUTH_STATE session->internals.reauth_state

typedef enum heartbeat_state_t {
	SHB_SEND1 = 0,
	SHB_SEND2,
	SHB_RECV
} heartbeat_state_t;

typedef enum recv_state_t {
	RECV_STATE_0 = 0,
	RECV_STATE_DTLS_RETRANSMIT,
	/* client-side false start state */
	RECV_STATE_FALSE_START_HANDLING, /* we are calling gnutls_handshake() within record_recv() */
	RECV_STATE_FALSE_START, /* gnutls_record_recv() should complete the handshake */
	/* async handshake msg state */
	RECV_STATE_ASYNC_HANDSHAKE, /* an incomplete async handshake message was seen */
	/* server-side early start under TLS1.3; enabled when no client cert is received */
	RECV_STATE_EARLY_START_HANDLING, /* we are calling gnutls_handshake() within record_recv() */
	RECV_STATE_EARLY_START, /* gnutls_record_recv() should complete the handshake */
	RECV_STATE_REHANDSHAKE, /* gnutls_record_recv() should complete any incoming re-handshake requests */
	RECV_STATE_REAUTH /* gnutls_record_recv() should complete any incoming reauthentication requests */
} recv_state_t;

#include "str.h"

/* This is the maximum number of algorithms (ciphers or macs etc).
 * keep it synced with GNUTLS_MAX_ALGORITHM_NUM in gnutls.h
 */
#define MAX_ALGOS GNUTLS_MAX_ALGORITHM_NUM

/* IDs are allocated in a way that all values fit in 64-bit integer as (1<<val) */
typedef enum extensions_t {
	GNUTLS_EXTENSION_INVALID = 0xffff,
	GNUTLS_EXTENSION_STATUS_REQUEST = 0,
	GNUTLS_EXTENSION_CERT_TYPE,
	GNUTLS_EXTENSION_CLIENT_CERT_TYPE,
	GNUTLS_EXTENSION_SERVER_CERT_TYPE,
	GNUTLS_EXTENSION_SUPPORTED_GROUPS,
	GNUTLS_EXTENSION_SUPPORTED_EC_POINT_FORMATS,
	GNUTLS_EXTENSION_SRP,
	GNUTLS_EXTENSION_SIGNATURE_ALGORITHMS,
	GNUTLS_EXTENSION_SRTP,
	GNUTLS_EXTENSION_HEARTBEAT,
	GNUTLS_EXTENSION_ALPN,
	GNUTLS_EXTENSION_ETM,
	GNUTLS_EXTENSION_EXT_MASTER_SECRET,
	GNUTLS_EXTENSION_SESSION_TICKET,
	GNUTLS_EXTENSION_KEY_SHARE,
	GNUTLS_EXTENSION_SUPPORTED_VERSIONS,
	GNUTLS_EXTENSION_POST_HANDSHAKE,
	GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
	GNUTLS_EXTENSION_SERVER_NAME,
	GNUTLS_EXTENSION_COOKIE,
	GNUTLS_EXTENSION_EARLY_DATA,
	GNUTLS_EXTENSION_PSK_KE_MODES,
	GNUTLS_EXTENSION_RECORD_SIZE_LIMIT,
	GNUTLS_EXTENSION_MAX_RECORD_SIZE,
	/*
	 * pre_shared_key and dumbfw must always be the last extensions,
	 * in that order */
	GNUTLS_EXTENSION_DUMBFW,
	GNUTLS_EXTENSION_PRE_SHARED_KEY,
	GNUTLS_EXTENSION_MAX /* not real extension - used for iterators */
} extensions_t;

#define GNUTLS_EXTENSION_MAX_VALUE 31
#define ext_track_t uint32_t

#if GNUTLS_EXTENSION_MAX >= GNUTLS_EXTENSION_MAX_VALUE
# error over limit
#endif

#if GNUTLS_EXTENSION_MAX >= MAX_EXT_TYPES
# error over limit
#endif

/* we must provide at least 16 extensions for users to register */
#if GNUTLS_EXTENSION_MAX_VALUE - GNUTLS_EXTENSION_MAX < 16
# error not enough extension types; increase GNUTLS_EXTENSION_MAX_VALUE, MAX_EXT_TYPES and used_exts type
#endif


typedef enum { CIPHER_STREAM, CIPHER_BLOCK, CIPHER_AEAD } cipher_type_t;

#define RESUME_TRUE 1
#define RESUME_FALSE 0

/* Record Protocol */
typedef enum content_type_t {
	GNUTLS_CHANGE_CIPHER_SPEC = 20, GNUTLS_ALERT,
	GNUTLS_HANDSHAKE, GNUTLS_APPLICATION_DATA,
	GNUTLS_HEARTBEAT
} content_type_t;


#define GNUTLS_PK_ANY (gnutls_pk_algorithm_t)-1
#define GNUTLS_PK_NONE (gnutls_pk_algorithm_t)-2

#define GNUTLS_PK_IS_RSA(pk) ((pk) == GNUTLS_PK_RSA || (pk) == GNUTLS_PK_RSA_PSS)

/* Message buffers (mbuffers) structures */

/* this is actually the maximum number of distinct handshake
 * messages that can arrive in a single flight
 */
#define MAX_HANDSHAKE_MSGS 6
typedef struct {
	/* Handshake layer type and sequence of message */
	gnutls_handshake_description_t htype;

	/* The "real" type received; that is, it does not distinguish
	 * HRR from server hello, while htype does */
	gnutls_handshake_description_t rtype;
	uint32_t length;

	/* valid in DTLS */
	uint16_t sequence;

	/* indicate whether that message is complete.
	 * complete means start_offset == 0 and end_offset == length
	 */
	uint32_t start_offset;
	uint32_t end_offset;

	uint8_t header[MAX_HANDSHAKE_HEADER_SIZE];
	int header_size;

	gnutls_buffer_st data;
} handshake_buffer_st;

typedef struct mbuffer_st {
	/* when used in mbuffer_head_st */
	struct mbuffer_st *next;
	struct mbuffer_st *prev;

	/* msg->size - mark = number of bytes left to process in this
	   message. Mark should only be non-zero when this buffer is the
	   head of the queue. */
	size_t mark;


	/* the data */
	gnutls_datum_t msg;
	size_t maximum_size;

	/* used during fill in, to separate header from data
	 * body. */
	unsigned int uhead_mark;

	/* Filled in by record layer on recv:
	 * type, record_sequence
	 */

	/* record layer content type */
	content_type_t type;

	/* record layer sequence */
	gnutls_uint64 record_sequence;

	/* Filled in by handshake layer on send:
	 * type, epoch, htype, handshake_sequence
	 */

	/* Record layer epoch of message */
	uint16_t epoch;

	/* Handshake layer type and sequence of message */
	gnutls_handshake_description_t htype;
	uint16_t handshake_sequence;
} mbuffer_st;

typedef struct mbuffer_head_st {
	mbuffer_st *head;
	mbuffer_st *tail;

	unsigned int length;
	size_t byte_length;
} mbuffer_head_st;

/* Store & Retrieve functions defines:
 */

typedef struct auth_cred_st {
	gnutls_credentials_type_t algorithm;

	/* the type of credentials depends on algorithm
	 */
	void *credentials;
	struct auth_cred_st *next;
} auth_cred_st;

/* session ticket definitions */
#define TICKET_MASTER_KEY_SIZE (TICKET_KEY_NAME_SIZE+TICKET_CIPHER_KEY_SIZE+TICKET_MAC_SECRET_SIZE)
#define TICKET_KEY_NAME_SIZE 16
#define TICKET_CIPHER_KEY_SIZE 32
#define TICKET_MAC_SECRET_SIZE 16

/* These are restricted by TICKET_CIPHER_KEY_SIZE and TICKET_MAC_SECRET_SIZE */
#define TICKET_CIPHER GNUTLS_CIPHER_AES_256_CBC
#define TICKET_IV_SIZE 16
#define TICKET_BLOCK_SIZE 16

#define TICKET_MAC_ALGO GNUTLS_MAC_SHA1
#define TICKET_MAC_SIZE 20 /* HMAC-SHA1 */

struct ticket_st {
	uint8_t key_name[TICKET_KEY_NAME_SIZE];
	uint8_t IV[TICKET_IV_SIZE];
	uint8_t *encrypted_state;
	uint16_t encrypted_state_len;
	uint8_t mac[TICKET_MAC_SIZE];
};

struct binder_data_st {
	const struct mac_entry_st *prf; /* non-null if this struct is set */
	gnutls_datum_t psk;

	/* 0-based index of the selected PSK.
	 * This only applies if the HSK_PSK_SELECTED flag is set in internals.hsk_flags,
	 * which signals a PSK has indeed been selected. */
	uint8_t idx;
	uint8_t resumption; /* whether it is a resumption binder */
};

typedef void (* gnutls_stek_rotation_callback_t) (const gnutls_datum_t *prev_key,
						const gnutls_datum_t *new_key,
						uint64_t t);

struct gnutls_key_st {
	struct { /* These are kept outside the TLS1.3 union as they are
	          * negotiated via extension, even before protocol is negotiated */
		gnutls_pk_params_st ecdh_params;
		gnutls_pk_params_st ecdhx_params;
		gnutls_pk_params_st dh_params;
	} kshare;

	/* The union contents depend on the negotiated protocol.
	 * It should not contain any values which are allocated
	 * prior to protocol negotiation, as it would be impossible
	 * to deinitialize.
	 */
	union {
		struct {
			/* the current (depending on state) secret, can be
			 * early_secret, client_early_traffic_secret, ... */
			uint8_t temp_secret[MAX_HASH_SIZE];
			unsigned temp_secret_size; /* depends on negotiated PRF size */
			uint8_t e_ckey[MAX_HASH_SIZE]; /* client_early_traffic_secret */
			uint8_t hs_ckey[MAX_HASH_SIZE]; /* client_hs_traffic_secret */
			uint8_t hs_skey[MAX_HASH_SIZE]; /* server_hs_traffic_secret */
			uint8_t ap_ckey[MAX_HASH_SIZE]; /* client_ap_traffic_secret */
			uint8_t ap_skey[MAX_HASH_SIZE]; /* server_ap_traffic_secret */
			uint8_t ap_expkey[MAX_HASH_SIZE]; /* {early_,}exporter_master_secret */
			uint8_t ap_rms[MAX_HASH_SIZE]; /* resumption_master_secret */
		} tls13; /* tls1.3 */

		/* Folow the SSL3.0 and TLS1.2 key exchanges */
		struct {
			/* For ECDH KX */
			struct {
				gnutls_pk_params_st params; /* private part */
				/* public part */
				bigint_t x;
				bigint_t y;
				gnutls_datum_t raw; /* public key used in ECDHX (point) */
			} ecdh;

			/* For DH KX */
			struct {
				gnutls_pk_params_st params;
				bigint_t client_Y;
			} dh;

			/* for SRP KX */
			struct {
				bigint_t srp_key;
				bigint_t srp_g;
				bigint_t srp_p;
				bigint_t A;
				bigint_t B;
				bigint_t u;
				bigint_t b;
				bigint_t a;
				bigint_t x;
			} srp;
		} tls12; /* from ssl3.0 to tls12 */
	} proto;

	/* binders / pre-shared keys in use; temporary storage.
	 * On client side it will hold data for the resumption and external
	 * PSKs After server hello is received the selected binder is set on 0 position
	 * and HSK_PSK_SELECTED is set.
	 *
	 * On server side the first value is populated with
	 * the selected PSK data if HSK_PSK_SELECTED flag is set. */
	struct binder_data_st binders[2];

	/* TLS pre-master key; applies to 1.2 and 1.3 */
	gnutls_datum_t key;

	uint8_t
		/* The key to encrypt and decrypt session tickets */
		session_ticket_key[TICKET_MASTER_KEY_SIZE],
		/* Static buffer for the previous key, whenever we need it */
		previous_ticket_key[TICKET_MASTER_KEY_SIZE],
		/* Initial key supplied by the caller */
		initial_stek[TICKET_MASTER_KEY_SIZE];

	/* this is used to hold the peers authentication data
	 */
	/* auth_info_t structures SHOULD NOT contain malloced
	 * elements. Check gnutls_session_pack.c, and gnutls_auth.c.
	 * Remember that this should be calloced!
	 */
	void *auth_info;
	gnutls_credentials_type_t auth_info_type;
	int auth_info_size;	/* needed in order to store to db for restoring
				 */
	auth_cred_st *cred;	/* used to specify keys/certificates etc */

	struct {
		uint64_t last_result;
		uint8_t was_rotated;
		gnutls_stek_rotation_callback_t cb;
	} totp;
};

typedef struct gnutls_key_st gnutls_key_st;

struct pin_info_st {
	gnutls_pin_callback_t cb;
	void *data;
};

struct record_state_st;
typedef struct record_state_st record_state_st;

struct record_parameters_st;
typedef struct record_parameters_st record_parameters_st;

/* cipher and mac parameters */
typedef struct cipher_entry_st {
	const char *name;
	gnutls_cipher_algorithm_t id;
	uint16_t blocksize;
	uint16_t keysize;
	cipher_type_t type;
	uint16_t implicit_iv;	/* the size of implicit IV - the IV generated but not sent */
	uint16_t explicit_iv;	/* the size of explicit IV - the IV stored in record */
	uint16_t cipher_iv;	/* the size of IV needed by the cipher */
	uint16_t tagsize;
	bool xor_nonce;	/* In this TLS AEAD cipher xor the implicit_iv with the nonce */
	bool only_aead; /* When set, this cipher is only available through the new AEAD API */
	bool no_rekey; /* whether this tls1.3 cipher doesn't need to rekey after 2^24 messages */
} cipher_entry_st;

typedef struct gnutls_cipher_suite_entry_st {
	const char *name;
	const uint8_t id[2];
	gnutls_cipher_algorithm_t block_algorithm;
	gnutls_kx_algorithm_t kx_algorithm;
	gnutls_mac_algorithm_t mac_algorithm;
	gnutls_protocol_t min_version;	/* this cipher suite is supported
					 * from 'version' and above;
					 */
	gnutls_protocol_t max_version;	/* this cipher suite is not supported
					 * after 'version' and above;
					 */
	gnutls_protocol_t min_dtls_version;	/* DTLS min version */
	gnutls_protocol_t max_dtls_version;	/* DTLS max version */
	gnutls_mac_algorithm_t prf;
} gnutls_cipher_suite_entry_st;


typedef struct gnutls_group_entry_st {
	const char *name;
	gnutls_group_t id;
	const gnutls_datum_t *prime;
	const gnutls_datum_t *q;
	const gnutls_datum_t *generator;
	const unsigned *q_bits;
	gnutls_ecc_curve_t curve;
	gnutls_pk_algorithm_t pk;
	unsigned tls_id;		/* The RFC4492 namedCurve ID or TLS 1.3 group ID */
} gnutls_group_entry_st;

/* This structure is used both for MACs and digests
 */
typedef struct mac_entry_st {
	const char *name;
	const char *oid;	/* OID of the hash - if it is a hash */
	const char *mac_oid;    /* OID of the MAC algorithm - if it is a MAC */
	gnutls_mac_algorithm_t id;
	unsigned output_size;
	unsigned key_size;
	unsigned nonce_size;
	unsigned placeholder;	/* if set, then not a real MAC */
	unsigned block_size;	/* internal block size for HMAC */
	unsigned preimage_insecure; /* if this algorithm should not be trusted for pre-image attacks */
} mac_entry_st;

typedef struct {
	const char *name;
	gnutls_protocol_t id;	/* gnutls internal version number */
	unsigned age;		/* internal ordering by protocol age */
	uint8_t major;		/* defined by the protocol */
	uint8_t minor;		/* defined by the protocol */
	transport_t transport;	/* Type of transport, stream or datagram */
	bool supported;	/* 0 not supported, > 0 is supported */
	bool explicit_iv;
	bool extensions;	/* whether it supports extensions */
	bool selectable_sighash;	/* whether signatures can be selected */
	bool selectable_prf;	/* whether the PRF is ciphersuite-defined */

	/* if SSL3 is disabled this flag indicates that this protocol is a placeholder,
	 * otherwise it prevents this protocol from being set as record version */
	bool obsolete;
	bool tls13_sem;		/* The TLS 1.3 handshake semantics */
	bool false_start;	/* That version can be used with false start */
	bool only_extension;	/* negotiated only with an extension */
	bool post_handshake_auth;	/* Supports the TLS 1.3 post handshake auth */
	bool key_shares;	/* TLS 1.3 key share key exchange */
	/*
	 * TLS versions modify the semantics of signature algorithms. This number
	 * is there to distinguish signature algorithms semantics between versions
	 * (maps to sign_algorithm_st->tls_sem)
	 */
	uint8_t tls_sig_sem;
} version_entry_st;


/* STATE (cont) */

#include <hash_int.h>
#include <cipher_int.h>

typedef struct {
	uint8_t id[2]; /* used to be (in TLS 1.2) hash algorithm , PK algorithm */
	uint8_t tls_sem; /* should match the protocol version's tls_sig_sem. */
} sign_algorithm_st;

/* This structure holds parameters got from TLS extension
 * mechanism. (some extensions may hold parameters in auth_info_t
 * structures also - see SRP).
 */

#define MAX_VERIFY_DATA_SIZE 36	/* in SSL 3.0, 12 in TLS 1.0 */

/* auth_info_t structures now MAY contain malloced
 * elements.
 */

/* This structure and auth_info_t, are stored in the resume database,
 * and are restored, in case of resume.
 * Holds all the required parameters to resume the current
 * session.
 */

/* Note that the security parameters structure is set up after the
 * handshake has finished. The only value you may depend on while
 * the handshake is in progress is the cipher suite value.
 */
typedef struct {
	unsigned int entity;	/* GNUTLS_SERVER or GNUTLS_CLIENT */

	/* The epoch used to read and write */
	uint16_t epoch_read;
	uint16_t epoch_write;

	/* The epoch that the next handshake will initialize. */
	uint16_t epoch_next;

	/* The epoch at index 0 of record_parameters. */
	uint16_t epoch_min;

	/* this is the ciphersuite we are going to use
	 * moved here from internals in order to be restored
	 * on resume;
	 */
	const struct gnutls_cipher_suite_entry_st *cs;

	/* This is kept outside the ciphersuite entry as on certain
	 * TLS versions we need a separate PRF MAC, i.e., MD5_SHA1. */
	const mac_entry_st *prf;

	uint8_t master_secret[GNUTLS_MASTER_SIZE];
	uint8_t client_random[GNUTLS_RANDOM_SIZE];
	uint8_t server_random[GNUTLS_RANDOM_SIZE];
	uint8_t session_id[GNUTLS_MAX_SESSION_ID_SIZE];
	uint8_t session_id_size;
	time_t timestamp;

	/* whether client has agreed in post handshake auth - only set on server side */
	uint8_t post_handshake_auth;

	/* The maximum amount of plaintext sent in a record,
	 * negotiated with the peer.
	 */
	uint16_t max_record_send_size;
	uint16_t max_record_recv_size;

	/* The maximum amount of plaintext sent in a record, set by
	 * the programmer.
	 */
	uint16_t max_user_record_send_size;
	uint16_t max_user_record_recv_size;

	/* The maximum amount of early data */
	uint32_t max_early_data_size;

	/* holds the negotiated certificate types */
	gnutls_certificate_type_t client_ctype;
	gnutls_certificate_type_t server_ctype;

	/* The selected (after server hello EC or DH group */
	const gnutls_group_entry_st *grp;

	/* Holds the signature algorithm that will be used in this session,
	 * selected by the server at the time of Ciphersuite/certificate
	 * selection - see select_sign_algorithm() */
	gnutls_sign_algorithm_t server_sign_algo;

	/* Holds the signature algorithm used in this session - If any */
	gnutls_sign_algorithm_t client_sign_algo;

	/* Whether the master secret negotiation will be according to
	 * draft-ietf-tls-session-hash-01
	 */
	uint8_t ext_master_secret;
	/* encrypt-then-mac -> rfc7366 */
	uint8_t etm;

	uint8_t client_auth_type; /* gnutls_credentials_type_t */
	uint8_t server_auth_type;

	/* Note: if you add anything in Security_Parameters struct, then
	 * also modify CPY_COMMON in constate.c, and session_pack.c,
	 * in order to save it in the session storage.
	 */

	/* Used by extensions that enable supplemental data: Which ones
	 * do that? Do they belong in security parameters?
	 */
	int do_recv_supplemental, do_send_supplemental;
	const version_entry_st *pversion;
} security_parameters_st;

typedef struct api_aead_cipher_hd_st {
	cipher_hd_st ctx_enc;
} api_aead_cipher_hd_st;

struct record_state_st {
	/* mac keys can be as long as the hash size */
	uint8_t mac_key[MAX_HASH_SIZE];
	unsigned mac_key_size;

	uint8_t iv[MAX_CIPHER_IV_SIZE];
	unsigned iv_size;

	uint8_t key[MAX_CIPHER_KEY_SIZE];
	unsigned key_size;

	union {
		auth_cipher_hd_st tls12;
		api_aead_cipher_hd_st aead;
	} ctx;
	unsigned aead_tag_size;
	unsigned is_aead;
	gnutls_uint64 sequence_number;
};


/* These are used to resolve relative epochs. These values are just
   outside the 16 bit range to prevent off-by-one errors. An absolute
   epoch may be referred to by its numeric id in the range
   0x0000-0xffff. */
#define EPOCH_READ_CURRENT  70000
#define EPOCH_WRITE_CURRENT 70001
#define EPOCH_NEXT	  70002

struct record_parameters_st {
	uint16_t epoch;
	int initialized;

	const cipher_entry_st *cipher;
	bool etm;
	const mac_entry_st *mac;

	/* for DTLS sliding window */
	uint64_t dtls_sw_next; /* The end point (next expected packet) of the sliding window without epoch */
	uint64_t dtls_sw_bits;
	unsigned dtls_sw_have_recv; /* whether at least a packet has been received */

	record_state_st read;
	record_state_st write;

	/* Whether this state is in use, i.e., if there is
	   a pending handshake message waiting to be encrypted
	   under this epoch's parameters.
	 */
	int usage_cnt;
};

typedef struct {
	unsigned int priorities[MAX_ALGOS];
	unsigned int num_priorities;
} priority_st;

typedef enum {
	SR_DISABLED,
	SR_UNSAFE,
	SR_PARTIAL,
	SR_SAFE
} safe_renegotiation_t;

#define MAX_CIPHERSUITE_SIZE 256

typedef struct ciphersuite_list_st {
	const gnutls_cipher_suite_entry_st *entry[MAX_CIPHERSUITE_SIZE];
	unsigned int size;
} ciphersuite_list_st;

typedef struct group_list_st {
	const gnutls_group_entry_st *entry[MAX_ALGOS];
	unsigned int size;
	bool have_ffdhe;
} group_list_st;

typedef struct sign_algo_list_st {
	const struct gnutls_sign_entry_st *entry[MAX_ALGOS];
	unsigned int size;
} sign_algo_list_st;

#include "atomic.h"

/* For the external api */
struct gnutls_priority_st {
	priority_st protocol;
	priority_st client_ctype;
	priority_st server_ctype;

	/* The following are not necessary to be stored in
	 * the structure; however they are required by the
	 * external APIs: gnutls_priority_*_list() */
	priority_st _cipher;
	priority_st _mac;
	priority_st _kx;
	priority_st _sign_algo;
	priority_st _supported_ecc;

	/* the supported groups */
	group_list_st groups;

	/* the supported signature algorithms */
	sign_algo_list_st sigalg;

	/* the supported ciphersuites */
	ciphersuite_list_st cs;

	/* to disable record padding */
	bool no_extensions;


	safe_renegotiation_t sr;
	bool min_record_version;
	bool server_precedence;
	bool allow_server_key_usage_violation; /* for test suite purposes only */
	bool no_tickets;
	bool have_cbc;
	bool have_psk;
	bool force_etm;
	unsigned int additional_verify_flags;

	/* TLS_FALLBACK_SCSV */
	bool fallback;

	/* The session's expected security level.
	 * Will be used to determine the minimum DH bits,
	 * (or the acceptable certificate security level).
	 */
	gnutls_sec_param_t level;

	/* these should be accessed from
	 * session->internals.VAR names */
	bool _allow_large_records;
	bool _allow_small_records;
	bool _no_etm;
	bool _no_ext_master_secret;
	bool _allow_key_usage_violation;
	bool _allow_wrong_pms;
	bool _dumbfw;
	unsigned int _dh_prime_bits;	/* old (deprecated) variable */

	DEF_ATOMIC_INT(usage_cnt);
};

/* Allow around 50KB of length-hiding padding
 * when using legacy padding,
 * or around 3.2MB when using new padding. */
#define DEFAULT_MAX_EMPTY_RECORDS 200

#define ENABLE_COMPAT(x) \
	      (x)->allow_large_records = 1; \
	      (x)->allow_small_records = 1; \
	      (x)->no_etm = 1; \
	      (x)->no_ext_master_secret = 1; \
	      (x)->allow_key_usage_violation = 1; \
	      (x)->allow_wrong_pms = 1; \
	      (x)->dumbfw = 1

#define ENABLE_PRIO_COMPAT(x) \
	      (x)->_allow_large_records = 1; \
	      (x)->_allow_small_records = 1; \
	      (x)->_no_etm = 1; \
	      (x)->_no_ext_master_secret = 1; \
	      (x)->_allow_key_usage_violation = 1; \
	      (x)->_allow_wrong_pms = 1; \
	      (x)->_dumbfw = 1

/* DH and RSA parameters types.
 */
typedef struct gnutls_dh_params_int {
	/* [0] is the prime, [1] is the generator, [2] is Q if available.
	 */
	bigint_t params[3];
	int q_bits;		/* length of q in bits. If zero then length is unknown.
				 */
} dh_params_st;

/* TLS 1.3 session ticket
 */
typedef struct {
	struct timespec arrival_time;
	struct timespec creation_time;
	uint32_t lifetime;
	uint32_t age_add;
	uint8_t nonce[255];
	size_t nonce_size;
	const mac_entry_st *prf;
	uint8_t resumption_master_secret[MAX_HASH_SIZE];
	gnutls_datum_t ticket;
} tls13_ticket_st;

/* DTLS session state
 */
typedef struct {
	/* HelloVerifyRequest DOS prevention cookie */
	gnutls_datum_t dcookie;

	/* For DTLS handshake fragmentation and reassembly. */
	uint16_t hsk_write_seq;
	/* the sequence number of the expected packet */
	unsigned int hsk_read_seq;
	uint16_t mtu;

	/* a flight transmission is in process */
	bool flight_init;
	/* whether this is the last flight in the protocol  */
	bool last_flight;

	/* the retransmission timeout in milliseconds */
	unsigned int retrans_timeout_ms;

	unsigned int hsk_hello_verify_requests;

	/* The actual retrans_timeout for the next message (e.g. doubled or so)
	 */
	unsigned int actual_retrans_timeout_ms;

	/* timers to handle async handshake after gnutls_handshake()
	 * has terminated. Required to handle retransmissions.
	 */
	time_t async_term;

	/* last retransmission triggered by record layer */
	struct timespec last_retransmit;
	unsigned int packets_dropped;
} dtls_st;

typedef struct tfo_st {
	int fd;
	int flags;
	bool connect_only; /* a previous sendmsg() failed, attempting connect() */
	struct sockaddr_storage connect_addr;
	socklen_t connect_addrlen;
} tfo_st;

typedef struct {
	/* holds all the parsed data received by the record layer */
	mbuffer_head_st record_buffer;

	int handshake_hash_buffer_prev_len;	/* keeps the length of handshake_hash_buffer, excluding
						 * the last received message */
	unsigned handshake_hash_buffer_client_hello_len; /* if non-zero it is the length of data until the client hello message */
	unsigned handshake_hash_buffer_client_kx_len;/* if non-zero it is the length of data until the
						 * the client key exchange message */
	unsigned handshake_hash_buffer_server_finished_len;/* if non-zero it is the length of data until the
						 * the server finished message */
	unsigned handshake_hash_buffer_client_finished_len;/* if non-zero it is the length of data until the
						 * the client finished message */
	gnutls_buffer_st handshake_hash_buffer;	/* used to keep the last received handshake
						 * message */

	bool resumable;	/* TRUE or FALSE - if we can resume that session */

	send_ticket_state_t ticket_state; /* used by gnutls_session_ticket_send() */
	bye_state_t bye_state; /* used by gnutls_bye() */
	reauth_state_t reauth_state; /* used by gnutls_reauth() */

	handshake_state_t handshake_final_state;
	handshake_state_t handshake_state;	/* holds
						 * a number which indicates where
						 * the handshake procedure has been
						 * interrupted. If it is 0 then
						 * no interruption has happened.
						 */

	bool invalid_connection;	/* true or FALSE - if this session is valid */

	bool may_not_read;	/* if it's 0 then we can read/write, otherwise it's forbidden to read/write
				 */
	bool may_not_write;
	bool read_eof;		/* non-zero if we have received a closure alert. */

	int last_alert;		/* last alert received */

	/* The last handshake messages sent or received.
	 */
	int last_handshake_in;
	int last_handshake_out;

	/* priorities */
	struct gnutls_priority_st *priorities;

	/* variables directly set when setting the priorities above, or
	 * when overriding them */
	bool allow_large_records;
	bool allow_small_records;
	bool no_etm;
	bool no_ext_master_secret;
	bool allow_key_usage_violation;
	bool allow_wrong_pms;
	bool dumbfw;

	/* old (deprecated) variable. This is used for both srp_prime_bits
	 * and dh_prime_bits as they don't overlap */
	/* For SRP: minimum bits to allow for SRP
	 * use gnutls_srp_set_prime_bits() to adjust it.
	 */
	uint16_t dh_prime_bits; /* srp_prime_bits */

	/* resumed session */
	bool resumed;	/* RESUME_TRUE or FALSE - if we are resuming a session */

	/* server side: non-zero if resumption was requested by client
	 * client side: non-zero if we set resumption parameters */
	bool resumption_requested;
	security_parameters_st resumed_security_parameters;
	gnutls_datum_t resumption_data; /* copy of input to gnutls_session_set_data() */

	/* These buffers are used in the handshake
	 * protocol only. freed using _gnutls_handshake_io_buffer_clear();
	 */
	mbuffer_head_st handshake_send_buffer;
	mbuffer_head_st handshake_header_recv_buffer;
	handshake_buffer_st handshake_recv_buffer[MAX_HANDSHAKE_MSGS];
	int handshake_recv_buffer_size;

	/* this buffer holds a record packet -mostly used for
	 * non blocking IO.
	 */
	mbuffer_head_st record_recv_buffer;	/* buffer holding the unparsed record that is currently
						 * being received */
	mbuffer_head_st record_send_buffer;	/* holds cached data
						 * for the gnutls_io_write_buffered()
						 * function.
						 */
	size_t record_send_buffer_user_size;	/* holds the
						 * size of the user specified data to
						 * send.
						 */

	mbuffer_head_st early_data_recv_buffer;
	gnutls_buffer_st early_data_presend_buffer;

	record_send_state_t rsend_state;
	/* buffer used temporarily during key update */
	gnutls_buffer_st record_key_update_buffer;
	gnutls_buffer_st record_presend_buffer;	/* holds cached data
						 * for the gnutls_record_send()
						 * function.
						 */

	/* buffer used temporarily during TLS1.3 reauthentication */
	gnutls_buffer_st reauth_buffer;

	time_t expire_time;	/* after expire_time seconds this session will expire */
	const struct mod_auth_st_int *auth_struct;	/* used in handshake packets and KX algorithms */

	/* this is the highest version available
	 * to the peer. (advertized version).
	 * This is obtained by the Handshake Client Hello
	 * message. (some implementations read the Record version)
	 */
	uint8_t adv_version_major;
	uint8_t adv_version_minor;

	/* if this is non zero a certificate request message
	 * will be sent to the client. - only if the ciphersuite
	 * supports it. In server side it contains GNUTLS_CERT_REQUIRE
	 * or similar.
	 */
	gnutls_certificate_request_t send_cert_req;

	size_t max_handshake_data_buffer_size;

	/* PUSH & PULL functions.
	 */
	gnutls_pull_timeout_func pull_timeout_func;
	gnutls_pull_func pull_func;
	gnutls_push_func push_func;
	gnutls_vec_push_func vec_push_func;
	gnutls_errno_func errno_func;
	/* Holds the first argument of PUSH and PULL
	 * functions;
	 */
	gnutls_transport_ptr_t transport_recv_ptr;
	gnutls_transport_ptr_t transport_send_ptr;

	/* STORE & RETRIEVE functions. Only used if other
	 * backend than gdbm is used.
	 */
	gnutls_db_store_func db_store_func;
	gnutls_db_retr_func db_retrieve_func;
	gnutls_db_remove_func db_remove_func;
	void *db_ptr;

	/* post client hello callback (server side only)
	 */
	gnutls_handshake_post_client_hello_func user_hello_func;
	/* handshake hook function */
	gnutls_handshake_hook_func h_hook;
	unsigned int h_type;	/* the hooked type */
	int16_t h_post;		/* whether post-generation/receive */

	/* holds the selected certificate and key.
	 * use _gnutls_selected_certs_deinit() and _gnutls_selected_certs_set()
	 * to change them.
	 */
	gnutls_pcert_st *selected_cert_list;
	uint16_t selected_cert_list_length;
	struct gnutls_privkey_st *selected_key;

	/* new callbacks such as gnutls_certificate_retrieve_function3
	 * set the selected_ocsp datum values. The older OCSP callback-based
	 * functions, set the ocsp_func. The former takes precedence when
	 * set.
	 */
	gnutls_ocsp_data_st *selected_ocsp;
	uint16_t selected_ocsp_length;
	gnutls_status_request_ocsp_func selected_ocsp_func;
	void *selected_ocsp_func_ptr;
	bool selected_need_free;


	/* This holds the default version that our first
	 * record packet will have. */
	uint8_t default_record_version[2];
	uint8_t default_hello_version[2];

	void *user_ptr;

	/* Holds 0 if the last called function was interrupted while
	 * receiving, and non zero otherwise.
	 */
	bool direction;

	/* If non zero the server will not advertise the CA's he
	 * trusts (do not send an RDN sequence).
	 */
	bool ignore_rdn_sequence;

	/* This is used to set an arbitrary version in the RSA
	 * PMS secret. Can be used by clients to test whether the
	 * server checks that version. (** only used in gnutls-cli-debug)
	 */
	uint8_t rsa_pms_version[2];

	/* To avoid using global variables, and especially on Windows where
	 * the application may use a different errno variable than GnuTLS,
	 * it is possible to use gnutls_transport_set_errno to set a
	 * session-specific errno variable in the user-replaceable push/pull
	 * functions.  This value is used by the send/recv functions.  (The
	 * strange name of this variable is because 'errno' is typically
	 * #define'd.)
	 */
	int errnum;

	/* A handshake process has been completed */
	bool initial_negotiation_completed;
	void *post_negotiation_lock; /* protects access to the variable above
				      * in the cases where negotiation is incomplete
				      * after gnutls_handshake() - early/false start */

	/* The type of transport protocol; stream or datagram */
	transport_t transport;

	/* DTLS session state */
	dtls_st dtls;
	/* Protect from infinite loops due to GNUTLS_E_LARGE_PACKET non-handling
	 * or due to multiple alerts being received. */
	unsigned handshake_suspicious_loops;
	/* should be non-zero when a handshake is in progress */
	bool handshake_in_progress;

	/* if set it means that the master key was set using
	 * gnutls_session_set_master() rather than being negotiated. */
	bool premaster_set;

	unsigned int cb_tls_unique_len;
	unsigned char cb_tls_unique[MAX_VERIFY_DATA_SIZE];

	/* starting time of current handshake */
	struct timespec handshake_start_time;

	/* expected end time of current handshake (start+timeout);
	 * this is only filled if a handshake_time_ms is set. */
	struct timespec handshake_abs_timeout;

	/* An estimation of round-trip time under TLS1.3; populated in client side only */
	unsigned ertt;

	unsigned int handshake_timeout_ms;	/* timeout in milliseconds */
	unsigned int record_timeout_ms;	/* timeout in milliseconds */

	/* saved context of post handshake certificate request. In
	 * client side is what we received in server's certificate request;
	 * in server side is what we sent to client. */
	gnutls_datum_t post_handshake_cr_context;
	/* it is a copy of the handshake hash buffer if post handshake is used */
	gnutls_buffer_st post_handshake_hash_buffer;

/* When either of PSK or DHE-PSK is received */
#define HSK_PSK_KE_MODES_RECEIVED (HSK_PSK_KE_MODE_PSK|HSK_PSK_KE_MODE_DHE_PSK|HSK_PSK_KE_MODE_INVALID)

#define HSK_CRT_VRFY_EXPECTED 1
#define HSK_CRT_ASKED (1<<2)
#define HSK_HRR_SENT (1<<3)
#define HSK_HRR_RECEIVED (1<<4)
#define HSK_CRT_REQ_SENT (1<<5)
#define HSK_KEY_UPDATE_ASKED (1<<7) /* flag is not used during handshake */
#define HSK_FALSE_START_USED (1<<8) /* TLS1.2 only */
#define HSK_HAVE_FFDHE (1<<9) /* whether the peer has advertized at least an FFDHE group */
#define HSK_USED_FFDHE (1<<10) /* whether ffdhe was actually negotiated and used */
#define HSK_PSK_KE_MODES_SENT (1<<11)
#define HSK_PSK_KE_MODE_PSK (1<<12) /* client: whether PSK without DH is allowed,
				     * server: whether PSK without DH is selected. */
#define HSK_PSK_KE_MODE_INVALID (1<<13) /* server: no compatible PSK modes were seen */
#define HSK_PSK_KE_MODE_DHE_PSK (1<<14) /* server: whether PSK with DH is selected
					 * client: whether PSK with DH is allowed
					 */
#define HSK_PSK_SELECTED (1<<15) /* server: whether PSK was selected, either for resumption or not;
				  *	    on resumption session->internals.resumed will be set as well.
				  * client: the same */
#define HSK_KEY_SHARE_SENT (1<<16) /* server: key share was sent to client */
#define HSK_KEY_SHARE_RECEIVED (1<<17) /* client: key share was received
					* server: key share was received and accepted */
#define HSK_TLS13_TICKET_SENT (1<<18) /* client: sent a ticket under TLS1.3;
					 * server: a ticket was sent to client.
					 */
#define HSK_TLS12_TICKET_SENT (1<<19) /* client: sent a ticket under TLS1.2;
				       * server: a ticket was sent to client.
				       */
#define HSK_TICKET_RECEIVED (1<<20) /* client: a session ticket was received */
#define HSK_EARLY_START_USED (1<<21)
#define HSK_EARLY_DATA_IN_FLIGHT (1<<22) /* client: sent early_data extension in ClientHello
					  * server: early_data extension was seen in ClientHello
					  */
#define HSK_EARLY_DATA_ACCEPTED (1<<23) /* client: early_data extension was seen in EncryptedExtensions
					 * server: intend to process early data
					 */
#define HSK_RECORD_SIZE_LIMIT_NEGOTIATED (1<<24)
#define HSK_RECORD_SIZE_LIMIT_SENT (1<<25) /* record_size_limit extension was sent */
#define HSK_RECORD_SIZE_LIMIT_RECEIVED (1<<26) /* server: record_size_limit extension was seen but not accepted yet */

	/* The hsk_flags are for use within the ongoing handshake;
	 * they are reset to zero prior to handshake start by gnutls_handshake. */
	unsigned hsk_flags;
	struct timespec last_key_update;
	unsigned key_update_count;
	/* Read-only pointer to the full ClientHello message */
	gnutls_buffer_st full_client_hello;
	/* The offset at which extensions start in the ClientHello buffer */
	int extensions_offset;

	gnutls_buffer_st hb_local_data;
	gnutls_buffer_st hb_remote_data;
	struct timespec hb_ping_start;	/* timestamp: when first HeartBeat ping was sent */
	struct timespec hb_ping_sent;	/* timestamp: when last HeartBeat ping was sent */
	unsigned int hb_actual_retrans_timeout_ms;	/* current timeout, in milliseconds */
	unsigned int hb_retrans_timeout_ms;	/* the default timeout, in milliseconds */
	unsigned int hb_total_timeout_ms;	/* the total timeout, in milliseconds */

	bool ocsp_check_ok;	/* will be zero if the OCSP response TLS extension
					 * check failed (OCSP was old/unrelated or so). */

	heartbeat_state_t hb_state;	/* for ping */

	recv_state_t recv_state;	/* state of the receive function */

	/* if set, server and client random were set by the application */
	bool sc_random_set;

#define INT_FLAG_NO_TLS13 (1LL<<60)
	uint64_t flags; /* the flags in gnutls_init() and GNUTLS_INT_FLAGS */

	/* a verify callback to override the verify callback from the credentials
	 * structure */
	gnutls_certificate_verify_function *verify_callback;
	gnutls_typed_vdata_st *vc_data;
	gnutls_typed_vdata_st vc_sdata;
	unsigned vc_elements;
	unsigned vc_status;
	unsigned int additional_verify_flags; /* may be set by priorities or the vc functions */

	/* we append the verify flags because these can be set,
	 * either by this function or by gnutls_session_set_verify_cert().
	 * However, we ensure that a single profile is set. */
#define ADD_PROFILE_VFLAGS(session, vflags) do { \
	if ((session->internals.additional_verify_flags & GNUTLS_VFLAGS_PROFILE_MASK) && \
	    (vflags & GNUTLS_VFLAGS_PROFILE_MASK)) \
		session->internals.additional_verify_flags &= ~GNUTLS_VFLAGS_PROFILE_MASK; \
	session->internals.additional_verify_flags |= vflags; \
	} while(0)

	/* the SHA256 hash of the peer's certificate */
	uint8_t cert_hash[32];
	bool cert_hash_set;

	/* The saved username from PSK or SRP auth */
	char saved_username[MAX_USERNAME_SIZE+1];
	bool saved_username_set;

	/* Needed for TCP Fast Open (TFO), set by gnutls_transport_set_fastopen() */
	tfo_st tfo;

	struct gnutls_supplemental_entry_st *rsup;
	unsigned rsup_size;

	struct hello_ext_entry_st *rexts;
	unsigned rexts_size;

	struct { /* ext_data[id] contains data for extension_t id */
		gnutls_ext_priv_data_t priv;
		gnutls_ext_priv_data_t resumed_priv;
		uint8_t set;
		uint8_t resumed_set;
	} ext_data[MAX_EXT_TYPES];

	/* In case of a client holds the extensions we sent to the peer;
	 * otherwise the extensions we received from the client. This is
	 * an OR of (1<<extensions_t values).
	 */
	ext_track_t used_exts;

	gnutls_ext_flags_t ext_msg; /* accessed through _gnutls_ext_get/set_msg() */

	/* this is not the negotiated max_record_recv_size, but the actual maximum
	 * receive size */
	unsigned max_recv_size;

	/* candidate groups to be selected for security params groups, they are
	 * prioritized in isolation under TLS1.2 */
	const gnutls_group_entry_st *cand_ec_group;
	const gnutls_group_entry_st *cand_dh_group;
	/* used under TLS1.3+ */
	const gnutls_group_entry_st *cand_group;

	/* the ciphersuite received in HRR */
	uint8_t hrr_cs[2];

	/* this is only used under TLS1.2 or earlier */
	int session_ticket_renew;

	tls13_ticket_st tls13_ticket;

	/* the amount of early data received so far */
	uint32_t early_data_received;

	/* anti-replay measure for 0-RTT mode */
	gnutls_anti_replay_t anti_replay;

	/* Protects _gnutls_epoch_gc() from _gnutls_epoch_get(); these may be
	 * called in parallel when false start is used and false start is used. */
	void *epoch_lock;

	/* If you add anything here, check _gnutls_handshake_internal_state_clear().
	 */
} internals_st;

/* Maximum number of epochs we keep around. */
#define MAX_EPOCH_INDEX 4

#define reset_cand_groups(session) \
	session->internals.cand_ec_group = session->internals.cand_dh_group = \
		session->internals.cand_group = NULL

struct gnutls_session_int {
	security_parameters_st security_parameters;
	record_parameters_st *record_parameters[MAX_EPOCH_INDEX];
	internals_st internals;
	gnutls_key_st key;
};


/* functions
 */
void _gnutls_free_auth_info(gnutls_session_t session);

/* These two macros return the advertised TLS version of
 * the peer.
 */
#define _gnutls_get_adv_version_major(session) \
	session->internals.adv_version_major

#define _gnutls_get_adv_version_minor(session) \
	session->internals.adv_version_minor

#define set_adv_version(session, major, minor) \
	session->internals.adv_version_major = major; \
	session->internals.adv_version_minor = minor

int _gnutls_is_secure_mem_null(const void *);

inline static const version_entry_st *get_version(gnutls_session_t session)
{
	return session->security_parameters.pversion;
}

inline static unsigned get_num_version(gnutls_session_t session)
{
	if (likely(session->security_parameters.pversion != NULL))
		return session->security_parameters.pversion->id;
	else
		return GNUTLS_VERSION_UNKNOWN;
}

void _gnutls_priority_update_fips(void);
void _gnutls_priority_update_non_aesni(void);
extern unsigned _gnutls_disable_tls13;

#define timespec_sub_ms _gnutls_timespec_sub_ms
unsigned int
/* returns a-b in ms */
 timespec_sub_ms(struct timespec *a, struct timespec *b);

inline static int _gnutls_timespec_cmp(struct timespec *a, struct timespec *b) {
	if (a->tv_sec < b->tv_sec)
		return -1;
	if (a->tv_sec > b->tv_sec)
		return 1;
	if (a->tv_nsec < b->tv_nsec)
		return -1;
	if (a->tv_nsec > b->tv_nsec)
		return 1;
	return 0;
}

#include <algorithms.h>
inline static int _gnutls_set_current_version(gnutls_session_t s, unsigned v)
{
	s->security_parameters.pversion = version_to_entry(v);
	if (s->security_parameters.pversion == NULL) {
		return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
	}
	return 0;
}

/* Returns the maximum amount of the plaintext to be sent, considering
 * both user-specified/negotiated maximum values.
 */
inline static size_t max_record_send_size(gnutls_session_t session,
					  record_parameters_st *
					  record_params)
{
	size_t max;

	max = MIN(session->security_parameters.max_record_send_size,
		  session->security_parameters.max_user_record_send_size);

	if (IS_DTLS(session))
		max = MIN(gnutls_dtls_get_data_mtu(session), max);

	return max;
}

/* Returns the during the handshake negotiated certificate type(s).
 * See state.c for the full function documentation.
 *
 * This function is made static inline for optimization reasons.
 */
inline static gnutls_certificate_type_t
get_certificate_type(gnutls_session_t session,
		     gnutls_ctype_target_t target)
{
	switch (target) {
		case GNUTLS_CTYPE_CLIENT:
			return session->security_parameters.client_ctype;
			break;
		case GNUTLS_CTYPE_SERVER:
			return session->security_parameters.server_ctype;
			break;
		case GNUTLS_CTYPE_OURS:
			if (IS_SERVER(session)) {
				return session->security_parameters.server_ctype;
			} else {
				return session->security_parameters.client_ctype;
			}
			break;
		case GNUTLS_CTYPE_PEERS:
			if (IS_SERVER(session)) {
				return session->security_parameters.client_ctype;
			} else {
				return session->security_parameters.server_ctype;
			}
			break;
		default:	// Illegal parameter passed
			return GNUTLS_CRT_UNKNOWN;
	}
}

/* Macros to aide constant time/mem checks */
#define CONSTCHECK_NOT_EQUAL(a, b) ((-((uint32_t)(a) ^ (uint32_t)(b))) >> 31)
#define CONSTCHECK_EQUAL(a, b) (1U - CONSTCHECK_NOT_EQUAL(a, b))

extern unsigned int _gnutls_global_version;

#endif /* GNUTLS_LIB_GNUTLS_INT_H */
