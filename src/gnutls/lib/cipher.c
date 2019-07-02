/*
 * Copyright (C) 2000-2013 Free Software Foundation, Inc.
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

/* Some high level functions to be used in the record encryption are
 * included here.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "cipher.h"
#include "algorithms.h"
#include "hash_int.h"
#include "cipher_int.h"
#include "debug.h"
#include "num.h"
#include "datum.h"
#include "kx.h"
#include "record.h"
#include "constate.h"
#include "mbuffers.h"
#include <state.h>
#include <random.h>

static int encrypt_packet(gnutls_session_t session,
			    uint8_t * cipher_data, int cipher_size,
			    gnutls_datum_t * plain,
			    size_t min_pad,
			    content_type_t _type,
			    record_parameters_st * params);

static int decrypt_packet(gnutls_session_t session,
			    gnutls_datum_t * ciphertext,
			    gnutls_datum_t * plain,
			    content_type_t type,
			    record_parameters_st * params,
			    gnutls_uint64 * sequence);

static int
decrypt_packet_tls13(gnutls_session_t session,
		     gnutls_datum_t * ciphertext,
		     gnutls_datum_t * plain,
		     content_type_t *type, record_parameters_st * params,
		     gnutls_uint64 * sequence);

static int
encrypt_packet_tls13(gnutls_session_t session,
		     uint8_t *cipher_data, size_t cipher_size,
		     gnutls_datum_t *plain,
		     size_t pad_size,
		     uint8_t type,
		     record_parameters_st *params);

/* returns ciphertext which contains the headers too. This also
 * calculates the size in the header field.
 * 
 */
int
_gnutls_encrypt(gnutls_session_t session,
		const uint8_t *data, size_t data_size,
		size_t min_pad,
		mbuffer_st *bufel,
		content_type_t type, record_parameters_st *params)
{
	gnutls_datum_t plaintext;
	const version_entry_st *vers = get_version(session);
	int ret;

	plaintext.data = (uint8_t *) data;
	plaintext.size = data_size;

	if (vers && vers->tls13_sem) {
		/* it fills the header, as it is included in the authenticated
		 * data of the AEAD cipher. */
		ret =
		    encrypt_packet_tls13(session,
					 _mbuffer_get_udata_ptr(bufel),
					 _mbuffer_get_udata_size(bufel),
					 &plaintext, min_pad, type,
					 params);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else {
		ret =
		    encrypt_packet(session,
				   _mbuffer_get_udata_ptr(bufel),
				   _mbuffer_get_udata_size
				   (bufel), &plaintext, min_pad, type,
				   params);
		if (ret < 0)
			return gnutls_assert_val(ret);

	}

	if (IS_DTLS(session))
		_gnutls_write_uint16(ret,
				     ((uint8_t *)
				      _mbuffer_get_uhead_ptr(bufel)) + 11);
	else
		_gnutls_write_uint16(ret,
				     ((uint8_t *)
				      _mbuffer_get_uhead_ptr(bufel)) + 3);

	_mbuffer_set_udata_size(bufel, ret);
	_mbuffer_set_uhead_size(bufel, 0);

	return _mbuffer_get_udata_size(bufel);
}

/* Decrypts the given data.
 * Returns the decrypted data length.
 *
 * The output is preallocated with the maximum allowed data size.
 */
int
_gnutls_decrypt(gnutls_session_t session,
		gnutls_datum_t *ciphertext,
		gnutls_datum_t *output,
		content_type_t *type,
		record_parameters_st *params,
		gnutls_uint64 *sequence)
{
	int ret;
	const version_entry_st *vers = get_version(session);

	if (ciphertext->size == 0)
		return 0;

	if (vers && vers->tls13_sem)
		ret =
		    decrypt_packet_tls13(session, ciphertext,
					 output, type, params,
					 sequence);
	else
		ret =
		    decrypt_packet(session, ciphertext,
				   output, *type, params,
				   sequence);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return ret;
}


inline static int
calc_enc_length_block(gnutls_session_t session,
		      const version_entry_st * ver,
		      int data_size,
		      int hash_size, uint8_t * pad,
		      unsigned auth_cipher,
		      uint16_t blocksize,
		      unsigned etm)
{
	/* pad is the LH pad the user wants us to add. Besides
	 * this LH pad, we only add minimal padding
	 */
	unsigned int pre_length = data_size + *pad;
	unsigned int length, new_pad;

	if (etm == 0)
		pre_length += hash_size;

	new_pad = (uint8_t) (blocksize - (pre_length % blocksize)) + *pad;

	if (new_pad > 255)
		new_pad -= blocksize;
	*pad = new_pad;

	length = data_size + hash_size + *pad;

	if (_gnutls_version_has_explicit_iv(ver))
		length += blocksize;	/* for the IV */

	return length;
}

inline static int
calc_enc_length_stream(gnutls_session_t session, int data_size,
		       int hash_size, unsigned auth_cipher,
		       unsigned exp_iv_size)
{
	unsigned int length;

	length = data_size + hash_size;
	if (auth_cipher)
		length += exp_iv_size;

	return length;
}

/* generates the authentication data (data to be hashed only
 * and are not to be sent). Returns their size.
 */
int
_gnutls_make_preamble(uint8_t * uint64_data, uint8_t type, unsigned int length,
		      const version_entry_st * ver, uint8_t preamble[MAX_PREAMBLE_SIZE])
{
	uint8_t *p = preamble;
	uint16_t c_length;

	c_length = _gnutls_conv_uint16(length);

	memcpy(p, uint64_data, 8);
	p += 8;
	*p = type;
	p++;
#ifdef ENABLE_SSL3
	if (ver->id != GNUTLS_SSL3)
#endif
	{	/* TLS protocols */
		*p = ver->major;
		p++;
		*p = ver->minor;
		p++;
	}
	memcpy(p, &c_length, 2);
	p += 2;
	return p - preamble;
}

/* This is the actual encryption 
 * Encrypts the given plaintext datum, and puts the result to cipher_data,
 * which has cipher_size size.
 * return the actual encrypted data length.
 */
static int
encrypt_packet(gnutls_session_t session,
			 uint8_t * cipher_data, int cipher_size,
			 gnutls_datum_t * plain,
			 size_t min_pad,
			 content_type_t type,
			 record_parameters_st * params)
{
	uint8_t pad;
	int length, ret;
	uint8_t preamble[MAX_PREAMBLE_SIZE];
	int preamble_size;
	int tag_size =
	    _gnutls_auth_cipher_tag_len(&params->write.ctx.tls12);
	int blocksize = _gnutls_cipher_get_block_size(params->cipher);
	unsigned algo_type = _gnutls_cipher_type(params->cipher);
	uint8_t *data_ptr, *full_cipher_ptr;
	const version_entry_st *ver = get_version(session);
	int explicit_iv = _gnutls_version_has_explicit_iv(ver);
	int auth_cipher =
	    _gnutls_auth_cipher_is_aead(&params->write.ctx.tls12);
	uint8_t nonce[MAX_CIPHER_IV_SIZE];
	unsigned imp_iv_size = 0, exp_iv_size = 0;
	bool etm = 0;

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (algo_type == CIPHER_BLOCK && params->etm != 0)
		etm = 1;

	_gnutls_hard_log("ENC[%p]: cipher: %s, MAC: %s, Epoch: %u\n",
			 session, _gnutls_cipher_get_name(params->cipher),
			 _gnutls_mac_get_name(params->mac),
			 (unsigned int) params->epoch);

	/* Calculate the encrypted length (padding etc.)
	 */
	if (algo_type == CIPHER_BLOCK) {
		/* Call gnutls_rnd() once. Get data used for the IV
		 */
		ret = gnutls_rnd(GNUTLS_RND_NONCE, nonce, blocksize);
		if (ret < 0)
			return gnutls_assert_val(ret);

		pad = min_pad;

		length =
		    calc_enc_length_block(session, ver, plain->size,
					  tag_size, &pad, auth_cipher,
					  blocksize, etm);
	} else { /* AEAD + STREAM */
		imp_iv_size = _gnutls_cipher_get_implicit_iv_size(params->cipher);
		exp_iv_size = _gnutls_cipher_get_explicit_iv_size(params->cipher);

		pad = 0;
		length =
		    calc_enc_length_stream(session, plain->size,
					   tag_size, auth_cipher,
					   exp_iv_size);
	}

	if (length < 0)
		return gnutls_assert_val(length);

	/* copy the encrypted data to cipher_data.
	 */
	if (cipher_size < length)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	data_ptr = cipher_data;
	full_cipher_ptr = data_ptr;

	if (algo_type == CIPHER_BLOCK || algo_type == CIPHER_STREAM) {
		if (algo_type == CIPHER_BLOCK && explicit_iv != 0) {
			/* copy the random IV.
			 */
			memcpy(data_ptr, nonce, blocksize);
			_gnutls_auth_cipher_setiv(&params->write.
						  ctx.tls12, data_ptr,
						  blocksize);

			/*data_ptr += blocksize;*/
			cipher_data += blocksize;
		}
	} else { /* AEAD */
		if (params->cipher->xor_nonce == 0) {
			/* Values in AEAD are pretty fixed in TLS 1.2 for 128-bit block
			 */
			 if (params->write.iv_size != imp_iv_size)
				return
				    gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			/* Instead of generating a new nonce on every packet, we use the
			 * write.sequence_number (It is a MAY on RFC 5288), and safer
			 * as it will never reuse a value.
			 */
			memcpy(nonce, params->write.iv,
			       params->write.iv_size);
			memcpy(&nonce[imp_iv_size],
			       UINT64DATA(params->write.sequence_number),
			       8);

			memcpy(data_ptr, &nonce[imp_iv_size],
			       exp_iv_size);

			/*data_ptr += exp_iv_size;*/
			cipher_data += exp_iv_size;
		} else { /* XOR nonce with IV */
			if (unlikely(params->write.iv_size != 12 || imp_iv_size != 12 || exp_iv_size != 0))
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			memset(nonce, 0, 4);
			memcpy(&nonce[4],
			       UINT64DATA(params->write.sequence_number), 8);

			memxor(nonce, params->write.iv, 12);
		}
	}

	if (etm)
		ret = length-tag_size;
	else
		ret = plain->size;

	preamble_size =
	    _gnutls_make_preamble(UINT64DATA(params->write.sequence_number),
				  type, ret, ver, preamble);

	if (algo_type == CIPHER_BLOCK || algo_type == CIPHER_STREAM) {
		/* add the authenticated data */
		ret =
		    _gnutls_auth_cipher_add_auth(&params->write.ctx.tls12,
					 preamble, preamble_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (etm && explicit_iv) {
			/* In EtM we need to hash the IV as well */
			ret =
			    _gnutls_auth_cipher_add_auth(&params->write.ctx.tls12,
						 full_cipher_ptr, blocksize);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}

		/* Actual encryption.
		 */
		ret =
		    _gnutls_auth_cipher_encrypt2_tag(&params->write.ctx.tls12,
						     plain->data,
						     plain->size, cipher_data,
						     cipher_size, pad);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else { /* AEAD */
		ret = _gnutls_aead_cipher_encrypt(&params->write.ctx.tls12.cipher,
						  nonce, imp_iv_size + exp_iv_size,
						  preamble, preamble_size,
						  tag_size,
						  plain->data, plain->size,
						  cipher_data, cipher_size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return length;
}

static int
encrypt_packet_tls13(gnutls_session_t session,
		     uint8_t *cipher_data, size_t cipher_size,
		     gnutls_datum_t *plain,
		     size_t pad_size,
		     uint8_t type,
		     record_parameters_st *params)
{
	int ret;
	unsigned int tag_size = params->write.aead_tag_size;
	const version_entry_st *ver = get_version(session);
	uint8_t nonce[MAX_CIPHER_IV_SIZE];
	unsigned iv_size = 0;
	ssize_t max, total;
	uint8_t aad[5];
	giovec_t auth_iov[1];
	giovec_t iov[2];

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	_gnutls_hard_log("ENC[%p]: cipher: %s, MAC: %s, Epoch: %u\n",
			 session, _gnutls_cipher_get_name(params->cipher),
			 _gnutls_mac_get_name(params->mac),
			 (unsigned int) params->epoch);

	iv_size = params->write.iv_size;

	if (params->cipher->id == GNUTLS_CIPHER_NULL) {
		if (cipher_size < plain->size+1)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		memcpy(cipher_data, plain->data, plain->size);
		return plain->size;
	}

	if (unlikely(iv_size < 8))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	memcpy(nonce, params->write.iv, iv_size);
	memxor(&nonce[iv_size-8], UINT64DATA(params->write.sequence_number), 8);

	max = MAX_RECORD_SEND_SIZE(session);

	/* make TLS 1.3 form of data */
	total = plain->size + 1 + pad_size;

	/* check whether padding would exceed max */
	if (total > max) {
		if (unlikely(max < (ssize_t)plain->size+1))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		pad_size = max - plain->size - 1;
		total = max;
	}

	/* create authenticated data header */
	aad[0] = GNUTLS_APPLICATION_DATA;
	aad[1] = 0x03;
	aad[2] = 0x03;
	_gnutls_write_uint16(total+tag_size, &aad[3]);

	auth_iov[0].iov_base = aad;
	auth_iov[0].iov_len = sizeof(aad);

	iov[0].iov_base = plain->data;
	iov[0].iov_len = plain->size;

	if (pad_size || (session->internals.flags & GNUTLS_SAFE_PADDING_CHECK)) {
		uint8_t *pad = gnutls_calloc(1, 1+pad_size);
		if (pad == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		pad[0] = type;

		iov[1].iov_base = pad;
		iov[1].iov_len = 1+pad_size;

		ret = gnutls_aead_cipher_encryptv(&params->write.ctx.aead,
						  nonce, iv_size,
						  auth_iov, 1,
						  tag_size,
						  iov, 2,
						  cipher_data, &cipher_size);
		gnutls_free(pad);
	} else {
		iov[1].iov_base = &type;
		iov[1].iov_len = 1;

		ret = gnutls_aead_cipher_encryptv(&params->write.ctx.aead,
						  nonce, iv_size,
						  auth_iov, 1,
						  tag_size,
						  iov, 2,
						  cipher_data, &cipher_size);
	}

	if (ret < 0)
		return gnutls_assert_val(ret);

	return cipher_size;
}


/* Deciphers the ciphertext packet, and puts the result to plain.
 * Returns the actual plaintext packet size.
 */
static int
decrypt_packet(gnutls_session_t session,
		gnutls_datum_t * ciphertext,
		gnutls_datum_t * plain,
		content_type_t type, record_parameters_st * params,
		gnutls_uint64 * sequence)
{
	uint8_t tag[MAX_HASH_SIZE];
	uint8_t nonce[MAX_CIPHER_IV_SIZE];
	const uint8_t *tag_ptr = NULL;
	unsigned int pad = 0;
	int length, length_to_decrypt;
	uint16_t blocksize;
	int ret;
	uint8_t preamble[MAX_PREAMBLE_SIZE];
	unsigned int preamble_size = 0;
	const version_entry_st *ver = get_version(session);
	unsigned int tag_size =
	    _gnutls_auth_cipher_tag_len(&params->read.ctx.tls12);
	unsigned int explicit_iv = _gnutls_version_has_explicit_iv(ver);
	unsigned imp_iv_size, exp_iv_size;
	unsigned cipher_type = _gnutls_cipher_type(params->cipher);
	bool etm = 0;

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	imp_iv_size = _gnutls_cipher_get_implicit_iv_size(params->cipher);
	exp_iv_size = _gnutls_cipher_get_explicit_iv_size(params->cipher);
	blocksize = _gnutls_cipher_get_block_size(params->cipher);

	if (params->etm !=0 && cipher_type == CIPHER_BLOCK)
		etm = 1;

	/* if EtM mode and not AEAD */
	if (etm) {
		if (unlikely(ciphertext->size < tag_size))
			return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		preamble_size = _gnutls_make_preamble(UINT64DATA(*sequence),
						      type, ciphertext->size-tag_size,
						      ver, preamble);

		ret = _gnutls_auth_cipher_add_auth(&params->read.
						   ctx.tls12, preamble,
						   preamble_size);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);

		ret = _gnutls_auth_cipher_add_auth(&params->read.
						   ctx.tls12,
						   ciphertext->data,
						   ciphertext->size-tag_size);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);

		ret = _gnutls_auth_cipher_tag(&params->read.ctx.tls12, tag, tag_size);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);
		
		if (unlikely(gnutls_memcmp(tag, &ciphertext->data[ciphertext->size-tag_size], tag_size) != 0)) {
			/* HMAC was not the same. */
			return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		}
	}

	/* actual decryption (inplace)
	 */
	switch (cipher_type) {
	case CIPHER_AEAD:
		/* The way AEAD ciphers are defined in RFC5246, it allows
		 * only stream ciphers.
		 */
		if (unlikely(_gnutls_auth_cipher_is_aead(&params->read.
						   ctx.tls12) == 0))
			return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);


		if (unlikely(ciphertext->size < (tag_size + exp_iv_size)))
			return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

		if (params->cipher->xor_nonce == 0) {
			/* Values in AEAD are pretty fixed in TLS 1.2 for 128-bit block
			 */
			 if (unlikely(params->read.iv_size != 4))
				return
				    gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

			memcpy(nonce, params->read.iv,
			       imp_iv_size);

			memcpy(&nonce[imp_iv_size],
			       ciphertext->data, exp_iv_size);

			ciphertext->data += exp_iv_size;
			ciphertext->size -= exp_iv_size;
		} else { /* XOR nonce with IV */
			if (unlikely(params->read.iv_size != 12 || imp_iv_size != 12 || exp_iv_size != 0))
				return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

			memset(nonce, 0, 4);
			memcpy(&nonce[4], UINT64DATA(*sequence), 8);

			memxor(nonce, params->read.iv, 12);
		}

		length =
		    ciphertext->size - tag_size;

		length_to_decrypt = ciphertext->size;

		/* Pass the type, version, length and plain through
		 * MAC.
		 */
		preamble_size =
		    _gnutls_make_preamble(UINT64DATA(*sequence), type,
					  length, ver, preamble);


		if (unlikely
		    ((unsigned) length_to_decrypt > plain->size)) {
			_gnutls_audit_log(session,
					  "Received %u bytes, while expecting less than %u\n",
					  (unsigned int) length_to_decrypt,
					  (unsigned int) plain->size);
			return
			    gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		}

		ret = _gnutls_aead_cipher_decrypt(&params->read.ctx.tls12.cipher,
						  nonce, exp_iv_size + imp_iv_size,
						  preamble, preamble_size,
						  tag_size,
						  ciphertext->data, length_to_decrypt,
						  plain->data, plain->size);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);

		return length;

		break;
	case CIPHER_STREAM:
		if (unlikely(ciphertext->size < tag_size))
			return
			    gnutls_assert_val
			    (GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		length_to_decrypt = ciphertext->size;
		length = ciphertext->size - tag_size;
		tag_ptr = plain->data + length;

		/* Pass the type, version, length and plain through
		 * MAC.
		 */
		preamble_size =
		    _gnutls_make_preamble(UINT64DATA(*sequence), type,
					  length, ver, preamble);

		ret =
		    _gnutls_auth_cipher_add_auth(&params->read.
						 ctx.tls12, preamble,
						 preamble_size);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);

		if (unlikely
		    ((unsigned) length_to_decrypt > plain->size)) {
			_gnutls_audit_log(session,
					  "Received %u bytes, while expecting less than %u\n",
					  (unsigned int) length_to_decrypt,
					  (unsigned int) plain->size);
			return
			    gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		}

		ret =
		    _gnutls_auth_cipher_decrypt2(&params->read.
						 ctx.tls12,
						 ciphertext->data,
						 length_to_decrypt,
						 plain->data,
						 plain->size);

		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);

		ret =
		    _gnutls_auth_cipher_tag(&params->read.ctx.tls12, tag,
					    tag_size);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);

		if (unlikely
		    (gnutls_memcmp(tag, tag_ptr, tag_size) != 0)) {
			return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		}

		break;
	case CIPHER_BLOCK:
		if (unlikely(ciphertext->size < blocksize))
			return
			    gnutls_assert_val
			    (GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		if (etm == 0) {
			if (unlikely(ciphertext->size % blocksize != 0))
				return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		} else {
			if (unlikely((ciphertext->size - tag_size) % blocksize != 0))
				return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		}

		/* ignore the IV in TLS 1.1+
		 */
		if (explicit_iv) {
			_gnutls_auth_cipher_setiv(&params->read.
						  ctx.tls12,
						  ciphertext->data,
						  blocksize);

			memcpy(nonce, ciphertext->data, blocksize);
			ciphertext->size -= blocksize;
			ciphertext->data += blocksize;
		}

		if (unlikely(ciphertext->size < tag_size + 1))
			return
			    gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

		/* we don't use the auth_cipher interface here, since
		 * TLS with block ciphers is impossible to be used under such
		 * an API. (the length of plaintext is required to calculate
		 * auth_data, but it is not available before decryption).
		 */
		if (unlikely(ciphertext->size > plain->size))
			return
			    gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

		if (etm == 0) {
			ret =
			    _gnutls_cipher_decrypt2(&params->read.ctx.tls12.
						    cipher, ciphertext->data,
						    ciphertext->size,
						    plain->data,
						    plain->size);
			if (unlikely(ret < 0))
				return gnutls_assert_val(ret);

			ret = cbc_mac_verify(session, params, preamble, type,
					     sequence, plain->data, ciphertext->size,
					     tag_size);
			if (unlikely(ret < 0))
				return gnutls_assert_val(ret);

			length = ret;
		} else { /* EtM */
			ret =
			    _gnutls_cipher_decrypt2(&params->read.ctx.tls12.
						    cipher, ciphertext->data,
						    ciphertext->size - tag_size,
						    plain->data,
						    plain->size);
			if (unlikely(ret < 0))
				return gnutls_assert_val(ret);

			pad = plain->data[ciphertext->size - tag_size - 1]; /* pad */
			length = ciphertext->size - tag_size - pad - 1;
			
			if (unlikely(length < 0))
				return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		}
		break;
	default:
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
	}


	return length;
}

static int
decrypt_packet_tls13(gnutls_session_t session,
		     gnutls_datum_t *ciphertext,
		     gnutls_datum_t *plain,
		     content_type_t *type, record_parameters_st *params,
		     gnutls_uint64 *sequence)
{
	uint8_t nonce[MAX_CIPHER_IV_SIZE];
	size_t length, length_to_decrypt;
	int ret;
	const version_entry_st *ver = get_version(session);
	unsigned int tag_size = params->read.aead_tag_size;
	unsigned iv_size;
	unsigned j;
	volatile unsigned length_set;
	uint8_t aad[5];

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (params->cipher->id == GNUTLS_CIPHER_NULL) {
		if (plain->size < ciphertext->size)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		length = ciphertext->size;
		memcpy(plain->data, ciphertext->data, length);

		return length;
	}

	iv_size = _gnutls_cipher_get_iv_size(params->cipher);

	/* The way AEAD ciphers are defined in RFC5246, it allows
	 * only stream ciphers.
	 */
	if (unlikely(ciphertext->size < tag_size))
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	if (unlikely(params->read.iv_size != iv_size || iv_size < 8))
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	memcpy(nonce, params->read.iv, params->read.iv_size);
	memxor(&nonce[iv_size-8], UINT64DATA(*sequence), 8);

	length =
	    ciphertext->size - tag_size;

	length_to_decrypt = ciphertext->size;

	if (unlikely
	    ((unsigned) length_to_decrypt > plain->size)) {
			_gnutls_audit_log(session,
				  "Received %u bytes, while expecting less than %u\n",
				  (unsigned int) length_to_decrypt,
				  (unsigned int) plain->size);
		return
		    gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
	}

	aad[0] = GNUTLS_APPLICATION_DATA;
	aad[1] = 0x03;
	aad[2] = 0x03;
	_gnutls_write_uint16(ciphertext->size, &aad[3]);

	ret = gnutls_aead_cipher_decrypt(&params->read.ctx.aead,
					 nonce, iv_size,
					 aad, sizeof(aad),
					 tag_size,
					 ciphertext->data, length_to_decrypt,
					 plain->data, &length);
	if (unlikely(ret < 0))
		return gnutls_assert_val(ret);

	/* 1 octet for content type */
	if (length > max_decrypted_size(session) + 1) {
		_gnutls_audit_log
		    (session, "Received packet with illegal length: %u\n",
		     (unsigned int) length);

		return gnutls_assert_val(GNUTLS_E_RECORD_OVERFLOW);
	}

	length_set = 0;

	/* now figure the actual data size. We intentionally iterate through all data,
	 * to avoid leaking the padding length due to timing differences in processing.
	 */
	for (j=length;j>0;j--) {
		if (plain->data[j-1]!=0 && length_set == 0) {
			*type = plain->data[j-1];
			length = j-1;
			length_set = 1;
			if (!(session->internals.flags & GNUTLS_SAFE_PADDING_CHECK))
				break;
		}
	}

	if (!length_set)
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	return length;
}
