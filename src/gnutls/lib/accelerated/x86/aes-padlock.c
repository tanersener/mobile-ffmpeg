/*
 * Copyright (C) 2011-2018 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
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

/*
 * The following code is an implementation of the AES-128-CBC cipher
 * using VIA Padlock instruction set. 
 */

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#include <aes-x86.h>
#include <x86-common.h>
#include <nettle/aes.h>		/* for key generation in 192 and 256 bits */
#include <sha-padlock.h>
#include <aes-padlock.h>

static int
aes_cipher_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_CBC
	    && algorithm != GNUTLS_CIPHER_AES_256_CBC)
		return GNUTLS_E_INVALID_REQUEST;

	*_ctx = gnutls_calloc(1, sizeof(struct padlock_ctx));
	if (*_ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	((struct padlock_ctx *) (*_ctx))->enc = enc;
	return 0;
}

int
padlock_aes_cipher_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct padlock_ctx *ctx = _ctx;
	struct padlock_cipher_data *pce;
	struct aes256_ctx nc;

	memset(_ctx, 0, sizeof(struct padlock_cipher_data));

	pce = ALIGN16(&ctx->expanded_key);

	pce->cword.b.encdec = (ctx->enc == 0);

	switch (keysize) {
	case 16:
		pce->cword.b.ksize = 0;
		pce->cword.b.rounds = 10;
		memcpy(pce->ks.rd_key, userkey, 16);
		pce->cword.b.keygen = 0;
		break;
	case 32:
		pce->cword.b.ksize = 2;
		pce->cword.b.rounds = 14;

		/* expand key using nettle */
		if (ctx->enc)
			aes256_set_encrypt_key(&nc, userkey);
		else
			aes256_set_decrypt_key(&nc, userkey);

		memcpy(pce->ks.rd_key, nc.keys, sizeof(nc.keys));
		pce->ks.rounds = _AES256_ROUNDS;

		pce->cword.b.keygen = 1;
		break;
	default:
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	padlock_reload_key();

	return 0;
}

static int aes_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct padlock_ctx *ctx = _ctx;
	struct padlock_cipher_data *pce;

	pce = ALIGN16(&ctx->expanded_key);

	if (iv_size != 16)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memcpy(pce->iv, iv, 16);

	return 0;
}

static int
padlock_aes_cbc_encrypt(void *_ctx, const void *src, size_t src_size,
			void *dst, size_t dst_size)
{
	struct padlock_ctx *ctx = _ctx;
	struct padlock_cipher_data *pce;

	pce = ALIGN16(&ctx->expanded_key);

	if (src_size > 0)
		padlock_cbc_encrypt(dst, src, pce, src_size);

	return 0;
}


static int
padlock_aes_cbc_decrypt(void *_ctx, const void *src, size_t src_size,
			void *dst, size_t dst_size)
{
	struct padlock_ctx *ctx = _ctx;
	struct padlock_cipher_data *pcd;

	pcd = ALIGN16(&ctx->expanded_key);

	if (src_size > 0)
		padlock_cbc_encrypt(dst, src, pcd, src_size);

	return 0;
}

static void aes_deinit(void *_ctx)
{
	struct padlock_ctx *ctx = _ctx;
	
	zeroize_temp_key(ctx, sizeof(*ctx));
	gnutls_free(ctx);
}

const gnutls_crypto_cipher_st _gnutls_aes_padlock = {
	.init = aes_cipher_init,
	.setkey = padlock_aes_cipher_setkey,
	.setiv = aes_setiv,
	.encrypt = padlock_aes_cbc_encrypt,
	.decrypt = padlock_aes_cbc_decrypt,
	.deinit = aes_deinit,
};

