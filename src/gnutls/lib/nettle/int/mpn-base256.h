/* gmp-glue.h

   Copyright (C) 2013 Niels Möller
   Copyright (C) 2013 Red Hat

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

#ifndef NETTLE_GMP_GLUE_H_INCLUDED
#define NETTLE_GMP_GLUE_H_INCLUDED

#include <nettle/bignum.h>

/* Like mpn_set_str, but always writes rn limbs. If input is larger,
   higher bits are ignored. */
void
mpn_set_base256 (mp_limb_t *rp, mp_size_t rn,
		 const uint8_t *xp, size_t xn);

void
mpn_get_base256 (uint8_t *rp, size_t rn,
	         const mp_limb_t *xp, mp_size_t xn);

#endif /* NETTLE_GMP_GLUE_H_INCLUDED */
