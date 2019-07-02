/* pbkdf2-hmac-streebog.c

   PKCS #5 PBKDF2 used with HMAC-STREEBOG.

   Copyright (C) 2016 Dmitry Eremin-Solenikov
   Copyright (C) 2012 Simon Josefsson

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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <gnutls_int.h>

#include "pbkdf2-gost.h"

#include "hmac-gost.h"

void
pbkdf2_hmac_streebog256 (size_t key_length, const uint8_t *key,
		    unsigned iterations,
		    size_t salt_length, const uint8_t *salt,
		    size_t length, uint8_t *dst)
{
  struct hmac_streebog256_ctx streebog256ctx;

  hmac_streebog256_set_key (&streebog256ctx, key_length, key);
  PBKDF2 (&streebog256ctx, hmac_streebog256_update, hmac_streebog256_digest,
	  STREEBOG256_DIGEST_SIZE, iterations, salt_length, salt, length, dst);
}

void
pbkdf2_hmac_streebog512 (size_t key_length, const uint8_t *key,
		    unsigned iterations,
		    size_t salt_length, const uint8_t *salt,
		    size_t length, uint8_t *dst)
{
  struct hmac_streebog512_ctx streebog512ctx;

  hmac_streebog512_set_key (&streebog512ctx, key_length, key);
  PBKDF2 (&streebog512ctx, hmac_streebog512_update, hmac_streebog512_digest,
	  STREEBOG512_DIGEST_SIZE, iterations, salt_length, salt, length, dst);
}
