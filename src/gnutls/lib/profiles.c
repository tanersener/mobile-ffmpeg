/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include <algorithms.h>
#include "errors.h"
#include <x509/common.h>
#include "c-strcase.h"
#include "profiles.h"

typedef struct {
	const char *name;
	gnutls_certificate_verification_profiles_t profile;
	gnutls_sec_param_t sec_param;
} gnutls_profile_entry;

static const gnutls_profile_entry profiles[] = {
	{"Very weak", GNUTLS_PROFILE_VERY_WEAK, GNUTLS_SEC_PARAM_VERY_WEAK},
	{"Low", GNUTLS_PROFILE_LOW, GNUTLS_SEC_PARAM_LOW},
	{"Legacy", GNUTLS_PROFILE_LEGACY, GNUTLS_SEC_PARAM_LEGACY},
	{"Medium", GNUTLS_PROFILE_MEDIUM, GNUTLS_SEC_PARAM_MEDIUM},
	{"High", GNUTLS_PROFILE_HIGH, GNUTLS_SEC_PARAM_HIGH},
	{"Ultra", GNUTLS_PROFILE_ULTRA, GNUTLS_SEC_PARAM_ULTRA},
	{"Future", GNUTLS_PROFILE_FUTURE, GNUTLS_SEC_PARAM_FUTURE},
	{"SuiteB128", GNUTLS_PROFILE_SUITEB128, GNUTLS_SEC_PARAM_HIGH},
	{"SuiteB192", GNUTLS_PROFILE_SUITEB192, GNUTLS_SEC_PARAM_ULTRA},
	{NULL, 0, 0}
};

gnutls_sec_param_t _gnutls_profile_to_sec_level(gnutls_certificate_verification_profiles_t profile)
{
	const gnutls_profile_entry *p;

	for(p = profiles; p->name != NULL; p++) {
		if (profile == p->profile)
			return p->sec_param;
	}

	return GNUTLS_SEC_PARAM_UNKNOWN;
}

/**
 * gnutls_certificate_verification_profile_get_id:
 * @name: is a profile name
 *
 * Convert a string to a #gnutls_certificate_verification_profiles_t value.  The names are
 * compared in a case insensitive way.
 *
 * Returns: a #gnutls_certificate_verification_profiles_t id of the specified profile,
 *   or %GNUTLS_PROFILE_UNKNOWN on failure.
 **/
gnutls_certificate_verification_profiles_t gnutls_certificate_verification_profile_get_id(const char *name)
{
	const gnutls_profile_entry *p;

	if (name == NULL)
		return GNUTLS_PROFILE_UNKNOWN;

	for (p = profiles; p->name != NULL; p++) {
		if (c_strcasecmp(p->name, name) == 0)
			return p->profile;
	}

	return GNUTLS_PROFILE_UNKNOWN;
}

/**
 * gnutls_certificate_verification_profile_get_name:
 * @id: is a profile ID
 *
 * Convert a #gnutls_certificate_verification_profiles_t value to a string.
 *
 * Returns: a string that contains the name of the specified profile or %NULL.
 **/
const char *
gnutls_certificate_verification_profile_get_name(gnutls_certificate_verification_profiles_t id)
{
	const gnutls_profile_entry *p;

	for (p = profiles; p->name != NULL; p++) {
		if (p->profile == id)
			return p->name;
	}

	return NULL;
}
