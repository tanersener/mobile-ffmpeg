/* pbkdf2.h

   PKCS #5 password-based key derivation function PBKDF2, see RFC 2898.

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

#ifndef GNUTLS_LIB_NETTLE_GOST_PBKDF2_GOST_H
#define GNUTLS_LIB_NETTLE_GOST_PBKDF2_GOST_H

#include "config.h"

#include <nettle/nettle-meta.h>
#include <nettle/pbkdf2.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Namespace mangling */
#ifndef HAVE_NETTLE_GOSTHASH94CP_UPDATE
#define pbkdf2_hmac_gosthash94cp _gnutls_pbkdf2_hmac_gosthash94cp
#endif
#ifndef HAVE_NETTLE_STREEBOG512_UPDATE
#define pbkdf2_hmac_streebog256 _gnutls_pbkdf2_hmac_streebog256
#define pbkdf2_hmac_streebog512 _gnutls_pbkdf2_hmac_streebog512
#endif

#ifndef HAVE_NETTLE_GOSTHASH94CP_UPDATE
void
pbkdf2_hmac_gosthash94cp (size_t key_length, const uint8_t *key,
			  unsigned iterations,
			  size_t salt_length, const uint8_t *salt,
			  size_t length, uint8_t *dst);
#endif

#ifndef HAVE_NETTLE_STREEBOG512_UPDATE
void
pbkdf2_hmac_streebog256 (size_t key_length, const uint8_t *key,
			 unsigned iterations,
			 size_t salt_length, const uint8_t *salt,
			 size_t length, uint8_t *dst);

void
pbkdf2_hmac_streebog512 (size_t key_length, const uint8_t *key,
			 unsigned iterations,
			 size_t salt_length, const uint8_t *salt,
			 size_t length, uint8_t *dst);
#endif

#ifdef __cplusplus
}
#endif

#endif /* GNUTLS_LIB_NETTLE_GOST_PBKDF2_GOST_H */
