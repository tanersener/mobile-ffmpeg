/*
 * Copyright (C) 2014-2015 Red Hat, Inc.
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
 * The following code is an implementation of the AES-128-CCM cipher
 * using AESNI (without PCLMUL)
 */

#include "errors.h"
#include "gnutls_int.h"

#ifdef HAVE_LIBNETTLE

#include <gnutls/crypto.h>
#include "errors.h"
#include <aes-x86.h>
#include <x86-common.h>
#include <byteswap.h>
#include <nettle/ccm.h>
#include <aes-x86.h>

typedef struct ccm_x86_aes_ctx {
	AES_KEY key;
} ccm_x86_aes_ctx;

/* CCM mode 
 */
static void x86_aes_encrypt(const void *_ctx,
			    size_t length, uint8_t * dst,
			    const uint8_t * src)
{
	AES_KEY *ctx = (void*)_ctx;
	aesni_ecb_encrypt(src, dst, length, ctx, 1);
}

static int
aes_ccm_cipher_init(gnutls_cipher_algorithm_t algorithm, void **ctx,
		    int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_CCM &&
	    algorithm != GNUTLS_CIPHER_AES_256_CCM &&
	    algorithm != GNUTLS_CIPHER_AES_128_CCM_8 &&
	    algorithm != GNUTLS_CIPHER_AES_256_CCM_8)
		return GNUTLS_E_INVALID_REQUEST;

	*ctx = gnutls_calloc(1, sizeof(ccm_x86_aes_ctx));
	if (*ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

static int
aes_ccm_cipher_setkey(void *_ctx, const void *key, size_t length)
{
	struct ccm_x86_aes_ctx *ctx = _ctx;
	aesni_set_encrypt_key(key, length*8, &ctx->key);

	return 0;
}

static int
aes_ccm_aead_encrypt(void *_ctx,
			const void *nonce, size_t nonce_size,
			const void *auth, size_t auth_size,
			size_t tag_size,
			const void *plain, size_t plain_size,
		   	void *encr, size_t encr_size)
{
	struct ccm_x86_aes_ctx *ctx = _ctx;
	/* proper AEAD cipher */

	if (unlikely(encr_size < plain_size + tag_size))
		return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);

	ccm_encrypt_message(&ctx->key, x86_aes_encrypt,
			    nonce_size, nonce,
			    auth_size, auth,
			    tag_size,
			    plain_size+tag_size, encr,
			    plain);
	return 0;
}

static int
aes_ccm_aead_decrypt(void *_ctx,
			const void *nonce, size_t nonce_size,
			const void *auth, size_t auth_size,
			size_t tag_size,
		   	const void *encr, size_t encr_size,
			void *plain, size_t plain_size)
{
	struct ccm_x86_aes_ctx *ctx = _ctx;
	int ret;

	if (unlikely(encr_size < tag_size))
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	ret = ccm_decrypt_message(&ctx->key, x86_aes_encrypt,
				  nonce_size, nonce,
				  auth_size, auth,
				  tag_size,
				  encr_size-tag_size, plain,
				  encr);
	if (unlikely(ret == 0))
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	return 0;
}


static void aes_ccm_deinit(void *_ctx)
{
	struct ccm_x86_aes_ctx *ctx = _ctx;

	zeroize_temp_key(ctx, sizeof(*ctx));
	gnutls_free(ctx);
}

const gnutls_crypto_cipher_st _gnutls_aes_ccm_x86_aesni = {
	.init = aes_ccm_cipher_init,
	.setkey = aes_ccm_cipher_setkey,
	.aead_encrypt = aes_ccm_aead_encrypt,
	.aead_decrypt = aes_ccm_aead_decrypt,
	.deinit = aes_ccm_deinit,
};

#endif
