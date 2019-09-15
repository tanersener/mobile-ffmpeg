/* hmac-gost.h

   HMAC message authentication code (RFC-2104).

   Copyright (C) 2001, 2002 Niels Möller

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

#ifndef GNUTLS_LIB_NETTLE_GOST_HMAC_GOST_H
#define GNUTLS_LIB_NETTLE_GOST_HMAC_GOST_H

#include "config.h"

#include <nettle/hmac.h>

#ifndef HAVE_NETTLE_GOSTHASH94CP_UPDATE
#include "gosthash94.h"
#endif
#ifndef HAVE_NETTLE_STREEBOG512_UPDATE
#include "streebog.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Namespace mangling */
#ifndef HAVE_NETTLE_GOSTHASH94CP_UPDATE
#define hmac_gosthash94cp_set_key _gnutls_hmac_gosthash94cp_set_key
#define hmac_gosthash94cp_update _gnutls_hmac_gosthash94cp_update
#define hmac_gosthash94cp_digest _gnutls_hmac_gosthash94cp_digest
#endif
#ifndef HAVE_NETTLE_STREEBOG512_UPDATE
#define hmac_streebog256_set_key _gnutls_hmac_streebog256_set_key
#define hmac_streebog256_digest _gnutls_hmac_streebog256_digest
#define hmac_streebog512_set_key _gnutls_hmac_streebog512_set_key
#define hmac_streebog512_update _gnutls_hmac_streebog512_update
#define hmac_streebog512_digest _gnutls_hmac_streebog512_digest
#endif

/* hmac-gosthash94 */
#ifndef HAVE_NETTLE_GOSTHASH94CP_UPDATE
struct hmac_gosthash94cp_ctx HMAC_CTX(struct gosthash94cp_ctx);

void
hmac_gosthash94cp_set_key(struct hmac_gosthash94cp_ctx *ctx,
			  size_t key_length, const uint8_t *key);

void
hmac_gosthash94cp_update(struct hmac_gosthash94cp_ctx *ctx,
			 size_t length, const uint8_t *data);

void
hmac_gosthash94cp_digest(struct hmac_gosthash94cp_ctx *ctx,
			 size_t length, uint8_t *digest);
#endif


/* hmac-streebog */
#ifndef HAVE_NETTLE_STREEBOG512_UPDATE
struct hmac_streebog512_ctx HMAC_CTX(struct streebog512_ctx);

void
hmac_streebog512_set_key(struct hmac_streebog512_ctx *ctx,
		    size_t key_length, const uint8_t *key);

void
hmac_streebog512_update(struct hmac_streebog512_ctx *ctx,
		   size_t length, const uint8_t *data);

void
hmac_streebog512_digest(struct hmac_streebog512_ctx *ctx,
		   size_t length, uint8_t *digest);

#define hmac_streebog256_ctx hmac_streebog512_ctx

void
hmac_streebog256_set_key(struct hmac_streebog256_ctx *ctx,
		    size_t key_length, const uint8_t *key);

#define hmac_streebog256_update hmac_streebog512_update

void
hmac_streebog256_digest(struct hmac_streebog256_ctx *ctx,
		   size_t length, uint8_t *digest);
#endif

#ifdef __cplusplus
}
#endif

#endif /* GNUTLS_LIB_NETTLE_GOST_HMAC_GOST_H */
