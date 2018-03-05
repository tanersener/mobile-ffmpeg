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

#include <stdlib.h>
#include <string.h>

/* Indices stored in the 'struct named_category' elements of the perfect hash
   table.  We don't use uc_general_category_t values or their addresses
   directly, because this would introduce load-time relocations.  */
enum
{
  UC_CATEGORY_INDEX_L,
  UC_CATEGORY_INDEX_LC,
  UC_CATEGORY_INDEX_Lu,
  UC_CATEGORY_INDEX_Ll,
  UC_CATEGORY_INDEX_Lt,
  UC_CATEGORY_INDEX_Lm,
  UC_CATEGORY_INDEX_Lo,
  UC_CATEGORY_INDEX_M,
  UC_CATEGORY_INDEX_Mn,
  UC_CATEGORY_INDEX_Mc,
  UC_CATEGORY_INDEX_Me,
  UC_CATEGORY_INDEX_N,
  UC_CATEGORY_INDEX_Nd,
  UC_CATEGORY_INDEX_Nl,
  UC_CATEGORY_INDEX_No,
  UC_CATEGORY_INDEX_P,
  UC_CATEGORY_INDEX_Pc,
  UC_CATEGORY_INDEX_Pd,
  UC_CATEGORY_INDEX_Ps,
  UC_CATEGORY_INDEX_Pe,
  UC_CATEGORY_INDEX_Pi,
  UC_CATEGORY_INDEX_Pf,
  UC_CATEGORY_INDEX_Po,
  UC_CATEGORY_INDEX_S,
  UC_CATEGORY_INDEX_Sm,
  UC_CATEGORY_INDEX_Sc,
  UC_CATEGORY_INDEX_Sk,
  UC_CATEGORY_INDEX_So,
  UC_CATEGORY_INDEX_Z,
  UC_CATEGORY_INDEX_Zs,
  UC_CATEGORY_INDEX_Zl,
  UC_CATEGORY_INDEX_Zp,
  UC_CATEGORY_INDEX_C,
  UC_CATEGORY_INDEX_Cc,
  UC_CATEGORY_INDEX_Cf,
  UC_CATEGORY_INDEX_Cs,
  UC_CATEGORY_INDEX_Co,
  UC_CATEGORY_INDEX_Cn
};

#include "unictype/categ_byname.h"

uc_general_category_t
uc_general_category_byname (const char *category_name)
{
  size_t len;

  len = strlen (category_name);
  if (len <= MAX_WORD_LENGTH)
    {
      char buf[MAX_WORD_LENGTH + 1];
      const struct named_category *found;

      /* Copy category_name into buf, converting '_' and '-' to ' '.  */
      {
        const char *p = category_name;
        char *q = buf;

        for (;; p++, q++)
          {
            char c = *p;

            if (c == '_' || c == '-')
              c = ' ';
            *q = c;
            if (c == '\0')
              break;
          }
      }
      /* Here q == buf + len.  */

      /* Do a hash table lookup, with case-insensitive comparison.  */
      found = uc_general_category_lookup (buf, len);
      if (found != NULL)
        /* Use a 'switch' statement here, because a table would introduce
           load-time relocations.  */
        switch (found->category_index)
          {
          case UC_CATEGORY_INDEX_L:
            return UC_CATEGORY_L;
          case UC_CATEGORY_INDEX_LC:
            return UC_CATEGORY_LC;
          case UC_CATEGORY_INDEX_Lu:
            return UC_CATEGORY_Lu;
          case UC_CATEGORY_INDEX_Ll:
            return UC_CATEGORY_Ll;
          case UC_CATEGORY_INDEX_Lt:
            return UC_CATEGORY_Lt;
          case UC_CATEGORY_INDEX_Lm:
            return UC_CATEGORY_Lm;
          case UC_CATEGORY_INDEX_Lo:
            return UC_CATEGORY_Lo;
          case UC_CATEGORY_INDEX_M:
            return UC_CATEGORY_M;
          case UC_CATEGORY_INDEX_Mn:
            return UC_CATEGORY_Mn;
          case UC_CATEGORY_INDEX_Mc:
            return UC_CATEGORY_Mc;
          case UC_CATEGORY_INDEX_Me:
            return UC_CATEGORY_Me;
          case UC_CATEGORY_INDEX_N:
            return UC_CATEGORY_N;
          case UC_CATEGORY_INDEX_Nd:
            return UC_CATEGORY_Nd;
          case UC_CATEGORY_INDEX_Nl:
            return UC_CATEGORY_Nl;
          case UC_CATEGORY_INDEX_No:
            return UC_CATEGORY_No;
          case UC_CATEGORY_INDEX_P:
            return UC_CATEGORY_P;
          case UC_CATEGORY_INDEX_Pc:
            return UC_CATEGORY_Pc;
          case UC_CATEGORY_INDEX_Pd:
            return UC_CATEGORY_Pd;
          case UC_CATEGORY_INDEX_Ps:
            return UC_CATEGORY_Ps;
          case UC_CATEGORY_INDEX_Pe:
            return UC_CATEGORY_Pe;
          case UC_CATEGORY_INDEX_Pi:
            return UC_CATEGORY_Pi;
          case UC_CATEGORY_INDEX_Pf:
            return UC_CATEGORY_Pf;
          case UC_CATEGORY_INDEX_Po:
            return UC_CATEGORY_Po;
          case UC_CATEGORY_INDEX_S:
            return UC_CATEGORY_S;
          case UC_CATEGORY_INDEX_Sm:
            return UC_CATEGORY_Sm;
          case UC_CATEGORY_INDEX_Sc:
            return UC_CATEGORY_Sc;
          case UC_CATEGORY_INDEX_Sk:
            return UC_CATEGORY_Sk;
          case UC_CATEGORY_INDEX_So:
            return UC_CATEGORY_So;
          case UC_CATEGORY_INDEX_Z:
            return UC_CATEGORY_Z;
          case UC_CATEGORY_INDEX_Zs:
            return UC_CATEGORY_Zs;
          case UC_CATEGORY_INDEX_Zl:
            return UC_CATEGORY_Zl;
          case UC_CATEGORY_INDEX_Zp:
            return UC_CATEGORY_Zp;
          case UC_CATEGORY_INDEX_C:
            return UC_CATEGORY_C;
          case UC_CATEGORY_INDEX_Cc:
            return UC_CATEGORY_Cc;
          case UC_CATEGORY_INDEX_Cf:
            return UC_CATEGORY_Cf;
          case UC_CATEGORY_INDEX_Cs:
            return UC_CATEGORY_Cs;
          case UC_CATEGORY_INDEX_Co:
            return UC_CATEGORY_Co;
          case UC_CATEGORY_INDEX_Cn:
            return UC_CATEGORY_Cn;
          default:
            abort ();
          }
    }
  /* Invalid category name.  */
  return _UC_CATEGORY_NONE;
}
