/* GnuTLS --- Guile bindings for GnuTLS.
   Copyright (C) 2007-2010, 2012 Free Software Foundation, Inc.

   GnuTLS is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   GnuTLS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GnuTLS; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA  */

#ifndef GUILE_GNUTLS_UTILS_H
#define GUILE_GNUTLS_UTILS_H

/* Common utilities.  */

#include <libguile.h>


/* Compiler twiddling.  */

#ifdef __GNUC__
#define EXPECT    __builtin_expect
#define NO_RETURN __attribute__ ((__noreturn__))
#else
#define EXPECT(_expr, _value) (_expr)
#define NO_RETURN
#endif

#define EXPECT_TRUE(_expr)  EXPECT ((_expr), 1)
#define EXPECT_FALSE(_expr) EXPECT ((_expr), 0)


/* Arrays as byte vectors.  */

extern const char scm_gnutls_array_error_message[];

/* Initialize C_HANDLE and C_LEN and return the contiguous C array
   corresponding to ARRAY.  */
static inline const char *
scm_gnutls_get_array (SCM array, scm_t_array_handle * c_handle,
                      size_t * c_len, const char *func_name)
{
  const char *c_array = NULL;
  const scm_t_array_dim *c_dims;

  scm_array_get_handle (array, c_handle);
  c_dims = scm_array_handle_dims (c_handle);
  if ((scm_array_handle_rank (c_handle) != 1) || (c_dims->inc != 1))
    {
      scm_array_handle_release (c_handle);
      scm_misc_error (func_name, scm_gnutls_array_error_message,
                      scm_list_1 (array));
    }
  else
    {
      size_t c_elem_size;

      c_elem_size = scm_array_handle_uniform_element_size (c_handle);
      *c_len = c_elem_size * (c_dims->ubnd - c_dims->lbnd + 1);

      c_array = (char *) scm_array_handle_uniform_elements (c_handle);
    }

  return (c_array);
}

/* Initialize C_HANDLE and C_LEN and return the contiguous C array
   corresponding to ARRAY.  The returned array can be written to.  */
static inline char *
scm_gnutls_get_writable_array (SCM array, scm_t_array_handle * c_handle,
                               size_t * c_len, const char *func_name)
{
  char *c_array = NULL;
  const scm_t_array_dim *c_dims;

  scm_array_get_handle (array, c_handle);
  c_dims = scm_array_handle_dims (c_handle);
  if ((scm_array_handle_rank (c_handle) != 1) || (c_dims->inc != 1))
    {
      scm_array_handle_release (c_handle);
      scm_misc_error (func_name, scm_gnutls_array_error_message,
                      scm_list_1 (array));
    }
  else
    {
      size_t c_elem_size;

      c_elem_size = scm_array_handle_uniform_element_size (c_handle);
      *c_len = c_elem_size * (c_dims->ubnd - c_dims->lbnd + 1);

      c_array =
        (char *) scm_array_handle_uniform_writable_elements (c_handle);
    }

  return (c_array);
}

#define scm_gnutls_release_array  scm_array_handle_release



/* Type conversion.  */

/* Return a list corresponding to the key usage values ORed in C_USAGE.  */
SCM_API SCM scm_from_gnutls_key_usage_flags (unsigned int c_usage);

#endif

/* arch-tag: a33400bc-b5e3-429e-80e0-6ff14cab79e7
 */
