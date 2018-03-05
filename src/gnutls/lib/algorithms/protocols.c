/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include <algorithms.h>
#include "errors.h"
#include <x509/common.h>

/* TLS Versions */
static const version_entry_st sup_versions[] = {
	{.name = "SSL3.0", 
	 .id = GNUTLS_SSL3,
	 .age = 0,
	 .major = 3,
	 .minor = 0,
	 .transport = GNUTLS_STREAM,
	 .supported = 1,
	 .explicit_iv = 0,
	 .extensions = 0,
	 .selectable_sighash = 0,
	 .selectable_prf = 0,
	 .obsolete = 1,
	 .false_start = 0
	},
	{.name = "TLS1.0", 
	 .id = GNUTLS_TLS1,
	 .age = 1,
	 .major = 3,
	 .minor = 1,
	 .transport = GNUTLS_STREAM,
	 .supported = 1,
	 .explicit_iv = 0,
	 .extensions = 1,
	 .selectable_sighash = 0,
	 .selectable_prf = 0,
	 .obsolete = 0,
	 .false_start = 0
	},
	{.name = "TLS1.1", 
	 .id = GNUTLS_TLS1_1,
	 .age = 2,
	 .major = 3,
	 .minor = 2,
	 .transport = GNUTLS_STREAM,
	 .supported = 1,
	 .explicit_iv = 1,
	 .extensions = 1,
	 .selectable_sighash = 0,
	 .selectable_prf = 0,
	 .obsolete = 0,
	 .false_start = 0
	},
	{.name = "TLS1.2", 
	 .id = GNUTLS_TLS1_2,
	 .age = 3,
	 .major = 3,
	 .minor = 3,
	 .transport = GNUTLS_STREAM,
	 .supported = 1,
	 .explicit_iv = 1,
	 .extensions = 1,
	 .selectable_sighash = 1,
	 .selectable_prf = 1,
	 .obsolete = 0,
	 .false_start = 1
	},
	{.name = "DTLS0.9", /* Cisco AnyConnect (based on about OpenSSL 0.9.8e) */
	 .id = GNUTLS_DTLS0_9,
	 .age = 200,
	 .major = 1,
	 .minor = 0,
	 .transport = GNUTLS_DGRAM,
	 .supported = 1,
	 .explicit_iv = 1,
	 .extensions = 1,
	 .selectable_sighash = 0,
	 .selectable_prf = 0,
	 .obsolete = 0,
	 .false_start = 0
	},
	{.name = "DTLS1.0", 
	 .id = GNUTLS_DTLS1_0,
	 .age = 201,
	 .major = 254,
	 .minor = 255,
	 .transport = GNUTLS_DGRAM,
	 .supported = 1,
	 .explicit_iv = 1,
	 .extensions = 1,
	 .selectable_sighash = 0,
	 .selectable_prf = 0,
	 .obsolete = 0,
	 .false_start = 0
	},
	{.name = "DTLS1.2", 
	 .id = GNUTLS_DTLS1_2,
	 .age = 202,
	 .major = 254,
	 .minor = 253,
	 .transport = GNUTLS_DGRAM,
	 .supported = 1,
	 .explicit_iv = 1,
	 .extensions = 1,
	 .selectable_sighash = 1,
	 .selectable_prf = 1,
	 .obsolete = 0,
	 .false_start = 1
	},
	{0, 0, 0, 0, 0}
};

#define GNUTLS_VERSION_LOOP(b) \
	const version_entry_st *p; \
		for(p = sup_versions; p->name != NULL; p++) { b ; }

#define GNUTLS_VERSION_ALG_LOOP(a) \
	GNUTLS_VERSION_LOOP( if(p->id == version) { a; break; })

const version_entry_st *version_to_entry(gnutls_protocol_t version)
{
	GNUTLS_VERSION_ALG_LOOP(return p);
	return NULL;
}

static int
version_is_valid_for_session(gnutls_session_t session,
			     const version_entry_st *v)
{
	if (v->supported && v->transport == session->internals.transport) {
		return 1;
	}
	return 0;
}

/* Return the priority of the provided version number */
int
_gnutls_version_priority(gnutls_session_t session,
			 gnutls_protocol_t version)
{
	unsigned int i;

	for (i = 0; i < session->internals.priorities.protocol.algorithms;
	     i++) {
		if (session->internals.priorities.protocol.priority[i] ==
		    version)
			return i;
	}
	return -1;
}

/* Returns the lowest TLS version number in the priorities.
 */
const version_entry_st *_gnutls_version_lowest(gnutls_session_t session)
{
	unsigned int i;
	gnutls_protocol_t cur_prot;
	const version_entry_st *v, *min_v = NULL;
	const version_entry_st *backup = NULL;

	for (i=0;i < session->internals.priorities.protocol.algorithms;i++) {
		cur_prot =
		    session->internals.priorities.protocol.priority[i];
		v = version_to_entry(cur_prot);

		if (v != NULL && version_is_valid_for_session(session, v)) {
			if (min_v == NULL) {
				if (v->obsolete != 0)
					backup = v;
				else
					min_v = v;
			} else if (v->obsolete == 0 && v->age < min_v->age) {
				min_v = v;
			}
		}
	}

	if (min_v == NULL)
		return backup;

	return min_v;
}

/* Returns the maximum version in the priorities 
 */
static const version_entry_st *version_max(gnutls_session_t session)
{
	int max_proto = _gnutls_version_max(session);

	if (max_proto < 0)
		return NULL;

	return version_to_entry(max_proto);
}

gnutls_protocol_t _gnutls_version_max(gnutls_session_t session)
{
	unsigned int i, max = 0x00;
	gnutls_protocol_t cur_prot;

	for (i = 0; i < session->internals.priorities.protocol.algorithms;
	     i++) {
		cur_prot =
		    session->internals.priorities.protocol.priority[i];

		if (cur_prot > max
		    && _gnutls_version_is_supported(session, cur_prot))
			max = cur_prot;
	}

	if (max == 0x00)
		return GNUTLS_VERSION_UNKNOWN;	/* unknown version */

	return max;
}

/* Returns true (1) if the given version is higher than the highest supported
 * and (0) otherwise */
unsigned _gnutls_version_is_too_high(gnutls_session_t session, uint8_t major, uint8_t minor)
{
	const version_entry_st *e;

	e = version_max(session);
	if (e == NULL) /* we don't know; but that means something is unconfigured */
		return 1;

	if (e->transport == GNUTLS_DGRAM) {
		if (major < e->major)
			return 1;

		if (e->major == major && minor < e->minor)
			return 1;
	} else {
		if (major > e->major)
			return 1;

		if (e->major == major && minor > e->minor)
			return 1;
	}

	return 0;
}

/**
 * gnutls_protocol_get_name:
 * @version: is a (gnutls) version number
 *
 * Convert a #gnutls_protocol_t value to a string.
 *
 * Returns: a string that contains the name of the specified TLS
 *   version (e.g., "TLS1.0"), or %NULL.
 **/
const char *gnutls_protocol_get_name(gnutls_protocol_t version)
{
	const char *ret = NULL;

	/* avoid prefix */
	GNUTLS_VERSION_ALG_LOOP(ret = p->name);
	return ret;
}

/**
 * gnutls_protocol_get_id:
 * @name: is a protocol name
 *
 * The names are compared in a case insensitive way.
 *
 * Returns: an id of the specified protocol, or
 * %GNUTLS_VERSION_UNKNOWN on error.
 **/
gnutls_protocol_t gnutls_protocol_get_id(const char *name)
{
	gnutls_protocol_t ret = GNUTLS_VERSION_UNKNOWN;

	GNUTLS_VERSION_LOOP(
		if (strcasecmp(p->name, name) == 0) {
			ret = p->id;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_protocol_list:
 *
 * Get a list of supported protocols, e.g. SSL 3.0, TLS 1.0 etc.
 *
 * This function is not thread safe.
 *
 * Returns: a (0)-terminated list of #gnutls_protocol_t integers
 * indicating the available protocols.
 *
 **/
const gnutls_protocol_t *gnutls_protocol_list(void)
{
	static gnutls_protocol_t supported_protocols[MAX_ALGOS] = { 0 };

	if (supported_protocols[0] == 0) {
		int i = 0;

		GNUTLS_VERSION_LOOP(supported_protocols[i++] = p->id);
		supported_protocols[i++] = 0;
	}

	return supported_protocols;
}

/* Returns a version number given the major and minor numbers.
 */
gnutls_protocol_t _gnutls_version_get(uint8_t major, uint8_t minor)
{
	int ret = GNUTLS_VERSION_UNKNOWN;

	GNUTLS_VERSION_LOOP(
		if ((p->major == major) && (p->minor == minor))
			    ret = p->id
	);
	return ret;
}

/* Version Functions */

int
_gnutls_version_is_supported(gnutls_session_t session,
			     const gnutls_protocol_t version)
{
	int ret = 0;

	GNUTLS_VERSION_LOOP(
		if(p->id == version) {
#ifndef ENABLE_SSL3
			if (p->obsolete != 0) return 0;
#endif
			ret = p->supported && p->transport == session->internals.transport;
			break;
		}
	)

	if (ret == 0)
		return 0;

	if (_gnutls_version_priority(session, version) < 0)
		return 0;	/* disabled by the user */
	else
		return 1;
}

