/* Store a character in UTF-32 string.
   Copyright (C) 2002, 2005-2006, 2009-2020 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

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

#if defined IN_LIBUNISTRING
/* Tell unistr.h to declare u32_uctomb as 'extern', not 'static inline'.  */
# include "unistring-notinline.h"
#endif

/* Specification.  */
#include "unistr.h"

#if !HAVE_INLINE

int
u32_uctomb (uint32_t *s, ucs4_t uc, int n)
{
  if (uc < 0xd800 || (uc >= 0xe000 && uc < 0x110000))
    {
      if (n > 0)
        {
          *s = uc;
          return 1;
        }
      else
        return -2;
    }
  else
    return -1;
}

#endif
