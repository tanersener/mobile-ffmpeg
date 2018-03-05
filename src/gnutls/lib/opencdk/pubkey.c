/* pubkey.c - Public key API
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "gnutls_int.h"
#include <datum.h>
#include <pk.h>
#include <tls-sig.h>
#include <x509/common.h>
#include "opencdk.h"
#include "main.h"
#include "packet.h"

/* This function gets the signature parameters and encodes
 * them into a way for _gnutls_pk_verify to use.
 */
static cdk_error_t
sig_to_datum(gnutls_datum_t * r_sig, cdk_pkt_signature_t sig)
{
	int err;
	cdk_error_t rc;

	if (!r_sig || !sig)
		return CDK_Inv_Value;

	rc = 0;
	if (is_RSA(sig->pubkey_algo)) {
		err = _gnutls_mpi_dprint(sig->mpi[0], r_sig);
		if (err < 0)
			rc = map_gnutls_error(err);
	} else if (is_DSA(sig->pubkey_algo)) {
		err =
		    _gnutls_encode_ber_rs(r_sig, sig->mpi[0], sig->mpi[1]);
		if (err < 0)
			rc = map_gnutls_error(err);
	} else
		rc = CDK_Inv_Algo;
	return rc;
}

/**
 * cdk_pk_verify:
 * @pk: the public key
 * @sig: signature
 * @md: the message digest
 *
 * Verify the signature in @sig and compare it with the message digest in @md.
 **/
cdk_error_t
cdk_pk_verify(cdk_pubkey_t pk, cdk_pkt_signature_t sig, const byte * md)
{
	gnutls_datum_t s_sig = { NULL, 0 }, di = {
	NULL, 0};
	byte *encmd = NULL;
	cdk_error_t rc;
	int ret, algo;
	unsigned int i;
	gnutls_pk_params_st params;
	const mac_entry_st *me;

	if (!pk || !sig || !md) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	if (is_DSA(pk->pubkey_algo))
		algo = GNUTLS_PK_DSA;
	else if (is_RSA(pk->pubkey_algo))
		algo = GNUTLS_PK_RSA;
	else {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	rc = sig_to_datum(&s_sig, sig);
	if (rc) {
		gnutls_assert();
		goto leave;
	}

	me = mac_to_entry(sig->digest_algo);
	rc = _gnutls_set_datum(&di, md, _gnutls_hash_get_algo_len(me));
	if (rc < 0) {
		rc = gnutls_assert_val(CDK_Out_Of_Core);
		goto leave;
	}

	rc = pk_prepare_hash(algo, me, &di);
	if (rc < 0) {
		rc = gnutls_assert_val(CDK_General_Error);
		goto leave;
	}

	params.params_nr = cdk_pk_get_npkey(pk->pubkey_algo);
	for (i = 0; i < params.params_nr; i++)
		params.params[i] = pk->mpi[i];
	params.flags = 0;
	ret = _gnutls_pk_verify(algo, &di, &s_sig, &params);

	if (ret < 0) {
		gnutls_assert();
		rc = map_gnutls_error(ret);
		goto leave;
	}

	rc = 0;

      leave:
	_gnutls_free_datum(&s_sig);
	_gnutls_free_datum(&di);
	cdk_free(encmd);
	return rc;
}


/**
 * cdk_pk_get_nbits:
 * @pk: the public key
 * 
 * Return the length of the public key in bits.
 * The meaning of length is actually the size of the 'prime'
 * object in the key. For RSA keys the modulus, for ElG/DSA
 * the size of the public prime.
 **/
int cdk_pk_get_nbits(cdk_pubkey_t pk)
{
	if (!pk || !pk->mpi[0])
		return 0;
	return _gnutls_mpi_get_nbits(pk->mpi[0]);
}


/**
 * cdk_pk_get_npkey:
 * @algo: The public key algorithm.
 * 
 * Return the number of multiprecison integer forming an public
 * key with the given algorithm.
 */
int cdk_pk_get_npkey(int algo)
{
	if (is_RSA(algo))
		return RSA_PUBLIC_PARAMS;
	else if (is_DSA(algo))
		return DSA_PUBLIC_PARAMS;
	else if (is_ELG(algo))
		return 3;
	else {
		gnutls_assert();
		return 0;
	}
}


/**
 * cdk_pk_get_nskey:
 * @algo: the public key algorithm
 * 
 * Return the number of multiprecision integers forming an
 * secret key with the given algorithm.
 **/
int cdk_pk_get_nskey(int algo)
{
	int ret;

	if (is_RSA(algo))
		ret = RSA_PRIVATE_PARAMS - 2;	/* we don't have exp1 and exp2 */
	else if (is_DSA(algo))
		ret = DSA_PRIVATE_PARAMS;
	else if (is_ELG(algo))
		ret = 4;
	else {
		gnutls_assert();
		return 0;
	}

	ret -= cdk_pk_get_npkey(algo);
	return ret;
}


/**
 * cdk_pk_get_nbits:
 * @algo: the public key algorithm
 * 
 * Return the number of MPIs a signature consists of.
 **/
int cdk_pk_get_nsig(int algo)
{
	if (is_RSA(algo))
		return 1;
	else if (is_DSA(algo))
		return 2;
	else
		return 0;
}


/**
 * cdk_pk_get_nenc: 
 * @algo: the public key algorithm
 * 
 * Return the number of MPI's the encrypted data consists of.
 **/
int cdk_pk_get_nenc(int algo)
{
	if (is_RSA(algo))
		return 1;
	else if (is_ELG(algo))
		return 2;
	else
		return 0;
}


int _cdk_pk_algo_usage(int algo)
{
	int usage;

	/* The ElGamal sign+encrypt algorithm is not supported any longer. */
	switch (algo) {
	case CDK_PK_RSA:
		usage = CDK_KEY_USG_SIGN | CDK_KEY_USG_ENCR;
		break;
	case CDK_PK_RSA_E:
		usage = CDK_KEY_USG_ENCR;
		break;
	case CDK_PK_RSA_S:
		usage = CDK_KEY_USG_SIGN;
		break;
	case CDK_PK_ELG_E:
		usage = CDK_KEY_USG_ENCR;
		break;
	case CDK_PK_DSA:
		usage = CDK_KEY_USG_SIGN;
		break;
	default:
		usage = 0;
	}
	return usage;
}

/* You can use a NULL buf to get the output size only
 */
static cdk_error_t
mpi_to_buffer(bigint_t a, byte * buf, size_t buflen,
	      size_t * r_nwritten, size_t * r_nbits)
{
	size_t nbits;
	int err;

	if (!a || !r_nwritten) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	nbits = _gnutls_mpi_get_nbits(a);
	if (r_nbits)
		*r_nbits = nbits;

	if (r_nwritten)
		*r_nwritten = (nbits + 7) / 8 + 2;

	if ((nbits + 7) / 8 + 2 > buflen)
		return CDK_Too_Short;

	*r_nwritten = buflen;
	err = _gnutls_mpi_print(a, buf, r_nwritten);
	if (err < 0) {
		gnutls_assert();
		return map_gnutls_error(err);
	}

	return 0;
}


/**
 * cdk_pk_get_mpi:
 * @pk: public key
 * @idx: index of the MPI to retrieve
 * @buf: buffer to hold the raw data
 * @r_nwritten: output how large the raw data is
 * @r_nbits: size of the MPI in bits.
 * 
 * Return the MPI with the given index of the public key.
 **/
cdk_error_t
cdk_pk_get_mpi(cdk_pubkey_t pk, size_t idx,
	       byte * buf, size_t buflen, size_t * r_nwritten,
	       size_t * r_nbits)
{
	if (!pk || !r_nwritten)
		return CDK_Inv_Value;

	if ((ssize_t) idx > cdk_pk_get_npkey(pk->pubkey_algo))
		return CDK_Inv_Value;
	return mpi_to_buffer(pk->mpi[idx], buf, buflen, r_nwritten,
			     r_nbits);
}


/**
 * cdk_sk_get_mpi:
 * @sk: secret key
 * @idx: index of the MPI to retrieve
 * @buf: buffer to hold the raw data
 * @r_nwritten: output length of the raw data
 * @r_nbits: length of the MPI data in bits.
 * 
 * Return the MPI of the given secret key with the
 * index @idx. It is important to check if the key
 * is protected and thus no real MPI data will be returned then.
 **/
cdk_error_t
cdk_sk_get_mpi(cdk_pkt_seckey_t sk, size_t idx,
	       byte * buf, size_t buflen, size_t * r_nwritten,
	       size_t * r_nbits)
{
	if (!sk || !r_nwritten)
		return CDK_Inv_Value;

	if ((ssize_t) idx > cdk_pk_get_nskey(sk->pubkey_algo))
		return CDK_Inv_Value;
	return mpi_to_buffer(sk->mpi[idx], buf, buflen, r_nwritten,
			     r_nbits);
}


static u16 checksum_mpi(bigint_t m)
{
	byte buf[MAX_MPI_BYTES + 2];
	size_t nread;
	unsigned int i;
	u16 chksum = 0;

	if (!m)
		return 0;
	nread = DIM(buf);
	if (_gnutls_mpi_print_pgp(m, buf, &nread) < 0)
		return 0;
	for (i = 0; i < nread; i++)
		chksum += buf[i];
	return chksum;
}

/**
 * cdk_pk_from_secret_key:
 * @sk: the secret key
 * @ret_pk: the new public key
 *
 * Create a new public key from a secret key.
 **/
cdk_error_t
cdk_pk_from_secret_key(cdk_pkt_seckey_t sk, cdk_pubkey_t * ret_pk)
{
	if (!sk)
		return CDK_Inv_Value;
	return _cdk_copy_pubkey(ret_pk, sk->pk);
}


int _cdk_sk_get_csum(cdk_pkt_seckey_t sk)
{
	u16 csum = 0, i;

	if (!sk)
		return 0;
	for (i = 0; i < cdk_pk_get_nskey(sk->pubkey_algo); i++)
		csum += checksum_mpi(sk->mpi[i]);
	return csum;
}


/**
 * cdk_pk_get_fingerprint:
 * @pk: the public key
 * @fpr: the buffer to hold the fingerprint
 * 
 * Return the fingerprint of the given public key.
 * The buffer must be at least 20 octets.
 * This function should be considered deprecated and
 * the new cdk_pk_to_fingerprint() should be used whenever
 * possible to avoid overflows.
 **/
cdk_error_t cdk_pk_get_fingerprint(cdk_pubkey_t pk, byte * fpr)
{
	digest_hd_st hd;
	int md_algo;
	int dlen = 0;
	int err;
	const mac_entry_st *me;

	if (!pk || !fpr)
		return CDK_Inv_Value;

	if (pk->version < 4 && is_RSA(pk->pubkey_algo))
		md_algo = GNUTLS_DIG_MD5;	/* special */
	else
		md_algo = GNUTLS_DIG_SHA1;

	me = mac_to_entry(md_algo);

	dlen = _gnutls_hash_get_algo_len(me);
	err = _gnutls_hash_init(&hd, me);
	if (err < 0) {
		gnutls_assert();
		return map_gnutls_error(err);
	}
	_cdk_hash_pubkey(pk, &hd, 1);
	_gnutls_hash_deinit(&hd, fpr);
	if (dlen == 16)
		memset(fpr + 16, 0, 4);
	return 0;
}


/**
 * cdk_pk_to_fingerprint:
 * @pk: the public key
 * @fprbuf: buffer to save the fingerprint
 * @fprbuflen: buffer size
 * @r_nout: actual length of the fingerprint.
 * 
 * Calculate a fingerprint of the given key and
 * return it in the given byte array.
 **/
cdk_error_t
cdk_pk_to_fingerprint(cdk_pubkey_t pk,
		      byte * fprbuf, size_t fprbuflen, size_t * r_nout)
{
	size_t key_fprlen;
	cdk_error_t err;

	if (!pk)
		return CDK_Inv_Value;

	if (pk->version < 4)
		key_fprlen = 16;
	else
		key_fprlen = 20;

	/* Only return the required buffer size for the fingerprint. */
	if (!fprbuf && !fprbuflen && r_nout) {
		*r_nout = key_fprlen;
		return 0;
	}

	if (!fprbuf || key_fprlen > fprbuflen)
		return CDK_Too_Short;

	err = cdk_pk_get_fingerprint(pk, fprbuf);
	if (r_nout)
		*r_nout = key_fprlen;

	return err;
}


/**
 * cdk_pk_fingerprint_get_keyid:
 * @fpr: the key fingerprint
 * @fprlen: the length of the fingerprint
 * 
 * Derive the key ID from the key fingerprint.
 * For version 3 keys, this is not working.
 **/
u32
cdk_pk_fingerprint_get_keyid(const byte * fpr, size_t fprlen, u32 * keyid)
{
	u32 lowbits = 0;

	/* In this case we say the key is a V3 RSA key and we can't
	   use the fingerprint to get the keyid. */
	if (fpr && fprlen == 16) {
		keyid[0] = 0;
		keyid[1] = 0;
		return 0;
	} else if (keyid && fpr) {
		keyid[0] = _cdk_buftou32(fpr + 12);
		keyid[1] = _cdk_buftou32(fpr + 16);
		lowbits = keyid[1];
	} else if (fpr)
		lowbits = _cdk_buftou32(fpr + 16);
	return lowbits;
}


/**
 * cdk_pk_get_keyid:
 * @pk: the public key
 * @keyid: buffer to store the key ID
 * 
 * Calculate the key ID of the given public key.
 **/
u32 cdk_pk_get_keyid(cdk_pubkey_t pk, u32 * keyid)
{
	u32 lowbits = 0;
	byte buf[24];
	int rc;

	if (pk && (!pk->keyid[0] || !pk->keyid[1])) {
		if (pk->version < 4 && is_RSA(pk->pubkey_algo)) {
			byte p[MAX_MPI_BYTES];
			size_t n;

			n = MAX_MPI_BYTES;
			rc = _gnutls_mpi_print(pk->mpi[0], p, &n);
			if (rc < 0 || n < 8) {
				keyid[0] = keyid[1] = (u32)-1;
				return (u32)-1;
			}

			pk->keyid[0] =
			    p[n - 8] << 24 | p[n - 7] << 16 | p[n -
								6] << 8 |
			    p[n - 5];
			pk->keyid[1] =
			    p[n - 4] << 24 | p[n - 3] << 16 | p[n -
								2] << 8 |
			    p[n - 1];
		} else if (pk->version == 4) {
			cdk_pk_get_fingerprint(pk, buf);
			pk->keyid[0] = _cdk_buftou32(buf + 12);
			pk->keyid[1] = _cdk_buftou32(buf + 16);
		}
	}
	lowbits = pk ? pk->keyid[1] : 0;
	if (keyid && pk) {
		keyid[0] = pk->keyid[0];
		keyid[1] = pk->keyid[1];
	}

	return lowbits;
}


/**
 * cdk_sk_get_keyid:
 * @sk: the secret key
 * @keyid: buffer to hold the key ID
 * 
 * Calculate the key ID of the secret key, actually the public key.
 **/
u32 cdk_sk_get_keyid(cdk_pkt_seckey_t sk, u32 * keyid)
{
	u32 lowbits = 0;

	if (sk && sk->pk) {
		lowbits = cdk_pk_get_keyid(sk->pk, keyid);
		sk->keyid[0] = sk->pk->keyid[0];
		sk->keyid[1] = sk->pk->keyid[1];
	}

	return lowbits;
}


/**
 * cdk_sig_get_keyid:
 * @sig: the signature
 * @keyid: buffer to hold the key ID
 * 
 * Retrieve the key ID from the given signature.
 **/
u32 cdk_sig_get_keyid(cdk_pkt_signature_t sig, u32 * keyid)
{
	u32 lowbits = sig ? sig->keyid[1] : 0;

	if (keyid && sig) {
		keyid[0] = sig->keyid[0];
		keyid[1] = sig->keyid[1];
	}
	return lowbits;
}


/* Return the key ID from the given packet.
   If this is not possible, 0 is returned */
u32 _cdk_pkt_get_keyid(cdk_packet_t pkt, u32 * keyid)
{
	u32 lowbits;

	if (!pkt)
		return 0;

	switch (pkt->pkttype) {
	case CDK_PKT_PUBLIC_KEY:
	case CDK_PKT_PUBLIC_SUBKEY:
		lowbits = cdk_pk_get_keyid(pkt->pkt.public_key, keyid);
		break;

	case CDK_PKT_SECRET_KEY:
	case CDK_PKT_SECRET_SUBKEY:
		lowbits = cdk_sk_get_keyid(pkt->pkt.secret_key, keyid);
		break;

	case CDK_PKT_SIGNATURE:
		lowbits = cdk_sig_get_keyid(pkt->pkt.signature, keyid);
		break;

	default:
		lowbits = 0;
		break;
	}

	return lowbits;
}


/* Get the fingerprint of the packet if possible. */
cdk_error_t _cdk_pkt_get_fingerprint(cdk_packet_t pkt, byte * fpr)
{
	if (!pkt || !fpr)
		return CDK_Inv_Value;

	switch (pkt->pkttype) {
	case CDK_PKT_PUBLIC_KEY:
	case CDK_PKT_PUBLIC_SUBKEY:
		return cdk_pk_get_fingerprint(pkt->pkt.public_key, fpr);

	case CDK_PKT_SECRET_KEY:
	case CDK_PKT_SECRET_SUBKEY:
		return cdk_pk_get_fingerprint(pkt->pkt.secret_key->pk,
					      fpr);

	default:
		return CDK_Inv_Mode;
	}
	return 0;
}
