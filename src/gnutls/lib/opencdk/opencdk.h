/* opencdk.h - Open Crypto Development Kit (OpenCDK)
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * This library is free software; you can redistribute it and/or
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

#ifndef OPENCDK_H
#define OPENCDK_H

#include <config.h>
#include "gnutls_int.h"
#include <stddef.h>		/* for size_t */
#include <stdarg.h>
#include <mem.h>
#include <gnutls/gnutls.h>
#include "errors.h"
#include <hash_int.h>

/* The OpenCDK version as a string. */
#define OPENCDK_VERSION "0.6.6"

/* The OpenCDK version as integer components major.minor.path */
#define OPENCDK_VERSION_MAJOR 0
#define OPENCDK_VERSION_MINOR 6
#define OPENCDK_VERSION_PATCH 6

#ifdef __cplusplus
extern "C" {
#endif

/* General contexts */

/* 'Session' handle to support the various options and run-time
   information. */
	struct cdk_ctx_s;
	typedef struct cdk_ctx_s *cdk_ctx_t;

/* A generic context to store list of strings. */
	struct cdk_strlist_s;
	typedef struct cdk_strlist_s *cdk_strlist_t;

/* Context used to list keys of a keyring. */
	struct cdk_listkey_s;
	typedef struct cdk_listkey_s *cdk_listkey_t;

/* Opaque String to Key (S2K) handle. */
	struct cdk_s2k_s;
	typedef struct cdk_s2k_s *cdk_s2k_t;

/* Abstract I/O object, a stream, which is used for most operations. */
	struct cdk_stream_s;
	typedef struct cdk_stream_s *cdk_stream_t;

/* Opaque handle for the user ID preferences. */
	struct cdk_prefitem_s;
	typedef struct cdk_prefitem_s *cdk_prefitem_t;

/* Node to store a single key node packet. */
	struct cdk_kbnode_s;
	typedef struct cdk_kbnode_s *cdk_kbnode_t;

/* Key database handle. */
	struct cdk_keydb_hd_s;
	typedef struct cdk_keydb_hd_s *cdk_keydb_hd_t;

	struct cdk_keydb_search_s;
	typedef struct cdk_keydb_search_s *cdk_keydb_search_t;

/* Context to store a list of recipient keys. */
	struct cdk_keylist_s;
	typedef struct cdk_keylist_s *cdk_keylist_t;

/* Context to encapsulate a single sub packet of a signature. */
	struct cdk_subpkt_s;
	typedef struct cdk_subpkt_s *cdk_subpkt_t;

/* Context used to generate key pairs. */
	struct cdk_keygen_ctx_s;
	typedef struct cdk_keygen_ctx_s *cdk_keygen_ctx_t;

/* Handle for a single designated revoker. */
	struct cdk_desig_revoker_s;
	typedef struct cdk_desig_revoker_s *cdk_desig_revoker_t;

/* Alias for backward compatibility. */
	typedef bigint_t cdk_mpi_t;


/* All valid error constants. */
	typedef enum {
		CDK_EOF = -1,
		CDK_Success = 0,
		CDK_General_Error = 1,
		CDK_File_Error = 2,
		CDK_Bad_Sig = 3,
		CDK_Inv_Packet = 4,
		CDK_Inv_Algo = 5,
		CDK_Not_Implemented = 6,
		CDK_Armor_Error = 8,
		CDK_Armor_CRC_Error = 9,
		CDK_MPI_Error = 10,
		CDK_Inv_Value = 11,
		CDK_Error_No_Key = 12,
		CDK_Chksum_Error = 13,
		CDK_Time_Conflict = 14,
		CDK_Zlib_Error = 15,
		CDK_Weak_Key = 16,
		CDK_Out_Of_Core = 17,
		CDK_Wrong_Seckey = 18,
		CDK_Bad_MDC = 19,
		CDK_Inv_Mode = 20,
		CDK_Error_No_Keyring = 21,
		CDK_Wrong_Format = 22,
		CDK_Inv_Packet_Ver = 23,
		CDK_Too_Short = 24,
		CDK_Unusable_Key = 25,
		CDK_No_Data = 26,
		CDK_No_Passphrase = 27,
		CDK_Network_Error = 28
	} cdk_error_t;


	enum cdk_control_flags {
		CDK_CTLF_SET = 0,	/* Value to set an option */
		CDK_CTLF_GET = 1,	/* Value to get an option */
		CDK_CTL_DIGEST = 10,	/* Option to set the digest algorithm. */
		CDK_CTL_ARMOR = 12,	/* Option to enable armor output. */
		CDK_CTL_COMPRESS = 13,	/* Option to enable compression. */
		CDK_CTL_COMPAT = 14,	/* Option to switch in compat mode. */
		CDK_CTL_OVERWRITE = 15,	/* Option to enable file overwritting. */
		CDK_CTL_S2K = 16,	/* Option to set S2K values. */
		CDK_CTL_FORCE_DIGEST = 19,	/* Force the use of a digest algorithm. */
		CDK_CTL_BLOCKMODE_ON = 20	/* Enable partial body lengths */
	};


/* Specifies all valid log levels. */
	enum cdk_log_level_t {
		CDK_LOG_NONE = 0,	/* No log message will be shown. */
		CDK_LOG_INFO = 1,
		CDK_LOG_DEBUG = 2,
		CDK_LOG_DEBUG_PKT = 3
	};


/* All valid compression algorithms in OpenPGP */
	enum cdk_compress_algo_t {
		CDK_COMPRESS_NONE = 0,
		CDK_COMPRESS_ZIP = 1,
		CDK_COMPRESS_ZLIB = 2,
		CDK_COMPRESS_BZIP2 = 3	/* Not supported in this version */
	};

/* All valid public key algorithms valid in OpenPGP */
	enum cdk_pubkey_algo_t {
		CDK_PK_UNKNOWN = 0,
		CDK_PK_RSA = 1,
		CDK_PK_RSA_E = 2,	/* RSA-E and RSA-S are deprecated use RSA instead */
		CDK_PK_RSA_S = 3,	/* and use the key flags in the self signatures. */
		CDK_PK_ELG_E = 16,
		CDK_PK_DSA = 17
	};

/* The valid 'String-To-Key' modes */
	enum cdk_s2k_type_t {
		CDK_S2K_SIMPLE = 0,
		CDK_S2K_SALTED = 1,
		CDK_S2K_ITERSALTED = 3,
		CDK_S2K_GNU_EXT = 101
		    /* GNU  extensions: refer to DETAILS from GnuPG: 
		       http://cvs.gnupg.org/cgi-bin/viewcvs.cgi/trunk/doc/DETAILS?root=GnuPG
		     */
	};

/* The different kind of user ID preferences. */
	enum cdk_pref_type_t {
		CDK_PREFTYPE_NONE = 0,
		CDK_PREFTYPE_SYM = 1,	/* Symmetric ciphers */
		CDK_PREFTYPE_HASH = 2,	/* Message digests */
		CDK_PREFTYPE_ZIP = 3	/* Compression algorithms */
	};


/* All valid sub packet types. */
	enum cdk_sig_subpacket_t {
		CDK_SIGSUBPKT_NONE = 0,
		CDK_SIGSUBPKT_SIG_CREATED = 2,
		CDK_SIGSUBPKT_SIG_EXPIRE = 3,
		CDK_SIGSUBPKT_EXPORTABLE = 4,
		CDK_SIGSUBPKT_TRUST = 5,
		CDK_SIGSUBPKT_REGEXP = 6,
		CDK_SIGSUBPKT_REVOCABLE = 7,
		CDK_SIGSUBPKT_KEY_EXPIRE = 9,
		CDK_SIGSUBPKT_PREFS_SYM = 11,
		CDK_SIGSUBPKT_REV_KEY = 12,
		CDK_SIGSUBPKT_ISSUER = 16,
		CDK_SIGSUBPKT_NOTATION = 20,
		CDK_SIGSUBPKT_PREFS_HASH = 21,
		CDK_SIGSUBPKT_PREFS_ZIP = 22,
		CDK_SIGSUBPKT_KS_FLAGS = 23,
		CDK_SIGSUBPKT_PREF_KS = 24,
		CDK_SIGSUBPKT_PRIMARY_UID = 25,
		CDK_SIGSUBPKT_POLICY = 26,
		CDK_SIGSUBPKT_KEY_FLAGS = 27,
		CDK_SIGSUBPKT_SIGNERS_UID = 28,
		CDK_SIGSUBPKT_REVOC_REASON = 29,
		CDK_SIGSUBPKT_FEATURES = 30
	};


/* All valid armor types. */
	enum cdk_armor_type_t {
		CDK_ARMOR_MESSAGE = 0,
		CDK_ARMOR_PUBKEY = 1,
		CDK_ARMOR_SECKEY = 2,
		CDK_ARMOR_SIGNATURE = 3,
		CDK_ARMOR_CLEARSIG = 4
	};

	enum cdk_keydb_flag_t {
		/* Valid database search modes */
		CDK_DBSEARCH_EXACT = 1,	/* Exact string search */
		CDK_DBSEARCH_SUBSTR = 2,	/* Sub string search */
		CDK_DBSEARCH_SHORT_KEYID = 3,	/* 32-bit keyid search */
		CDK_DBSEARCH_KEYID = 4,	/* 64-bit keyid search */
		CDK_DBSEARCH_FPR = 5,	/* 160-bit fingerprint search */
		CDK_DBSEARCH_NEXT = 6,	/* Enumerate all keys */
		CDK_DBSEARCH_AUTO = 7,	/* Try to classify the string */
		/* Valid database types */
		CDK_DBTYPE_PK_KEYRING = 100,	/* A file with one or more public keys */
		CDK_DBTYPE_SK_KEYRING = 101,	/* A file with one or more secret keys */
		CDK_DBTYPE_DATA = 102,	/* A buffer with at least one public key */
	};


/* All valid modes for cdk_data_transform() */
	enum cdk_crypto_mode_t {
		CDK_CRYPTYPE_NONE = 0,
		CDK_CRYPTYPE_ENCRYPT = 1,
		CDK_CRYPTYPE_DECRYPT = 2,
		CDK_CRYPTYPE_SIGN = 3,
		CDK_CRYPTYPE_VERIFY = 4,
		CDK_CRYPTYPE_EXPORT = 5,
		CDK_CRYPTYPE_IMPORT = 6
	};

#define CDK_KEY_USG_ENCR (CDK_KEY_USG_COMM_ENCR | CDK_KEY_USG_STORAGE_ENCR)
#define CDK_KEY_USG_SIGN (CDK_KEY_USG_DATA_SIGN | CDK_KEY_USG_CERT_SIGN)
/* A list of valid public key usages. */
	enum cdk_key_usage_t {
		CDK_KEY_USG_CERT_SIGN = 1,
		CDK_KEY_USG_DATA_SIGN = 2,
		CDK_KEY_USG_COMM_ENCR = 4,
		CDK_KEY_USG_STORAGE_ENCR = 8,
		CDK_KEY_USG_SPLIT_KEY = 16,
		CDK_KEY_USG_AUTH = 32,
		CDK_KEY_USG_SHARED_KEY = 128
	};


/* Valid flags for keys. */
	enum cdk_key_flag_t {
		CDK_KEY_VALID = 0,
		CDK_KEY_INVALID = 1,	/* Missing or wrong self signature */
		CDK_KEY_EXPIRED = 2,	/* Key is expired. */
		CDK_KEY_REVOKED = 4,	/* Key has been revoked. */
		CDK_KEY_NOSIGNER = 8
	};


/* Trust values and flags for keys and user IDs */
	enum cdk_trust_flag_t {
		CDK_TRUST_UNKNOWN = 0,
		CDK_TRUST_EXPIRED = 1,
		CDK_TRUST_UNDEFINED = 2,
		CDK_TRUST_NEVER = 3,
		CDK_TRUST_MARGINAL = 4,
		CDK_TRUST_FULLY = 5,
		CDK_TRUST_ULTIMATE = 6,
		/* trust flags */
		CDK_TFLAG_REVOKED = 32,
		CDK_TFLAG_SUB_REVOKED = 64,
		CDK_TFLAG_DISABLED = 128
	};


/* Signature states and the signature modes. */
	enum cdk_signature_stat_t {
		/* Signature status */
		CDK_SIGSTAT_NONE = 0,
		CDK_SIGSTAT_GOOD = 1,
		CDK_SIGSTAT_BAD = 2,
		CDK_SIGSTAT_NOKEY = 3,
		CDK_SIGSTAT_VALID = 4,	/* True if made with a valid key. */
		/* FIXME: We need indicators for revoked/expires signatures. */

		/* Signature modes */
		CDK_SIGMODE_NORMAL = 100,
		CDK_SIGMODE_DETACHED = 101,
		CDK_SIGMODE_CLEAR = 102
	};


/* Key flags. */
	typedef enum {
		CDK_FLAG_KEY_REVOKED = 256,
		CDK_FLAG_KEY_EXPIRED = 512,
		CDK_FLAG_SIG_EXPIRED = 1024
	} cdk_key_flags_t;


/* Possible format for the literal data. */
	typedef enum {
		CDK_LITFMT_BINARY = 0,
		CDK_LITFMT_TEXT = 1,
		CDK_LITFMT_UNICODE = 2
	} cdk_lit_format_t;

/* Valid OpenPGP packet types and their IDs */
	typedef enum {
		CDK_PKT_RESERVED = 0,
		CDK_PKT_PUBKEY_ENC = 1,
		CDK_PKT_SIGNATURE = 2,
		CDK_PKT_ONEPASS_SIG = 4,
		CDK_PKT_SECRET_KEY = 5,
		CDK_PKT_PUBLIC_KEY = 6,
		CDK_PKT_SECRET_SUBKEY = 7,
		CDK_PKT_COMPRESSED = 8,
		CDK_PKT_MARKER = 10,
		CDK_PKT_LITERAL = 11,
		CDK_PKT_RING_TRUST = 12,
		CDK_PKT_USER_ID = 13,
		CDK_PKT_PUBLIC_SUBKEY = 14,
		CDK_PKT_OLD_COMMENT = 16,
		CDK_PKT_ATTRIBUTE = 17,
		CDK_PKT_MDC = 19
	} cdk_packet_type_t;

/* Define the maximal number of multiprecion integers for
   a public key. */
#define MAX_CDK_PK_PARTS 4

/* Define the maximal number of multiprecision integers for
   a signature/encrypted blob issued by a secret key. */
#define MAX_CDK_DATA_PARTS 2


/* Helper macro to figure out if the packet is encrypted */
#define CDK_PKT_IS_ENCRYPTED(pkttype) (\
     ((pkttype)==CDK_PKT_ENCRYPTED_MDC) \
  || ((pkttype)==CDK_PKT_ENCRYPTED))


	struct cdk_pkt_signature_s {
		unsigned char version;
		unsigned char sig_class;
		unsigned int timestamp;
		unsigned int expiredate;
		unsigned int keyid[2];
		unsigned char pubkey_algo;
		unsigned char digest_algo;
		unsigned char digest_start[2];
		unsigned short hashed_size;
		cdk_subpkt_t hashed;
		unsigned short unhashed_size;
		cdk_subpkt_t unhashed;
		bigint_t mpi[MAX_CDK_DATA_PARTS];
		cdk_desig_revoker_t revkeys;
		struct {
			unsigned exportable:1;
			unsigned revocable:1;
			unsigned policy_url:1;
			unsigned notation:1;
			unsigned expired:1;
			unsigned checked:1;
			unsigned valid:1;
			unsigned missing_key:1;
		} flags;
		unsigned int key[2];	/* only valid for key signatures */
	};
	typedef struct cdk_pkt_signature_s *cdk_pkt_signature_t;


	struct cdk_pkt_userid_s {
		unsigned int len;
		unsigned is_primary:1;
		unsigned is_revoked:1;
		unsigned mdc_feature:1;
		cdk_prefitem_t prefs;
		size_t prefs_size;
		unsigned char *attrib_img;	/* Tag 17 if not null */
		size_t attrib_len;
		cdk_pkt_signature_t selfsig;
		char *name;
	};
	typedef struct cdk_pkt_userid_s *cdk_pkt_userid_t;


	struct cdk_pkt_pubkey_s {
		unsigned char version;
		unsigned char pubkey_algo;
		unsigned char fpr[20];
		unsigned int keyid[2];
		unsigned int main_keyid[2];
		unsigned int timestamp;
		unsigned int expiredate;
		bigint_t mpi[MAX_CDK_PK_PARTS];
		unsigned is_revoked:1;
		unsigned is_invalid:1;
		unsigned has_expired:1;
		int pubkey_usage;
		cdk_pkt_userid_t uid;
		cdk_prefitem_t prefs;
		size_t prefs_size;
		cdk_desig_revoker_t revkeys;
	};
	typedef struct cdk_pkt_pubkey_s *cdk_pkt_pubkey_t;

/* Alias to define a generic public key context. */
	typedef cdk_pkt_pubkey_t cdk_pubkey_t;


	struct cdk_pkt_seckey_s {
		cdk_pkt_pubkey_t pk;
		unsigned int expiredate;
		int version;
		int pubkey_algo;
		unsigned int keyid[2];
		unsigned int main_keyid[2];
		unsigned char s2k_usage;
		struct {
			unsigned char algo;
			unsigned char sha1chk;	/* SHA1 is used instead of a 16 bit checksum */
			cdk_s2k_t s2k;
			unsigned char iv[16];
			unsigned char ivlen;
		} protect;
		unsigned short csum;
		bigint_t mpi[MAX_CDK_PK_PARTS];
		unsigned char *encdata;
		size_t enclen;
		unsigned char is_protected;
		unsigned is_primary:1;
		unsigned has_expired:1;
		unsigned is_revoked:1;
	};
	typedef struct cdk_pkt_seckey_s *cdk_pkt_seckey_t;

/* Alias to define a generic secret key context. */
	typedef cdk_pkt_seckey_t cdk_seckey_t;


	struct cdk_pkt_onepass_sig_s {
		unsigned char version;
		unsigned int keyid[2];
		unsigned char sig_class;
		unsigned char digest_algo;
		unsigned char pubkey_algo;
		unsigned char last;
	};
	typedef struct cdk_pkt_onepass_sig_s *cdk_pkt_onepass_sig_t;


	struct cdk_pkt_pubkey_enc_s {
		unsigned char version;
		unsigned int keyid[2];
		int throw_keyid;
		unsigned char pubkey_algo;
		bigint_t mpi[MAX_CDK_DATA_PARTS];
	};
	typedef struct cdk_pkt_pubkey_enc_s *cdk_pkt_pubkey_enc_t;

	struct cdk_pkt_encrypted_s {
		unsigned int len;
		int extralen;
		unsigned char mdc_method;
		cdk_stream_t buf;
	};
	typedef struct cdk_pkt_encrypted_s *cdk_pkt_encrypted_t;


	struct cdk_pkt_mdc_s {
		unsigned char hash[20];
	};
	typedef struct cdk_pkt_mdc_s *cdk_pkt_mdc_t;


	struct cdk_pkt_literal_s {
		unsigned int len;
		cdk_stream_t buf;
		int mode;
		unsigned int timestamp;
		int namelen;
		char *name;
	};
	typedef struct cdk_pkt_literal_s *cdk_pkt_literal_t;


	struct cdk_pkt_compressed_s {
		unsigned int len;
		int algorithm;
		cdk_stream_t buf;
	};
	typedef struct cdk_pkt_compressed_s *cdk_pkt_compressed_t;


/* Structure which represents a single OpenPGP packet. */
	struct cdk_packet_s {
		size_t pktlen;	/* real packet length */
		size_t pktsize;	/* length with all headers */
		int old_ctb;	/* 1 if RFC1991 mode is used */
		cdk_packet_type_t pkttype;
		union {
			cdk_pkt_mdc_t mdc;
			cdk_pkt_userid_t user_id;
			cdk_pkt_pubkey_t public_key;
			cdk_pkt_seckey_t secret_key;
			cdk_pkt_signature_t signature;
			cdk_pkt_pubkey_enc_t pubkey_enc;
			cdk_pkt_compressed_t compressed;
			cdk_pkt_encrypted_t encrypted;
			cdk_pkt_literal_t literal;
			cdk_pkt_onepass_sig_t onepass_sig;
		} pkt;
	};
	typedef struct cdk_packet_s *cdk_packet_t;

/* Allocate a new packet or a new packet with the given packet type. */
	cdk_error_t cdk_pkt_new(cdk_packet_t * r_pkt);
	cdk_error_t cdk_pkt_alloc(cdk_packet_t * r_pkt,
				  cdk_packet_type_t pkttype);

/* Only release the contents of the packet but not @PKT itself. */
	void cdk_pkt_free(cdk_packet_t pkt);

/* Release the packet contents and the packet structure @PKT itself. */
	void cdk_pkt_release(cdk_packet_t pkt);

/* Read or write the given output from or to the stream. */
	cdk_error_t cdk_pkt_read(cdk_stream_t inp, cdk_packet_t pkt, unsigned public);
	cdk_error_t cdk_pkt_write(cdk_stream_t out, cdk_packet_t pkt);

/* Sub packet routines */
	cdk_subpkt_t cdk_subpkt_new(size_t size);
	void cdk_subpkt_free(cdk_subpkt_t ctx);
	cdk_subpkt_t cdk_subpkt_find(cdk_subpkt_t ctx, size_t type);
	cdk_subpkt_t cdk_subpkt_find_next(cdk_subpkt_t root, size_t type);
	size_t cdk_subpkt_type_count(cdk_subpkt_t ctx, size_t type);
	cdk_subpkt_t cdk_subpkt_find_nth(cdk_subpkt_t ctx, size_t type,
					 size_t index);
	cdk_error_t cdk_subpkt_add(cdk_subpkt_t root, cdk_subpkt_t node);
	const unsigned char *cdk_subpkt_get_data(cdk_subpkt_t ctx,
						 size_t * r_type,
						 size_t * r_nbytes);
	void cdk_subpkt_init(cdk_subpkt_t node, size_t type,
			     const void *buf, size_t buflen);

/* Designated Revoker routines */
	const unsigned char *cdk_key_desig_revoker_walk(cdk_desig_revoker_t
							root,
							cdk_desig_revoker_t
							* ctx,
							int *r_class,
							int *r_algid);

#define is_RSA(a) ((a) == CDK_PK_RSA		\
		   || (a) == CDK_PK_RSA_E	\
		   || (a) == CDK_PK_RSA_S)
#define is_ELG(a) ((a) == CDK_PK_ELG_E)
#define is_DSA(a) ((a) == CDK_PK_DSA)

/* Encrypt the given session key @SK with the public key @PK
   and write the contents into the packet @PKE. */
	cdk_error_t cdk_pk_encrypt(cdk_pubkey_t pk,
				   cdk_pkt_pubkey_enc_t pke, bigint_t sk);

/* Decrypt the given encrypted session key in @PKE with the secret key
   @SK and store it in @R_SK. */
	cdk_error_t cdk_pk_decrypt(cdk_seckey_t sk,
				   cdk_pkt_pubkey_enc_t pke,
				   bigint_t * r_sk);

/* Sign the given message digest @MD with the secret key @SK and
   store the signature in the packet @SIG. */
	cdk_error_t cdk_pk_sign(cdk_seckey_t sk, cdk_pkt_signature_t sig,
				const unsigned char *md);

/* Verify the given signature in @SIG with the public key @PK
   and compare it against the message digest @MD. */
	cdk_error_t cdk_pk_verify(cdk_pubkey_t pk, cdk_pkt_signature_t sig,
				  const unsigned char *md);

/* Use cdk_pk_get_npkey() and cdk_pk_get_nskey to find out how much
   multiprecision integers a key consists of. */

/* Return a multi precision integer of the public key with the index @IDX
   in the buffer @BUF. @R_NWRITTEN will contain the length in octets.
   Optional @R_NBITS may contain the size in bits. */
	cdk_error_t cdk_pk_get_mpi(cdk_pubkey_t pk, size_t idx,
				   unsigned char *buf, size_t buflen,
				   size_t * r_nwritten, size_t * r_nbits);

/* Same as the function above but of the secret key. */
	cdk_error_t cdk_sk_get_mpi(cdk_seckey_t sk, size_t idx,
				   unsigned char *buf, size_t buflen,
				   size_t * r_nwritten, size_t * r_nbits);

/* Helper to get the exact number of multi precision integers
   for the given object. */
	int cdk_pk_get_nbits(cdk_pubkey_t pk);
	int cdk_pk_get_npkey(int algo);
	int cdk_pk_get_nskey(int algo);
	int cdk_pk_get_nsig(int algo);
	int cdk_pk_get_nenc(int algo);

/* Fingerprint and key ID routines. */

/* Calculate the fingerprint of the given public key.
   the FPR parameter must be at least 20 octets to hold the SHA1 hash. */
	cdk_error_t cdk_pk_get_fingerprint(cdk_pubkey_t pk,
					   unsigned char *fpr);

/* Same as above, but with additional sanity checks of the buffer size. */
	cdk_error_t cdk_pk_to_fingerprint(cdk_pubkey_t pk,
					  unsigned char *fpr,
					  size_t fprlen, size_t * r_nout);

/* Derive the keyid from the fingerprint. This is only possible for
   modern, version 4 keys. */
	unsigned int cdk_pk_fingerprint_get_keyid(const unsigned char *fpr,
						  size_t fprlen,
						  unsigned int *keyid);

/* Various functions to get the keyid from the specific packet type. */
	unsigned int cdk_pk_get_keyid(cdk_pubkey_t pk,
				      unsigned int *keyid);
	unsigned int cdk_sk_get_keyid(cdk_seckey_t sk,
				      unsigned int *keyid);
	unsigned int cdk_sig_get_keyid(cdk_pkt_signature_t sig,
				       unsigned int *keyid);

/* Key release functions. */
	void cdk_pk_release(cdk_pubkey_t pk);
	void cdk_sk_release(cdk_seckey_t sk);

/* Create a public key with the data from the secret key @SK. */
	cdk_error_t cdk_pk_from_secret_key(cdk_seckey_t sk,
					   cdk_pubkey_t * ret_pk);

/* Sexp conversion of keys. */
	cdk_error_t cdk_pubkey_to_sexp(cdk_pubkey_t pk, char **sexp,
				       size_t * len);
	cdk_error_t cdk_seckey_to_sexp(cdk_seckey_t sk, char **sexp,
				       size_t * len);


/* String to Key routines. */
	cdk_error_t cdk_s2k_new(cdk_s2k_t * ret_s2k, int mode,
				int digest_algo,
				const unsigned char *salt);
	void cdk_s2k_free(cdk_s2k_t s2k);

/* Protect the inbuf with ASCII armor of the specified type.
   If @outbuf and @outlen are NULL, the function returns the calculated
   size of the base64 encoded data in @nwritten. */
	cdk_error_t cdk_armor_encode_buffer(const unsigned char *inbuf,
					    size_t inlen, char *outbuf,
					    size_t outlen,
					    size_t * nwritten, int type);


/* This context contain user callbacks for different stream operations.
   Some of these callbacks might be NULL to indicate that the callback
   is not used. */
	struct cdk_stream_cbs_s {
		cdk_error_t(*open) (void *);
		cdk_error_t(*release) (void *);
		int (*read) (void *, void *buf, size_t);
		int (*write) (void *, const void *buf, size_t);
		int (*seek) (void *, off_t);
	};
	typedef struct cdk_stream_cbs_s *cdk_stream_cbs_t;

	int cdk_stream_is_compressed(cdk_stream_t s);

/* Return a stream object which is associated to a socket. */
	cdk_error_t cdk_stream_sockopen(const char *host,
					unsigned short port,
					cdk_stream_t * ret_out);

/* Return a stream object which is associated to an existing file. */
	cdk_error_t cdk_stream_open(const char *file,
				    cdk_stream_t * ret_s);

/* Return a stream object which is associated to a file which will
   be created when the stream is closed. */
	cdk_error_t cdk_stream_new(const char *file, cdk_stream_t * ret_s);

/* Return a stream object with custom callback functions for the
   various core operations. */
	cdk_error_t cdk_stream_new_from_cbs(cdk_stream_cbs_t cbs,
					    void *opa,
					    cdk_stream_t * ret_s);
	cdk_error_t cdk_stream_create(const char *file,
				      cdk_stream_t * ret_s);
	cdk_error_t cdk_stream_tmp_new(cdk_stream_t * r_out);
	cdk_error_t cdk_stream_tmp_from_mem(const void *buf, size_t buflen,
					    cdk_stream_t * r_out);
	void cdk_stream_tmp_set_mode(cdk_stream_t s, int val);
	cdk_error_t cdk_stream_flush(cdk_stream_t s);
	cdk_error_t cdk_stream_enable_cache(cdk_stream_t s, int val);
	cdk_error_t cdk_stream_filter_disable(cdk_stream_t s, int type);
	cdk_error_t cdk_stream_close(cdk_stream_t s);
	off_t cdk_stream_get_length(cdk_stream_t s);
	int cdk_stream_read(cdk_stream_t s, void *buf, size_t count);
	int cdk_stream_write(cdk_stream_t s, const void *buf,
			     size_t count);
	int cdk_stream_putc(cdk_stream_t s, int c);
	int cdk_stream_getc(cdk_stream_t s);
	int cdk_stream_eof(cdk_stream_t s);
	off_t cdk_stream_tell(cdk_stream_t s);
	cdk_error_t cdk_stream_seek(cdk_stream_t s, off_t offset);
	cdk_error_t cdk_stream_set_armor_flag(cdk_stream_t s, int type);

/* Push the literal filter for the given stream. */
	cdk_error_t cdk_stream_set_literal_flag(cdk_stream_t s,
						cdk_lit_format_t mode,
						const char *fname);

	cdk_error_t cdk_stream_set_compress_flag(cdk_stream_t s, int algo,
						 int level);
	cdk_error_t cdk_stream_set_hash_flag(cdk_stream_t s, int algo);
	cdk_error_t cdk_stream_set_text_flag(cdk_stream_t s,
					     const char *lf);
	cdk_error_t cdk_stream_kick_off(cdk_stream_t inp,
					cdk_stream_t out);
	cdk_error_t cdk_stream_mmap(cdk_stream_t s,
				    unsigned char **ret_buf,
				    size_t * ret_buflen);
	cdk_error_t cdk_stream_mmap_part(cdk_stream_t s, off_t off,
					 size_t len,
					 unsigned char **ret_buf,
					 size_t * ret_buflen);

/* Read from the stream but restore the file pointer after reading
   the requested amount of bytes. */
	int cdk_stream_peek(cdk_stream_t inp, unsigned char *buf,
			    size_t buflen);

/* Create a new key db handle from a memory buffer. */
	cdk_error_t cdk_keydb_new_from_mem(cdk_keydb_hd_t * r_hd,
					   int secret, int armor,
					   const void *data,
					   size_t datlen);

/* Check that a secret key with the given key ID is available. */
	cdk_error_t cdk_keydb_check_sk(cdk_keydb_hd_t hd,
				       unsigned int *keyid);

/* Prepare the key db search. */
	cdk_error_t cdk_keydb_search_start(cdk_keydb_search_t * st,
					   cdk_keydb_hd_t db, int type,
					   void *desc);

	void cdk_keydb_search_release(cdk_keydb_search_t st);

/* Return a key which matches a valid description given in
   cdk_keydb_search_start(). */
	cdk_error_t cdk_keydb_search(cdk_keydb_search_t st,
				     cdk_keydb_hd_t hd,
				     cdk_kbnode_t * ret_key);

/* Release the key db handle and all its resources. */
	void cdk_keydb_free(cdk_keydb_hd_t hd);

/* The following functions will try to find a key in the given key
   db handle either by keyid, by fingerprint or by some pattern. */
	cdk_error_t cdk_keydb_get_bykeyid(cdk_keydb_hd_t hd,
					  unsigned int *keyid,
					  cdk_kbnode_t * ret_pk);
	cdk_error_t cdk_keydb_get_byfpr(cdk_keydb_hd_t hd,
					const unsigned char *fpr,
					cdk_kbnode_t * ret_pk);
	cdk_error_t cdk_keydb_get_bypattern(cdk_keydb_hd_t hd,
					    const char *patt,
					    cdk_kbnode_t * ret_pk);

/* These function, in contrast to most other key db functions, only
   return the public or secret key packet without the additional
   signatures and user IDs. */
	cdk_error_t cdk_keydb_get_pk(cdk_keydb_hd_t khd,
				     unsigned int *keyid,
				     cdk_pubkey_t * ret_pk);
	cdk_error_t cdk_keydb_get_sk(cdk_keydb_hd_t khd,
				     unsigned int *keyid,
				     cdk_seckey_t * ret_sk);

/* Try to read the next key block from the given input stream.
   The key will be returned in @RET_KEY on success. */
	cdk_error_t cdk_keydb_get_keyblock(cdk_stream_t inp,
					   cdk_kbnode_t * ret_key,
					   unsigned public);

/* Rebuild the key db index if possible. */
	cdk_error_t cdk_keydb_idx_rebuild(cdk_keydb_hd_t db,
					  cdk_keydb_search_t dbs);

/* Export one or more keys from the given key db handle into
   the stream @OUT. The export is done by substring search and
   uses the string list @REMUSR for the pattern. */
	cdk_error_t cdk_keydb_export(cdk_keydb_hd_t hd, cdk_stream_t out,
				     cdk_strlist_t remusr);

/* Import the given key node @knode into the key db. */
	cdk_error_t cdk_keydb_import(cdk_keydb_hd_t hd,
				     cdk_kbnode_t knode);


/* List or enumerate keys from a given key db handle. */

/* Start the key list process. Either use @PATT for a pattern search
   or @FPATT for a list of pattern. */
	cdk_error_t cdk_listkey_start(cdk_listkey_t * r_ctx,
				      cdk_keydb_hd_t db, const char *patt,
				      cdk_strlist_t fpatt);
	void cdk_listkey_close(cdk_listkey_t ctx);

/* Return the next key which matches the pattern. */
	cdk_error_t cdk_listkey_next(cdk_listkey_t ctx,
				     cdk_kbnode_t * ret_key);

	cdk_kbnode_t cdk_kbnode_new(cdk_packet_t pkt);
	cdk_error_t cdk_kbnode_read_from_mem(cdk_kbnode_t * ret_node,
					     int armor,
					     const unsigned char *buf,
					     size_t buflen, unsigned public);
	cdk_error_t cdk_kbnode_write_to_mem(cdk_kbnode_t node,
					    unsigned char *buf,
					    size_t * r_nbytes);
	cdk_error_t cdk_kbnode_write_to_mem_alloc(cdk_kbnode_t node,
						  unsigned char **r_buf,
						  size_t * r_buflen);

	void cdk_kbnode_release(cdk_kbnode_t node);
	void cdk_kbnode_delete(cdk_kbnode_t node);
	void cdk_kbnode_insert(cdk_kbnode_t root, cdk_kbnode_t node,
			       cdk_packet_type_t pkttype);
	int cdk_kbnode_commit(cdk_kbnode_t * root);
	void cdk_kbnode_remove(cdk_kbnode_t * root, cdk_kbnode_t node);
	void cdk_kbnode_move(cdk_kbnode_t * root, cdk_kbnode_t node,
			     cdk_kbnode_t where);
	cdk_kbnode_t cdk_kbnode_walk(cdk_kbnode_t root, cdk_kbnode_t * ctx,
				     int all);
	cdk_packet_t cdk_kbnode_find_packet(cdk_kbnode_t node,
					    cdk_packet_type_t pkttype);
	cdk_packet_t cdk_kbnode_get_packet(cdk_kbnode_t node);
	cdk_kbnode_t cdk_kbnode_find(cdk_kbnode_t node,
				     cdk_packet_type_t pkttype);
	cdk_kbnode_t cdk_kbnode_find_prev(cdk_kbnode_t root,
					  cdk_kbnode_t node,
					  cdk_packet_type_t pkttype);
	cdk_kbnode_t cdk_kbnode_find_next(cdk_kbnode_t node,
					  cdk_packet_type_t pkttype);
	cdk_error_t cdk_kbnode_hash(cdk_kbnode_t node, digest_hd_st * md,
				    int is_v4, cdk_packet_type_t pkttype,
				    int flags);

/* Check each signature in the key node and return a summary of the
   key status in @r_status. Values of cdk_key_flag_t are used. */
	cdk_error_t cdk_pk_check_sigs(cdk_kbnode_t knode,
				      cdk_keydb_hd_t hd, int *r_status);

/* Check the self signature of the key to make sure it is valid. */
	cdk_error_t cdk_pk_check_self_sig(cdk_kbnode_t knode,
					  int *r_status);

/* Return a matching  algorithm from the given public key list.
   @PREFTYPE can be either sym-cipher/compress/digest. */
	int cdk_pklist_select_algo(cdk_keylist_t pkl, int preftype);

/* Return 0 or 1 if the given key list is able to understand the
   MDC feature. */
	int cdk_pklist_use_mdc(cdk_keylist_t pkl);
	cdk_error_t cdk_pklist_build(cdk_keylist_t * ret_pkl,
				     cdk_keydb_hd_t hd,
				     cdk_strlist_t remusr, int use);
	void cdk_pklist_release(cdk_keylist_t pkl);

/* Secret key lists */
	cdk_error_t cdk_sklist_build(cdk_keylist_t * ret_skl,
				     cdk_keydb_hd_t db, cdk_ctx_t hd,
				     cdk_strlist_t locusr,
				     int unlock, unsigned int use);
	void cdk_sklist_release(cdk_keylist_t skl);
	cdk_error_t cdk_sklist_write(cdk_keylist_t skl, cdk_stream_t outp,
				     digest_hd_st * mdctx, int sigclass,
				     int sigver);
	cdk_error_t cdk_sklist_write_onepass(cdk_keylist_t skl,
					     cdk_stream_t outp,
					     int sigclass, int mdalgo);

/* Encrypt the given stream @INP with the recipients given in @REMUSR.
   If @REMUSR is NULL, symmetric encryption will be used. The output
   will be written to @OUT. */
	cdk_error_t cdk_stream_encrypt(cdk_ctx_t hd, cdk_strlist_t remusr,
				       cdk_stream_t inp, cdk_stream_t out);

/* Decrypt the @INP stream into @OUT. */
	cdk_error_t cdk_stream_decrypt(cdk_ctx_t hd, cdk_stream_t inp,
				       cdk_stream_t out);

/* Same as the function above but it works on files. */
	cdk_error_t cdk_file_encrypt(cdk_ctx_t hd, cdk_strlist_t remusr,
				     const char *file, const char *output);
	cdk_error_t cdk_file_decrypt(cdk_ctx_t hd, const char *file,
				     const char *output);

/* Generic function to transform data. The mode can be either sign,
   verify, encrypt, decrypt, import or export. The meanings of the
   parameters are similar to the functions above.
   @OUTBUF will contain the output and @OUTSIZE the length of it. */
	cdk_error_t cdk_data_transform(cdk_ctx_t hd,
				       enum cdk_crypto_mode_t mode,
				       cdk_strlist_t locusr,
				       cdk_strlist_t remusr,
				       const void *inbuf, size_t insize,
				       unsigned char **outbuf,
				       size_t * outsize, int modval);

	int cdk_trustdb_get_validity(cdk_stream_t inp, cdk_pkt_userid_t id,
				     int *r_val);
	int cdk_trustdb_get_ownertrust(cdk_stream_t inp, cdk_pubkey_t pk,
				       int *r_val, int *r_flags);

	void cdk_strlist_free(cdk_strlist_t sl);
	cdk_strlist_t cdk_strlist_add(cdk_strlist_t * list,
				      const char *string);
	const char *cdk_check_version(const char *req_version);
/* UTF8 */
	char *cdk_utf8_encode(const char *string);
	char *cdk_utf8_decode(const char *string, size_t length,
			      int delim);

#ifdef __cplusplus
}
#endif
#endif				/* OPENCDK_H */
