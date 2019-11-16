/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>

#include "utils.h"
#include <assert.h>

/* This test verifies whether a system-wide configuration which disables SHA256
 * and SHA512 is seen from the library side.
 */

void doit(void)
{
	/* sanity */
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_RSA_PSS_SHA384) != 0);
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_RSA_MD5) == 0);

	/* check whether the values set by the calling script are the expected */
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_DSA_SHA256) == 0);
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_RSA_SHA256) == 0);
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_RSA_SHA512) == 0);
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_ECDSA_SHA256) == 0);
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_ECDSA_SHA512) == 0);
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_RSA_PSS_SHA256) == 0);
	assert(gnutls_sign_is_secure(GNUTLS_SIGN_RSA_PSS_SHA512) == 0);
}
