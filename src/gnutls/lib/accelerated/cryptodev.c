/*
 * Copyright (C) 2009-2010, 2012 Free Software Foundation, Inc.
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

#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>

#ifndef CRYPTO_CIPHER_MAX_KEY_LEN
#define CRYPTO_CIPHER_MAX_KEY_LEN 64
#endif

#ifndef EALG_MAX_BLOCK_LEN
#define EALG_MAX_BLOCK_LEN 16
#endif

int _gnutls_cryptodev_fd = -1;

static int register_mac_digest(int cfd);

struct cryptodev_ctx {
	struct session_op sess;
	struct crypt_op cryp;
	uint8_t iv[EALG_MAX_BLOCK_LEN];

	int cfd;
};

static const int gnutls_cipher_map[] = {
	[GNUTLS_CIPHER_AES_128_CBC] = CRYPTO_AES_CBC,
	[GNUTLS_CIPHER_AES_192_CBC] = CRYPTO_AES_CBC,
	[GNUTLS_CIPHER_AES_256_CBC] = CRYPTO_AES_CBC,
	[GNUTLS_CIPHER_3DES_CBC] = CRYPTO_3DES_CBC,
	[GNUTLS_CIPHER_CAMELLIA_128_CBC] = CRYPTO_CAMELLIA_CBC,
	[GNUTLS_CIPHER_CAMELLIA_192_CBC] = CRYPTO_CAMELLIA_CBC,
	[GNUTLS_CIPHER_CAMELLIA_256_CBC] = CRYPTO_CAMELLIA_CBC,
	[GNUTLS_CIPHER_DES_CBC] = CRYPTO_DES_CBC,
};

static int
cryptodev_cipher_init(gnutls_cipher_algorithm_t algorithm, void **_ctx,
		      int enc)
{
	struct cryptodev_ctx *ctx;
	int cipher = gnutls_cipher_map[algorithm];

	*_ctx = gnutls_calloc(1, sizeof(struct cryptodev_ctx));
	if (*_ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ctx = *_ctx;

	ctx->cfd = _gnutls_cryptodev_fd;
	ctx->sess.cipher = cipher;
	ctx->cryp.iv = ctx->iv;

	return 0;
}

static int
cryptodev_cipher_setkey(void *_ctx, const void *key, size_t keysize)
{
	struct cryptodev_ctx *ctx = _ctx;

	CHECK_AES_KEYSIZE(keysize);

	ctx->sess.keylen = keysize;
	ctx->sess.key = (void *) key;

	if (ioctl(ctx->cfd, CIOCGSESSION, &ctx->sess)) {
		gnutls_assert();
		return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
	}
	ctx->cryp.ses = ctx->sess.ses;

	return 0;
}

static int cryptodev_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct cryptodev_ctx *ctx = _ctx;

	memcpy(ctx->iv, iv, iv_size);

	return 0;
}

static int
cryptodev_encrypt(void *_ctx, const void *src, size_t src_size,
		  void *dst, size_t dst_size)
{
	struct cryptodev_ctx *ctx = _ctx;
	ctx->cryp.len = src_size;
	ctx->cryp.src = (void *) src;
	ctx->cryp.dst = dst;
	ctx->cryp.op = COP_ENCRYPT;
	ctx->cryp.flags = COP_FLAG_WRITE_IV;

	if (ioctl(ctx->cfd, CIOCCRYPT, &ctx->cryp)) {
		gnutls_assert();
		return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
	}

	return 0;
}

static int
cryptodev_decrypt(void *_ctx, const void *src, size_t src_size,
		  void *dst, size_t dst_size)
{
	struct cryptodev_ctx *ctx = _ctx;

	ctx->cryp.len = src_size;
	ctx->cryp.src = (void *) src;
	ctx->cryp.dst = dst;
	ctx->cryp.op = COP_DECRYPT;
	ctx->cryp.flags = COP_FLAG_WRITE_IV;

	if (ioctl(ctx->cfd, CIOCCRYPT, &ctx->cryp)) {
		gnutls_assert();
		return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
	}

	return 0;
}

static void cryptodev_deinit(void *_ctx)
{
	struct cryptodev_ctx *ctx = _ctx;

	ioctl(ctx->cfd, CIOCFSESSION, &ctx->sess.ses);
	gnutls_free(ctx);
}

static const gnutls_crypto_cipher_st cipher_struct = {
	.init = cryptodev_cipher_init,
	.setkey = cryptodev_cipher_setkey,
	.setiv = cryptodev_setiv,
	.encrypt = cryptodev_encrypt,
	.decrypt = cryptodev_decrypt,
	.deinit = cryptodev_deinit,
};

static int register_crypto(int cfd)
{
	struct session_op sess;
	uint8_t fake_key[CRYPTO_CIPHER_MAX_KEY_LEN];
	unsigned int i;
	int ret;
#ifdef CIOCGSESSINFO
	struct session_info_op siop;
#endif

	memset(&sess, 0, sizeof(sess));

	for (i = 0;
	     i < sizeof(gnutls_cipher_map) / sizeof(gnutls_cipher_map[0]);
	     i++) {
		if (gnutls_cipher_map[i] == 0)
			continue;

		/* test if a cipher is supported and if yes register it */
		sess.cipher = gnutls_cipher_map[i];
		sess.keylen = gnutls_cipher_get_key_size(i);
		sess.key = fake_key;

		if (ioctl(cfd, CIOCGSESSION, &sess)) {
			continue;
		}
#ifdef CIOCGSESSINFO
		memset(&siop, 0, sizeof(siop));

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

#ifdef CIOCAUTHCRYPT
	return _cryptodev_register_gcm_crypto(cfd);
#else
	return 0;
#endif
}

int _gnutls_cryptodev_init(void)
{
	int ret;

	/* Open the crypto device */
	_gnutls_cryptodev_fd = open("/dev/crypto", O_RDWR, 0);
	if (_gnutls_cryptodev_fd < 0) {
		gnutls_assert();
		return GNUTLS_E_CRYPTODEV_DEVICE_ERROR;
	}
#ifndef CRIOGET_NOT_NEEDED
	{
		int cfd = -1;
		/* Clone file descriptor */
		if (ioctl(_gnutls_cryptodev_fd, CRIOGET, &cfd)) {
			gnutls_assert();
			return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
		}

		/* Set close-on-exec (not really neede here) */
		if (fcntl(cfd, F_SETFD, 1) == -1) {
			gnutls_assert();
			return GNUTLS_E_CRYPTODEV_IOCTL_ERROR;
		}

		close(_gnutls_cryptodev_fd);
		_gnutls_cryptodev_fd = cfd;
	}
#endif

	ret = register_crypto(_gnutls_cryptodev_fd);
	if (ret < 0)
		gnutls_assert();

	if (ret >= 0) {
		ret = register_mac_digest(_gnutls_cryptodev_fd);
		if (ret < 0)
			gnutls_assert();
	}

	if (ret < 0) {
		gnutls_assert();
		close(_gnutls_cryptodev_fd);
	}

	return ret;
}

void _gnutls_cryptodev_deinit(void)
{
	if (_gnutls_cryptodev_fd != -1)
		close(_gnutls_cryptodev_fd);
}

/* MAC and digest stuff */

/* if we are using linux /dev/crypto
 */
#if defined(COP_FLAG_UPDATE) && defined(COP_FLAG_RESET)

static const int gnutls_mac_map[] = {
	[GNUTLS_MAC_MD5] = CRYPTO_MD5_HMAC,
	[GNUTLS_MAC_SHA1] = CRYPTO_SHA1_HMAC,
	[GNUTLS_MAC_SHA256] = CRYPTO_SHA2_256_HMAC,
	[GNUTLS_MAC_SHA384] = CRYPTO_SHA2_384_HMAC,
	[GNUTLS_MAC_SHA512] = CRYPTO_SHA2_512_HMAC,
};

static int
cryptodev_mac_fast(gnutls_mac_algorithm_t algo,
		   const void *nonce, size_t nonce_size,
		   const void *key, size_t key_size, const void *text,
		   size_t text_size, void *digest)
{
	struct cryptodev_ctx ctx;
	int ret;

	assert(nonce_size == 0);

	memset(&ctx, 0, sizeof(ctx));
	ctx.cfd = _gnutls_cryptodev_fd;
	ctx.sess.mac = gnutls_mac_map[algo];

	ctx.sess.mackeylen = key_size;
	ctx.sess.mackey = (void *) key;

	if (ioctl(ctx.cfd, CIOCGSESSION, &ctx.sess))
		return gnutls_assert_val(GNUTLS_E_CRYPTODEV_IOCTL_ERROR);

	ctx.cryp.ses = ctx.sess.ses;

	ctx.cryp.len = text_size;
	ctx.cryp.src = (void *) text;
	ctx.cryp.dst = NULL;
	ctx.cryp.op = COP_ENCRYPT;
	ctx.cryp.mac = digest;

	ret = ioctl(ctx.cfd, CIOCCRYPT, &ctx.cryp);

	ioctl(_gnutls_cryptodev_fd, CIOCFSESSION, &ctx.sess.ses);
	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_CRYPTODEV_IOCTL_ERROR);

	return 0;
}

#define cryptodev_mac_deinit cryptodev_deinit

static const gnutls_crypto_mac_st mac_struct = {
	.init = NULL,
	.setkey = NULL,
	.setnonce = NULL,
	.hash = NULL,
	.output = NULL,
	.deinit = NULL,
	.fast = cryptodev_mac_fast
};

/* Digest algorithms */

static const int gnutls_digest_map[] = {
	[GNUTLS_DIG_MD5] = CRYPTO_MD5,
	[GNUTLS_DIG_SHA1] = CRYPTO_SHA1,
	[GNUTLS_DIG_SHA256] = CRYPTO_SHA2_256,
	[GNUTLS_DIG_SHA384] = CRYPTO_SHA2_384,
	[GNUTLS_DIG_SHA512] = CRYPTO_SHA2_512,
};

static int
cryptodev_digest_fast(gnutls_digest_algorithm_t algo,
		      const void *text, size_t text_size, void *digest)
{
	struct cryptodev_ctx ctx;
	int ret;

	memset(&ctx, 0, sizeof(ctx));
	ctx.cfd = _gnutls_cryptodev_fd;
	ctx.sess.mac = gnutls_digest_map[algo];

	if (ioctl(ctx.cfd, CIOCGSESSION, &ctx.sess))
		return gnutls_assert_val(GNUTLS_E_CRYPTODEV_IOCTL_ERROR);

	ctx.cryp.ses = ctx.sess.ses;

	ctx.cryp.len = text_size;
	ctx.cryp.src = (void *) text;
	ctx.cryp.dst = NULL;
	ctx.cryp.op = COP_ENCRYPT;
	ctx.cryp.mac = digest;

	ret = ioctl(ctx.cfd, CIOCCRYPT, &ctx.cryp);

	ioctl(_gnutls_cryptodev_fd, CIOCFSESSION, &ctx.sess.ses);
	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_CRYPTODEV_IOCTL_ERROR);

	return 0;
}

static const gnutls_crypto_digest_st digest_struct = {
	.init = NULL,
	.hash = NULL,
	.output = NULL,
	.deinit = NULL,
	.fast = cryptodev_digest_fast
};

static int register_mac_digest(int cfd)
{
	struct session_op sess;
	uint8_t fake_key[CRYPTO_CIPHER_MAX_KEY_LEN];
	unsigned int i;
	int ret;
#ifdef CIOCGSESSINFO
	struct session_info_op siop;
#endif

	memset(&sess, 0, sizeof(sess));
	for (i = 0; i < sizeof(gnutls_mac_map) / sizeof(gnutls_mac_map[0]);
	     i++) {
		if (gnutls_mac_map[i] == 0)
			continue;

		sess.mac = gnutls_mac_map[i];
		sess.mackeylen = 8;
		sess.mackey = fake_key;

		if (ioctl(cfd, CIOCGSESSION, &sess)) {
			continue;
		}
#ifdef CIOCGSESSINFO
		memset(&siop, 0, sizeof(siop));

		siop.ses = sess.ses;	/* do not register ciphers that are not hw accelerated */
		if (ioctl(cfd, CIOCGSESSINFO, &siop) == 0) {
			if (!(siop.flags & SIOP_FLAG_KERNEL_DRIVER_ONLY)) {
				ioctl(cfd, CIOCFSESSION, &sess.ses);
				continue;
			}
		}
#endif
		_gnutls_debug_log("/dev/crypto: registering: HMAC-%s\n",
				  gnutls_mac_get_name(i));

		ioctl(cfd, CIOCFSESSION, &sess.ses);

		ret =
		    gnutls_crypto_single_mac_register(i, 90, &mac_struct, 0);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	memset(&sess, 0, sizeof(sess));
	for (i = 0;
	     i < sizeof(gnutls_digest_map) / sizeof(gnutls_digest_map[0]);
	     i++) {
		if (gnutls_digest_map[i] == 0)
			continue;

		sess.mac = gnutls_digest_map[i];

		if (ioctl(cfd, CIOCGSESSION, &sess)) {
			continue;
		}
#ifdef CIOCGSESSINFO
		memset(&siop, 0, sizeof(siop));

		siop.ses = sess.ses;
		if (ioctl(cfd, CIOCGSESSINFO, &siop) == 0) {
			if (!(siop.flags & SIOP_FLAG_KERNEL_DRIVER_ONLY)) {
				ioctl(cfd, CIOCFSESSION, &sess.ses);
				continue;
			}
		}
#endif

		ioctl(cfd, CIOCFSESSION, &sess.ses);

		_gnutls_debug_log("/dev/crypto: registering: %s\n",
				  gnutls_mac_get_name(i));
		ret =
		    gnutls_crypto_single_digest_register(i, 90,
							 &digest_struct, 0);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	return 0;
}

#else
static int register_mac_digest(int cfd)
{
	return 0;
}

#endif				/* defined(COP_FLAG_UPDATE) */

#else				/* ENABLE_CRYPTODEV */
int _gnutls_cryptodev_init(void)
{
	return 0;
}

void _gnutls_cryptodev_deinit(void)
{
	return;
}
#endif				/* ENABLE_CRYPTODEV */
