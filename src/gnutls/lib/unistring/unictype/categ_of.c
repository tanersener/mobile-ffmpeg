/* Categories of Unicode characters.
   Copyright (C) 2002, 2006-2007, 2009-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

   This program is free software: you can redistribute it and/or modify it
   under the terms of either:

    * the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   or

   * the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   or both in parallel, as here.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "unictype.h"

/* Define u_category table.  */
#include "categ_of.h"

static inline int
lookup_withtable (ucs4_t uc)
{
  unsigned int index1 = uc >> category_header_0;
  if (index1 < category_header_1)
    {
      int lookup1 = u_category.level1[index1];
      if (lookup1 >= 0)
        {
          unsigned int index2 = (uc >> category_header_2) & category_header_3;
          int lookup2 = u_category.level2[lookup1 + index2];
          if (lookup2 >= 0)
            {
              unsigned int index3 = ((uc & category_header_4) + lookup2) * 5;
              /* level3 contains 5-bit values, packed into 16-bit words.  */
              unsigned int lookup3 =
                ((u_category.level3[index3>>4]
                  | ((unsigned int) u_category.level3[(index3>>4)+1] << 16))
                 >> (index3 % 16))
                & 0x1f;

              return lookup3;
            }
        }
      return 29; /* = log2(UC_CATEGORY_MASK_Cn) */
    }
  return -1;
}

bool
uc_is_general_category_withtable (ucs4_t uc, uint32_t bitmask)
{
  int bit = lookup_withtable (uc);

  if (bit >= 0)
    return ((bitmask >> bit) & 1);
  else
    return false;
}

uc_general_category_t
uc_general_category (ucs4_t uc)
{
  int bit = lookup_withtable (uc);
  uc_general_category_t result;

  if (bit >= 0)
    {
      result.bitmask = 1 << bit;
      result.generic = 1;
      result.lookup.lookup_fn = &uc_is_general_category_withtable;
      return result;
    }
  else
    return _UC_CATEGORY_NONE;
}
