/* gosthash94.h

   The GOST R 34.11-94 hash function, described in RFC 5831.

   Copyright (C) 2012 Nikos Mavrogiannopoulos, Niels Möller

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

/* Based on rhash gost.h. */

/* Copyright: 2009-2012 Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Ported to nettle by Nikos Mavrogiannopoulos.
 */

#ifndef GNUTLS_LIB_NETTLE_GOST_GOSTHASH94_H
#define GNUTLS_LIB_NETTLE_GOST_GOSTHASH94_H

#include "config.h"

#ifndef HAVE_NETTLE_GOSTHASH94CP_UPDATE

#include <nettle/nettle-types.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <nettle/gosthash94.h>
#include <nettle/nettle-meta.h>

#define gosthash94cp_update _gnutls_gosthash94cp_update
#define gosthash94cp_digest _gnutls_gosthash94cp_digest

#define GOSTHASH94CP_BLOCK_SIZE GOSTHASH94_BLOCK_SIZE
#define GOSTHASH94CP_DIGEST_SIZE GOSTHASH94_DIGEST_SIZE

#define gosthash94cp_ctx gosthash94_ctx

#define gosthash94cp_init gosthash94_init
void gosthash94cp_update(struct gosthash94_ctx *ctx,
			 size_t length, const uint8_t *msg);
void gosthash94cp_digest(struct gosthash94_ctx *ctx,
			 size_t length, uint8_t *result);

#define nettle_gosthash94cp _gnutls_gosthash94cp
extern const struct nettle_hash _gnutls_gosthash94cp;

#ifdef __cplusplus
}
#endif

#endif

#endif /* GNUTLS_LIB_NETTLE_GOST_GOSTHASH94_H */
