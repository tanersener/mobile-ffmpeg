/* Categories of Unicode characters.
   Copyright (C) 2002, 2006-2007, 2011-2016 Free Software Foundation, Inc.
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

static const char u_category_name[30][3] =
{
  "Lu", "Ll", "Lt", "Lm", "Lo", "Mn", "Mc", "Me", "Nd", "Nl",
  "No", "Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po", "Sm", "Sc",
  "Sk", "So", "Zs", "Zl", "Zp", "Cc", "Cf", "Cs", "Co", "Cn"
};

const char *
uc_general_category_name (uc_general_category_t category)
{
  uint32_t bitmask = category.bitmask;
  /* bitmask should consist of a single bit.  */
  if (bitmask != 0)
    {
      if ((bitmask & (bitmask - 1)) == 0)
        {
          int bit;
          /* Take log2 using a variant of Robert Harley's method.
             Found by Bruno Haible 1996.  */
          uint32_t n = bitmask;
          static const char ord2_tab[64] =
            {
              -1,  0,  1, 12,  2,  6, -1, 13,  3, -1,  7, -1, -1, -1, -1, 14,
              10,  4, -1, -1,  8, -1, -1, 25, -1, -1, -1, -1, -1, 21, 27, 15,
              31, 11,  5, -1, -1, -1, -1, -1,  9, -1, -1, 24, -1, -1, 20, 26,
              30, -1, -1, -1, -1, 23, -1, 19, 29, -1, 22, 18, 28, 17, 16, -1
            };
          n += n << 4;
          n += n << 6;
          n = (n << 16) - n;
          bit = ord2_tab[n >> 26];

          if (bit < sizeof (u_category_name) / sizeof (u_category_name[0]))
            return u_category_name[bit];
        }
      else
        {
          if (bitmask == UC_CATEGORY_MASK_L)
            return "L";
          if (bitmask == UC_CATEGORY_MASK_LC)
            return "LC";
          if (bitmask == UC_CATEGORY_MASK_M)
            return "M";
          if (bitmask == UC_CATEGORY_MASK_N)
            return "N";
          if (bitmask == UC_CATEGORY_MASK_P)
            return "P";
          if (bitmask == UC_CATEGORY_MASK_S)
            return "S";
          if (bitmask == UC_CATEGORY_MASK_Z)
            return "Z";
          if (bitmask == UC_CATEGORY_MASK_C)
            return "C";
        }
    }
  return NULL;
}
