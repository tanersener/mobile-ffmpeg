/* hash-pjw-bare.c -- compute a hash value from a provided buffer.

   Copyright (C) 2012-2020 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include "hash-pjw-bare.h"

#include <limits.h>

#define SIZE_BITS (sizeof (size_t) * CHAR_BIT)

/* Return a hash of the N bytes of X using the method described by
   Bruno Haible in https://www.haible.de/bruno/hashfunc.html.
   Note that while many hash functions reduce their result via modulo
   to a 0..table_size-1 range, this function does not do that.  */

size_t
hash_pjw_bare (const void *x, size_t n)
{
  const unsigned char *s = x;
  size_t h = 0;
  unsigned i;

  for (i = 0; i < n; i++)
    h = s[i] + ((h << 9) | (h >> (SIZE_BITS - 9)));

  return h;
}
