/*
 * Copyright (C) 2002-2014 Free Software Foundation, Inc.
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#include <int.h>
#include "gstr.h"

/* These function are like strcat, strcpy. They only
 * do bounds checking (they shouldn't cause buffer overruns),
 * and they always produce null terminated strings.
 *
 * They should be used only with null terminated strings.
 */
void
_asn1_str_cat (char *dest, size_t dest_tot_size, const char *src)
{
  size_t str_size = strlen (src);
  size_t dest_size = strlen (dest);

  if (dest_tot_size - dest_size > str_size)
    {
      strcat (dest, src);
    }
  else
    {
      if (dest_tot_size - dest_size > 0)
	{
	  strncat (dest, src, (dest_tot_size - dest_size) - 1);
	  dest[dest_tot_size - 1] = 0;
	}
    }
}

/* Returns the bytes copied (not including the null terminator) */
unsigned int
_asn1_str_cpy (char *dest, size_t dest_tot_size, const char *src)
{
  size_t str_size = strlen (src);

  if (dest_tot_size > str_size)
    {
      strcpy (dest, src);
      return str_size;
    }
  else
    {
      if (dest_tot_size > 0)
	{
	  str_size = dest_tot_size - 1;
	  memcpy (dest, src, str_size);
	  dest[str_size] = 0;
	  return str_size;
	}
      else
	return 0;
    }
}
