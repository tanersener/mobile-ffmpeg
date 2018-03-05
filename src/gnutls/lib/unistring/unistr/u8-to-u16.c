/* Convert UTF-8 string to UTF-16 string.
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
#include "unistr.h"

#define FUNC u8_to_u16
#define SRC_UNIT uint8_t
#define DST_UNIT uint16_t

#include <errno.h>
#include <stdlib.h>
#include <string.h>

DST_UNIT *
FUNC (const SRC_UNIT *s, size_t n, DST_UNIT *resultbuf, size_t *lengthp)
{
  const SRC_UNIT *s_end = s + n;
  /* Output string accumulator.  */
  DST_UNIT *result;
  size_t allocated;
  size_t length;

  if (resultbuf != NULL)
    {
      result = resultbuf;
      allocated = *lengthp;
    }
  else
    {
      result = NULL;
      allocated = 0;
    }
  length = 0;
  /* Invariants:
     result is either == resultbuf or == NULL or malloc-allocated.
     If length > 0, then result != NULL.  */

  while (s < s_end)
    {
      ucs4_t uc;
      int count;

      /* Fetch a Unicode character from the input string.  */
      count = u8_mbtoucr (&uc, s, s_end - s);
      if (count < 0)
        {
          if (!(result == resultbuf || result == NULL))
            free (result);
          errno = EILSEQ;
          return NULL;
        }
      s += count;

      /* Store it in the output string.  */
      count = u16_uctomb (result + length, uc, allocated - length);
      if (count == -1)
        {
          if (!(result == resultbuf || result == NULL))
            free (result);
          errno = EILSEQ;
          return NULL;
        }
      if (count == -2)
        {
          DST_UNIT *memory;

          allocated = (allocated > 0 ? 2 * allocated : 12);
          if (length + 2 > allocated)
            allocated = length + 2;
          if (result == resultbuf || result == NULL)
            memory = (DST_UNIT *) malloc (allocated * sizeof (DST_UNIT));
          else
            memory =
              (DST_UNIT *) realloc (result, allocated * sizeof (DST_UNIT));

          if (memory == NULL)
            {
              if (!(result == resultbuf || result == NULL))
                free (result);
              errno = ENOMEM;
              return NULL;
            }
          if (result == resultbuf && length > 0)
            memcpy ((char *) memory, (char *) result,
                    length * sizeof (DST_UNIT));
          result = memory;
          count = u16_uctomb (result + length, uc, allocated - length);
          if (count < 0)
            abort ();
        }
      length += count;
    }

  if (length == 0)
    {
      if (result == NULL)
        {
          /* Return a non-NULL value.  NULL means error.  */
          result = (DST_UNIT *) malloc (1);
          if (result == NULL)
            {
              errno = ENOMEM;
              return NULL;
            }
        }
    }
  else if (result != resultbuf && length < allocated)
    {
      /* Shrink the allocated memory if possible.  */
      DST_UNIT *memory;

      memory = (DST_UNIT *) realloc (result, length * sizeof (DST_UNIT));
      if (memory != NULL)
        result = memory;
    }

  *lengthp = length;
  return result;
}
