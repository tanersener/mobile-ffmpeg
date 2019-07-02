/* xts.c

   XEX-based tweaked-codebook mode with ciphertext stealing (XTS)

   Copyright (C) 2018 Red Hat, Inc.

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

#include "xts.h"

#include "macros.h"
#include "memxor.h"
#include "nettle-internal.h"

/* shift left one and XOR with 0x87 if there is carry. */
/* the algorithm reads this as a 128bit Little Endian number */
/* src and dest can point to the same buffer for in-place operations */
#if WORDS_BIGENDIAN
#define BE_SHIFT(x) ((((x) & 0x7f7f7f7f7f7f7f7f) << 1) | \
                     (((x) & 0x8080808080808080) >> 15))
static void
xts_shift(union nettle_block16 *dst,
          const union nettle_block16 *src)
{
  uint64_t carry = (src->u64[1] & 0x80) >> 7;
  dst->u64[1] = BE_SHIFT(src->u64[1]) | ((src->u64[0] & 0x80) << 49);
  dst->u64[0] = BE_SHIFT(src->u64[0]) ^ (0x8700000000000000 & -carry);
}
#else /* !WORDS_BIGENDIAN */
static void
xts_shift(union nettle_block16 *dst,
          const union nettle_block16 *src)
{
  uint64_t carry = src->u64[1] >> 63;
  dst->u64[1] = (src->u64[1] << 1) | (src->u64[0] >> 63);
  dst->u64[0] = (src->u64[0] << 1) ^ (0x87 & -carry);
}
#endif /* !WORDS_BIGNDIAN */

static void
check_length(size_t length, uint8_t *dst)
{
  assert(length >= XTS_BLOCK_SIZE);
  /* asserts may be compiled out, try to save the user by zeroing the dst in
   * case the buffer contains sensitive data (like the clear text for inplace
   * encryption) */
  if (length < XTS_BLOCK_SIZE)
    memset(dst, '\0', length);
}

/* works also for inplace encryption/decryption */

void
xts_encrypt_message(const void *enc_ctx, const void *twk_ctx,
	            nettle_cipher_func *encf,
	            const uint8_t *tweak, size_t length,
	            uint8_t *dst, const uint8_t *src)
{
  union nettle_block16 T;
  union nettle_block16 P;

  check_length(length, dst);

  encf(twk_ctx, XTS_BLOCK_SIZE, T.b, tweak);

  /* the zeroth power of alpha is the initial ciphertext value itself, so we
   * skip shifting and do it at the end of each block operation instead */
  for (;length >= 2 * XTS_BLOCK_SIZE || length == XTS_BLOCK_SIZE;
       length -= XTS_BLOCK_SIZE, src += XTS_BLOCK_SIZE, dst += XTS_BLOCK_SIZE)
    {
      memxor3(P.b, src, T.b, XTS_BLOCK_SIZE);	/* P -> PP */
      encf(enc_ctx, XTS_BLOCK_SIZE, dst, P.b);  /* CC */
      memxor(dst, T.b, XTS_BLOCK_SIZE);	        /* CC -> C */

      /* shift T for next block if any */
      if (length > XTS_BLOCK_SIZE)
          xts_shift(&T, &T);
    }

  /* if the last block is partial, handle via stealing */
  if (length)
    {
      /* S Holds the real C(n-1) (Whole last block to steal from) */
      union nettle_block16 S;

      memxor3(P.b, src, T.b, XTS_BLOCK_SIZE);	/* P -> PP */
      encf(enc_ctx, XTS_BLOCK_SIZE, S.b, P.b);  /* CC */
      memxor(S.b, T.b, XTS_BLOCK_SIZE);	        /* CC -> S */

      /* shift T for next block */
      xts_shift(&T, &T);

      length -= XTS_BLOCK_SIZE;
      src += XTS_BLOCK_SIZE;

      memxor3(P.b, src, T.b, length);           /* P |.. */
      /* steal ciphertext to complete block */
      memxor3(P.b + length, S.b + length, T.b + length,
              XTS_BLOCK_SIZE - length);         /* ..| S_2 -> PP */

      encf(enc_ctx, XTS_BLOCK_SIZE, dst, P.b);  /* CC */
      memxor(dst, T.b, XTS_BLOCK_SIZE);         /* CC -> C(n-1) */

      /* Do this after we read src so inplace operations do not break */
      dst += XTS_BLOCK_SIZE;
      memcpy(dst, S.b, length);                 /* S_1 -> C(n) */
    }
}

void
xts_decrypt_message(const void *dec_ctx, const void *twk_ctx,
	            nettle_cipher_func *decf, nettle_cipher_func *encf,
	            const uint8_t *tweak, size_t length,
	            uint8_t *dst, const uint8_t *src)
{
  union nettle_block16 T;
  union nettle_block16 C;

  check_length(length, dst);

  encf(twk_ctx, XTS_BLOCK_SIZE, T.b, tweak);

  for (;length >= 2 * XTS_BLOCK_SIZE || length == XTS_BLOCK_SIZE;
       length -= XTS_BLOCK_SIZE, src += XTS_BLOCK_SIZE, dst += XTS_BLOCK_SIZE)
    {
      memxor3(C.b, src, T.b, XTS_BLOCK_SIZE);	/* c -> CC */
      decf(dec_ctx, XTS_BLOCK_SIZE, dst, C.b);  /* PP */
      memxor(dst, T.b, XTS_BLOCK_SIZE);	        /* PP -> P */

      /* shift T for next block if any */
      if (length > XTS_BLOCK_SIZE)
          xts_shift(&T, &T);
    }

  /* if the last block is partial, handle via stealing */
  if (length)
    {
      union nettle_block16 T1;
      /* S Holds the real P(n) (with part of stolen ciphertext) */
      union nettle_block16 S;

      /* we need the last T(n) and save the T(n-1) for later */
      xts_shift(&T1, &T);

      memxor3(C.b, src, T1.b, XTS_BLOCK_SIZE);	/* C -> CC */
      decf(dec_ctx, XTS_BLOCK_SIZE, S.b, C.b);  /* PP */
      memxor(S.b, T1.b, XTS_BLOCK_SIZE);	/* PP -> S */

      /* process next block (Pn-1) */
      length -= XTS_BLOCK_SIZE;
      src += XTS_BLOCK_SIZE;

      /* Prepare C, P holds the real P(n) */
      memxor3(C.b, src, T.b, length);	        /* C_1 |.. */
      memxor3(C.b + length, S.b + length, T.b + length,
              XTS_BLOCK_SIZE - length);         /* ..| S_2 -> CC */
      decf(dec_ctx, XTS_BLOCK_SIZE, dst, C.b);  /* PP */
      memxor(dst, T.b, XTS_BLOCK_SIZE);	        /* PP -> P(n-1) */

      /* Do this after we read src so inplace operations do not break */
      dst += XTS_BLOCK_SIZE;
      memcpy(dst, S.b, length);                 /* S_1 -> P(n) */
    }
}
