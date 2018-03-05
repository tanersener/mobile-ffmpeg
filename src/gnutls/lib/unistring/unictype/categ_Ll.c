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

/* Define u_categ_Ll table.  */
#include "categ_Ll.h"

const uc_general_category_t UC_CATEGORY_Ll =
  { UC_CATEGORY_MASK_Ll, 0, { &u_categ_Ll } };
