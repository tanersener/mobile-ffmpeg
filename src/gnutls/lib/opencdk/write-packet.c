/* write-packet.c - Write OpenPGP packets
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
#include <assert.h>

#include "opencdk.h"
#include "main.h"


static int stream_write(cdk_stream_t s, const void *buf, size_t buflen)
{
	int nwritten;

	nwritten = cdk_stream_write(s, buf, buflen);
	if (nwritten == EOF)
		return _cdk_stream_get_errno(s);
	return 0;
}


static int
stream_read(cdk_stream_t s, void *buf, size_t buflen, size_t * r_nread)
{
	int nread;

	assert(r_nread);

	nread = cdk_stream_read(s, buf, buflen);
	if (nread == EOF)
		return _cdk_stream_get_errno(s);
	*r_nread = nread;
	return 0;
}


static int stream_putc(cdk_stream_t s, int c)
{
	int nwritten = cdk_stream_putc(s, c);
	if (nwritten == EOF)
		return _cdk_stream_get_errno(s);
	return 0;
}


static int write_32(cdk_stream_t out, u32 u)
{
	byte buf[4];

	buf[0] = u >> 24;
	buf[1] = u >> 16;
	buf[2] = u >> 8;
	buf[3] = u;
	return stream_write(out, buf, 4);
}


static int write_16(cdk_stream_t out, u16 u)
{
	byte buf[2];

	buf[0] = u >> 8;
	buf[1] = u;
	return stream_write(out, buf, 2);
}


static size_t calc_mpisize(bigint_t mpi[MAX_CDK_PK_PARTS], size_t ncount)
{
	size_t size, i;

	size = 0;
	for (i = 0; i < ncount; i++)
		size += (_gnutls_mpi_get_nbits(mpi[i]) + 7) / 8 + 2;
	return size;
}


static int write_mpi(cdk_stream_t out, bigint_t m)
{
	byte buf[MAX_MPI_BYTES + 2];
	size_t nbits, nread;
	int err;

	if (!out || !m)
		return CDK_Inv_Value;
	nbits = _gnutls_mpi_get_nbits(m);
	if (nbits > MAX_MPI_BITS || nbits < 1)
		return CDK_MPI_Error;

	nread = MAX_MPI_BYTES + 2;
	err = _gnutls_mpi_print_pgp(m, buf, &nread);
	if (err < 0)
		return map_gnutls_error(err);
	return stream_write(out, buf, nread);
}


static cdk_error_t
write_mpibuf(cdk_stream_t out, bigint_t mpi[MAX_CDK_PK_PARTS],
	     size_t count)
{
	size_t i;
	cdk_error_t rc;

	for (i = 0; i < count; i++) {
		rc = write_mpi(out, mpi[i]);
		if (rc)
			return rc;
	}
	return 0;
}


static cdk_error_t pkt_encode_len(cdk_stream_t out, size_t pktlen)
{
	cdk_error_t rc;

	if (!out)
		return CDK_Inv_Value;

	if (!pktlen) {
		/* Block mode, partial bodies, with 'DEF_BLOCKSIZE' from main.h */
		rc = stream_putc(out, (0xE0 | DEF_BLOCKBITS));
	} else if (pktlen < 192)
		rc = stream_putc(out, pktlen);
	else if (pktlen < 8384) {
		pktlen -= 192;
		rc = stream_putc(out, (pktlen >> 8) + 192);
		if (!rc)
			rc = stream_putc(out, (pktlen & 0xff));
	} else {
		rc = stream_putc(out, 255);
		if (!rc)
			rc = write_32(out, pktlen);
	}

	return rc;
}


static cdk_error_t write_head_new(cdk_stream_t out, size_t size, int type)
{
	cdk_error_t rc;

	if (!out)
		return CDK_Inv_Value;

	if (type < 0 || type > 63)
		return CDK_Inv_Packet;
	rc = stream_putc(out, (0xC0 | type));
	if (!rc)
		rc = pkt_encode_len(out, size);
	return rc;
}


static cdk_error_t write_head_old(cdk_stream_t out, size_t size, int type)
{
	cdk_error_t rc;
	int ctb;

	if (!out)
		return CDK_Inv_Value;

	if (type < 0 || type > 16)
		return CDK_Inv_Packet;
	ctb = 0x80 | (type << 2);
	if (!size)
		ctb |= 3;
	else if (size < 256);
	else if (size < 65536)
		ctb |= 1;
	else
		ctb |= 2;
	rc = stream_putc(out, ctb);
	if (!size)
		return rc;
	if (!rc) {
		if (size < 256)
			rc = stream_putc(out, size);
		else if (size < 65536)
			rc = write_16(out, size);
		else
			rc = write_32(out, size);
	}

	return rc;
}


/* Write special PGP2 packet header. PGP2 (wrongly) uses two byte header
   length for signatures and keys even if the size is < 256. */
static cdk_error_t pkt_write_head2(cdk_stream_t out, size_t size, int type)
{
	cdk_error_t rc;

	rc = cdk_stream_putc(out, 0x80 | (type << 2) | 1);
	if (!rc)
		rc = cdk_stream_putc(out, size >> 8);
	if (!rc)
		rc = cdk_stream_putc(out, size & 0xff);
	return rc;
}


static int
pkt_write_head(cdk_stream_t out, int old_ctb, size_t size, int type)
{
	if (old_ctb)
		return write_head_old(out, size, type);
	return write_head_new(out, size, type);
}


static int
write_pubkey_enc(cdk_stream_t out, cdk_pkt_pubkey_enc_t pke, int old_ctb)
{
	size_t size;
	int rc, nenc;

	if (!out || !pke)
		return CDK_Inv_Value;

	if (pke->version < 2 || pke->version > 3)
		return CDK_Inv_Packet;
	if (!KEY_CAN_ENCRYPT(pke->pubkey_algo))
		return CDK_Inv_Algo;

	if (DEBUG_PKT)
		_gnutls_write_log("write_pubkey_enc:\n");

	nenc = cdk_pk_get_nenc(pke->pubkey_algo);
	size = 10 + calc_mpisize(pke->mpi, nenc);
	rc = pkt_write_head(out, old_ctb, size, CDK_PKT_PUBKEY_ENC);
	if (rc)
		return rc;

	rc = stream_putc(out, pke->version);
	if (!rc)
		rc = write_32(out, pke->keyid[0]);
	if (!rc)
		rc = write_32(out, pke->keyid[1]);
	if (!rc)
		rc = stream_putc(out,
				 _cdk_pub_algo_to_pgp(pke->pubkey_algo));
	if (!rc)
		rc = write_mpibuf(out, pke->mpi, nenc);
	return rc;
}


static cdk_error_t write_mdc(cdk_stream_t out, cdk_pkt_mdc_t mdc)
{
	cdk_error_t rc;

	if (!out || !mdc)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("write_mdc:\n");

	/* This packet requires a fixed header encoding */
	rc = stream_putc(out, 0xD3);	/* packet ID and 1 byte length */
	if (!rc)
		rc = stream_putc(out, 0x14);
	if (!rc)
		rc = stream_write(out, mdc->hash, DIM(mdc->hash));
	return rc;
}


static size_t calc_subpktsize(cdk_subpkt_t s)
{
	size_t nbytes;

	/* In the count mode, no buffer is returned. */
	_cdk_subpkt_get_array(s, 1, &nbytes);
	return nbytes;
}


static cdk_error_t
write_v3_sig(cdk_stream_t out, cdk_pkt_signature_t sig, int nsig)
{
	size_t size;
	cdk_error_t rc;

	size = 19 + calc_mpisize(sig->mpi, nsig);
	if (is_RSA(sig->pubkey_algo))
		rc = pkt_write_head2(out, size, CDK_PKT_SIGNATURE);
	else
		rc = pkt_write_head(out, 1, size, CDK_PKT_SIGNATURE);
	if (!rc)
		rc = stream_putc(out, sig->version);
	if (!rc)
		rc = stream_putc(out, 5);
	if (!rc)
		rc = stream_putc(out, sig->sig_class);
	if (!rc)
		rc = write_32(out, sig->timestamp);
	if (!rc)
		rc = write_32(out, sig->keyid[0]);
	if (!rc)
		rc = write_32(out, sig->keyid[1]);
	if (!rc)
		rc = stream_putc(out,
				 _cdk_pub_algo_to_pgp(sig->pubkey_algo));
	if (!rc)
		rc = stream_putc(out,
				 _gnutls_hash_algo_to_pgp(sig->
							  digest_algo));
	if (!rc)
		rc = stream_putc(out, sig->digest_start[0]);
	if (!rc)
		rc = stream_putc(out, sig->digest_start[1]);
	if (!rc)
		rc = write_mpibuf(out, sig->mpi, nsig);
	return rc;
}


static cdk_error_t
write_signature(cdk_stream_t out, cdk_pkt_signature_t sig, int old_ctb)
{
	byte *buf;
	size_t nbytes, size, nsig;
	cdk_error_t rc;

	if (!out || !sig)
		return CDK_Inv_Value;

	if (!KEY_CAN_SIGN(sig->pubkey_algo))
		return gnutls_assert_val(CDK_Inv_Algo);
	if (sig->version < 2 || sig->version > 4)
		return gnutls_assert_val(CDK_Inv_Packet);

	if (DEBUG_PKT)
		_gnutls_write_log("write_signature:\n");

	nsig = cdk_pk_get_nsig(sig->pubkey_algo);
	if (!nsig)
		return gnutls_assert_val(CDK_Inv_Algo);
	if (sig->version < 4)
		return write_v3_sig(out, sig, nsig);

	size = 10 + calc_subpktsize(sig->hashed)
	    + calc_subpktsize(sig->unhashed) + calc_mpisize(sig->mpi,
							    nsig);

	rc = pkt_write_head(out, 0, size, CDK_PKT_SIGNATURE);
	if (rc)
		return gnutls_assert_val(rc);

	rc = stream_putc(out, 4);
	if (rc)
		return gnutls_assert_val(rc);

	rc = stream_putc(out, sig->sig_class);
	if (rc)
		return gnutls_assert_val(rc);

	rc = stream_putc(out, _cdk_pub_algo_to_pgp(sig->pubkey_algo));
	if (rc)
		return gnutls_assert_val(rc);

	rc = stream_putc(out, _gnutls_hash_algo_to_pgp(sig->digest_algo));
	if (rc)
		return gnutls_assert_val(rc);

	rc = write_16(out, sig->hashed_size);
	if (rc)
		return gnutls_assert_val(rc);

	buf = _cdk_subpkt_get_array(sig->hashed, 0, &nbytes);
	if (!buf)
		return gnutls_assert_val(CDK_Out_Of_Core);

	rc = stream_write(out, buf, nbytes);
	cdk_free(buf);
	if (rc)
		return gnutls_assert_val(rc);

	rc = write_16(out, sig->unhashed_size);
	if (rc)
		return gnutls_assert_val(rc);

	buf = _cdk_subpkt_get_array(sig->unhashed, 0, &nbytes);
	if (!buf)
		return gnutls_assert_val(CDK_Out_Of_Core);

	rc = stream_write(out, buf, nbytes);
	cdk_free(buf);
	if (rc)
		return gnutls_assert_val(rc);

	rc = stream_putc(out, sig->digest_start[0]);
	if (rc)
		return gnutls_assert_val(rc);

	rc = stream_putc(out, sig->digest_start[1]);
	if (rc)
		return gnutls_assert_val(rc);

	rc = write_mpibuf(out, sig->mpi, nsig);
	if (rc)
		return gnutls_assert_val(rc);

	return 0;
}


static cdk_error_t
write_public_key(cdk_stream_t out, cdk_pkt_pubkey_t pk,
		 int is_subkey, int old_ctb)
{
	int pkttype, ndays = 0;
	size_t npkey = 0, size = 6;
	cdk_error_t rc;

	if (!out || !pk)
		return CDK_Inv_Value;

	if (pk->version < 2 || pk->version > 4)
		return CDK_Inv_Packet;

	if (DEBUG_PKT)
		_gnutls_write_log("write_public_key: subkey=%d\n",
				  is_subkey);

	pkttype = is_subkey ? CDK_PKT_PUBLIC_SUBKEY : CDK_PKT_PUBLIC_KEY;
	npkey = cdk_pk_get_npkey(pk->pubkey_algo);
	if (!npkey)
		return CDK_Inv_Algo;
	if (pk->version < 4)
		size += 2;	/* expire date */
	if (is_subkey)
		old_ctb = 0;
	size += calc_mpisize(pk->mpi, npkey);
	if (old_ctb)
		rc = pkt_write_head2(out, size, pkttype);
	else
		rc = pkt_write_head(out, old_ctb, size, pkttype);
	if (!rc)
		rc = stream_putc(out, pk->version);
	if (!rc)
		rc = write_32(out, pk->timestamp);
	if (!rc && pk->version < 4) {
		if (pk->expiredate)
			ndays =
			    (u16) ((pk->expiredate -
				    pk->timestamp) / 86400L);
		rc = write_16(out, ndays);
	}
	if (!rc)
		rc = stream_putc(out,
				 _cdk_pub_algo_to_pgp(pk->pubkey_algo));
	if (!rc)
		rc = write_mpibuf(out, pk->mpi, npkey);
	return rc;
}


static int calc_s2ksize(cdk_pkt_seckey_t sk)
{
	size_t nbytes = 0;

	if (!sk->is_protected)
		return 0;
	switch (sk->protect.s2k->mode) {
	case CDK_S2K_SIMPLE:
		nbytes = 2;
		break;
	case CDK_S2K_SALTED:
		nbytes = 10;
		break;
	case CDK_S2K_ITERSALTED:
		nbytes = 11;
		break;
	case CDK_S2K_GNU_EXT:
		nbytes = 2;
		break;
	}
	nbytes += sk->protect.ivlen;
	nbytes++;		/* single cipher byte */
	return nbytes;
}


static cdk_error_t
write_secret_key(cdk_stream_t out, cdk_pkt_seckey_t sk,
		 int is_subkey, int old_ctb)
{
	cdk_pkt_pubkey_t pk = NULL;
	size_t size = 6, npkey, nskey;
	int pkttype, s2k_mode;
	cdk_error_t rc;

	if (!out || !sk)
		return CDK_Inv_Value;

	if (!sk->pk)
		return CDK_Inv_Value;
	pk = sk->pk;
	if (pk->version < 2 || pk->version > 4)
		return CDK_Inv_Packet;

	if (DEBUG_PKT)
		_gnutls_write_log("write_secret_key:\n");

	npkey = cdk_pk_get_npkey(pk->pubkey_algo);
	nskey = cdk_pk_get_nskey(pk->pubkey_algo);
	if (!npkey || !nskey) {
		gnutls_assert();
		return CDK_Inv_Algo;
	}
	if (pk->version < 4)
		size += 2;
	/* If the key is unprotected, the 1 extra byte:
	   1 octet  - cipher algorithm byte (0x00)
	   the other bytes depend on the mode:
	   a) simple checksum -  2 octets
	   b) sha-1 checksum  - 20 octets */
	size = !sk->is_protected ? size + 1 : size + 1 + calc_s2ksize(sk);
	size += calc_mpisize(pk->mpi, npkey);
	if (sk->version == 3 || !sk->is_protected) {
		if (sk->version == 3) {
			size += 2;	/* force simple checksum */
			sk->protect.sha1chk = 0;
		} else
			size += sk->protect.sha1chk ? 20 : 2;
		size += calc_mpisize(sk->mpi, nskey);
	} else			/* We do not know anything about the encrypted mpi's so we
				   treat the data as uint8_t. */
		size += sk->enclen;

	pkttype = is_subkey ? CDK_PKT_SECRET_SUBKEY : CDK_PKT_SECRET_KEY;
	rc = pkt_write_head(out, old_ctb, size, pkttype);
	if (!rc)
		rc = stream_putc(out, pk->version);
	if (!rc)
		rc = write_32(out, pk->timestamp);
	if (!rc && pk->version < 4) {
		u16 ndays = 0;
		if (pk->expiredate)
			ndays =
			    (u16) ((pk->expiredate -
				    pk->timestamp) / 86400L);
		rc = write_16(out, ndays);
	}
	if (!rc)
		rc = stream_putc(out,
				 _cdk_pub_algo_to_pgp(pk->pubkey_algo));

	if (!rc)
		rc = write_mpibuf(out, pk->mpi, npkey);

	if (!rc) {
		if (sk->is_protected == 0)
			rc = stream_putc(out, 0x00);
		else {
			if (is_RSA(pk->pubkey_algo) && pk->version < 4)
				rc = stream_putc(out,
						 _gnutls_cipher_to_pgp(sk->
								       protect.
								       algo));
			else if (sk->protect.s2k) {
				s2k_mode = sk->protect.s2k->mode;
				rc = stream_putc(out,
						 sk->protect.
						 sha1chk ? 0xFE : 0xFF);
				if (!rc)
					rc = stream_putc(out,
							 _gnutls_cipher_to_pgp
							 (sk->protect.
							  algo));
				if (!rc)
					rc = stream_putc(out,
							 sk->protect.s2k->
							 mode);
				if (!rc)
					rc = stream_putc(out,
							 sk->protect.s2k->
							 hash_algo);
				if (!rc
				    && (s2k_mode == 1 || s2k_mode == 3)) {
					rc = stream_write(out,
							  sk->protect.s2k->
							  salt, 8);
					if (!rc && s2k_mode == 3)
						rc = stream_putc(out,
								 sk->
								 protect.
								 s2k->
								 count);
				}
			} else
				return CDK_Inv_Value;
			if (!rc)
				rc = stream_write(out, sk->protect.iv,
						  sk->protect.ivlen);
		}
	}
	if (!rc && sk->is_protected && pk->version == 4) {
		if (sk->encdata && sk->enclen)
			rc = stream_write(out, sk->encdata, sk->enclen);
	} else {
		if (!rc)
			rc = write_mpibuf(out, sk->mpi, nskey);
		if (!rc) {
			if (!sk->csum)
				sk->csum = _cdk_sk_get_csum(sk);
			rc = write_16(out, sk->csum);
		}
	}

	return rc;
}


static cdk_error_t
write_compressed(cdk_stream_t out, cdk_pkt_compressed_t cd)
{
	cdk_error_t rc;

	if (!out || !cd)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("packet: write_compressed\n");

	/* Use an old (RFC1991) header for this packet. */
	rc = pkt_write_head(out, 1, 0, CDK_PKT_COMPRESSED);
	if (!rc)
		rc = stream_putc(out, cd->algorithm);
	return rc;
}


static cdk_error_t
write_literal(cdk_stream_t out, cdk_pkt_literal_t pt, int old_ctb)
{
	byte buf[BUFSIZE];
	size_t size;
	cdk_error_t rc;

	if (!out || !pt)
		return CDK_Inv_Value;

	/* We consider a packet without a body as an invalid packet.
	   At least one octet must be present. */
	if (!pt->len)
		return CDK_Inv_Packet;

	if (DEBUG_PKT)
		_gnutls_write_log("write_literal:\n");

	size = 6 + pt->namelen + pt->len;
	rc = pkt_write_head(out, old_ctb, size, CDK_PKT_LITERAL);
	if (rc)
		return rc;

	rc = stream_putc(out, pt->mode);
	if (rc)
		return rc;
	rc = stream_putc(out, pt->namelen);
	if (rc)
		return rc;

	if (pt->namelen > 0)
		rc = stream_write(out, pt->name, pt->namelen);
	if (!rc)
		rc = write_32(out, pt->timestamp);
	if (rc)
		return rc;

	while (!cdk_stream_eof(pt->buf) && !rc) {
		rc = stream_read(pt->buf, buf, DIM(buf), &size);
		if (!rc)
			rc = stream_write(out, buf, size);
	}

	memset(buf, 0, sizeof(buf));
	return rc;
}


static cdk_error_t
write_onepass_sig(cdk_stream_t out, cdk_pkt_onepass_sig_t sig)
{
	cdk_error_t rc;

	if (!out || !sig)
		return CDK_Inv_Value;

	if (sig->version != 3)
		return CDK_Inv_Packet;

	if (DEBUG_PKT)
		_gnutls_write_log("write_onepass_sig:\n");

	rc = pkt_write_head(out, 0, 13, CDK_PKT_ONEPASS_SIG);
	if (!rc)
		rc = stream_putc(out, sig->version);
	if (!rc)
		rc = stream_putc(out, sig->sig_class);
	if (!rc)
		rc = stream_putc(out,
				 _gnutls_hash_algo_to_pgp(sig->
							  digest_algo));
	if (!rc)
		rc = stream_putc(out,
				 _cdk_pub_algo_to_pgp(sig->pubkey_algo));
	if (!rc)
		rc = write_32(out, sig->keyid[0]);
	if (!rc)
		rc = write_32(out, sig->keyid[1]);
	if (!rc)
		rc = stream_putc(out, sig->last);
	return rc;
}


static cdk_error_t
write_user_id(cdk_stream_t out, cdk_pkt_userid_t id, int old_ctb,
	      int pkttype)
{
	cdk_error_t rc;

	if (!out || !id)
		return CDK_Inv_Value;

	if (pkttype == CDK_PKT_ATTRIBUTE) {
		if (!id->attrib_img)
			return CDK_Inv_Value;
		rc = pkt_write_head(out, old_ctb, id->attrib_len + 6,
				    CDK_PKT_ATTRIBUTE);
		if (rc)
			return rc;
		/* Write subpacket part. */
		stream_putc(out, 255);
		write_32(out, id->attrib_len + 1);
		stream_putc(out, 1);
		rc = stream_write(out, id->attrib_img, id->attrib_len);
	} else {
		if (!id->name)
			return CDK_Inv_Value;
		rc = pkt_write_head(out, old_ctb, id->len,
				    CDK_PKT_USER_ID);
		if (!rc)
			rc = stream_write(out, id->name, id->len);
	}

	return rc;
}


/**
 * cdk_pkt_write:
 * @out: the output stream handle
 * @pkt: the packet itself
 *
 * Write the contents of @pkt into the @out stream.
 * Return 0 on success.
 **/
cdk_error_t cdk_pkt_write(cdk_stream_t out, cdk_packet_t pkt)
{
	cdk_error_t rc;

	if (!out || !pkt)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("write packet pkttype=%d\n",
				  pkt->pkttype);

	switch (pkt->pkttype) {
	case CDK_PKT_LITERAL:
		rc = write_literal(out, pkt->pkt.literal, pkt->old_ctb);
		break;
	case CDK_PKT_ONEPASS_SIG:
		rc = write_onepass_sig(out, pkt->pkt.onepass_sig);
		break;
	case CDK_PKT_MDC:
		rc = write_mdc(out, pkt->pkt.mdc);
		break;
	case CDK_PKT_PUBKEY_ENC:
		rc = write_pubkey_enc(out, pkt->pkt.pubkey_enc,
				      pkt->old_ctb);
		break;
	case CDK_PKT_SIGNATURE:
		rc = write_signature(out, pkt->pkt.signature,
				     pkt->old_ctb);
		break;
	case CDK_PKT_PUBLIC_KEY:
		rc = write_public_key(out, pkt->pkt.public_key, 0,
				      pkt->old_ctb);
		break;
	case CDK_PKT_PUBLIC_SUBKEY:
		rc = write_public_key(out, pkt->pkt.public_key, 1,
				      pkt->old_ctb);
		break;
	case CDK_PKT_COMPRESSED:
		rc = write_compressed(out, pkt->pkt.compressed);
		break;
	case CDK_PKT_SECRET_KEY:
		rc = write_secret_key(out, pkt->pkt.secret_key, 0,
				      pkt->old_ctb);
		break;
	case CDK_PKT_SECRET_SUBKEY:
		rc = write_secret_key(out, pkt->pkt.secret_key, 1,
				      pkt->old_ctb);
		break;
	case CDK_PKT_USER_ID:
	case CDK_PKT_ATTRIBUTE:
		rc = write_user_id(out, pkt->pkt.user_id, pkt->old_ctb,
				   pkt->pkttype);
		break;
	default:
		rc = CDK_Inv_Packet;
		break;
	}

	if (DEBUG_PKT)
		_gnutls_write_log("write_packet rc=%d pkttype=%d\n", rc,
				  pkt->pkttype);
	return rc;
}


cdk_error_t _cdk_pkt_write2(cdk_stream_t out, int pkttype, void *pktctx)
{
	cdk_packet_t pkt;
	cdk_error_t rc;

	rc = cdk_pkt_new(&pkt);
	if (rc)
		return rc;

	switch (pkttype) {
	case CDK_PKT_PUBLIC_KEY:
	case CDK_PKT_PUBLIC_SUBKEY:
		pkt->pkt.public_key = pktctx;
		break;
	case CDK_PKT_SIGNATURE:
		pkt->pkt.signature = pktctx;
		break;
	case CDK_PKT_SECRET_KEY:
	case CDK_PKT_SECRET_SUBKEY:
		pkt->pkt.secret_key = pktctx;
		break;

	case CDK_PKT_USER_ID:
		pkt->pkt.user_id = pktctx;
		break;
	}
	pkt->pkttype = pkttype;
	rc = cdk_pkt_write(out, pkt);
	cdk_free(pkt);
	return rc;
}


cdk_error_t _cdk_pkt_write_fp(FILE * out, cdk_packet_t pkt)
{
	cdk_stream_t so;
	cdk_error_t rc;

	rc = _cdk_stream_fpopen(out, 1, &so);
	if (rc)
		return rc;
	rc = cdk_pkt_write(so, pkt);
	cdk_stream_close(so);
	return rc;
}
