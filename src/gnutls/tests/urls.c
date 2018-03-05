/*
 * Copyright (C) 2016 Red Hat, Inc.
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <assert.h>

#include "utils.h"

void doit(void)
{
	global_init();

#ifdef ENABLE_PKCS11
	assert(gnutls_url_is_supported("pkcs11:xxx") > 0);
#endif

#ifdef HAVE_TROUSERS
	assert(gnutls_url_is_supported("tpmkey:xxx") > 0);
#endif

	assert(gnutls_url_is_supported("unknown-url:xxx") == 0);

	gnutls_global_deinit();
}
