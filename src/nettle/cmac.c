/*
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
   not, see http://www.gnu.org/licenses/.
*/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cmac.h"

#include "memxor.h"
#include "nettle-internal.h"
#include "macros.h"

/* shift one and XOR with 0x87. */
#if WORDS_BIGENDIAN
static void
block_mulx(union nettle_block16 *dst,
	   const union nettle_block16 *src)
{
  uint64_t carry = src->u64[0] >> 63;
  dst->u64[0] = (src->u64[0] << 1) | (src->u64[1] >> 63);
  dst->u64[1] = (src->u64[1] << 1) ^ (0x87 & -carry);
}
#else /* !WORDS_BIGENDIAN */
#define LE_SHIFT(x) ((((x) & 0x7f7f7f7f7f7f7f7f) << 1) | \
                     (((x) & 0x8080808080808080) >> 15))
static void
block_mulx(union nettle_block16 *dst,
	   const union nettle_block16 *src)
{
  uint64_t carry = (src->u64[0] & 0x80) >> 7;
  dst->u64[0] = LE_SHIFT(src->u64[0]) | ((src->u64[1] & 0x80) << 49);
  dst->u64[1] = LE_SHIFT(src->u64[1]) ^ (0x8700000000000000 & -carry);
}
#endif /* !WORDS_BIGENDIAN */

void
cmac128_set_key(struct cmac128_key *key, const void *cipher,
		nettle_cipher_func *encrypt)
{
  static const union nettle_block16 zero_block;
  union nettle_block16 L;

  /* step 1 - generate subkeys k1 and k2 */
  encrypt(cipher, 16, L.b, zero_block.b);

  block_mulx(&key->K1, &L);
  block_mulx(&key->K2, &key->K1);
}

void
cmac128_init(struct cmac128_ctx *ctx)
{
  memset(&ctx->X, 0, sizeof(ctx->X));
  ctx->index = 0;
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
cmac128_digest(struct cmac128_ctx *ctx, const struct cmac128_key *key,
	       const void *cipher, nettle_cipher_func *encrypt,
	       unsigned length, uint8_t *dst)
{
  union nettle_block16 Y;

  /* re-use ctx->block for memxor output */
  if (ctx->index < 16)
    {
      ctx->block.b[ctx->index] = 0x80;
      memset(ctx->block.b + ctx->index + 1, 0, 16 - 1 - ctx->index);

      memxor(ctx->block.b, key->K2.b, 16);
    }
  else
    {
      memxor(ctx->block.b, key->K1.b, 16);
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
  cmac128_init(ctx);
}
