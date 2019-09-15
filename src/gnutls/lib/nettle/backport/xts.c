/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Simo Sorce
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

/* #############################################
 * THIS IS A BACKPORT FROM NETTLE, DO NOT MODIFY
 * #############################################
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_NETTLE_XTS_ENCRYPT_MESSAGE
#include "xts.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <nettle/macros.h>
#include <nettle/memxor.h>

/* An aligned 16-byte block. */
union _backport_nettle_block16
{
  uint8_t b[16];
  unsigned long w[16 / sizeof(unsigned long)];
  uint64_t u64[2];
};

/* shift left one and XOR with 0x87 if there is carry. */
/* the algorithm reads this as a 128bit Little Endian number */
/* src and dest can point to the same buffer for in-place operations */
#if WORDS_BIGENDIAN
#define BE_SHIFT(x) ((((x) & 0x7f7f7f7f7f7f7f7f) << 1) | \
                     (((x) & 0x8080808080808080) >> 15))
static void
xts_shift(union _backport_nettle_block16 *dst,
          const union _backport_nettle_block16 *src)
{
  uint64_t carry = (src->u64[1] & 0x80) >> 7;
  dst->u64[1] = BE_SHIFT(src->u64[1]) | ((src->u64[0] & 0x80) << 49);
  dst->u64[0] = BE_SHIFT(src->u64[0]);
  dst->u64[0] ^= 0x8700000000000000 & -carry;
}
#else /* !WORDS_BIGENDIAN */
static void
xts_shift(union _backport_nettle_block16 *dst,
          const union _backport_nettle_block16 *src)
{
  uint64_t carry = src->u64[1] >> 63;
  dst->u64[1] = (src->u64[1] << 1) | (src->u64[0] >> 63);
  dst->u64[0] = src->u64[0] << 1;
  dst->u64[0] ^= 0x87 & -carry;
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
  union _backport_nettle_block16 T;
  union _backport_nettle_block16 P;

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
      union _backport_nettle_block16 S;

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
  union _backport_nettle_block16 T;
  union _backport_nettle_block16 C;

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
      union _backport_nettle_block16 T1;
      /* S Holds the real P(n) (with part of stolen ciphertext) */
      union _backport_nettle_block16 S;

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

void
xts_aes128_set_encrypt_key(struct xts_aes128_key *xts_key, const uint8_t *key)
{
    aes128_set_encrypt_key(&xts_key->cipher, key);
    aes128_set_encrypt_key(&xts_key->tweak_cipher, &key[AES128_KEY_SIZE]);
}

void
xts_aes128_set_decrypt_key(struct xts_aes128_key *xts_key, const uint8_t *key)
{
    aes128_set_decrypt_key(&xts_key->cipher, key);
    aes128_set_encrypt_key(&xts_key->tweak_cipher, &key[AES128_KEY_SIZE]);
}

void
xts_aes128_encrypt_message(struct xts_aes128_key *xts_key,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src)
{
    xts_encrypt_message(&xts_key->cipher, &xts_key->tweak_cipher,
                        (nettle_cipher_func *) aes128_encrypt,
                        tweak, length, dst, src);
}

void
xts_aes128_decrypt_message(struct xts_aes128_key *xts_key,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src)
{
    xts_decrypt_message(&xts_key->cipher, &xts_key->tweak_cipher,
                        (nettle_cipher_func *) aes128_decrypt,
                        (nettle_cipher_func *) aes128_encrypt,
                        tweak, length, dst, src);
}

void
xts_aes256_set_encrypt_key(struct xts_aes256_key *xts_key, const uint8_t *key)
{
    aes256_set_encrypt_key(&xts_key->cipher, key);
    aes256_set_encrypt_key(&xts_key->tweak_cipher, &key[AES256_KEY_SIZE]);
}

void
xts_aes256_set_decrypt_key(struct xts_aes256_key *xts_key, const uint8_t *key)
{
    aes256_set_decrypt_key(&xts_key->cipher, key);
    aes256_set_encrypt_key(&xts_key->tweak_cipher, &key[AES256_KEY_SIZE]);
}

void
xts_aes256_encrypt_message(struct xts_aes256_key *xts_key,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src)
{
    xts_encrypt_message(&xts_key->cipher, &xts_key->tweak_cipher,
                        (nettle_cipher_func *) aes256_encrypt,
                        tweak, length, dst, src);
}

void
xts_aes256_decrypt_message(struct xts_aes256_key *xts_key,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src)
{
    xts_decrypt_message(&xts_key->cipher, &xts_key->tweak_cipher,
                        (nettle_cipher_func *) aes256_decrypt,
                        (nettle_cipher_func *) aes256_encrypt,
                        tweak, length, dst, src);
}

#endif /* HAVE_NETTLE_XTS_ENCRYPT_MESSAGE */
