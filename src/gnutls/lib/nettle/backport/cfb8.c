/* backport of cfb.c for CFB8

   Cipher feedback mode.

   Copyright (C) 2015, 2017 Dmitry Eremin-Solenikov
   Copyright (C) 2001, 2011 Niels Möller

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

#ifndef HAVE_NETTLE_CFB8_ENCRYPT

#include "cfb8.h"
#include <string.h>
#include <nettle/memxor.h>

/* CFB-8 uses slight optimization: it encrypts or decrypts up to block_size
 * bytes and does memcpy/memxor afterwards */
void
cfb8_encrypt(const void *ctx, nettle_cipher_func *f,
	     size_t block_size, uint8_t *iv,
	     size_t length, uint8_t *dst,
	     const uint8_t *src)
{
  TMP_DECL(buffer, uint8_t, NETTLE_MAX_CIPHER_BLOCK_SIZE * 2);
  TMP_DECL(outbuf, uint8_t, NETTLE_MAX_CIPHER_BLOCK_SIZE);
  TMP_ALLOC(buffer, block_size * 2);
  TMP_ALLOC(outbuf, block_size);
  uint8_t pos;

  memcpy(buffer, iv, block_size);
  pos = 0;
  while (length)
    {
      uint8_t t;

      if (pos == block_size)
	{
	  memcpy(buffer, buffer + block_size, block_size);
	  pos = 0;
	}

      f(ctx, block_size, outbuf, buffer + pos);
      t = *(dst++) = *(src++) ^ outbuf[0];
      buffer[pos + block_size] = t;
      length--;
      pos ++;
    }
  memcpy(iv, buffer + pos, block_size);
}

void
cfb8_decrypt(const void *ctx, nettle_cipher_func *f,
	     size_t block_size, uint8_t *iv,
	     size_t length, uint8_t *dst,
	     const uint8_t *src)
{
  TMP_DECL(buffer, uint8_t, NETTLE_MAX_CIPHER_BLOCK_SIZE * 2);
  TMP_DECL(outbuf, uint8_t, NETTLE_MAX_CIPHER_BLOCK_SIZE * 2);
  TMP_ALLOC(buffer, block_size * 2);
  TMP_ALLOC(outbuf, block_size * 2);
  uint8_t i = 0;

  memcpy(buffer, iv, block_size);
  memcpy(buffer + block_size, src,
	 length < block_size ? length : block_size);

  while (length)
    {

      for (i = 0; i < length && i < block_size; i++)
	f(ctx, block_size, outbuf + i, buffer + i);

      memxor3(dst, src, outbuf, i);

      length -= i;
      src += i;
      dst += i;

      if (i == block_size)
	{
	  memcpy(buffer, buffer + block_size, block_size);
	  memcpy(buffer + block_size, src,
		 length < block_size ? length : block_size);
	}
    }

  memcpy(iv, buffer + i, block_size);
}
#endif /* HAVE_NETTLE_CFB8_ENCRYPT */
