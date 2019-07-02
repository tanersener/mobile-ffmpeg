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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include <algorithms.h>
#include "errors.h"
#include <x509/common.h>
#include "c-strcase.h"

/* TLS Versions */
static const version_entry_st sup_versions[] = {
	{.name = "SSL3.0",
	 .id = GNUTLS_SSL3,
	 .age = 0,
	 .major = 3,
	 .minor = 0,
	 .transport = GNUTLS_STREAM,
#ifdef ENABLE_SSL3
	 .supported = 1,
#endif
	 .explicit_iv = 0,
	 .extensions = 0,
	 .selectable_sighash = 0,
	 .selectable_prf = 0,
	 .obsolete = 1,
	 .only_extension = 0,
	 .tls_sig_sem = SIG_SEM_PRE_TLS12,
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
	 .only_extension = 0,
	 .tls_sig_sem = SIG_SEM_PRE_TLS12,
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
	 .only_extension = 0,
	 .tls_sig_sem = SIG_SEM_PRE_TLS12,
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
	 .only_extension = 0,
	 .tls_sig_sem = SIG_SEM_PRE_TLS12,
	 .false_start = 1
	},
	{.name = "TLS1.3",
	 .id = GNUTLS_TLS1_3,
	 .age = 5,
	 .major = 3,
	 .minor = 4,
	 .transport = GNUTLS_STREAM,
	 .supported = 1,
	 .explicit_iv = 0,
	 .extensions = 1,
	 .selectable_sighash = 1,
	 .selectable_prf = 1,
	 .tls13_sem = 1,
	 .obsolete = 0,
	 .only_extension = 1,
	 .post_handshake_auth = 1,
	 .key_shares = 1,
	 .false_start = 0, /* doesn't make sense */
	 .tls_sig_sem = SIG_SEM_TLS13
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
	 .only_extension = 0,
	 .tls_sig_sem = SIG_SEM_PRE_TLS12,
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
	 .only_extension = 0,
	 .tls_sig_sem = SIG_SEM_PRE_TLS12,
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
	 .only_extension = 0,
	 .tls_sig_sem = SIG_SEM_PRE_TLS12,
	 .false_start = 1
	},
	{0, 0, 0, 0, 0}
};

const version_entry_st *version_to_entry(gnutls_protocol_t version)
{
	const version_entry_st *p;

	for (p = sup_versions; p->name != NULL; p++)
		if (p->id == version)
			return p;
	return NULL;
}

const version_entry_st *nversion_to_entry(uint8_t major, uint8_t minor)
{
	const version_entry_st *p;

	for (p = sup_versions; p->name != NULL; p++) {
		if ((p->major == major) && (p->minor == minor))
			    return p;
	}
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

	for (i = 0; i < session->internals.priorities->protocol.num_priorities;
	     i++) {
		if (session->internals.priorities->protocol.priorities[i] ==
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

	for (i=0;i < session->internals.priorities->protocol.num_priorities;i++) {
		cur_prot =
		    session->internals.priorities->protocol.priorities[i];
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
const version_entry_st *_gnutls_version_max(gnutls_session_t session)
{
	unsigned int i;
	gnutls_protocol_t cur_prot;
	const version_entry_st *p, *max = NULL;

	for (i = 0; i < session->internals.priorities->protocol.num_priorities;
	     i++) {
		cur_prot =
		    session->internals.priorities->protocol.priorities[i];

		for (p = sup_versions; p->name != NULL; p++) {
			if(p->id == cur_prot) {
#ifndef ENABLE_SSL3
				if (p->obsolete != 0)
					break;
#endif
				if (!p->supported || p->transport != session->internals.transport)
					break;

				if (p->tls13_sem && (session->internals.flags & INT_FLAG_NO_TLS13))
				    break;

				if (max == NULL || cur_prot > max->id) {
					max = p;
				}

				break;
			}
		}
	}

	return max;
}

const version_entry_st *_gnutls_legacy_version_max(gnutls_session_t session)
{
	const version_entry_st *max = _gnutls_version_max(session);

	if (max && max->only_extension != 0) {
		/* TLS 1.3 or later found */
		if (max->transport == GNUTLS_STREAM) {
			return version_to_entry(GNUTLS_TLS1_2);
		} else {
			return version_to_entry(GNUTLS_DTLS1_2);
		}
	}

	return max;
}

/* Returns the number of bytes written to buffer or a negative
 * error code. It will return GNUTLS_E_UNSUPPORTED_VERSION_PACKET
 * if there is no version >= TLS 1.3.
 */
int _gnutls_write_supported_versions(gnutls_session_t session, uint8_t *buffer, ssize_t buffer_size)
{
	gnutls_protocol_t cur_prot;
	size_t written_bytes = 0;
	unsigned at_least_one_new = 0;
	unsigned i;
	const version_entry_st *p;

	for (i = 0; i < session->internals.priorities->protocol.num_priorities; i++) {
		cur_prot =
		    session->internals.priorities->protocol.priorities[i];

		for (p = sup_versions; p->name != NULL; p++) {
			if(p->id == cur_prot) {
				if (p->obsolete != 0)
					break;

				if (!p->supported || p->transport != session->internals.transport)
					break;

				if (p->only_extension)
					at_least_one_new = 1;

				if (buffer_size > 2) {
					_gnutls_debug_log("Advertizing version %d.%d\n", (int)p->major, (int)p->minor);
					buffer[0] = p->major;
					buffer[1] = p->minor;
					written_bytes += 2;
					buffer += 2;
				}

				buffer_size -= 2;

				if (buffer_size <= 0)
					goto finish;

				break;
			}
		}
	}

 finish:
	if (written_bytes == 0)
		return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);

	if (at_least_one_new == 0)
		return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;

	return written_bytes;
}

/* Returns true (1) if the given version is higher than the highest supported
 * and (0) otherwise */
unsigned _gnutls_version_is_too_high(gnutls_session_t session, uint8_t major, uint8_t minor)
{
	const version_entry_st *e;

	e = _gnutls_legacy_version_max(session);
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
	const version_entry_st *p;
	/* avoid prefix */
	for (p = sup_versions; p->name != NULL; p++)
		if (p->id == version)
			return p->name;
	return NULL;
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
	const version_entry_st *p;
	gnutls_protocol_t ret = GNUTLS_VERSION_UNKNOWN;

	for (p = sup_versions; p->name != NULL; p++) {
		if (c_strcasecmp(p->name, name) == 0) {
			ret = p->id;
			break;
		}
	}

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
	const version_entry_st *p;
	static gnutls_protocol_t supported_protocols[MAX_ALGOS] = { 0 };

	if (supported_protocols[0] == 0) {
		int i = 0;

		for (p = sup_versions; p->name != NULL; p++)
			supported_protocols[i++] = p->id;
		supported_protocols[i++] = 0;
	}

	return supported_protocols;
}

/* Returns a version number given the major and minor numbers.
 */
gnutls_protocol_t _gnutls_version_get(uint8_t major, uint8_t minor)
{
	const version_entry_st *p;
	int ret = GNUTLS_VERSION_UNKNOWN;

	for (p = sup_versions; p->name != NULL; p++)
		if ((p->major == major) && (p->minor == minor))
			ret = p->id;
	return ret;
}

/* Version Functions */

int
_gnutls_nversion_is_supported(gnutls_session_t session,
			      unsigned char major, unsigned char minor)
{
	const version_entry_st *p;
	int version = 0;

	for (p = sup_versions; p->name != NULL; p++) {
		if(p->major == major && p->minor == minor) {
#ifndef ENABLE_SSL3
			if (p->obsolete != 0) return 0;
#endif
			if (p->tls13_sem && (session->internals.flags & INT_FLAG_NO_TLS13))
				return 0;

			if (!p->supported || p->transport != session->internals.transport)
				return 0;

			version = p->id;
			break;
		}
	}

	if (version == 0)
		return 0;

	if (_gnutls_version_priority(session, version) < 0)
		return 0;	/* disabled by the user */
	else
		return 1;
}
