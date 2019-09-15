/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

/* Here lie nettle's wrappers for cipher support.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <cipher_int.h>
#include <nettle/aes.h>
#include <nettle/camellia.h>
#include <nettle/arcfour.h>
#include <nettle/arctwo.h>
#include <nettle/salsa20.h>
#include <nettle/des.h>
#include <nettle/version.h>
#if ENABLE_GOST
#ifndef HAVE_NETTLE_GOST28147_SET_KEY
#include "gost/gost28147.h"
#else
#include <nettle/gost28147.h>
#endif
#endif
#include <nettle/nettle-meta.h>
#include <nettle/cbc.h>
#include <nettle/gcm.h>
#include <nettle/ccm.h>
#include <nettle/chacha-poly1305.h>
#ifdef HAVE_NETTLE_CFB8_ENCRYPT
#include <nettle/cfb.h>
#else
#include "cfb8.h"
#endif /* HAVE_NETTLE_CFB8_ENCRYPT */
#include "xts.h"
#include <fips.h>

struct nettle_cipher_ctx;

/* Functions that refer to the nettle library.
 */
typedef void (*encrypt_func) (struct nettle_cipher_ctx*,
			      size_t length, 
			      uint8_t * dst,
			      const uint8_t * src);
typedef void (*decrypt_func) (struct nettle_cipher_ctx*,
			      size_t length, uint8_t *dst,
			      const uint8_t *src);

typedef void (*aead_encrypt_func) (struct nettle_cipher_ctx*,
			      size_t nonce_size, const void *nonce,
			      size_t auth_size, const void *auth,
			      size_t tag_size,
			      size_t length, uint8_t * dst,
			      const uint8_t * src);
typedef int (*aead_decrypt_func) (struct nettle_cipher_ctx*,
			      size_t nonce_size, const void *nonce,
			      size_t auth_size, const void *auth,
			      size_t tag_size,
			      size_t length, uint8_t * dst,
			      const uint8_t * src);

typedef void (*setiv_func) (void *ctx, size_t length, const uint8_t *);
typedef void (*gen_setkey_func) (void *ctx, size_t length, const uint8_t *);

struct nettle_cipher_st {
	gnutls_cipher_algorithm_t algo;
	unsigned ctx_size;
	nettle_cipher_func *encrypt_block;
	nettle_cipher_func *decrypt_block;
	unsigned block_size;
	unsigned key_size;
	unsigned max_iv_size;

	encrypt_func encrypt;
	decrypt_func decrypt;
	aead_encrypt_func aead_encrypt;
	aead_decrypt_func aead_decrypt;
	nettle_hash_update_func* auth;
	nettle_hash_digest_func* tag;
	nettle_set_key_func* set_encrypt_key;
	nettle_set_key_func* set_decrypt_key;
	gen_setkey_func gen_set_key; /* for arcfour which has variable key size */
	setiv_func set_iv;
};

struct nettle_cipher_ctx {
	const struct nettle_cipher_st *cipher;
	void *ctx_ptr; /* always 16-aligned */
	uint8_t iv[MAX_CIPHER_BLOCK_SIZE];
	unsigned iv_size;

	bool enc;
};

static void
_stream_encrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		const uint8_t * src)
{
	ctx->cipher->encrypt_block(ctx->ctx_ptr, length, dst, src);
}

static void
_stream_decrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		const uint8_t * src)
{
	ctx->cipher->decrypt_block(ctx->ctx_ptr, length, dst, src);
}

static void
_cbc_encrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		const uint8_t * src)
{
	cbc_encrypt(ctx->ctx_ptr, ctx->cipher->encrypt_block,
		    ctx->iv_size, ctx->iv,
		    length, dst, src);
}

static void
_cbc_decrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		const uint8_t * src)
{
	cbc_decrypt(ctx->ctx_ptr, ctx->cipher->decrypt_block,
		    ctx->iv_size, ctx->iv,
		    length, dst, src);
}

#if ENABLE_GOST
static void
_cfb_encrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		const uint8_t * src)
{
	cfb_encrypt(ctx->ctx_ptr, ctx->cipher->encrypt_block,
		    ctx->iv_size, ctx->iv,
		    length, dst, src);
}

static void
_cfb_decrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		const uint8_t * src)
{
	cfb_decrypt(ctx->ctx_ptr, ctx->cipher->encrypt_block,
		    ctx->iv_size, ctx->iv,
		    length, dst, src);
}

static void
_gost28147_set_key_tc26z(void *ctx, const uint8_t *key)
{
	gost28147_set_key(ctx, key);
	gost28147_set_param(ctx, &gost28147_param_TC26_Z);
}

static void
_gost28147_set_key_cpa(void *ctx, const uint8_t *key)
{
	gost28147_set_key(ctx, key);
	gost28147_set_param(ctx, &gost28147_param_CryptoPro_A);
}

static void
_gost28147_set_key_cpb(void *ctx, const uint8_t *key)
{
	gost28147_set_key(ctx, key);
	gost28147_set_param(ctx, &gost28147_param_CryptoPro_B);
}

static void
_gost28147_set_key_cpc(void *ctx, const uint8_t *key)
{
	gost28147_set_key(ctx, key);
	gost28147_set_param(ctx, &gost28147_param_CryptoPro_C);
}

static void
_gost28147_set_key_cpd(void *ctx, const uint8_t *key)
{
	gost28147_set_key(ctx, key);
	gost28147_set_param(ctx, &gost28147_param_CryptoPro_D);
}
#endif

static void
_ccm_encrypt(struct nettle_cipher_ctx *ctx,
	      size_t nonce_size, const void *nonce,
	      size_t auth_size, const void *auth,
	      size_t tag_size,
	      size_t length, uint8_t * dst,
	      const uint8_t * src)
{
	ccm_encrypt_message((void*)ctx->ctx_ptr, ctx->cipher->encrypt_block,
			    nonce_size, nonce,
			    auth_size, auth,
			    tag_size, length, dst, src);
}

static int
_ccm_decrypt(struct nettle_cipher_ctx *ctx,
	      size_t nonce_size, const void *nonce,
	      size_t auth_size, const void *auth,
	      size_t tag_size,
	      size_t length, uint8_t * dst,
	      const uint8_t * src)
{
	return ccm_decrypt_message((void*)ctx->ctx_ptr, ctx->cipher->encrypt_block,
				    nonce_size, nonce,
				    auth_size, auth,
				    tag_size, length, dst, src);
}

static void
_chacha_poly1305_set_nonce (struct chacha_poly1305_ctx *ctx,
		   size_t length, const uint8_t *nonce)
{
	chacha_poly1305_set_nonce(ctx, nonce);
}
		   
struct gcm_cast_st { struct gcm_key key; struct gcm_ctx gcm; unsigned long xx[1]; };
#define GCM_CTX_GET_KEY(ptr) (&((struct gcm_cast_st*)ptr)->key)
#define GCM_CTX_GET_CTX(ptr) (&((struct gcm_cast_st*)ptr)->gcm)
#define GCM_CTX_GET_CIPHER(ptr) ((void*)&((struct gcm_cast_st*)ptr)->xx)

static void
_gcm_encrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		 const uint8_t * src)
{
	gcm_encrypt(GCM_CTX_GET_CTX(ctx->ctx_ptr), GCM_CTX_GET_KEY(ctx->ctx_ptr),
		    GCM_CTX_GET_CIPHER(ctx->ctx_ptr), ctx->cipher->encrypt_block,
		    length, dst, src);
}

static void
_gcm_decrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		 const uint8_t * src)
{
	gcm_decrypt(GCM_CTX_GET_CTX(ctx->ctx_ptr), GCM_CTX_GET_KEY(ctx->ctx_ptr),
		    GCM_CTX_GET_CIPHER(ctx->ctx_ptr), ctx->cipher->encrypt_block,
		    length, dst, src);
}

static void _des_set_key(struct des_ctx *ctx, const uint8_t *key)
{
	des_set_key(ctx, key);
}

static void _des3_set_key(struct des3_ctx *ctx, const uint8_t *key)
{
	des3_set_key(ctx, key);
}

static void
_cfb8_encrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
	      const uint8_t * src)
{
	cfb8_encrypt(ctx->ctx_ptr, ctx->cipher->encrypt_block,
		     ctx->iv_size, ctx->iv,
		     length, dst, src);
}

static void
_cfb8_decrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
	      const uint8_t * src)
{
	cfb8_decrypt(ctx->ctx_ptr, ctx->cipher->encrypt_block,
		     ctx->iv_size, ctx->iv,
		     length, dst, src);
}

static void
_xts_aes128_encrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		    const uint8_t * src)
{
	xts_aes128_encrypt_message(ctx->ctx_ptr, ctx->iv, length, dst, src);
}

static void
_xts_aes128_decrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		    const uint8_t * src)
{
	xts_aes128_decrypt_message(ctx->ctx_ptr, ctx->iv, length, dst, src);
}

static void
_xts_aes256_encrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		    const uint8_t * src)
{
	xts_aes256_encrypt_message(ctx->ctx_ptr, ctx->iv, length, dst, src);
}

static void
_xts_aes256_decrypt(struct nettle_cipher_ctx *ctx, size_t length, uint8_t * dst,
		    const uint8_t * src)
{
	xts_aes256_decrypt_message(ctx->ctx_ptr, ctx->iv, length, dst, src);
}

static const struct nettle_cipher_st builtin_ciphers[] = {
	{  .algo = GNUTLS_CIPHER_AES_128_GCM,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES128_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes128_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes128_decrypt,

	   .ctx_size = sizeof(struct gcm_aes128_ctx),
	   .encrypt = _gcm_encrypt,
	   .decrypt = _gcm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)gcm_aes128_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)gcm_aes128_set_key,

	   .tag = (nettle_hash_digest_func*)gcm_aes128_digest,
	   .auth = (nettle_hash_update_func*)gcm_aes128_update,
	   .set_iv = (setiv_func)gcm_aes128_set_iv,
	   .max_iv_size = GCM_IV_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_256_GCM,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes256_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes256_decrypt,

	   .ctx_size = sizeof(struct gcm_aes256_ctx),
	   .encrypt = _gcm_encrypt,
	   .decrypt = _gcm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)gcm_aes256_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)gcm_aes256_set_key,
	   
	   .tag = (nettle_hash_digest_func*)gcm_aes256_digest,
	   .auth = (nettle_hash_update_func*)gcm_aes256_update,
	   .set_iv = (setiv_func)gcm_aes256_set_iv,
	   .max_iv_size = GCM_IV_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_128_CCM,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES128_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes128_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes128_decrypt,

	   .ctx_size = sizeof(struct aes128_ctx),
	   .aead_encrypt = _ccm_encrypt,
	   .aead_decrypt = _ccm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes128_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes128_set_encrypt_key,
	   .max_iv_size = CCM_MAX_NONCE_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_128_CCM_8,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES128_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes128_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes128_decrypt,

	   .ctx_size = sizeof(struct aes128_ctx),
	   .aead_encrypt = _ccm_encrypt,
	   .aead_decrypt = _ccm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes128_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes128_set_encrypt_key,
	   .max_iv_size = CCM_MAX_NONCE_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_256_CCM,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes256_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes256_decrypt,

	   .ctx_size = sizeof(struct aes256_ctx),
	   .aead_encrypt = _ccm_encrypt,
	   .aead_decrypt = _ccm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes256_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes256_set_encrypt_key,
	   .max_iv_size = CCM_MAX_NONCE_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_256_CCM_8,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes256_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes256_decrypt,

	   .ctx_size = sizeof(struct aes256_ctx),
	   .aead_encrypt = _ccm_encrypt,
	   .aead_decrypt = _ccm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes256_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes256_set_encrypt_key,
	   .max_iv_size = CCM_MAX_NONCE_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_CAMELLIA_128_GCM,
	   .block_size = CAMELLIA_BLOCK_SIZE,
	   .key_size = CAMELLIA128_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)camellia128_crypt,
	   .decrypt_block = (nettle_cipher_func*)camellia128_crypt,

	   .ctx_size = sizeof(struct gcm_camellia128_ctx),
	   .encrypt = _gcm_encrypt,
	   .decrypt = _gcm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)gcm_camellia128_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)gcm_camellia128_set_key,
	   .tag = (nettle_hash_digest_func*)gcm_camellia128_digest,
	   .auth = (nettle_hash_update_func*)gcm_camellia128_update,
	   .max_iv_size = GCM_IV_SIZE,
	   .set_iv = (setiv_func)gcm_camellia128_set_iv
	},
	{  .algo = GNUTLS_CIPHER_CAMELLIA_256_GCM,
	   .block_size = CAMELLIA_BLOCK_SIZE,
	   .key_size = CAMELLIA256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)camellia256_crypt,
	   .decrypt_block = (nettle_cipher_func*)camellia256_crypt,

	   .ctx_size = sizeof(struct gcm_camellia256_ctx),
	   .encrypt = _gcm_encrypt,
	   .decrypt = _gcm_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)gcm_camellia256_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)gcm_camellia256_set_key,
	   .tag = (nettle_hash_digest_func*)gcm_camellia256_digest,
	   .auth = (nettle_hash_update_func*)gcm_camellia256_update,
	   .max_iv_size = GCM_IV_SIZE,
	   .set_iv = (setiv_func)gcm_camellia256_set_iv
	},
	{  .algo = GNUTLS_CIPHER_AES_128_CBC,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES128_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes128_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes128_decrypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct aes128_ctx, AES_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes128_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes128_set_decrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_192_CBC,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES192_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes192_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes192_decrypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct aes192_ctx, AES_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes192_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes192_set_decrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_256_CBC,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes256_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes256_decrypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct aes256_ctx, AES_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes256_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes256_set_decrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_CAMELLIA_128_CBC,
	   .block_size = CAMELLIA_BLOCK_SIZE,
	   .key_size = CAMELLIA128_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)camellia128_crypt,
	   .decrypt_block = (nettle_cipher_func*)camellia128_crypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct camellia128_ctx, CAMELLIA_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)camellia128_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)camellia128_set_decrypt_key,
	   .max_iv_size = CAMELLIA_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_CAMELLIA_192_CBC,
	   .block_size = CAMELLIA_BLOCK_SIZE,
	   .key_size = CAMELLIA192_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)camellia192_crypt,
	   .decrypt_block = (nettle_cipher_func*)camellia192_crypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct camellia192_ctx, CAMELLIA_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)camellia192_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)camellia192_set_decrypt_key,
	   .max_iv_size = CAMELLIA_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_CAMELLIA_256_CBC,
	   .block_size = CAMELLIA_BLOCK_SIZE,
	   .key_size = CAMELLIA256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)camellia256_crypt,
	   .decrypt_block = (nettle_cipher_func*)camellia256_crypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct camellia256_ctx, CAMELLIA_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)camellia256_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)camellia256_set_decrypt_key,
	   .max_iv_size = CAMELLIA_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_RC2_40_CBC,
	   .block_size = ARCTWO_BLOCK_SIZE,
	   .key_size = 5,
	   .encrypt_block = (nettle_cipher_func*)arctwo_encrypt,
	   .decrypt_block = (nettle_cipher_func*)arctwo_decrypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct arctwo_ctx, ARCTWO_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)arctwo40_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)arctwo40_set_key,
	   .max_iv_size = ARCTWO_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_DES_CBC,
	   .block_size = DES_BLOCK_SIZE,
	   .key_size = DES_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)des_encrypt,
	   .decrypt_block = (nettle_cipher_func*)des_decrypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct des_ctx, DES_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)_des_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)_des_set_key,
	   .max_iv_size = DES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_3DES_CBC,
	   .block_size = DES3_BLOCK_SIZE,
	   .key_size = DES3_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)des3_encrypt,
	   .decrypt_block = (nettle_cipher_func*)des3_decrypt,

	   .ctx_size = sizeof(struct CBC_CTX(struct des3_ctx, DES3_BLOCK_SIZE)),
	   .encrypt = _cbc_encrypt,
	   .decrypt = _cbc_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)_des3_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)_des3_set_key,
	   .max_iv_size = DES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_ARCFOUR_128,
	   .block_size = 1,
	   .key_size = 0,
	   .encrypt_block = (nettle_cipher_func*)arcfour_crypt,
	   .decrypt_block = (nettle_cipher_func*)arcfour_crypt,

	   .ctx_size = sizeof(struct arcfour_ctx),
	   .encrypt = _stream_encrypt,
	   .decrypt = _stream_encrypt,
	   .gen_set_key = (gen_setkey_func)arcfour_set_key,
	   .set_encrypt_key = (nettle_set_key_func*)arcfour128_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)arcfour128_set_key,
	},
	{  .algo = GNUTLS_CIPHER_SALSA20_256,
	   .block_size = 1,
	   .key_size = SALSA20_256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)salsa20_crypt,
	   .decrypt_block = (nettle_cipher_func*)salsa20_crypt,

	   .ctx_size = sizeof(struct salsa20_ctx),
	   .encrypt = _stream_encrypt,
	   .decrypt = _stream_encrypt,
	   .set_encrypt_key = (nettle_set_key_func*)salsa20_256_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)salsa20_256_set_key,
	   .max_iv_size = SALSA20_NONCE_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_ESTREAM_SALSA20_256,
	   .block_size = 1,
	   .key_size = SALSA20_256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)salsa20r12_crypt,
	   .decrypt_block = (nettle_cipher_func*)salsa20r12_crypt,

	   .ctx_size = sizeof(struct salsa20_ctx),
	   .encrypt = _stream_encrypt,
	   .decrypt = _stream_encrypt,
	   .set_encrypt_key = (nettle_set_key_func*)salsa20_256_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)salsa20_256_set_key,
	   .max_iv_size = SALSA20_NONCE_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_CHACHA20_POLY1305,
	   .block_size = CHACHA_POLY1305_BLOCK_SIZE,
	   .key_size = CHACHA_POLY1305_KEY_SIZE,

	   .ctx_size = sizeof(struct chacha_poly1305_ctx),
	   .encrypt_block = (nettle_cipher_func*)chacha_poly1305_encrypt,
	   .decrypt_block = (nettle_cipher_func*)chacha_poly1305_decrypt,
	   .encrypt = _stream_encrypt,
	   .decrypt = _stream_decrypt,
	   .auth = (nettle_hash_update_func*)chacha_poly1305_update,
	   .tag = (nettle_hash_digest_func*)chacha_poly1305_digest,
	   .set_encrypt_key = (nettle_set_key_func*)chacha_poly1305_set_key,
	   .set_decrypt_key = (nettle_set_key_func*)chacha_poly1305_set_key,
	   .set_iv = (setiv_func)_chacha_poly1305_set_nonce,
	   .max_iv_size = CHACHA_POLY1305_NONCE_SIZE,
	},
#if ENABLE_GOST
	{
	   .algo = GNUTLS_CIPHER_GOST28147_TC26Z_CFB,
	   .block_size = GOST28147_BLOCK_SIZE,
	   .key_size = GOST28147_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,
	   .decrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,

	   .ctx_size = sizeof(struct CFB_CTX(struct gost28147_ctx, GOST28147_BLOCK_SIZE)),
	   .encrypt = _cfb_encrypt,
	   .decrypt = _cfb_decrypt,
	   .set_encrypt_key = _gost28147_set_key_tc26z,
	   .set_decrypt_key = _gost28147_set_key_tc26z,
	},
	{
	   .algo = GNUTLS_CIPHER_GOST28147_CPA_CFB,
	   .block_size = GOST28147_BLOCK_SIZE,
	   .key_size = GOST28147_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,
	   .decrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,

	   .ctx_size = sizeof(struct CFB_CTX(struct gost28147_ctx, GOST28147_BLOCK_SIZE)),
	   .encrypt = _cfb_encrypt,
	   .decrypt = _cfb_decrypt,
	   .set_encrypt_key = _gost28147_set_key_cpa,
	   .set_decrypt_key = _gost28147_set_key_cpa,
	},
	{
	   .algo = GNUTLS_CIPHER_GOST28147_CPB_CFB,
	   .block_size = GOST28147_BLOCK_SIZE,
	   .key_size = GOST28147_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,
	   .decrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,

	   .ctx_size = sizeof(struct CFB_CTX(struct gost28147_ctx, GOST28147_BLOCK_SIZE)),
	   .encrypt = _cfb_encrypt,
	   .decrypt = _cfb_decrypt,
	   .set_encrypt_key = _gost28147_set_key_cpb,
	   .set_decrypt_key = _gost28147_set_key_cpb,
	},
	{
	   .algo = GNUTLS_CIPHER_GOST28147_CPC_CFB,
	   .block_size = GOST28147_BLOCK_SIZE,
	   .key_size = GOST28147_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,
	   .decrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,

	   .ctx_size = sizeof(struct CFB_CTX(struct gost28147_ctx, GOST28147_BLOCK_SIZE)),
	   .encrypt = _cfb_encrypt,
	   .decrypt = _cfb_decrypt,
	   .set_encrypt_key = _gost28147_set_key_cpc,
	   .set_decrypt_key = _gost28147_set_key_cpc,
	},
	{
	   .algo = GNUTLS_CIPHER_GOST28147_CPD_CFB,
	   .block_size = GOST28147_BLOCK_SIZE,
	   .key_size = GOST28147_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,
	   .decrypt_block = (nettle_cipher_func*)gost28147_encrypt_for_cfb,

	   .ctx_size = sizeof(struct CFB_CTX(struct gost28147_ctx, GOST28147_BLOCK_SIZE)),
	   .encrypt = _cfb_encrypt,
	   .decrypt = _cfb_decrypt,
	   .set_encrypt_key = _gost28147_set_key_cpd,
	   .set_decrypt_key = _gost28147_set_key_cpd,
	},
#endif
	{  .algo = GNUTLS_CIPHER_AES_128_CFB8,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES128_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes128_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes128_encrypt,

	   .ctx_size = sizeof(struct CFB8_CTX(struct aes128_ctx, AES_BLOCK_SIZE)),
	   .encrypt = _cfb8_encrypt,
	   .decrypt = _cfb8_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes128_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes128_set_encrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_192_CFB8,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES192_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes192_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes192_encrypt,

	   .ctx_size = sizeof(struct CFB8_CTX(struct aes192_ctx, AES_BLOCK_SIZE)),
	   .encrypt = _cfb8_encrypt,
	   .decrypt = _cfb8_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes192_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes192_set_encrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_256_CFB8,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES256_KEY_SIZE,
	   .encrypt_block = (nettle_cipher_func*)aes256_encrypt,
	   .decrypt_block = (nettle_cipher_func*)aes256_encrypt,

	   .ctx_size = sizeof(struct CFB8_CTX(struct aes256_ctx, AES_BLOCK_SIZE)),
	   .encrypt = _cfb8_encrypt,
	   .decrypt = _cfb8_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)aes256_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)aes256_set_encrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_128_XTS,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES128_KEY_SIZE * 2,

	   .ctx_size = sizeof(struct xts_aes128_key),
	   .encrypt = _xts_aes128_encrypt,
	   .decrypt = _xts_aes128_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)xts_aes128_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)xts_aes128_set_decrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
	{  .algo = GNUTLS_CIPHER_AES_256_XTS,
	   .block_size = AES_BLOCK_SIZE,
	   .key_size = AES256_KEY_SIZE * 2,

	   .ctx_size = sizeof(struct xts_aes256_key),
	   .encrypt = _xts_aes256_encrypt,
	   .decrypt = _xts_aes256_decrypt,
	   .set_encrypt_key = (nettle_set_key_func*)xts_aes256_set_encrypt_key,
	   .set_decrypt_key = (nettle_set_key_func*)xts_aes256_set_decrypt_key,
	   .max_iv_size = AES_BLOCK_SIZE,
	},
};

static int wrap_nettle_cipher_exists(gnutls_cipher_algorithm_t algo)
{
	unsigned i;
	for (i=0;i<sizeof(builtin_ciphers)/sizeof(builtin_ciphers[0]);i++) {
		if (algo == builtin_ciphers[i].algo) {
			return 1;
		}
	}
	return 0;
}

static int
wrap_nettle_cipher_init(gnutls_cipher_algorithm_t algo, void **_ctx,
			int enc)
{
	struct nettle_cipher_ctx *ctx;
	ptrdiff_t cur_alignment;
	int idx = -1;
	unsigned i;
	uint8_t *ctx_ptr;

	for (i=0;i<sizeof(builtin_ciphers)/sizeof(builtin_ciphers[0]);i++) {
		if (algo == builtin_ciphers[i].algo) {
			idx = i;
			break;
		}
	}

	if (idx == -1)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ctx = gnutls_calloc(1, sizeof(*ctx)+builtin_ciphers[idx].ctx_size+16);
	if (ctx == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ctx->enc = enc;
	ctx_ptr = ((uint8_t*)ctx) + sizeof(*ctx);

	cur_alignment = ((ptrdiff_t)ctx_ptr) % 16;
	if (cur_alignment > 0)
		ctx_ptr += 16 - cur_alignment;

	ctx->ctx_ptr = ctx_ptr;
	ctx->cipher = &builtin_ciphers[idx];

	*_ctx = ctx;

	return 0;
}

static int
wrap_nettle_cipher_setkey(void *_ctx, const void *key, size_t keysize)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	if (ctx->cipher->key_size > 0 && unlikely(keysize != ctx->cipher->key_size)) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	} else if (ctx->cipher->key_size == 0) {
		ctx->cipher->gen_set_key(ctx->ctx_ptr, keysize, key);
		return 0;
	}

	if (ctx->enc)
		ctx->cipher->set_encrypt_key(ctx->ctx_ptr, key);
	else
		ctx->cipher->set_decrypt_key(ctx->ctx_ptr, key);

	return 0;
}

static int
wrap_nettle_cipher_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;
	unsigned max_iv;

	switch (ctx->cipher->algo) {
	case GNUTLS_CIPHER_AES_128_GCM:
	case GNUTLS_CIPHER_AES_256_GCM:
		FIPS_RULE(iv_size < GCM_IV_SIZE, GNUTLS_E_INVALID_REQUEST, "access to short GCM nonce size\n");
		break;
	case GNUTLS_CIPHER_SALSA20_256:
	case GNUTLS_CIPHER_ESTREAM_SALSA20_256:
		if (iv_size != SALSA20_IV_SIZE)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		break;
	default:
		break;
	}

	max_iv = ctx->cipher->max_iv_size;
	if (max_iv == 0)
		max_iv = MAX_CIPHER_BLOCK_SIZE;

	if (iv_size > max_iv)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (ctx->cipher->set_iv) {
		ctx->cipher->set_iv(ctx->ctx_ptr, iv_size, iv);
	} else {
		if (iv)
			memcpy(ctx->iv, iv, iv_size);
		ctx->iv_size = iv_size;
	}

	return 0;
}

static int
wrap_nettle_cipher_getiv(void *_ctx, void *iv, size_t iv_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	if (iv_size < ctx->iv_size)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memcpy(iv, ctx->iv, ctx->iv_size);

	return (int) ctx->iv_size;
}

static int
wrap_nettle_cipher_decrypt(void *_ctx, const void *encr, size_t encr_size,
			   void *plain, size_t plain_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	if (unlikely(ctx->cipher->decrypt == NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ctx->cipher->decrypt(ctx, encr_size, plain, encr);

	return 0;
}

static int
wrap_nettle_cipher_encrypt(void *_ctx, const void *plain, size_t plain_size,
			   void *encr, size_t encr_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	if (unlikely(ctx->cipher->encrypt == NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ctx->cipher->encrypt(ctx, plain_size, encr, plain);

	return 0;
}

static int
wrap_nettle_cipher_aead_encrypt(void *_ctx,
				const void *nonce, size_t nonce_size,
				const void *auth, size_t auth_size,
				size_t tag_size,
				const void *plain, size_t plain_size,
				void *encr, size_t encr_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	if (ctx->cipher->aead_encrypt == NULL) {
		/* proper AEAD cipher */
		if (encr_size < plain_size + tag_size)
			return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);

		ctx->cipher->set_iv(ctx->ctx_ptr, nonce_size, nonce);
		ctx->cipher->auth(ctx->ctx_ptr, auth_size, auth);

		ctx->cipher->encrypt(ctx, plain_size, encr, plain);

		ctx->cipher->tag(ctx->ctx_ptr, tag_size, ((uint8_t*)encr) + plain_size);
	} else {
		/* CCM-style cipher */
		ctx->cipher->aead_encrypt(ctx,
					  nonce_size, nonce,
					  auth_size, auth,
					  tag_size,
					  tag_size+plain_size, encr,
					  plain);
	}
	return 0;
}

static int
wrap_nettle_cipher_aead_decrypt(void *_ctx,
				const void *nonce, size_t nonce_size,
				const void *auth, size_t auth_size,
				size_t tag_size,
				const void *encr, size_t encr_size,
				void *plain, size_t plain_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;
	int ret;

	if (unlikely(encr_size < tag_size))
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	if (ctx->cipher->aead_decrypt == NULL) {
		/* proper AEAD cipher */
		uint8_t tag[MAX_HASH_SIZE];

		ctx->cipher->set_iv(ctx->ctx_ptr, nonce_size, nonce);
		ctx->cipher->auth(ctx->ctx_ptr, auth_size, auth);

		encr_size -= tag_size;
		ctx->cipher->decrypt(ctx, encr_size, plain, encr);

		ctx->cipher->tag(ctx->ctx_ptr, tag_size, tag);

		if (gnutls_memcmp(((uint8_t*)encr)+encr_size, tag, tag_size) != 0)
			return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
	} else {
		/* CCM-style cipher */
		encr_size -= tag_size;
		ret = ctx->cipher->aead_decrypt(ctx,
						nonce_size, nonce,
						auth_size, auth,
						tag_size,
						encr_size, plain,
						encr);
		if (unlikely(ret == 0))
				return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
	}
	return 0;
}

static int
wrap_nettle_cipher_auth(void *_ctx, const void *plain, size_t plain_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	ctx->cipher->auth(ctx->ctx_ptr, plain_size, plain);

	return 0;
}

static void wrap_nettle_cipher_tag(void *_ctx, void *tag, size_t tag_size)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	ctx->cipher->tag(ctx->ctx_ptr, tag_size, tag);

}

static void wrap_nettle_cipher_close(void *_ctx)
{
	struct nettle_cipher_ctx *ctx = _ctx;

	zeroize_temp_key(ctx->ctx_ptr, ctx->cipher->ctx_size);
	gnutls_free(ctx);
}

gnutls_crypto_cipher_st _gnutls_cipher_ops = {
	.init = wrap_nettle_cipher_init,
	.exists = wrap_nettle_cipher_exists,
	.setiv = wrap_nettle_cipher_setiv,
	.getiv = wrap_nettle_cipher_getiv,
	.setkey = wrap_nettle_cipher_setkey,
	.encrypt = wrap_nettle_cipher_encrypt,
	.decrypt = wrap_nettle_cipher_decrypt,
	.aead_encrypt = wrap_nettle_cipher_aead_encrypt,
	.aead_decrypt = wrap_nettle_cipher_aead_decrypt,
	.deinit = wrap_nettle_cipher_close,
	.auth = wrap_nettle_cipher_auth,
	.tag = wrap_nettle_cipher_tag,
};
