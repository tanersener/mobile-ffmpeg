/* Decomposition of Unicode characters.
   Copyright (C) 2001-2003, 2009-2019 Free Software Foundation, Inc.
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


#include "unitypes.h"

/* The decomposition table is made of two parts:
     - A table containing the actual arrays of decomposed equivalents.
       (This table is separate because the maximum length of a decomposition
       is 18, much larger than the average length 1.497 of a decomposition).
     - A 3-level table of indices into this array.  */

#include "decomposition-table1.h"

static inline unsigned short
decomp_index (ucs4_t uc)
{
  unsigned int index1 = uc >> decomp_header_0;
  if (index1 < decomp_header_1)
    {
      int lookup1 = gl_uninorm_decomp_index_table.level1[index1];
      if (lookup1 >= 0)
        {
          unsigned int index2 = (uc >> decomp_header_2) & decomp_header_3;
          int lookup2 = gl_uninorm_decomp_index_table.level2[lookup1 + index2];
          if (lookup2 >= 0)
            {
              unsigned int index3 = uc & decomp_header_4;
              return gl_uninorm_decomp_index_table.level3[lookup2 + index3];
            }
        }
    }
  return (unsigned short)(-1);
}
