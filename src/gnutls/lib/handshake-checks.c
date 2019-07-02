/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

/* Functions that relate to the TLS handshake procedure.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "debug.h"
#include "handshake.h"
#include <auth/cert.h>
#include "constate.h"
#include <record.h>
#include <state.h>
#include <ext/safe_renegotiation.h>
#include <auth/anon.h>		/* for gnutls_anon_server_credentials_t */
#include <auth/psk.h>		/* for gnutls_psk_server_credentials_t */
#ifdef ENABLE_SRP
# include <auth/srp_kx.h>
#endif

int _gnutls_check_id_for_change(gnutls_session_t session)
{
	int cred_type;

	/* This checks in PSK and SRP ciphersuites that the username remained the
	 * same on a rehandshake. */
	if (session->internals.flags & GNUTLS_ALLOW_ID_CHANGE)
		return 0;

	cred_type = gnutls_auth_get_type(session);
	if (cred_type == GNUTLS_CRD_PSK || cred_type == GNUTLS_CRD_SRP) {
		const char *username = NULL;

		if (cred_type == GNUTLS_CRD_PSK) {
			psk_auth_info_t ai;

			ai = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (ai == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			username = ai->username;
#ifdef ENABLE_SRP
		} else {
			srp_server_auth_info_t ai = _gnutls_get_auth_info(session, GNUTLS_CRD_SRP);
			if (ai == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			username = ai->username;
#endif
		}

		if (username == NULL)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (session->internals.saved_username_set) {
			if (strcmp(session->internals.saved_username, username) != 0) {
				_gnutls_debug_log("Session's PSK username changed during rehandshake; aborting!\n");
				return gnutls_assert_val(GNUTLS_E_SESSION_USER_ID_CHANGED);
			}
		} else {
			size_t len = strlen(username);

			memcpy(session->internals.saved_username, username, len);
			session->internals.saved_username[len] = 0;
			session->internals.saved_username_set = 1;
		}
	}

	return 0;
}

int _gnutls_check_if_cert_hash_is_same(gnutls_session_t session, gnutls_certificate_credentials_t cred)
{
	cert_auth_info_t ai;
	char tmp[32];
	int ret;

	if (session->internals.flags & GNUTLS_ALLOW_ID_CHANGE)
		return 0;

	ai = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (ai == NULL || ai->ncerts == 0)
		return 0;

	ret = gnutls_hash_fast(GNUTLS_DIG_SHA256, 
			       ai->raw_certificate_list[0].data,
			       ai->raw_certificate_list[0].size,
			       tmp);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (session->internals.cert_hash_set) {
		if (memcmp(tmp, session->internals.cert_hash, 32) != 0) {
			_gnutls_debug_log("Session certificate changed during rehandshake; aborting!\n");
			return gnutls_assert_val(GNUTLS_E_SESSION_USER_ID_CHANGED);
		}
	} else {
		memcpy(session->internals.cert_hash, tmp, 32);
		session->internals.cert_hash_set = 1;
	}

	return 0;
}
