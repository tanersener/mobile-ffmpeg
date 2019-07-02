/* backport of cmac*.c for CMAC

   AES-CMAC-128 (rfc 4493)
   Copyright (C) Stefan Metzmacher 2012
   Copyright (C) Jeremy Allison 2012
   Copyright (C) Michael Adam 2012
   Copyright (C) 2017, Red Hat Inc.

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see https://www.gnu.org/licenses/.
*/

/* #############################################
 * THIS IS A BACKPORT FROM NETTLE, DO NOT MODIFY
 * #############################################
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_NETTLE_CMAC128_UPDATE

#include <nettle/aes.h>
#include "cmac.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <nettle/memxor.h>
#include <nettle/macros.h>

/* shift one and XOR with 0x87. */
static void
block_mulx(union nettle_block16 *dst,
	   const union nettle_block16 *src)
{
  uint64_t b1 = READ_UINT64(src->b);
  uint64_t b2 = READ_UINT64(src->b+8);

  b1 = (b1 << 1) | (b2 >> 63);
  b2 <<= 1;

  if (src->b[0] & 0x80)
    b2 ^= 0x87;

  WRITE_UINT64(dst->b, b1);
  WRITE_UINT64(dst->b+8, b2);
}

void
cmac128_set_key(struct cmac128_ctx *ctx, const void *cipher,
		nettle_cipher_func *encrypt)
{
  static const uint8_t const_zero[] = {
    0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00
  };
  union nettle_block16 *L = &ctx->block;
  memset(ctx, 0, sizeof(*ctx));

  /* step 1 - generate subkeys k1 and k2 */
  encrypt(cipher, 16, L->b, const_zero);

  block_mulx(&ctx->K1, L);
  block_mulx(&ctx->K2, &ctx->K1);
}

#define MIN(x,y) ((x)<(y)?(x):(y))

void
cmac128_update(struct cmac128_ctx *ctx, const void *cipher,
	       nettle_cipher_func *encrypt,
	       size_t msg_len, const uint8_t *msg)
{
  union nettle_block16 Y;
  /*
   * check if we expand the block
   */
  if (ctx->index < 16)
    {
      size_t len = MIN(16 - ctx->index, msg_len);
      memcpy(&ctx->block.b[ctx->index], msg, len);
      msg += len;
      msg_len -= len;
      ctx->index += len;
    }

  if (msg_len == 0) {
    /* if it is still the last block, we are done */
    return;
  }

  /*
   * now checksum everything but the last block
   */
  memxor3(Y.b, ctx->X.b, ctx->block.b, 16);
  encrypt(cipher, 16, ctx->X.b, Y.b);

  while (msg_len > 16)
    {
      memxor3(Y.b, ctx->X.b, msg, 16);
      encrypt(cipher, 16, ctx->X.b, Y.b);
      msg += 16;
      msg_len -= 16;
    }

  /*
   * copy the last block, it will be processed in
   * cmac128_digest().
   */
  memcpy(ctx->block.b, msg, msg_len);
  ctx->index = msg_len;
}

void
cmac128_digest(struct cmac128_ctx *ctx, const void *cipher,
	       nettle_cipher_func *encrypt,
	       unsigned length,
	       uint8_t *dst)
{
  union nettle_block16 Y;

  memset(ctx->block.b+ctx->index, 0, sizeof(ctx->block.b)-ctx->index);

  /* re-use ctx->block for memxor output */
  if (ctx->index < 16)
    {
      ctx->block.b[ctx->index] = 0x80;
      memxor(ctx->block.b, ctx->K2.b, 16);
    }
  else
    {
      memxor(ctx->block.b, ctx->K1.b, 16);
    }

  memxor3(Y.b, ctx->block.b, ctx->X.b, 16);

  assert(length <= 16);
  if (length == 16)
    {
      encrypt(cipher, 16, dst, Y.b);
    }
  else
    {
      encrypt(cipher, 16, ctx->block.b, Y.b);
      memcpy(dst, ctx->block.b, length);
    }

  /* reset state for re-use */
  memset(&ctx->X, 0, sizeof(ctx->X));
  ctx->index = 0;
}

void
cmac_aes128_set_key(struct cmac_aes128_ctx *ctx, const uint8_t *key)
{
  CMAC128_SET_KEY(ctx, aes128_set_encrypt_key, aes128_encrypt, key);
}

void
cmac_aes128_update (struct cmac_aes128_ctx *ctx,
		   size_t length, const uint8_t *data)
{
  CMAC128_UPDATE (ctx, aes128_encrypt, length, data);
}

void
cmac_aes128_digest(struct cmac_aes128_ctx *ctx,
		  size_t length, uint8_t *digest)
{
  CMAC128_DIGEST(ctx, aes128_encrypt, length, digest);
}

void
cmac_aes256_set_key(struct cmac_aes256_ctx *ctx, const uint8_t *key)
{
  CMAC128_SET_KEY(ctx, aes256_set_encrypt_key, aes256_encrypt, key);
}

void
cmac_aes256_update (struct cmac_aes256_ctx *ctx,
		   size_t length, const uint8_t *data)
{
  CMAC128_UPDATE (ctx, aes256_encrypt, length, data);
}

void
cmac_aes256_digest(struct cmac_aes256_ctx *ctx,
		  size_t length, uint8_t *digest)
{
  CMAC128_DIGEST(ctx, aes256_encrypt, length, digest);
}
#endif /* HAVE_NETTLE_CMAC128_UPDATE */
