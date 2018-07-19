/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

#ifndef GNUTLS_INT_H
#define GNUTLS_INT_H

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

/* For some reason gnulib likes to provide alternatives for
 * functions it doesn't include. Even worse these functions seem
 * to be available on the target systems.
 */
#undef strdup

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

#ifdef HAVE_LIBNETTLE
#include <nettle/memxor.h>
#else
#include <gl/memxor.h>
#define memxor gl_memxor
#endif

#define ENABLE_ALIGN16

#ifdef __GNUC__
#ifndef _GNUTLS_GCC_VERSION
#define _GNUTLS_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif
#if _GNUTLS_GCC_VERSION >= 30100
#define likely(x)      __builtin_expect((x), 1)
#define unlikely(x)    __builtin_expect((x), 0)
#endif
#if _GNUTLS_GCC_VERSION >= 70100
#define FALLTHROUGH      __attribute__ ((fallthrough))
#endif
#endif

#ifndef FALLTHROUGH
# define FALLTHROUGH
#endif

#ifndef likely
#define likely
#define unlikely
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

/* The maximum digest size of hash algorithms. 
 */
#define MAX_FILENAME 512
#define MAX_HASH_SIZE 64
#define MAX_CIPHER_BLOCK_SIZE 16
#define MAX_CIPHER_KEY_SIZE 32

#define MAX_USERNAME_SIZE 128
#define MAX_SERVER_NAME_SIZE 256

#define AEAD_EXPLICIT_DATA_SIZE 8
#define AEAD_IMPLICIT_DATA_SIZE 4

#define GNUTLS_MASTER_SIZE 48
#define GNUTLS_RANDOM_SIZE 32

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
#define DEFAULT_EXPIRE_TIME 3600
#define DEFAULT_HANDSHAKE_TIMEOUT_MS 40*1000

typedef enum transport_t {
	GNUTLS_STREAM,
	GNUTLS_DGRAM
} transport_t;

typedef enum record_flush_t {
	RECORD_FLUSH = 0,
	RECORD_CORKED,
} record_flush_t;

/* the maximum size of encrypted packets */
#define IS_DTLS(session) (session->internals.transport == GNUTLS_DGRAM)

#define DEFAULT_MAX_RECORD_SIZE 16384
#define TLS_RECORD_HEADER_SIZE 5
#define DTLS_RECORD_HEADER_SIZE (TLS_RECORD_HEADER_SIZE+8)
#define RECORD_HEADER_SIZE(session) (IS_DTLS(session) ? DTLS_RECORD_HEADER_SIZE : TLS_RECORD_HEADER_SIZE)
#define MAX_RECORD_HEADER_SIZE DTLS_RECORD_HEADER_SIZE

/* The following macro is used to calculate the overhead when sending.
 * when receiving we use a different way as there are implementations that
 * store more data than allowed.
 */
#define MAX_RECORD_SEND_OVERHEAD(session) (MAX_CIPHER_BLOCK_SIZE/*iv*/+MAX_PAD_SIZE+((gnutls_compression_get(session)!=GNUTLS_COMP_NULL)?(EXTRA_COMP_SIZE):(0))+MAX_HASH_SIZE/*MAC*/)
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
	STATE30 = 30, STATE31, STATE40 = 40, STATE41, STATE50 = 50
} handshake_state_t;

typedef enum bye_state_t {
	BYE_STATE0 = 0, BYE_STATE1, BYE_STATE2
} bye_state_t;

#define BYE_STATE session->internals.bye_state

typedef enum heartbeat_state_t {
	SHB_SEND1 = 0,
	SHB_SEND2,
	SHB_RECV
} heartbeat_state_t;

typedef enum recv_state_t {
	RECV_STATE_0 = 0,
	RECV_STATE_DTLS_RETRANSMIT,
	RECV_STATE_FALSE_START_HANDLING, /* we are calling gnutls_handshake() within record_recv() */
	RECV_STATE_FALSE_START /* gnutls_record_recv() should complete the handshake */
} recv_state_t;

#include "str.h"

/* This is the maximum number of algorithms (ciphers or macs etc).
 * keep it synced with GNUTLS_MAX_ALGORITHM_NUM in gnutls.h
 */
#define MAX_ALGOS GNUTLS_MAX_ALGORITHM_NUM

/* http://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml
 */
typedef enum extensions_t {
	GNUTLS_EXTENSION_SERVER_NAME = 0,
	GNUTLS_EXTENSION_MAX_RECORD_SIZE = 1,
	GNUTLS_EXTENSION_STATUS_REQUEST = 5,
	GNUTLS_EXTENSION_CERT_TYPE = 9,
	GNUTLS_EXTENSION_SUPPORTED_ECC = 10,
	GNUTLS_EXTENSION_SUPPORTED_ECC_PF = 11,
	GNUTLS_EXTENSION_SRP = 12,
	GNUTLS_EXTENSION_SIGNATURE_ALGORITHMS = 13,
	GNUTLS_EXTENSION_SRTP = 14,
	GNUTLS_EXTENSION_HEARTBEAT = 15,
	GNUTLS_EXTENSION_ALPN = 16,
	GNUTLS_EXTENSION_DUMBFW = 21,
	GNUTLS_EXTENSION_ETM = 22,
	GNUTLS_EXTENSION_EXT_MASTER_SECRET = 23,
	GNUTLS_EXTENSION_SESSION_TICKET = 35,
	GNUTLS_EXTENSION_SAFE_RENEGOTIATION = 65281	/* aka: 0xff01 */
} extensions_t;

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

/* Message buffers (mbuffers) structures */

/* this is actually the maximum number of distinct handshake
 * messages that can arrive in a single flight
 */
#define MAX_HANDSHAKE_MSGS 6
typedef struct {
	/* Handshake layer type and sequence of message */
	gnutls_handshake_description_t htype;
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

struct gnutls_key_st {
	/* For ECDH KX */
	gnutls_pk_params_st ecdh_params; /* private part */
	/* public part */
	bigint_t ecdh_x;
	bigint_t ecdh_y;
	gnutls_datum_t ecdhx; /* public key used in ECDHX (point) */

	/* For DH KX */
	gnutls_datum_t key;
	
	/* For DH KX */
	gnutls_pk_params_st dh_params;
	bigint_t client_Y;
	/* for SRP */

	bigint_t srp_key;
	bigint_t srp_g;
	bigint_t srp_p;
	bigint_t A;
	bigint_t B;
	bigint_t u;
	bigint_t b;
	bigint_t a;
	bigint_t x;

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
	uint8_t crypt_algo;

	auth_cred_st *cred;	/* used to specify keys/certificates etc */

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
	gnutls_protocol_t min_dtls_version;	/* DTLS min version */
	gnutls_mac_algorithm_t prf;
} gnutls_cipher_suite_entry_st;

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
	unsigned secure;	/* must be set to zero if this hash is known to be broken */
	unsigned block_size;	/* internal block size for HMAC */
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
	bool false_start;	/* That version can be used with false start */
} version_entry_st;


/* STATE (cont) */

#include <hash_int.h>
#include <cipher_int.h>
#include <compress.h>

typedef struct {
	uint8_t hash_algorithm;
	uint8_t sign_algorithm;	/* pk algorithm actually */
} sign_algorithm_st;

/* This structure holds parameters got from TLS extension
 * mechanism. (some extensions may hold parameters in auth_info_t
 * structures also - see SRP).
 */

#define MAX_SIGNATURE_ALGORITHMS 16
#define MAX_SIGN_ALGO_SIZE (2 + MAX_SIGNATURE_ALGORITHMS * 2)

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
	gnutls_kx_algorithm_t kx_algorithm;

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
	uint8_t cipher_suite[2];
	gnutls_compression_method_t compression_method;
	uint8_t master_secret[GNUTLS_MASTER_SIZE];
	uint8_t client_random[GNUTLS_RANDOM_SIZE];
	uint8_t server_random[GNUTLS_RANDOM_SIZE];
	uint8_t session_id[GNUTLS_MAX_SESSION_ID_SIZE];
	uint8_t session_id_size;
	time_t timestamp;

	/* The send size is the one requested by the programmer.
	 * The recv size is the one negotiated with the peer.
	 */
	uint16_t max_record_send_size;
	uint16_t max_record_recv_size;
	/* holds the negotiated certificate type */
	gnutls_certificate_type_t cert_type;
	gnutls_ecc_curve_t ecc_curve;	/* holds the first supported ECC curve requested by client */

	/* Holds the signature algorithm used in this session - If any */
	gnutls_sign_algorithm_t server_sign_algo;
	gnutls_sign_algorithm_t client_sign_algo;

	/* Whether the master secret negotiation will be according to
	 * draft-ietf-tls-session-hash-01
	 */
	uint8_t ext_master_secret;
	/* encrypt-then-mac -> rfc7366 */
	uint8_t etm;

	/* Note: if you add anything in Security_Parameters struct, then
	 * also modify CPY_COMMON in gnutls_constate.c, and gnutls_session_pack.c,
	 * in order to save it in the session storage.
	 */

	/* Used by extensions that enable supplemental data: Which ones
	 * do that? Do they belong in security parameters?
	 */
	int do_recv_supplemental, do_send_supplemental;
	const version_entry_st *pversion;
} security_parameters_st;

struct record_state_st {
	gnutls_datum_t mac_secret;
	gnutls_datum_t IV;
	gnutls_datum_t key;
	auth_cipher_hd_st cipher_state;
	comp_hd_st compression_state;
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

	gnutls_compression_method_t compression_algorithm;

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
	unsigned int priority[MAX_ALGOS];
	unsigned int algorithms;
} priority_st;

typedef enum {
	SR_DISABLED,
	SR_UNSAFE,
	SR_PARTIAL,
	SR_SAFE
} safe_renegotiation_t;

/* For the external api */
struct gnutls_priority_st {
	priority_st cipher;
	priority_st mac;
	priority_st kx;
	priority_st compression;
	priority_st protocol;
	priority_st cert_type;
	priority_st sign_algo;
	priority_st supported_ecc;

	/* to disable record padding */
	bool no_extensions;
	bool no_ext_master_secret;
	bool allow_large_records;
	unsigned int dumbfw;
	safe_renegotiation_t sr;
	bool min_record_version;
	bool server_precedence;
	bool allow_key_usage_violation;
	bool allow_server_key_usage_violation; /* for test suite purposes only */
	bool allow_wrong_pms;
	bool no_tickets;
	bool no_etm;
	bool have_cbc;
	/* Whether stateless compression will be used */
	bool stateless_compression;
	unsigned int additional_verify_flags;

	/* The session's expected security level.
	 * Will be used to determine the minimum DH bits,
	 * (or the acceptable certificate security level).
	 */
	gnutls_sec_param_t level;
	unsigned int dh_prime_bits;	/* old (deprecated) variable */

	/* TLS_FALLBACK_SCSV */
	bool fallback;
};

/* Allow around 50KB of length-hiding padding
 * when using legacy padding,
 * or around 3.2MB when using new padding. */
#define DEFAULT_MAX_EMPTY_RECORDS 200

#define ENABLE_COMPAT(x) \
	      (x)->allow_large_records = 1; \
	      (x)->no_etm = 1; \
	      (x)->no_ext_master_secret = 1; \
	      (x)->allow_key_usage_violation = 1; \
	      (x)->allow_wrong_pms = 1; \
	      (x)->dumbfw = 1

/* DH and RSA parameters types.
 */
typedef struct gnutls_dh_params_int {
	/* [0] is the prime, [1] is the generator.
	 */
	bigint_t params[2];
	int q_bits;		/* length of q in bits. If zero then length is unknown.
				 */
} dh_params_st;

typedef struct {
	gnutls_dh_params_t dh_params;
	int free_dh_params;
} internal_params_st;

/* DTLS session state
 */
typedef struct {
	/* HelloVerifyRequest DOS prevention cookie */
	uint8_t cookie[DTLS_MAX_COOKIE_SIZE];
	uint8_t cookie_len;

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
	unsigned handshake_hash_buffer_client_kx_len;/* if non-zero it is the length of data until the
						 * the client key exchange message */
	gnutls_buffer_st handshake_hash_buffer;	/* used to keep the last received handshake 
						 * message */
	bool resumable;	/* TRUE or FALSE - if we can resume that session */
	bool ticket_sent;	/* whether a session ticket was sent */
	bye_state_t bye_state; /* used by gnutls_bye() */
	handshake_state_t handshake_final_state;
	handshake_state_t handshake_state;	/* holds
						 * a number which indicates where
						 * the handshake procedure has been
						 * interrupted. If it is 0 then
						 * no interruption has happened.
						 */

	bool invalid_connection;	/* true or FALSE - if this session is valid */

	bool may_not_read;	/* if it's 0 then we can read/write, otherwise it's forbiden to read/write
				 */
	bool may_not_write;
	bool read_eof;		/* non-zero if we have received a closure alert. */

	int last_alert;		/* last alert received */

	/* The last handshake messages sent or received.
	 */
	int last_handshake_in;
	int last_handshake_out;

	/* priorities */
	struct gnutls_priority_st priorities;

	/* resumed session */
	bool resumed;	/* RESUME_TRUE or FALSE - if we are resuming a session */
	bool resumption_requested;	/* non-zero if resumption was requested by client */
	security_parameters_st resumed_security_parameters;
	gnutls_datum_t resumption_data; /* copy of input to gnutls_session_set_data() */

	/* These buffers are used in the handshake
	 * protocol only. freed using _gnutls_handshake_io_buffer_clear();
	 */
	mbuffer_head_st handshake_send_buffer;
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

	record_flush_t record_flush_mode;	/* GNUTLS_FLUSH or GNUTLS_CORKED */
	gnutls_buffer_st record_presend_buffer;	/* holds cached data
						 * for the gnutls_record_send()
						 * function.
						 */

	time_t expire_time;	/* after expire_time seconds this session will expire */
	struct mod_auth_st_int *auth_struct;	/* used in handshake packets and KX algorithms */

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
	unsigned send_cert_req;

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
	int16_t selected_cert_list_length;
	struct gnutls_privkey_st *selected_key;
	bool selected_need_free;
	gnutls_status_request_ocsp_func selected_ocsp_func;
	void *selected_ocsp_func_ptr;

	/* In case of a client holds the extensions we sent to the peer;
	 * otherwise the extensions we received from the client.
	 */
	uint16_t extensions_sent[MAX_EXT_TYPES];
	uint16_t extensions_sent_size;

	/* is 0 if we are to send the whole PGP key, or non zero
	 * if the fingerprint is to be sent.
	 */
	bool pgp_fingerprint;

	/* This holds the default version that our first
	 * record packet will have. */
	uint8_t default_record_version[2];
	uint8_t default_hello_version[2];

	void *user_ptr;

	bool enable_private;	/* non zero to
					 * enable cipher suites
					 * which have 0xFF status.
					 */

	/* Holds 0 if the last called function was interrupted while
	 * receiving, and non zero otherwise.
	 */
	bool direction;

	/* This callback will be used (if set) to receive an
	 * openpgp key. (if the peer sends a fingerprint)
	 */
	gnutls_openpgp_recv_key_func openpgp_recv_key_func;

	/* If non zero the server will not advertise the CA's he
	 * trusts (do not send an RDN sequence).
	 */
	bool ignore_rdn_sequence;

	/* This is used to set an arbitary version in the RSA
	 * PMS secret. Can be used by clients to test whether the
	 * server checks that version. (** only used in gnutls-cli-debug)
	 */
	uint8_t rsa_pms_version[2];

	/* Here we cache the DH or RSA parameters got from the
	 * credentials structure, or from a callback. That is to
	 * minimize external calls.
	 */
	internal_params_st params;

	/* To avoid using global variables, and especially on Windows where
	 * the application may use a different errno variable than GnuTLS,
	 * it is possible to use gnutls_transport_set_errno to set a
	 * session-specific errno variable in the user-replaceable push/pull
	 * functions.  This value is used by the send/recv functions.  (The
	 * strange name of this variable is because 'errno' is typically
	 * #define'd.)
	 */
	int errnum;

	/* minimum bits to allow for SRP
	 * use gnutls_srp_set_prime_bits() to adjust it.
	 */
	uint16_t srp_prime_bits;

	/* A handshake process has been completed */
	bool initial_negotiation_completed;

	struct {
		uint16_t type;
		gnutls_ext_priv_data_t priv;
		bool set;
	} extension_int_data[MAX_EXT_TYPES];

	struct {
		uint16_t type;
		gnutls_ext_priv_data_t priv;
		bool set;
	} resumed_extension_int_data[MAX_EXT_TYPES];
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

	time_t handshake_endtime;	/* end time in seconds */
	unsigned int handshake_timeout_ms;	/* timeout in milliseconds */
	unsigned int record_timeout_ms;	/* timeout in milliseconds */

	unsigned crt_requested; /* 1 if client auth was requested (i.e., client cert).
	 * In case of a server this holds 1 if we should wait
	 * for a client certificate verify
	 */

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

	bool sc_random_set;

	unsigned flags; /* the flags in gnutls_init() */

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

	bool false_start_used; /* non-zero if false start was used for appdata */

	/* Needed for TCP Fast Open (TFO), set by gnutls_transport_set_fastopen() */
	tfo_st tfo;

	struct gnutls_supplemental_entry_st *rsup;
	unsigned rsup_size;

	struct extension_entry_st *rexts;
	unsigned rexts_size;
	/* If you add anything here, check _gnutls_handshake_internal_state_clear().
	 */
} internals_st;

/* Maximum number of epochs we keep around. */
#define MAX_EPOCH_INDEX 16

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
#define _gnutls_get_adv_version_major( session) \
	session->internals.adv_version_major

#define _gnutls_get_adv_version_minor( session) \
	session->internals.adv_version_minor

#define set_adv_version( session, major, minor) \
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

#define timespec_sub_ms _gnutls_timespec_sub_ms
unsigned int
/* returns a-b in ms */
 timespec_sub_ms(struct timespec *a, struct timespec *b);

#include <algorithms.h>
inline static int _gnutls_set_current_version(gnutls_session_t s, unsigned v)
{
	s->security_parameters.pversion = version_to_entry(v);
	if (s->security_parameters.pversion == NULL) {
		return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
	}
	return 0;
}

inline static size_t max_user_send_size(gnutls_session_t session,
					record_parameters_st *
					record_params)
{
	size_t max;

	if (IS_DTLS(session)) {
		max = MIN(gnutls_dtls_get_data_mtu(session), session->security_parameters.max_record_send_size);
	} else {
		max = session->security_parameters.max_record_send_size;
	}

	return max;
}

#endif				/* GNUTLS_INT_H */
