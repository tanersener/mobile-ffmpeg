/*
 * Copyright (C) 2018 ARPA2 project
 *
 * Author: Tom Vrancken (dev@tomvrancken.nl)
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
 * This file provides common functionality for certificate type
 * handling during TLS hello extensions.
 *
 */

#ifndef GNUTLS_LIB_EXT_CERT_TYPES_H
#define GNUTLS_LIB_EXT_CERT_TYPES_H

/* Maps IANA TLS Certificate Types identifiers to internal
 * certificate type representation.
 */
static inline gnutls_certificate_type_t IANA2cert_type(int num)
{
	switch (num) {
		case 0:
			return GNUTLS_CRT_X509;
		case 2:
			return GNUTLS_CRT_RAWPK;
		default:
			return GNUTLS_CRT_UNKNOWN;
	}
}

/* Maps internal certificate type representation to
 * IANA TLS Certificate Types identifiers.
 */
static inline int cert_type2IANA(gnutls_certificate_type_t cert_type)
{
	switch (cert_type) {
		case GNUTLS_CRT_X509:
			return 0;
		case GNUTLS_CRT_RAWPK:
			return 2;
		default:
			return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
	}
}

/* Checks whether the given cert type is enabled in the application
 */
static inline bool is_cert_type_enabled(gnutls_session_t session, gnutls_certificate_type_t cert_type)
{
	switch(cert_type) {
		case GNUTLS_CRT_X509:
			// Default cert type, always enabled
			return true;
		case GNUTLS_CRT_RAWPK:
			return session->internals.flags & GNUTLS_ENABLE_RAWPK;
		default:
			// When not explicitly supported here disable it
			return false;
	}
}

/* Checks whether alternative cert types (i.e. other than X.509)
 * are enabled in the application
 */
static inline bool are_alternative_cert_types_allowed(gnutls_session_t session)
{
	// OR-ed list of defined cert type init flags
	#define CERT_TYPES_FLAGS GNUTLS_ENABLE_RAWPK

	return session->internals.flags & CERT_TYPES_FLAGS;

	#undef CERT_TYPES_FLAGS
}

#endif /* GNUTLS_LIB_EXT_CERT_TYPES_H */
