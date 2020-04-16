/* Categories of Unicode characters.
   Copyright (C) 2007, 2009-2020 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2007.

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
#include "unictype.h"

uc_general_category_t
uc_general_category_and_not (uc_general_category_t category1,
                             uc_general_category_t category2)
{
  uint32_t bitmask;
  uc_general_category_t result;

  bitmask = category1.bitmask & ~category2.bitmask;

  if (bitmask == category1.bitmask)
    return category1;

  if (bitmask == 0)
    return _UC_CATEGORY_NONE;

  result.bitmask = bitmask;
  result.generic = 1;
  result.lookup.lookup_fn = &uc_is_general_category_withtable;
  return result;
}
