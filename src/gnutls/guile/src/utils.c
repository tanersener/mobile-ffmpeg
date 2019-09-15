/* GnuTLS --- Guile bindings for GnuTLS.
   Copyright (C) 2007-2012, 2019 Free Software Foundation, Inc.

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

/* Written by Ludovic Courtès <ludo@chbouib.org>.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utils.h"

#include <gnutls/gnutls.h>
#include <libguile.h>

#include "enums.h"
#include "errors.h"

SCM
scm_from_gnutls_key_usage_flags (unsigned int c_usage)
{
  SCM usage = SCM_EOL;

#define MATCH_USAGE(_value)					\
  if (c_usage & (_value))					\
    {								\
      usage = scm_cons (scm_from_gnutls_key_usage (_value),	\
			usage);					\
      c_usage &= ~(_value);					\
    }

  /* when the key is to be used for signing: */
  MATCH_USAGE (GNUTLS_KEY_DIGITAL_SIGNATURE);
  MATCH_USAGE (GNUTLS_KEY_NON_REPUDIATION);
  /* when the key is to be used for encryption: */
  MATCH_USAGE (GNUTLS_KEY_KEY_ENCIPHERMENT);
  MATCH_USAGE (GNUTLS_KEY_DATA_ENCIPHERMENT);
  MATCH_USAGE (GNUTLS_KEY_KEY_AGREEMENT);
  MATCH_USAGE (GNUTLS_KEY_KEY_CERT_SIGN);
  MATCH_USAGE (GNUTLS_KEY_CRL_SIGN);
  MATCH_USAGE (GNUTLS_KEY_ENCIPHER_ONLY);
  MATCH_USAGE (GNUTLS_KEY_DECIPHER_ONLY);

  if (EXPECT_FALSE (c_usage != 0))
    /* XXX: We failed to interpret one of the usage flags.  */
    scm_gnutls_error (GNUTLS_E_UNIMPLEMENTED_FEATURE, __func__);

#undef MATCH_USAGE

  return usage;
}

/* arch-tag: a55fe230-ead7-495d-ab11-dfe18452ca2a
 */
