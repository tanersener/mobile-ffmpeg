/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/*
 * The following code is an implementation of the AES-128-CBC cipher
 * using intel's AES instruction set. 
 */

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#include <aes-x86.h>
#include <sha-x86.h>
#include <x86-common.h>

struct aes_ctx {
	AES_KEY expanded_key;
	uint8_t iv[16];
	int enc;
};

static int
aes_cipher_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_CBC
	    && algorithm != GNUTLS_CIPHER_AES_192_CBC
	    && algorithm != GNUTLS_CIPHER_AES_256_CBC)
		return GNUTLS_E_INVALID_REQUEST;

	*_ctx = gnutls_calloc(1, sizeof(struct aes_ctx));
	if (*_ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	((struct aes_ctx *) (*_ctx))->enc = enc;

	return 0;
}

static int
aes_cipher_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct aes_ctx *ctx = _ctx;
	int ret;

	CHECK_AES_KEYSIZE(keysize);

	if (ctx->enc)
		ret =
		    aesni_set_encrypt_key(userkey, keysize * 8,
					  ALIGN16(&ctx->expanded_key));
	else
		ret =
		    aesni_set_decrypt_key(userkey, keysize * 8,
					  ALIGN16(&ctx->expanded_key));

	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_ENCRYPTION_FAILED);

	return 0;
}

static int aes_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct aes_ctx *ctx = _ctx;

	memcpy(ctx->iv, iv, 16);
	return 0;
}

static int
aes_encrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct aes_ctx *ctx = _ctx;

	aesni_cbc_encrypt(src, dst, src_size, ALIGN16(&ctx->expanded_key),
			  ctx->iv, 1);
	return 0;
}

static int
aes_decrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct aes_ctx *ctx = _ctx;

	aesni_cbc_encrypt(src, dst, src_size, ALIGN16(&ctx->expanded_key),
			  ctx->iv, 0);

	return 0;
}

static void aes_deinit(void *_ctx)
{
	struct aes_ctx *ctx = _ctx;
	
	zeroize_temp_key(ctx, sizeof(*ctx));
	gnutls_free(ctx);
}

const gnutls_crypto_cipher_st _gnutls_aesni_x86 = {
	.init = aes_cipher_init,
	.setkey = aes_cipher_setkey,
	.setiv = aes_setiv,
	.encrypt = aes_encrypt,
	.decrypt = aes_decrypt,
	.deinit = aes_deinit,
};

