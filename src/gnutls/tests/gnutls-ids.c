/*
 * Copyright (C) 2017 Red Hat
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/dane.h>
#include <assert.h>

#include "utils.h"

void doit(void)
{
	assert(gnutls_certificate_verification_profile_get_id("very weak") == GNUTLS_PROFILE_VERY_WEAK);
	assert(gnutls_certificate_verification_profile_get_id("low") == GNUTLS_PROFILE_LOW);
	assert(gnutls_certificate_verification_profile_get_id("legacy") == GNUTLS_PROFILE_LEGACY);
	assert(gnutls_certificate_verification_profile_get_id("MedIum") == GNUTLS_PROFILE_MEDIUM);
	assert(gnutls_certificate_verification_profile_get_id("ultra") == GNUTLS_PROFILE_ULTRA);
	assert(gnutls_certificate_verification_profile_get_id("future") == GNUTLS_PROFILE_FUTURE);
	assert(gnutls_certificate_verification_profile_get_id("xxx") == GNUTLS_PROFILE_UNKNOWN);
}
