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

#include "gnutls_int.h"
#include "cipher.h"

static void dummy_wait(record_parameters_st *params,
		       const uint8_t *data, size_t data_size,
		       unsigned int mac_data, unsigned int max_mac_data)
{
	/* this hack is only needed on CBC ciphers when Encrypt-then-MAC mode
	 * is not supported by the peer. */
	unsigned v;
	unsigned int tag_size =
	    _gnutls_auth_cipher_tag_len(&params->read.ctx.tls12);
	unsigned hash_block = _gnutls_mac_block_size(params->mac);

	/* force additional hash compression function evaluations to prevent timing
	 * attacks that distinguish between wrong-mac + correct pad, from wrong-mac + incorrect pad.
	 */

	if (params->mac && params->mac->id == GNUTLS_MAC_SHA384)
		/* v = 1 for the hash function padding + 16 for message length */
		v = 17;
	else /* v = 1 for the hash function padding + 8 for message length */
		v = 9;

	if (hash_block > 0) {
		int max_blocks = (max_mac_data+v+hash_block-1)/hash_block;
		int hashed_blocks = (mac_data+v+hash_block-1)/hash_block;
		unsigned to_hash;

		max_blocks -= hashed_blocks;
		if (max_blocks < 1)
			return;

		to_hash = max_blocks * hash_block;
		if ((unsigned)to_hash+1+tag_size < data_size) {
			_gnutls_auth_cipher_add_auth
				    (&params->read.ctx.tls12,
				     data+data_size-tag_size-to_hash-1,
				     to_hash);
		}
	}
}

/* Verifies the CBC HMAC. That's a special case as it tries to avoid
 * any leaks which could make CBC ciphersuites without EtM usable as an
 * oracle to attacks.
 */
int cbc_mac_verify(gnutls_session_t session, record_parameters_st *params,
		   uint8_t preamble[MAX_PREAMBLE_SIZE],
		   content_type_t type,
		   gnutls_uint64 *sequence,
		   const uint8_t *data, size_t data_size,
		   size_t tag_size)
{
	int ret;
	const version_entry_st *ver = get_version(session);
	unsigned int tmp_pad_failed = 0;
	unsigned int pad_failed = 0;
	unsigned int pad, i, length;
	const uint8_t *tag_ptr = NULL;
	unsigned preamble_size;
	uint8_t tag[MAX_HASH_SIZE];
#ifdef ENABLE_SSL3
	unsigned blocksize = _gnutls_cipher_get_block_size(params->cipher);
#endif

	pad = data[data_size - 1];	/* pad */

	/* Check the padding bytes (TLS 1.x).
	 * Note that we access all 256 bytes of ciphertext for padding check
	 * because there is a timing channel in that memory access (in certain CPUs).
	 */
#ifdef ENABLE_SSL3
	if (ver->id == GNUTLS_SSL3) {
		if (pad >= blocksize)
			pad_failed = 1;
	} else
#endif
	{
		for (i = 2; i <= MIN(256, data_size); i++) {
			tmp_pad_failed |=
			    (data[data_size - i] != pad);
			pad_failed |=
			    ((i <= (1 + pad)) & (tmp_pad_failed));
		}
	}

	if (unlikely
	    (pad_failed != 0
	     || (1 + pad > ((int) data_size - tag_size)))) {
		/* We do not fail here. We check below for the
		 * the pad_failed. If zero means success.
		 */
		pad_failed = 1;
		pad = 0;
	}

	length = data_size - tag_size - pad - 1;
	tag_ptr = &data[length];

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

	ret =
	    _gnutls_auth_cipher_add_auth(&params->read.
					 ctx.tls12,
					 data, length);
	if (unlikely(ret < 0))
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_auth_cipher_tag(&params->read.ctx.tls12, tag,
				    tag_size);
	if (unlikely(ret < 0))
		return gnutls_assert_val(ret);

	if (unlikely
	    (gnutls_memcmp(tag, tag_ptr, tag_size) != 0 || pad_failed != 0)) {
		/* HMAC was not the same. */
		dummy_wait(params, data, data_size,
			   length + preamble_size,
			   preamble_size + data_size - tag_size - 1);

		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
	}

	return length;
}
