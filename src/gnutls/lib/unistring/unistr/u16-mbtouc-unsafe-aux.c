/* Conversion UTF-16 to UCS-4.
   Copyright (C) 2001-2002, 2006-2007, 2009-2019 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

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
#include "unistr.h"

#if defined IN_LIBUNISTRING || HAVE_INLINE

int
u16_mbtouc_unsafe_aux (ucs4_t *puc, const uint16_t *s, size_t n)
{
  uint16_t c = *s;

  if (c < 0xdc00)
    {
      if (n >= 2)
        {
          if (s[1] >= 0xdc00 && s[1] < 0xe000)
            {
              *puc = 0x10000 + ((c - 0xd800) << 10) + (s[1] - 0xdc00);
              return 2;
            }
          /* invalid multibyte character */
        }
      else
        {
          /* incomplete multibyte character */
        }
    }
  /* invalid multibyte character */
  *puc = 0xfffd;
  return 1;
}

#endif
