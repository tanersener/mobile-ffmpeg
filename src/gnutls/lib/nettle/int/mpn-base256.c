/* gmp-glue.c

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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mpn-base256.h"

void
mpn_set_base256 (mp_limb_t *rp, mp_size_t rn,
		 const uint8_t *xp, size_t xn)
{
  size_t xi;
  mp_limb_t out;
  unsigned bits;
  for (xi = xn, out = bits = 0; xi > 0 && rn > 0; )
    {
      mp_limb_t in = xp[--xi];
      out |= (in << bits) & GMP_NUMB_MASK;
      bits += 8;
      if (bits >= GMP_NUMB_BITS)
	{
	  *rp++ = out;
	  rn--;

	  bits -= GMP_NUMB_BITS;
	  out = in >> (8 - bits);
	}
    }
  if (rn > 0)
    {
      *rp++ = out;
      if (--rn > 0)
	mpn_zero (rp, rn);
    }
}

void
mpn_get_base256 (uint8_t *rp, size_t rn,
		 const mp_limb_t *xp, mp_size_t xn)
{
  unsigned bits;
  mp_limb_t in;
  for (bits = in = 0; xn > 0 && rn > 0; )
    {
      if (bits >= 8)
	{
	  rp[--rn] = in;
	  in >>= 8;
	  bits -= 8;
	}
      else
	{
	  uint8_t old = in;
	  in = *xp++;
	  xn--;
	  rp[--rn] = old | (in << bits);
	  in >>= (8 - bits);
	  bits += GMP_NUMB_BITS - 8;
	}
    }
  while (rn > 0)
    {
      rp[--rn] = in;
      in >>= 8;
    }
}
