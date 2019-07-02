/* Test whether __func__ is available
   Copyright (C) 2008-2019 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2008.  */

#include <config.h>

#include <string.h>

#include "macros.h"

int
main ()
{
  ASSERT (strlen (__func__) > 0);

  /* On SunPRO C 5.9, sizeof __func__ evaluates to 0.  The compiler warns:
     "warning: null dimension: sizeof()".  */
#if !defined __SUNPRO_C
  ASSERT (strlen (__func__) + 1 == sizeof __func__);
#endif

  ASSERT (strcmp (__func__, "main") == 0
          || strcmp (__func__, "<unknown function>") == 0);

  return 0;
}
