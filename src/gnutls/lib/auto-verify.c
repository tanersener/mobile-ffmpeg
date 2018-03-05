/*
 * Copyright (C) 2015 Red Hat, Inc.
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
#include "errors.h"
#include <auth/cert.h>
#include <gnutls/gnutls.h>

/* This file implements the client certificate auto verification functionality.
 */

/* The actual verification callback. */
static int auto_verify_cb(gnutls_session_t session)
{
	unsigned int status;
	int ret;

	if (session->internals.vc_elements == 0) {
		ret = gnutls_certificate_verify_peers2(session, &status);
	} else {
		ret = gnutls_certificate_verify_peers(session, session->internals.vc_data,
						      session->internals.vc_elements, &status);
	}
	if (ret < 0) {
		return gnutls_assert_val(GNUTLS_E_CERTIFICATE_ERROR);
	}

	session->internals.vc_status = status;

	if (status != 0)	/* Certificate is not trusted */
		return gnutls_assert_val(GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR);

	/* notify gnutls to continue handshake normally */
	return 0;
}

/**
 * gnutls_session_set_verify_cert:
 * @session: is a gnutls session
 * @hostname: is the expected name of the peer; may be %NULL
 * @flags: flags for certificate verification -- #gnutls_certificate_verify_flags
 *
 * This function instructs GnuTLS to verify the peer's certificate
 * using the provided hostname. If the verification fails the handshake
 * will also fail with %GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR. In that
 * case the verification result can be obtained using gnutls_session_get_verify_cert_status().
 *
 * The @hostname pointer provided must remain valid for the lifetime
 * of the session. More precisely it should be available during any subsequent
 * handshakes. If no hostname is provided, no hostname verification
 * will be performed. For a more advanced verification function check
 * gnutls_session_set_verify_cert2().
 *
 * If @flags is provided which contain a profile, this function should be
 * called after any session priority setting functions.
 *
 * The gnutls_session_set_verify_cert() function is intended to be used by TLS
 * clients to verify the server's certificate.
 *
 * Since: 3.4.6
 **/
void gnutls_session_set_verify_cert(gnutls_session_t session,
				     const char *hostname, unsigned flags)
{
	if (hostname) {
		session->internals.vc_sdata.type = GNUTLS_DT_DNS_HOSTNAME;
		session->internals.vc_sdata.data = (void*)hostname;
		session->internals.vc_sdata.size = 0;
		session->internals.vc_elements = 1;
		session->internals.vc_data = &session->internals.vc_sdata;
	} else {
		session->internals.vc_elements = 0;
	}

	if (flags) {
		ADD_PROFILE_VFLAGS(session, flags);
	}

	gnutls_session_set_verify_function(session, auto_verify_cb);
}

/**
 * gnutls_session_set_verify_cert2:
 * @session: is a gnutls session
 * @data: an array of typed data
 * @elements: the number of data elements
 * @flags: flags for certificate verification -- #gnutls_certificate_verify_flags
 *
 * This function instructs GnuTLS to verify the peer's certificate
 * using the provided typed data information. If the verification fails the handshake
 * will also fail with %GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR. In that
 * case the verification result can be obtained using gnutls_session_get_verify_cert_status().
 *
 * The acceptable typed data are the same as in gnutls_certificate_verify_peers(),
 * and once set must remain valid for the lifetime of the session. More precisely
 * they should be available during any subsequent handshakes.
 *
 * If @flags is provided which contain a profile, this function should be
 * called after any session priority setting functions.
 *
 * Since: 3.4.6
 **/
void gnutls_session_set_verify_cert2(gnutls_session_t session,
				     gnutls_typed_vdata_st * data,
				     unsigned elements,
				     unsigned flags)
{
	session->internals.vc_data = data;
	session->internals.vc_elements = elements;

	if (flags)
		session->internals.additional_verify_flags |= flags;

	gnutls_session_set_verify_function(session, auto_verify_cb);
}

/**
 * gnutls_session_get_verify_cert_status:
 * @session: is a gnutls session
 *
 * This function returns the status of the verification when initiated
 * via auto-verification, i.e., by gnutls_session_set_verify_cert2() or
 * gnutls_session_set_verify_cert(). If no certificate verification
 * was occurred then the return value would be set to ((unsigned int)-1).
 *
 * The certificate verification status is the same as in gnutls_certificate_verify_peers().
 *
 * Returns: the certificate verification status.
 *
 * Since: 3.4.6
 **/
unsigned int gnutls_session_get_verify_cert_status(gnutls_session_t session)
{
	return session->internals.vc_status;
}
