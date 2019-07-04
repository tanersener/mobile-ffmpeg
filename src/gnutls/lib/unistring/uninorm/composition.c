/* Canonical composition of Unicode characters.
   Copyright (C) 2002, 2006, 2009, 2011-2019 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

   This program is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "uninorm.h"

#include <string.h>

struct composition_rule { char codes[6]; unsigned int combined; };

#include "composition-table.h"

ucs4_t
uc_composition (ucs4_t uc1, ucs4_t uc2)
{
  if (uc1 < 0x12000 && uc2 < 0x12000)
    {
      if (uc2 >= 0x1161 && uc2 < 0x1161 + 21
          && uc1 >= 0x1100 && uc1 < 0x1100 + 19)
        {
          /* Hangul: Combine single letter L and single letter V to form
             two-letter syllable LV.  */
          return 0xAC00 + ((uc1 - 0x1100) * 21 + (uc2 - 0x1161)) * 28;
        }
      else if (uc2 > 0x11A7 && uc2 < 0x11A7 + 28
               && uc1 >= 0xAC00 && uc1 < 0xD7A4 && ((uc1 - 0xAC00) % 28) == 0)
        {
          /* Hangul: Combine two-letter syllable LV with single-letter T
             to form three-letter syllable LVT.  */
          return uc1 + (uc2 - 0x11A7);
        }
      else
        {
#if 0
          unsigned int uc = MUL1 * uc1 * MUL2 * uc2;
          unsigned int index1 = uc >> composition_header_0;
          if (index1 < composition_header_1)
            {
              int lookup1 = u_composition.level1[index1];
              if (lookup1 >= 0)
                {
                  unsigned int index2 = (uc >> composition_header_2) & composition_header_3;
                  int lookup2 = u_composition.level2[lookup1 + index2];
                  if (lookup2 >= 0)
                    {
                      unsigned int index3 = (uc & composition_header_4);
                      unsigned int lookup3 = u_composition.level3[lookup2 + index3];
                      if ((lookup3 >> 16) == uc2)
                        return lookup3 & ((1U << 16) - 1);
                    }
                }
            }
#else
          char codes[6];
          const struct composition_rule *rule;

          codes[0] = (uc1 >> 16) & 0xff;
          codes[1] = (uc1 >> 8) & 0xff;
          codes[2] = uc1 & 0xff;
          codes[3] = (uc2 >> 16) & 0xff;
          codes[4] = (uc2 >> 8) & 0xff;
          codes[5] = uc2 & 0xff;

          rule = gl_uninorm_compose_lookup (codes, 6);
          if (rule != NULL)
            return rule->combined;
#endif
        }
    }
  return 0;
}
