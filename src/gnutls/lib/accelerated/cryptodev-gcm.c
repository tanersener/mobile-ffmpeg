/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#include <accelerated/cryptodev.h>

#ifdef ENABLE_CRYPTODEV

#include <fcntl.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>

#ifndef CRYPTO_CIPHER_MAX_KEY_LEN
#define CRYPTO_CIPHER_MAX_KEY_LEN 64
#endif

#ifndef EALG_MAX_BLOCK_LEN
#define EALG_MAX_BLOCK_LEN 16
#endif


#ifdef CIOCAUTHCRYPT

#define GCM_BLOCK_SIZE 16

struct cryptodev_gcm_ctx {
	struct session_op sess;
	struct crypt_auth_op cryp;
	uint8_t iv[GCM_BLOCK_SIZE];
	uint8_t tag[GCM_BLOCK_SIZE];

	void *auth_data;
	unsigned int auth_data_size;

	int op;			/* whether encryption op has been executed */

	int cfd;
};

static void aes_gcm_deinit(void *_ctx)
{
	struct cryptodev_gcm_ctx *ctx = _ctx;

	ioctl(ctx->cfd, CIOCFSESSION, &ctx->sess.ses);
	gnutls_free(ctx);
}

static const int cipher_map[] = {
	[GNUTLS_CIPHER_AES_128_GCM] = CRYPTO_AES_GCM,
	[GNUTLS_CIPHER_AES_256_GCM] = CRYPTO_AES_GCM,
};

static int
aes_gcm_cipher_init(gnutls_cipher_algorithm_t algorithm, void **_ctx,
		    int enc)
{
	struct cryptodev_gcm_ctx *ctx;

	*_ctx = gnutls_calloc(1, sizeof(struct cryptodev_gcm_ctx));
	if (*_ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}


	ctx = *_ctx;

	ctx->cfd = _gnutls_cryptodev_fd;
	ctx->sess.cipher = cipher_map[algorithm];
	ctx->cryp.iv = ctx->iv;

	return 0;
}

static int
aes_gcm_cipher_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct cryptodev_gcm_ctx *ctx = _ctx;

	CHECK_AES_KEYSIZE(keysize);

	ctx->sess.keylen = keysize;
	ctx->sess.key = (void *) userkey;

	if (ioctl(ctx->cfd, CIOCGSESSION, &ctx->sess)) {
		gnutls_assert();
		return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
	}
	ctx->cryp.ses = ctx->sess.ses;

	return 0;
}

static int aes_gcm_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct cryptodev_gcm_ctx *ctx = _ctx;

	if (iv_size != GCM_BLOCK_SIZE - 4)
		return GNUTLS_E_INVALID_REQUEST;

	memcpy(ctx->iv, iv, GCM_BLOCK_SIZE - 4);

	ctx->cryp.iv = (void *) ctx->iv;

	return 0;
}

static int
aes_gcm_encrypt(void *_ctx, const void *src, size_t src_size,
		void *dst, size_t dst_size)
{
	struct cryptodev_gcm_ctx *ctx = _ctx;

	/* the GCM in kernel will place the tag after the
	 * encrypted data.
	 */
	if (dst_size < src_size + GCM_BLOCK_SIZE)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ctx->cryp.len = src_size;
	ctx->cryp.src = (void *) src;
	ctx->cryp.dst = dst;
	ctx->cryp.op = COP_ENCRYPT;

	ctx->cryp.auth_len = ctx->auth_data_size;
	ctx->cryp.auth_src = ctx->auth_data;

	if (ioctl(ctx->cfd, CIOCAUTHCRYPT, &ctx->cryp)) {
		gnutls_assert();
		return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
	}

	ctx->cryp.auth_len = 0;
	ctx->op = 1;
	memcpy(ctx->tag, &((uint8_t *) dst)[src_size], GCM_BLOCK_SIZE);
	return 0;
}

static int
aes_gcm_decrypt(void *_ctx, const void *src, size_t src_size,
		void *dst, size_t dst_size)
{
	struct cryptodev_gcm_ctx *ctx = _ctx;

	/* the GCM in kernel will place the tag after the
	 * encrypted data.
	 */
	ctx->cryp.len = src_size + GCM_BLOCK_SIZE;
	ctx->cryp.src = (void *) src;
	ctx->cryp.dst = dst;
	ctx->cryp.op = COP_DECRYPT;

	ctx->cryp.auth_len = ctx->auth_data_size;
	ctx->cryp.auth_src = ctx->auth_data;

	if (ioctl(ctx->cfd, CIOCAUTHCRYPT, &ctx->cryp)) {
		gnutls_assert();
		return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
	}

	ctx->cryp.auth_len = 0;
	ctx->op = 1;
	memcpy(ctx->tag, &((uint8_t *) dst)[src_size], GCM_BLOCK_SIZE);
	return 0;
}

static int aes_gcm_auth(void *_ctx, const void *src, size_t src_size)
{
	struct cryptodev_gcm_ctx *ctx = _ctx;

	ctx->op = 0;
	ctx->auth_data = (void *) src;
	ctx->auth_data_size = src_size;

	return 0;
}

static void aes_gcm_tag(void *_ctx, void *tag, size_t tagsize)
{
	struct cryptodev_gcm_ctx *ctx = _ctx;

	if (ctx->op == 0) {
		ctx->cryp.len = 0;
		ctx->cryp.src = NULL;
		ctx->cryp.dst = ctx->tag;
		ctx->cryp.op = COP_ENCRYPT;

		ctx->cryp.auth_len = ctx->auth_data_size;
		ctx->cryp.auth_src = ctx->auth_data;

		if (ioctl(ctx->cfd, CIOCAUTHCRYPT, &ctx->cryp)) {
			gnutls_assert();
			return;
		}
	}

	memcpy(tag, ctx->tag, tagsize);
	ctx->op = 0;
}

#include "x86/aes-gcm-aead.h"

static const gnutls_crypto_cipher_st cipher_struct = {
	.init = aes_gcm_cipher_init,
	.setkey = aes_gcm_cipher_setkey,
	.setiv = aes_gcm_setiv,
	.aead_encrypt = aes_gcm_aead_encrypt,
	.aead_decrypt = aes_gcm_aead_decrypt,
	.encrypt = aes_gcm_encrypt,
	.decrypt = aes_gcm_decrypt,
	.deinit = aes_gcm_deinit,
	.tag = aes_gcm_tag,
	.auth = aes_gcm_auth,
};

int _cryptodev_register_gcm_crypto(int cfd)
{
	struct session_op sess;
	uint8_t fake_key[CRYPTO_CIPHER_MAX_KEY_LEN];
	unsigned int i;
	int ret;
#ifdef CIOCGSESSINFO
	struct session_info_op siop;

	memset(&siop, 0, sizeof(siop));
#endif

	memset(&sess, 0, sizeof(sess));

	for (i = 0; i < sizeof(cipher_map) / sizeof(cipher_map[0]); i++) {
		if (cipher_map[i] == 0)
			continue;

		/* test if a cipher is support it and if yes register it */
		sess.cipher = cipher_map[i];
		sess.keylen = gnutls_cipher_get_key_size(i);
		sess.key = fake_key;

		if (ioctl(cfd, CIOCGSESSION, &sess)) {
			continue;
		}
#ifdef CIOCGSESSINFO
		siop.ses = sess.ses;	/* do not register ciphers that are not hw accelerated */
		if (ioctl(cfd, CIOCGSESSINFO, &siop) == 0) {
			if (!(siop.flags & SIOP_FLAG_KERNEL_DRIVER_ONLY)) {
				ioctl(cfd, CIOCFSESSION, &sess.ses);
				continue;
			}
		}
#endif

		ioctl(cfd, CIOCFSESSION, &sess.ses);

		_gnutls_debug_log("/dev/crypto: registering: %s\n",
				  gnutls_cipher_get_name(i));
		ret =
		    gnutls_crypto_single_cipher_register(i, 90,
							 &cipher_struct, 0);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

	}

	return 0;
}

#endif				/* CIOCAUTHCRYPT */

#endif				/* ENABLE_CRYPTODEV */
