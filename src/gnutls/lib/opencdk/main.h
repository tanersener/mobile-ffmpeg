/* main.h
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * The OpenCDK library is free software; you can redistribute it and/or
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

#ifndef CDK_MAIN_H
#define CDK_MAIN_H

#include "types.h"

#define _cdk_log_debug _gnutls_hard_log
#define _cdk_log_info _gnutls_debug_log
#define _cdk_get_log_level() _gnutls_log_level

#define cdk_malloc gnutls_malloc
#define cdk_free gnutls_free
#define cdk_calloc gnutls_calloc
#define cdk_realloc gnutls_realloc_fast
#define cdk_strdup gnutls_strdup
#define cdk_salloc gnutls_secure_calloc

#define map_gnutls_error _cdk_map_gnutls_error

cdk_error_t map_gnutls_error(int err);

/* The general size of a buffer for the variou modules. */
#define BUFSIZE 8192

/* This is the default block size for the partial length packet mode. */
#define DEF_BLOCKSIZE 8192
#define DEF_BLOCKBITS   13	/* 2^13 = 8192 */

/* For now SHA-1 is used to create fingerprint for keys.
   But if this will ever change, it is a good idea to
   have a constant for it to avoid to change it in all files. */
#define KEY_FPR_LEN 20

#include "context.h"

/* The maximal amount of bits a multi precsion integer can have. */
#define MAX_MPI_BITS 16384
#define MAX_MPI_BYTES (MAX_MPI_BITS/8)


/* Because newer DSA variants are not limited to SHA-1, we must consider 
 that SHA-512 is used and increase the buffer size of the digest. */
#define MAX_DIGEST_LEN 64

/* Helper to find out if the signature were made over a user ID
   or if the signature revokes a previous user ID. */
#define IS_UID_SIG(s) (((s)->sig_class & ~3) == 0x10)
#define IS_UID_REV(s) ((s)->sig_class == 0x30)

/* Helper to find out if a key has the requested capability. */
#define KEY_CAN_ENCRYPT(a) ((_cdk_pk_algo_usage ((a))) & CDK_KEY_USG_ENCR)
#define KEY_CAN_SIGN(a)    ((_cdk_pk_algo_usage ((a))) & CDK_KEY_USG_SIGN)
#define KEY_CAN_AUTH(a)    ((_cdk_pk_algo_usage ((a))) & CDK_KEY_USG_AUTH)

#define DEBUG_PKT 0

/*-- main.c --*/
char *_cdk_passphrase_get(cdk_ctx_t hd, const char *prompt);

/*-- misc.c --*/
int _cdk_check_args(int overwrite, const char *in, const char *out);
u32 _cdk_buftou32(const byte * buf);
void _cdk_u32tobuf(u32 u, byte * buf);
const char *_cdk_memistr(const char *buf, size_t buflen, const char *sub);
FILE *_cdk_tmpfile(void);

/* Helper to provide case insentensive strstr version. */
#define stristr(haystack, needle) \
    _cdk_memistr((haystack), strlen (haystack), (needle))

/*-- proc-packet.c --*/
cdk_error_t _cdk_pkt_write2(cdk_stream_t out, int pkttype, void *pktctx);

/*-- pubkey.c --*/
u32 _cdk_pkt_get_keyid(cdk_packet_t pkt, u32 * keyid);
cdk_error_t _cdk_pkt_get_fingerprint(cdk_packet_t pkt, byte * fpr);
int _cdk_pk_algo_usage(int algo);
int _cdk_pk_test_algo(int algo, unsigned int usage);
int _cdk_sk_get_csum(cdk_pkt_seckey_t sk);

/*-- new-packet.c --*/
byte *_cdk_subpkt_get_array(cdk_subpkt_t s, int count, size_t * r_nbytes);
cdk_error_t _cdk_subpkt_copy(cdk_subpkt_t * r_dst, cdk_subpkt_t src);
void _cdk_pkt_detach_free(cdk_packet_t pkt, int *r_pkttype, void **ctx);

/*-- sig-check.c --*/
cdk_error_t _cdk_sig_check(cdk_pkt_pubkey_t pk, cdk_pkt_signature_t sig,
			   digest_hd_st * digest, int *r_expired);
cdk_error_t _cdk_hash_sig_data(cdk_pkt_signature_t sig, digest_hd_st * hd);
cdk_error_t _cdk_hash_userid(cdk_pkt_userid_t uid, int sig_version,
			     digest_hd_st * md);
cdk_error_t _cdk_hash_pubkey(cdk_pkt_pubkey_t pk, digest_hd_st * md,
			     int use_fpr);
cdk_error_t _cdk_pk_check_sig(cdk_keydb_hd_t hd, cdk_kbnode_t knode,
			      cdk_kbnode_t snode, int *is_selfsig,
			      char **ret_uid);

/*-- kbnode.c --*/
void _cdk_kbnode_add(cdk_kbnode_t root, cdk_kbnode_t node);
void _cdk_kbnode_clone(cdk_kbnode_t node);

/*-- sesskey.c --*/
cdk_error_t _cdk_sk_unprotect_auto(cdk_ctx_t hd, cdk_pkt_seckey_t sk);

/*-- keydb.c --*/
int _cdk_keydb_is_secret(cdk_keydb_hd_t db);
cdk_error_t _cdk_keydb_get_pk_byusage(cdk_keydb_hd_t hd, const char *name,
				      cdk_pkt_pubkey_t * ret_pk,
				      int usage);
cdk_error_t _cdk_keydb_get_sk_byusage(cdk_keydb_hd_t hd, const char *name,
				      cdk_pkt_seckey_t * ret_sk,
				      int usage);
cdk_error_t _cdk_keydb_check_userid(cdk_keydb_hd_t hd, u32 * keyid,
				    const char *id);

/*-- sign.c --*/
int _cdk_sig_hash_for(cdk_pkt_pubkey_t pk);
cdk_error_t _cdk_sig_create(cdk_pkt_pubkey_t pk, cdk_pkt_signature_t sig);
cdk_error_t _cdk_sig_complete(cdk_pkt_signature_t sig, cdk_pkt_seckey_t sk,
			      digest_hd_st * hd);

/*-- stream.c --*/
void _cdk_stream_set_compress_algo(cdk_stream_t s, int algo);
cdk_error_t _cdk_stream_open_mode(const char *file, const char *mode,
				  cdk_stream_t * ret_s);
void *_cdk_stream_get_uint8_t(cdk_stream_t s, int fid);
const char *_cdk_stream_get_fname(cdk_stream_t s);
FILE *_cdk_stream_get_fp(cdk_stream_t s);
int _cdk_stream_gets(cdk_stream_t s, char *buf, size_t count);
cdk_error_t _cdk_stream_append(const char *file, cdk_stream_t * ret_s);
int _cdk_stream_get_errno(cdk_stream_t s);
cdk_error_t _cdk_stream_set_blockmode(cdk_stream_t s, size_t nbytes);
int _cdk_stream_get_blockmode(cdk_stream_t s);
int _cdk_stream_puts(cdk_stream_t s, const char *buf);
cdk_error_t _cdk_stream_fpopen(FILE * fp, unsigned write_mode,
			       cdk_stream_t * ret_out);


/*-- read-packet.c --*/
size_t _cdk_pkt_read_len(FILE * inp, size_t * ret_partial);

/*-- write-packet.c --*/
cdk_error_t _cdk_pkt_write_fp(FILE * out, cdk_packet_t pkt);

/*-- seskey.c --*/
cdk_error_t _cdk_s2k_copy(cdk_s2k_t * r_dst, cdk_s2k_t src);

#define _cdk_pub_algo_to_pgp(algo) (algo)
#define _pgp_pub_algo_to_cdk(algo) (algo)
int _gnutls_hash_algo_to_pgp(int algo);
int _pgp_hash_algo_to_gnutls(int algo);
int _gnutls_cipher_to_pgp(int cipher);
int _pgp_cipher_to_gnutls(int cipher);

#endif				/* CDK_MAIN_H */
