/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_STATE_H
#define GNUTLS_LIB_STATE_H

#include "gnutls_int.h"

void _gnutls_session_client_cert_type_set(gnutls_session_t session,
					 gnutls_certificate_type_t);
void _gnutls_session_server_cert_type_set(gnutls_session_t session,
					 gnutls_certificate_type_t);

inline static const gnutls_group_entry_st *
get_group(gnutls_session_t session)
{
	return session->security_parameters.grp;
}

int _gnutls_session_is_ecc(gnutls_session_t session);

inline static void
_gnutls_session_group_set(gnutls_session_t session,
			  const gnutls_group_entry_st *e)
{
	_gnutls_handshake_log("HSK[%p]: Selected group %s (%d)\n",
			      session, e->name, e->id);
	session->security_parameters.grp = e;
}

inline static
void set_default_version(gnutls_session_t session, const version_entry_st *ver)
{
	if (ver->tls13_sem) {
		session->internals.default_record_version[0] = 3;
		session->internals.default_record_version[1] = 1;
	} else {
		session->internals.default_record_version[0] = ver->major;
		session->internals.default_record_version[1] = ver->minor;
	}
}

void
_gnutls_record_set_default_version(gnutls_session_t session,
				   unsigned char major,
				   unsigned char minor);

void
_gnutls_hello_set_default_version(gnutls_session_t session,
				   unsigned char major,
				   unsigned char minor);

#include <auth.h>

#define CHECK_AUTH_TYPE(auth, ret) if (gnutls_auth_get_type(session) != auth) { \
	gnutls_assert(); \
	return ret; \
	}


int _gnutls_session_cert_type_supported(gnutls_session_t session,
				    gnutls_certificate_type_t cert_type,
				    bool check_credentials,
				    gnutls_ctype_target_t target);
int _gnutls_dh_set_secret_bits(gnutls_session_t session, unsigned bits);

int _gnutls_dh_set_peer_public(gnutls_session_t session, bigint_t public);
int _gnutls_dh_save_group(gnutls_session_t session, bigint_t gen,
			 bigint_t prime);

static inline int _gnutls_dh_get_min_prime_bits(gnutls_session_t session)
{
	if (session->internals.dh_prime_bits != 0)
		return session->internals.dh_prime_bits;
	else
		return gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH,
						   session->internals.
						   priorities->level);
}

void _gnutls_handshake_internal_state_clear(gnutls_session_t);

int _gnutls_session_is_resumable(gnutls_session_t session);

int _gnutls_session_is_psk(gnutls_session_t session);

int _gnutls_openpgp_send_fingerprint(gnutls_session_t session);

void reset_binders(gnutls_session_t session);

inline static int
_gnutls_PRF(gnutls_session_t session,
	    const uint8_t * secret, unsigned int secret_size,
	    const char *label, int label_size, const uint8_t * seed,
	    int seed_size, int total_bytes, void *ret)
{
	return _gnutls_prf_raw(session->security_parameters.prf->id,
			       secret_size, secret,
			       label_size, label,
			       seed_size, seed,
			       total_bytes, ret);
}

#define DEFAULT_CERT_TYPE GNUTLS_CRT_X509

#endif /* GNUTLS_LIB_STATE_H */
