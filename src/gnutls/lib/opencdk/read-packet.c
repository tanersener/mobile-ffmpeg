/* read-packet.c - Read OpenPGP packets
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include "opencdk.h"
#include "main.h"
#include "packet.h"
#include "types.h"
#include <algorithms.h>
#include <str.h>
#include <minmax.h>

/* The version of the MDC packet considering the lastest OpenPGP draft. */

static int
stream_read(cdk_stream_t s, void *buf, size_t buflen, size_t * r_nread)
{
	int res = cdk_stream_read(s, buf, buflen);

	if (res > 0) {
		*r_nread = res;
		return 0;
	} else {
		return (cdk_stream_eof(s) ? EOF : _cdk_stream_get_errno(s));
	}
}


/* Try to read 4 octets from the stream. */
static u32 read_32(cdk_stream_t s)
{
	byte buf[4];
	size_t nread = 0;

	assert(s != NULL);

	stream_read(s, buf, 4, &nread);
	if (nread != 4)
		return (u32) -1;
	return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}


/* Try to read 2 octets from a stream. */
static u16 read_16(cdk_stream_t s)
{
	byte buf[2];
	size_t nread = 0;

	assert(s != NULL);

	stream_read(s, buf, 2, &nread);
	if (nread != 2)
		return (u16) - 1;
	return buf[0] << 8 | buf[1];
}


/* read about S2K at http://tools.ietf.org/html/rfc4880#section-3.7.1 */
static cdk_error_t read_s2k(cdk_stream_t inp, cdk_s2k_t s2k)
{
	size_t nread;

	s2k->mode = cdk_stream_getc(inp);
	s2k->hash_algo = cdk_stream_getc(inp);
	if (s2k->mode == CDK_S2K_SIMPLE)
		return 0;
	else if (s2k->mode == CDK_S2K_SALTED
		 || s2k->mode == CDK_S2K_ITERSALTED) {
		if (stream_read(inp, s2k->salt, DIM(s2k->salt), &nread))
			return CDK_Inv_Packet;
		if (nread != DIM(s2k->salt))
			return CDK_Inv_Packet;

		if (s2k->mode == CDK_S2K_ITERSALTED)
			s2k->count = cdk_stream_getc(inp);
	} else if (s2k->mode == CDK_S2K_GNU_EXT) {
		/* GNU extensions to the S2K : read DETAILS from gnupg */
		return 0;
	} else
		return CDK_Not_Implemented;

	return 0;
}


static cdk_error_t read_mpi(cdk_stream_t inp, bigint_t * ret_m, int secure)
{
	bigint_t m;
	int err;
	byte buf[MAX_MPI_BYTES + 2];
	size_t nread, nbits;
	cdk_error_t rc;

	if (!inp || !ret_m)
		return CDK_Inv_Value;

	*ret_m = NULL;
	nbits = read_16(inp);
	nread = (nbits + 7) / 8;

	if (nbits > MAX_MPI_BITS || nbits == 0) {
		_gnutls_write_log("read_mpi: too large %d bits\n",
				  (int) nbits);
		return gnutls_assert_val(CDK_MPI_Error);	/* Sanity check */
	}

	rc = stream_read(inp, buf + 2, nread, &nread);
	if (!rc && nread != ((nbits + 7) / 8)) {
		_gnutls_write_log("read_mpi: too short %d < %d\n",
				  (int) nread, (int) ((nbits + 7) / 8));
		return gnutls_assert_val(CDK_MPI_Error);
	}

	buf[0] = nbits >> 8;
	buf[1] = nbits >> 0;
	nread += 2;
	err = _gnutls_mpi_init_scan_pgp(&m, buf, nread);
	if (err < 0)
		return gnutls_assert_val(map_gnutls_error(err));

	*ret_m = m;
	return rc;
}


/* Read the encoded packet length directly from the file 
   object INP and return it. Reset RET_PARTIAL if this is
   the last packet in block mode. */
size_t _cdk_pkt_read_len(FILE * inp, size_t * ret_partial)
{
	int c1, c2;
	size_t pktlen;

	c1 = fgetc(inp);
	if (c1 == EOF)
		return (size_t) EOF;
	if (c1 < 224 || c1 == 255)
		*ret_partial = 0;	/* End of partial data */
	if (c1 < 192)
		pktlen = c1;
	else if (c1 >= 192 && c1 <= 223) {
		c2 = fgetc(inp);
		if (c2 == EOF)
			return (size_t) EOF;
		pktlen = ((c1 - 192) << 8) + c2 + 192;
	} else if (c1 == 255) {
		pktlen = fgetc(inp) << 24;
		pktlen |= fgetc(inp) << 16;
		pktlen |= fgetc(inp) << 8;
		pktlen |= fgetc(inp) << 0;
	} else
		pktlen = 1 << (c1 & 0x1f);
	return pktlen;
}


static cdk_error_t
read_pubkey_enc(cdk_stream_t inp, size_t pktlen, cdk_pkt_pubkey_enc_t pke)
{
	size_t i, nenc;

	if (!inp || !pke)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_pubkey_enc: %d octets\n",
				  (int) pktlen);

	if (pktlen < 12)
		return CDK_Inv_Packet;
	pke->version = cdk_stream_getc(inp);
	if (pke->version < 2 || pke->version > 3)
		return CDK_Inv_Packet;
	pke->keyid[0] = read_32(inp);
	pke->keyid[1] = read_32(inp);
	if (!pke->keyid[0] && !pke->keyid[1])
		pke->throw_keyid = 1;	/* RFC2440 "speculative" keyID */
	pke->pubkey_algo = _pgp_pub_algo_to_cdk(cdk_stream_getc(inp));
	nenc = cdk_pk_get_nenc(pke->pubkey_algo);
	if (!nenc)
		return CDK_Inv_Algo;
	for (i = 0; i < nenc; i++) {
		cdk_error_t rc = read_mpi(inp, &pke->mpi[i], 0);
		if (rc)
			return gnutls_assert_val(rc);
	}

	return 0;
}



static cdk_error_t read_mdc(cdk_stream_t inp, cdk_pkt_mdc_t mdc)
{
	size_t n;
	cdk_error_t rc;

	if (!inp || !mdc)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_mdc:\n");

	rc = stream_read(inp, mdc->hash, DIM(mdc->hash), &n);
	if (rc)
		return rc;

	return n != DIM(mdc->hash) ? CDK_Inv_Packet : 0;
}


static cdk_error_t
read_compressed(cdk_stream_t inp, size_t pktlen, cdk_pkt_compressed_t c)
{
	if (!inp || !c)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_compressed: %d octets\n",
				  (int) pktlen);

	c->algorithm = cdk_stream_getc(inp);
	if (c->algorithm > 3)
		return CDK_Inv_Packet;

	/* don't know the size, so we read until EOF */
	if (!pktlen) {
		c->len = 0;
		c->buf = inp;
	}

	/* FIXME: Support partial bodies. */
	return 0;
}


static cdk_error_t
read_public_key(cdk_stream_t inp, size_t pktlen, cdk_pkt_pubkey_t pk)
{
	size_t i, ndays, npkey;

	if (!inp || !pk)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_public_key: %d octets\n",
				  (int) pktlen);

	pk->is_invalid = 1;	/* default to detect missing self signatures */
	pk->is_revoked = 0;
	pk->has_expired = 0;

	pk->version = cdk_stream_getc(inp);
	if (pk->version < 2 || pk->version > 4)
		return CDK_Inv_Packet_Ver;
	pk->timestamp = read_32(inp);
	if (pk->version < 4) {
		ndays = read_16(inp);
		if (ndays)
			pk->expiredate = pk->timestamp + ndays * 86400L;
	}

	pk->pubkey_algo = _pgp_pub_algo_to_cdk(cdk_stream_getc(inp));
	npkey = cdk_pk_get_npkey(pk->pubkey_algo);
	if (!npkey) {
		gnutls_assert();
		_gnutls_write_log("invalid public key algorithm %d\n",
				  pk->pubkey_algo);
		return CDK_Inv_Algo;
	}
	for (i = 0; i < npkey; i++) {
		cdk_error_t rc = read_mpi(inp, &pk->mpi[i], 0);
		if (rc)
			return gnutls_assert_val(rc);
	}

	/* This value is just for the first run and will be
	   replaced with the actual key flags from the self signature. */
	pk->pubkey_usage = 0;
	return 0;
}


static cdk_error_t
read_public_subkey(cdk_stream_t inp, size_t pktlen, cdk_pkt_pubkey_t pk)
{
	if (!inp || !pk)
		return CDK_Inv_Value;
	return read_public_key(inp, pktlen, pk);
}

static cdk_error_t
read_secret_key(cdk_stream_t inp, size_t pktlen, cdk_pkt_seckey_t sk)
{
	size_t p1, p2, nread;
	int i, nskey;
	int rc;

	if (!inp || !sk || !sk->pk)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_secret_key: %d octets\n",
				  (int) pktlen);

	p1 = cdk_stream_tell(inp);
	rc = read_public_key(inp, pktlen, sk->pk);
	if (rc)
		return rc;

	sk->s2k_usage = cdk_stream_getc(inp);
	sk->protect.sha1chk = 0;
	if (sk->s2k_usage == 254 || sk->s2k_usage == 255) {
		sk->protect.sha1chk = (sk->s2k_usage == 254);
		sk->protect.algo =
		    _pgp_cipher_to_gnutls(cdk_stream_getc(inp));
		if (sk->protect.algo == GNUTLS_CIPHER_UNKNOWN)
			return gnutls_assert_val(CDK_Inv_Algo);

		sk->protect.s2k = cdk_calloc(1, sizeof *sk->protect.s2k);
		if (!sk->protect.s2k)
			return CDK_Out_Of_Core;
		rc = read_s2k(inp, sk->protect.s2k);
		if (rc)
			return rc;
		/* refer to --export-secret-subkeys in gpg(1) */
		if (sk->protect.s2k->mode == CDK_S2K_GNU_EXT)
			sk->protect.ivlen = 0;
		else {
			sk->protect.ivlen =
			    gnutls_cipher_get_block_size(sk->protect.algo);
			if (!sk->protect.ivlen)
				return CDK_Inv_Packet;
			rc = stream_read(inp, sk->protect.iv,
					 sk->protect.ivlen, &nread);
			if (rc)
				return rc;
			if (nread != sk->protect.ivlen)
				return CDK_Inv_Packet;
		}
	} else
		sk->protect.algo = _pgp_cipher_to_gnutls(sk->s2k_usage);
	if (sk->protect.algo == GNUTLS_CIPHER_UNKNOWN)
		return gnutls_assert_val(CDK_Inv_Algo);
	else if (sk->protect.algo == GNUTLS_CIPHER_NULL) {
		sk->csum = 0;
		nskey = cdk_pk_get_nskey(sk->pk->pubkey_algo);
		if (!nskey) {
			gnutls_assert();
			return CDK_Inv_Algo;
		}
		for (i = 0; i < nskey; i++) {
			rc = read_mpi(inp, &sk->mpi[i], 1);
			if (rc)
				return gnutls_assert_val(rc);
		}
		sk->csum = read_16(inp);
		sk->is_protected = 0;
	} else if (sk->pk->version < 4) {
		/* The length of each multiprecision integer is stored in plaintext. */
		nskey = cdk_pk_get_nskey(sk->pk->pubkey_algo);
		if (!nskey) {
			gnutls_assert();
			return CDK_Inv_Algo;
		}
		for (i = 0; i < nskey; i++) {
			rc = read_mpi(inp, &sk->mpi[i], 1);
			if (rc)
				return gnutls_assert_val(rc);
		}
		sk->csum = read_16(inp);
		sk->is_protected = 1;
	} else {
		/* We need to read the rest of the packet because we do not
		   have any information how long the encrypted mpi's are */
		p2 = cdk_stream_tell(inp);
		p2 -= p1;
		sk->enclen = pktlen - p2;
		if (sk->enclen < 2)
			return CDK_Inv_Packet;	/* at least 16 bits for the checksum! */
		sk->encdata = cdk_calloc(1, sk->enclen + 1);
		if (!sk->encdata)
			return CDK_Out_Of_Core;
		if (stream_read(inp, sk->encdata, sk->enclen, &nread))
			return CDK_Inv_Packet;
		/* Handle the GNU S2K extensions we know (just gnu-dummy right now): */
		if (sk->protect.s2k->mode == CDK_S2K_GNU_EXT) {
			unsigned char gnumode;
			if ((sk->enclen < strlen("GNU") + 1) ||
			    (0 !=
			     memcmp("GNU", sk->encdata, strlen("GNU"))))
				return CDK_Inv_Packet;
			gnumode = sk->encdata[strlen("GNU")];
			/* we only handle gnu-dummy (mode 1).
			   mode 2 should refer to external smart cards.
			 */
			if (gnumode != 1)
				return CDK_Inv_Packet;
			/* gnu-dummy should have no more data */
			if (sk->enclen != strlen("GNU") + 1)
				return CDK_Inv_Packet;
		}
		nskey = cdk_pk_get_nskey(sk->pk->pubkey_algo);
		if (!nskey) {
			gnutls_assert();
			return CDK_Inv_Algo;
		}
		/* We mark each MPI entry with NULL to indicate a protected key. */
		for (i = 0; i < nskey; i++)
			sk->mpi[i] = NULL;
		sk->is_protected = 1;
	}

	sk->is_primary = 1;
	_cdk_copy_pk_to_sk(sk->pk, sk);
	return 0;
}


static cdk_error_t
read_secret_subkey(cdk_stream_t inp, size_t pktlen, cdk_pkt_seckey_t sk)
{
	cdk_error_t rc;

	if (!inp || !sk || !sk->pk)
		return CDK_Inv_Value;

	rc = read_secret_key(inp, pktlen, sk);
	sk->is_primary = 0;
	return rc;
}

#define ATTRIBUTE "[attribute]"

static cdk_error_t
read_attribute(cdk_stream_t inp, size_t pktlen, cdk_pkt_userid_t attr,
	       int name_size)
{
	const byte *p;
	byte *buf;
	size_t len, nread;
	cdk_error_t rc;

	if (!inp || !attr || !pktlen)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_attribute: %d octets\n",
				  (int) pktlen);

	_gnutls_str_cpy(attr->name, name_size, ATTRIBUTE);
	attr->len = MIN(name_size, sizeof(ATTRIBUTE) - 1);

	buf = cdk_calloc(1, pktlen);
	if (!buf)
		return CDK_Out_Of_Core;
	rc = stream_read(inp, buf, pktlen, &nread);
	if (rc) {
		gnutls_assert();
		rc = CDK_Inv_Packet;
		goto error;
	}

	p = buf;
	len = *p++;
	pktlen--;

	if (len == 255) {
		if (pktlen < 4) {
			gnutls_assert();
			rc = CDK_Inv_Packet;
			goto error;
		}

		len = _cdk_buftou32(p);
		p += 4;
		pktlen -= 4;
	} else if (len >= 192) {
		if (pktlen < 2) {
			gnutls_assert();
			rc = CDK_Inv_Packet;
			goto error;
		}

		len = ((len - 192) << 8) + *p + 192;
		p++;
		pktlen--;
	}

	if (!len || pktlen == 0 || *p != 1) {	/* Currently only 1, meaning an image, is defined. */
		rc = CDK_Inv_Packet;
		goto error;
	}

	p++;
	len--;

	if (len >= pktlen) {
		rc = CDK_Inv_Packet;
		goto error;
	}

	attr->attrib_img = cdk_calloc(1, len);
	if (!attr->attrib_img) {
		rc = CDK_Out_Of_Core;
		goto error;
	}

	attr->attrib_len = len;
	memcpy(attr->attrib_img, p, len);
	cdk_free(buf);
	return rc;

 error:
	cdk_free(buf);
	return rc;
}


static cdk_error_t
read_user_id(cdk_stream_t inp, size_t pktlen, cdk_pkt_userid_t user_id)
{
	size_t nread;
	cdk_error_t rc;

	if (!inp || !user_id)
		return CDK_Inv_Value;
	if (!pktlen)
		return CDK_Inv_Packet;

	if (DEBUG_PKT)
		_gnutls_write_log("read_user_id: %lu octets\n",
				  (unsigned long) pktlen);

	user_id->len = pktlen;
	rc = stream_read(inp, user_id->name, pktlen, &nread);
	if (rc)
		return rc;
	if (nread != pktlen)
		return CDK_Inv_Packet;
	user_id->name[nread] = '\0';
	return rc;
}


#define MAX_PACKET_LEN (1<<24)


static cdk_error_t
read_subpkt(cdk_stream_t inp, cdk_subpkt_t * r_ctx, size_t * r_nbytes)
{
	int c, c1;
	size_t size, nread, n;
	cdk_subpkt_t node;
	cdk_error_t rc;

	if (!inp || !r_nbytes)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_subpkt:\n");

	n = 0;
	*r_nbytes = 0;
	c = cdk_stream_getc(inp);
	n++;

	if (c == 255) {
		size = read_32(inp);
		if (size == (u32)-1)
			return CDK_Inv_Packet;

		n += 4;
	} else if (c >= 192 && c < 255) {
		c1 = cdk_stream_getc(inp);
		if (c1 == EOF)
			return CDK_Inv_Packet;

		n++;
		if (c1 == 0)
			return 0;
		size = ((c - 192) << 8) + c1 + 192;
	} else if (c < 192)
		size = c;
	else
		return CDK_Inv_Packet;

	if (size >= MAX_PACKET_LEN) {
		return CDK_Inv_Packet;
	}

	node = cdk_subpkt_new(size);
	if (!node)
		return CDK_Out_Of_Core;
	node->size = size;
	node->type = cdk_stream_getc(inp);
	if (DEBUG_PKT)
		_gnutls_write_log(" %d octets %d type\n", node->size,
				  node->type);
	n++;
	node->size--;
	rc = stream_read(inp, node->d, node->size, &nread);
	n += nread;
	if (rc) {
		cdk_subpkt_free(node);
		return rc;
	}
	*r_nbytes = n;
	if (!*r_ctx)
		*r_ctx = node;
	else
		cdk_subpkt_add(*r_ctx, node);
	return rc;
}


static cdk_error_t
read_onepass_sig(cdk_stream_t inp, size_t pktlen,
		 cdk_pkt_onepass_sig_t sig)
{
	if (!inp || !sig)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_onepass_sig: %d octets\n",
				  (int) pktlen);

	if (pktlen != 13)
		return CDK_Inv_Packet;
	sig->version = cdk_stream_getc(inp);
	if (sig->version != 3)
		return CDK_Inv_Packet_Ver;
	sig->sig_class = cdk_stream_getc(inp);
	sig->digest_algo = _pgp_hash_algo_to_gnutls(cdk_stream_getc(inp));
	sig->pubkey_algo = _pgp_pub_algo_to_cdk(cdk_stream_getc(inp));
	sig->keyid[0] = read_32(inp);
	sig->keyid[1] = read_32(inp);
	sig->last = cdk_stream_getc(inp);
	return 0;
}


static cdk_error_t parse_sig_subpackets(cdk_pkt_signature_t sig)
{
	cdk_subpkt_t node;

	/* Setup the standard packet entries, so we can use V4
	   signatures similar to V3. */
	for (node = sig->unhashed; node; node = node->next) {
		if (node->type == CDK_SIGSUBPKT_ISSUER && node->size >= 8) {
			sig->keyid[0] = _cdk_buftou32(node->d);
			sig->keyid[1] = _cdk_buftou32(node->d + 4);
		} else if (node->type == CDK_SIGSUBPKT_EXPORTABLE
			   && node->d[0] == 0) {
			/* Sometimes this packet might be placed in the unhashed area */
			sig->flags.exportable = 0;
		}
	}
	for (node = sig->hashed; node; node = node->next) {
		if (node->type == CDK_SIGSUBPKT_SIG_CREATED
		    && node->size >= 4)
			sig->timestamp = _cdk_buftou32(node->d);
		else if (node->type == CDK_SIGSUBPKT_SIG_EXPIRE
			 && node->size >= 4) {
			sig->expiredate = _cdk_buftou32(node->d);
			if (sig->expiredate > 0
			    && sig->expiredate < (u32) gnutls_time(NULL))
				sig->flags.expired = 1;
		} else if (node->type == CDK_SIGSUBPKT_POLICY)
			sig->flags.policy_url = 1;
		else if (node->type == CDK_SIGSUBPKT_NOTATION)
			sig->flags.notation = 1;
		else if (node->type == CDK_SIGSUBPKT_REVOCABLE
			 && node->d[0] == 0)
			sig->flags.revocable = 0;
		else if (node->type == CDK_SIGSUBPKT_EXPORTABLE
			 && node->d[0] == 0)
			sig->flags.exportable = 0;
	}
	if (sig->sig_class == 0x1F) {
		cdk_desig_revoker_t r, rnode;

		for (node = sig->hashed; node; node = node->next) {
			if (node->type == CDK_SIGSUBPKT_REV_KEY) {
				if (node->size < 22)
					continue;
				rnode = cdk_calloc(1, sizeof *rnode);
				if (!rnode)
					return CDK_Out_Of_Core;
				rnode->r_class = node->d[0];
				rnode->algid = node->d[1];
				memcpy(rnode->fpr, node->d + 2,
				       KEY_FPR_LEN);
				if (!sig->revkeys)
					sig->revkeys = rnode;
				else {
					for (r = sig->revkeys; r->next;
					     r = r->next);
					r->next = rnode;
				}
			}
		}
	}

	return 0;
}


static cdk_error_t
read_signature(cdk_stream_t inp, size_t pktlen, cdk_pkt_signature_t sig)
{
	size_t nbytes;
	size_t i, nsig;
	ssize_t size;
	cdk_error_t rc;

	if (!inp || !sig)
		return gnutls_assert_val(CDK_Inv_Value);

	if (DEBUG_PKT)
		_gnutls_write_log("read_signature: %d octets\n",
				  (int) pktlen);

	if (pktlen < 16)
		return gnutls_assert_val(CDK_Inv_Packet);
	sig->version = cdk_stream_getc(inp);
	if (sig->version < 2 || sig->version > 4)
		return gnutls_assert_val(CDK_Inv_Packet_Ver);

	sig->flags.exportable = 1;
	sig->flags.revocable = 1;

	if (sig->version < 4) {
		if (cdk_stream_getc(inp) != 5)
			return gnutls_assert_val(CDK_Inv_Packet);
		sig->sig_class = cdk_stream_getc(inp);
		sig->timestamp = read_32(inp);
		sig->keyid[0] = read_32(inp);
		sig->keyid[1] = read_32(inp);
		sig->pubkey_algo =
		    _pgp_pub_algo_to_cdk(cdk_stream_getc(inp));
		sig->digest_algo =
		    _pgp_hash_algo_to_gnutls(cdk_stream_getc(inp));
		sig->digest_start[0] = cdk_stream_getc(inp);
		sig->digest_start[1] = cdk_stream_getc(inp);
		nsig = cdk_pk_get_nsig(sig->pubkey_algo);
		if (!nsig)
			return gnutls_assert_val(CDK_Inv_Algo);
		for (i = 0; i < nsig; i++) {
			rc = read_mpi(inp, &sig->mpi[i], 0);
			if (rc)
				return gnutls_assert_val(rc);
		}
	} else {
		sig->sig_class = cdk_stream_getc(inp);
		sig->pubkey_algo =
		    _pgp_pub_algo_to_cdk(cdk_stream_getc(inp));
		sig->digest_algo =
		    _pgp_hash_algo_to_gnutls(cdk_stream_getc(inp));
		sig->hashed_size = read_16(inp);
		size = sig->hashed_size;
		sig->hashed = NULL;
		while (size > 0) {
			rc = read_subpkt(inp, &sig->hashed, &nbytes);
			if (rc)
				return gnutls_assert_val(rc);
			size -= nbytes;
		}
		sig->unhashed_size = read_16(inp);
		size = sig->unhashed_size;
		sig->unhashed = NULL;
		while (size > 0) {
			rc = read_subpkt(inp, &sig->unhashed, &nbytes);
			if (rc)
				return gnutls_assert_val(rc);
			size -= nbytes;
		}

		rc = parse_sig_subpackets(sig);
		if (rc)
			return gnutls_assert_val(rc);

		sig->digest_start[0] = cdk_stream_getc(inp);
		sig->digest_start[1] = cdk_stream_getc(inp);
		nsig = cdk_pk_get_nsig(sig->pubkey_algo);
		if (!nsig)
			return gnutls_assert_val(CDK_Inv_Algo);
		for (i = 0; i < nsig; i++) {
			rc = read_mpi(inp, &sig->mpi[i], 0);
			if (rc)
				return gnutls_assert_val(rc);
		}
	}

	return 0;
}


static cdk_error_t
read_literal(cdk_stream_t inp, size_t pktlen,
	     cdk_pkt_literal_t * ret_pt, int is_partial)
{
	cdk_pkt_literal_t pt = *ret_pt;
	size_t nread;
	cdk_error_t rc;

	if (!inp || !pt)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_literal: %d octets\n",
				  (int) pktlen);

	pt->mode = cdk_stream_getc(inp);
	if (pt->mode != 0x62 && pt->mode != 0x74 && pt->mode != 0x75)
		return CDK_Inv_Packet;
	if (cdk_stream_eof(inp))
		return CDK_Inv_Packet;

	pt->namelen = cdk_stream_getc(inp);
	if (pt->namelen > 0) {
		*ret_pt = pt =
		    cdk_realloc(pt, sizeof *pt + pt->namelen + 2);
		if (!pt)
			return CDK_Out_Of_Core;
		pt->name = (char *) pt + sizeof(*pt);
		rc = stream_read(inp, pt->name, pt->namelen, &nread);
		if (rc)
			return rc;
		if ((int) nread != pt->namelen)
			return CDK_Inv_Packet;
		pt->name[pt->namelen] = '\0';
	}
	pt->timestamp = read_32(inp);
	pktlen = pktlen - 6 - pt->namelen;
	if (is_partial)
		_cdk_stream_set_blockmode(inp, pktlen);
	pt->buf = inp;
	pt->len = pktlen;
	return 0;
}


/* Read an old packet CTB and return the length of the body. */
static void
read_old_length(cdk_stream_t inp, int ctb, size_t * r_len, size_t * r_size)
{
	int llen = ctb & 0x03;
	int c;

	if (llen == 0) {
		c = cdk_stream_getc(inp);
		if (c == EOF)
			goto fail;

		*r_len = c;
		(*r_size)++;
	} else if (llen == 1) {
		*r_len = read_16(inp);
		if (*r_len == (u16)-1)
			goto fail;
		(*r_size) += 2;
	} else if (llen == 2) {
		*r_len = read_32(inp);
		if (*r_len == (u32)-1) {
			goto fail;
		}

		(*r_size) += 4;
	} else {
 fail:
		*r_len = 0;
		*r_size = 0;
	}
}


/* Read a new CTB and decode the body length. */
static void
read_new_length(cdk_stream_t inp,
		size_t * r_len, size_t * r_size, size_t * r_partial)
{
	int c, c1;

	c = cdk_stream_getc(inp);
	if (c == EOF)
		return;

	(*r_size)++;
	if (c < 192)
		*r_len = c;
	else if (c >= 192 && c <= 223) {
		c1 = cdk_stream_getc(inp);
		if (c1 == EOF)
			return;

		(*r_size)++;
		*r_len = ((c - 192) << 8) + c1 + 192;
	} else if (c == 255) {
		c1 = read_32(inp);
		if (c1 == -1) {
			return;
		}

		*r_len = c1;
		(*r_size) += 4;
	} else {
		*r_len = 1 << (c & 0x1f);
		*r_partial = 1;
	}
}


/* Skip the current packet body. */
static cdk_error_t skip_packet(cdk_stream_t inp, size_t pktlen)
{
	byte buf[BUFSIZE];
	size_t nread, buflen = DIM(buf);

	while (pktlen > 0) {
		cdk_error_t rc;
		rc = stream_read(inp, buf, pktlen > buflen ? buflen : pktlen,
			    &nread);
		if (rc)
			return rc;
		pktlen -= nread;
	}

	assert(pktlen == 0);
	return 0;
}

/**
 * cdk_pkt_read:
 * @inp: the input stream
 * @pkt: allocated packet handle to store the packet
 *
 * Parse the next packet on the @inp stream and return its contents in @pkt.
 **/
cdk_error_t cdk_pkt_read(cdk_stream_t inp, cdk_packet_t pkt, unsigned public)
{
	int ctb, is_newctb;
	int pkttype;
	size_t pktlen = 0, pktsize = 0, is_partial = 0;
	cdk_error_t rc;

	if (!inp || !pkt)
		return CDK_Inv_Value;

	ctb = cdk_stream_getc(inp);
	if (cdk_stream_eof(inp) || ctb == EOF)
		return CDK_EOF;
	else if (!ctb)
		return gnutls_assert_val(CDK_Inv_Packet);

	pktsize++;
	if (!(ctb & 0x80)) {
		_cdk_log_info("cdk_pkt_read: no openpgp data found. "
			      "(ctb=%02X; fpos=%02X)\n", (int) ctb,
			      (int) cdk_stream_tell(inp));
		return gnutls_assert_val(CDK_Inv_Packet);
	}

	if (ctb & 0x40) {	/* RFC2440 packet format. */
		pkttype = ctb & 0x3f;
		is_newctb = 1;
	} else {		/* the old RFC1991 packet format. */

		pkttype = ctb & 0x3f;
		pkttype >>= 2;
		is_newctb = 0;
	}

	if (pkttype > 63) {
		_cdk_log_info("cdk_pkt_read: unknown type %d\n", pkttype);
		return gnutls_assert_val(CDK_Inv_Packet);
	}

	if (is_newctb)
		read_new_length(inp, &pktlen, &pktsize, &is_partial);
	else
		read_old_length(inp, ctb, &pktlen, &pktsize);

	/* enforce limits to ensure that the following calculations
	 * do not overflow */
	if (pktlen >= MAX_PACKET_LEN || pktsize >= MAX_PACKET_LEN) {
		_cdk_log_info("cdk_pkt_read: too long packet\n");
		return gnutls_assert_val(CDK_Inv_Packet);
	}

	pkt->pkttype = pkttype;
	pkt->pktlen = pktlen;
	pkt->pktsize = pktsize + pktlen;
	pkt->old_ctb = is_newctb ? 0 : 1;

	rc = 0;
	switch (pkt->pkttype) {
	case CDK_PKT_ATTRIBUTE:
#define NAME_SIZE (pkt->pktlen + 16 + 1)
		pkt->pkt.user_id = cdk_calloc(1, sizeof *pkt->pkt.user_id
					      + NAME_SIZE);
		if (!pkt->pkt.user_id)
			return gnutls_assert_val(CDK_Out_Of_Core);
		pkt->pkt.user_id->name =
		    (char *) pkt->pkt.user_id + sizeof(*pkt->pkt.user_id);

		rc = read_attribute(inp, pktlen, pkt->pkt.user_id,
				    NAME_SIZE);
		pkt->pkttype = CDK_PKT_ATTRIBUTE;
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_USER_ID:

		pkt->pkt.user_id = cdk_calloc(1, sizeof *pkt->pkt.user_id
					      + pkt->pktlen + 1);
		if (!pkt->pkt.user_id)
			return gnutls_assert_val(CDK_Out_Of_Core);
		pkt->pkt.user_id->name =
		    (char *) pkt->pkt.user_id + sizeof(*pkt->pkt.user_id);
		rc = read_user_id(inp, pktlen, pkt->pkt.user_id);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_PUBLIC_KEY:
		pkt->pkt.public_key =
		    cdk_calloc(1, sizeof *pkt->pkt.public_key);
		if (!pkt->pkt.public_key)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_public_key(inp, pktlen, pkt->pkt.public_key);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_PUBLIC_SUBKEY:
		pkt->pkt.public_key =
		    cdk_calloc(1, sizeof *pkt->pkt.public_key);
		if (!pkt->pkt.public_key)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_public_subkey(inp, pktlen, pkt->pkt.public_key);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_SECRET_KEY:
		if (public) {
			/* read secret key when expecting public */
			return gnutls_assert_val(CDK_Inv_Packet);
		}
		pkt->pkt.secret_key =
		    cdk_calloc(1, sizeof *pkt->pkt.secret_key);
		if (!pkt->pkt.secret_key)
			return gnutls_assert_val(CDK_Out_Of_Core);
		pkt->pkt.secret_key->pk = cdk_calloc(1,
						     sizeof *pkt->pkt.
						     secret_key->pk);
		if (!pkt->pkt.secret_key->pk)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_secret_key(inp, pktlen, pkt->pkt.secret_key);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_SECRET_SUBKEY:
		if (public) {
			/* read secret key when expecting public */
			return gnutls_assert_val(CDK_Inv_Packet);
		}
		pkt->pkt.secret_key =
		    cdk_calloc(1, sizeof *pkt->pkt.secret_key);
		if (!pkt->pkt.secret_key)
			return gnutls_assert_val(CDK_Out_Of_Core);
		pkt->pkt.secret_key->pk = cdk_calloc(1,
						     sizeof *pkt->pkt.
						     secret_key->pk);
		if (!pkt->pkt.secret_key->pk)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_secret_subkey(inp, pktlen, pkt->pkt.secret_key);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_LITERAL:
		pkt->pkt.literal = cdk_calloc(1, sizeof *pkt->pkt.literal);
		if (!pkt->pkt.literal)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_literal(inp, pktlen, &pkt->pkt.literal,
				  is_partial);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_ONEPASS_SIG:
		pkt->pkt.onepass_sig =
		    cdk_calloc(1, sizeof *pkt->pkt.onepass_sig);
		if (!pkt->pkt.onepass_sig)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_onepass_sig(inp, pktlen, pkt->pkt.onepass_sig);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_SIGNATURE:
		pkt->pkt.signature =
		    cdk_calloc(1, sizeof *pkt->pkt.signature);
		if (!pkt->pkt.signature)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_signature(inp, pktlen, pkt->pkt.signature);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_PUBKEY_ENC:
		pkt->pkt.pubkey_enc =
		    cdk_calloc(1, sizeof *pkt->pkt.pubkey_enc);
		if (!pkt->pkt.pubkey_enc)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_pubkey_enc(inp, pktlen, pkt->pkt.pubkey_enc);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_COMPRESSED:
		pkt->pkt.compressed =
		    cdk_calloc(1, sizeof *pkt->pkt.compressed);
		if (!pkt->pkt.compressed)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_compressed(inp, pktlen, pkt->pkt.compressed);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	case CDK_PKT_MDC:
		pkt->pkt.mdc = cdk_calloc(1, sizeof *pkt->pkt.mdc);
		if (!pkt->pkt.mdc)
			return gnutls_assert_val(CDK_Out_Of_Core);
		rc = read_mdc(inp, pkt->pkt.mdc);
		if (rc)
			return gnutls_assert_val(rc);
		break;

	default:
		/* Skip all packets we don't understand */
		rc = skip_packet(inp, pktlen);
		if (rc)
			return gnutls_assert_val(rc);
		break;
	}

	return rc;
}
