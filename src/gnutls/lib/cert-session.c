/*
 * Copyright (C) 2001-2015 Free Software Foundation, Inc.
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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

/* This file contains certificate authentication functions to be exported in the
 * API which did not fit elsewhere.
 */

#include "gnutls_int.h"
#include <auth/srp_kx.h>
#include <auth/anon.h>
#include <auth/cert.h>
#include <auth/psk.h>
#include "errors.h"
#include <auth.h>
#include <state.h>
#include <datum.h>
#include <algorithms.h>
#include <gnutls/ocsp.h>
#include "x509.h"
#include "hello_ext.h"
#include "x509/ocsp.h"

/**
 * gnutls_certificate_get_ours:
 * @session: is a gnutls session
 *
 * Gets the certificate as sent to the peer in the last handshake.
 * The certificate is in raw (DER) format.  No certificate
 * list is being returned. Only the first certificate.
 *
 * This function returns the certificate that was sent in the current
 * handshake. In subsequent resumed sessions this function will return
 * %NULL. That differs from gnutls_certificate_get_peers() which always
 * returns the peer's certificate used in the original session.
 *
 * Returns: a pointer to a #gnutls_datum_t containing our
 *   certificate, or %NULL in case of an error or if no certificate
 *   was used.
 **/
const gnutls_datum_t *gnutls_certificate_get_ours(gnutls_session_t session)
{
	gnutls_certificate_credentials_t cred;

	CHECK_AUTH_TYPE(GNUTLS_CRD_CERTIFICATE, NULL);

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return NULL;
	}

	if (session->internals.selected_cert_list == NULL)
		return NULL;

	return &session->internals.selected_cert_list[0].cert;
}

/**
 * gnutls_certificate_get_peers:
 * @session: is a gnutls session
 * @list_size: is the length of the certificate list (may be %NULL)
 *
 * Get the peer's raw certificate (chain) as sent by the peer.  These
 * certificates are in raw format (DER encoded for X.509).  In case of
 * a X.509 then a certificate list may be present.  The list
 * is provided as sent by the server; the server must send as first
 * certificate in the list its own certificate, following the
 * issuer's certificate, then the issuer's issuer etc. However, there
 * are servers which violate this principle and thus on certain
 * occasions this may be an unsorted list.
 *
 * In resumed sessions, this function will return the peer's certificate
 * list as used in the first/original session.
 *
 * Returns: a pointer to a #gnutls_datum_t containing the peer's
 *   certificates, or %NULL in case of an error or if no certificate
 *   was used.
 **/
const gnutls_datum_t *gnutls_certificate_get_peers(gnutls_session_t
						   session,
						   unsigned int *list_size)
{
	cert_auth_info_t info;

	CHECK_AUTH_TYPE(GNUTLS_CRD_CERTIFICATE, NULL);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL)
		return NULL;

	if (list_size)
		*list_size = info->ncerts;
	return info->raw_certificate_list;
}

/**
 * gnutls_certificate_client_get_request_status:
 * @session: is a gnutls session
 *
 * Get whether client certificate was requested on the last
 * handshake or not.
 *
 * Returns: 0 if the peer (server) did not request client
 *   authentication or 1 otherwise.
 **/
unsigned
gnutls_certificate_client_get_request_status(gnutls_session_t session)
{
	return (session->internals.hsk_flags & HSK_CRT_ASKED)?1:0;
}

/**
 * gnutls_certificate_set_params_function:
 * @res: is a gnutls_certificate_credentials_t type
 * @func: is the function to be called
 *
 * This function will set a callback in order for the server to get
 * the Diffie-Hellman or RSA parameters for certificate
 * authentication.  The callback should return %GNUTLS_E_SUCCESS (0) on success.
 *
 * Deprecated: This function is unnecessary and discouraged on GnuTLS 3.6.0
 * or later. Since 3.6.0, DH parameters are negotiated
 * following RFC7919.
 *
 **/
void
gnutls_certificate_set_params_function(gnutls_certificate_credentials_t
				       res, gnutls_params_function * func)
{
	res->params_func = func;
}

/**
 * gnutls_certificate_set_flags:
 * @res: is a gnutls_certificate_credentials_t type
 * @flags: are the flags of #gnutls_certificate_flags type
 *
 * This function will set flags to tweak the operation of
 * the credentials structure. See the #gnutls_certificate_flags enumerations
 * for more information on the available flags. 
 *
 * Since: 3.4.7
 **/
void
gnutls_certificate_set_flags(gnutls_certificate_credentials_t res,
			     unsigned int flags)
{
	res->flags = flags;
}

/**
 * gnutls_certificate_set_verify_flags:
 * @res: is a gnutls_certificate_credentials_t type
 * @flags: are the flags
 *
 * This function will set the flags to be used for verification 
 * of certificates and override any defaults.  The provided flags must be an OR of the
 * #gnutls_certificate_verify_flags enumerations. 
 *
 **/
void
gnutls_certificate_set_verify_flags(gnutls_certificate_credentials_t
				    res, unsigned int flags)
{
	res->verify_flags = flags;
}

/**
 * gnutls_certificate_get_verify_flags:
 * @res: is a gnutls_certificate_credentials_t type
 *
 * Returns the verification flags set with
 * gnutls_certificate_set_verify_flags().
 *
 * Returns: The certificate verification flags used by @res.
 *
 * Since: 3.4.0
 */
unsigned int
gnutls_certificate_get_verify_flags(gnutls_certificate_credentials_t res)
{
	return res->verify_flags;
}

/**
 * gnutls_certificate_set_verify_limits:
 * @res: is a gnutls_certificate_credentials type
 * @max_bits: is the number of bits of an acceptable certificate (default 8200)
 * @max_depth: is maximum depth of the verification of a certificate chain (default 5)
 *
 * This function will set some upper limits for the default
 * verification function, gnutls_certificate_verify_peers2(), to avoid
 * denial of service attacks.  You can set them to zero to disable
 * limits.
 **/
void
gnutls_certificate_set_verify_limits(gnutls_certificate_credentials_t res,
				     unsigned int max_bits,
				     unsigned int max_depth)
{
	res->verify_depth = max_depth;
	res->verify_bits = max_bits;
}

#ifdef ENABLE_OCSP
/* If the certificate is revoked status will be GNUTLS_CERT_REVOKED.
 * 
 * Returns:
 *  Zero on success, a negative error code otherwise.
 */
static int
check_ocsp_response(gnutls_session_t session, gnutls_x509_crt_t cert,
		    gnutls_x509_trust_list_t tl,
		    unsigned verify_flags,
		    gnutls_x509_crt_t *cand_issuers, unsigned cand_issuers_size,
		    gnutls_datum_t * data, unsigned int *ostatus)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	unsigned int status, cert_status;
	time_t rtime, vtime, ntime, now;
	int check_failed = 0;

	now = gnutls_time(0);

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_ocsp_resp_import(resp, data);
	if (ret < 0) {
		_gnutls_audit_log(session,
				  "There was an error parsing the OCSP response: %s.\n",
				  gnutls_strerror(ret));
		ret = gnutls_assert_val(0);
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	ret = gnutls_ocsp_resp_check_crt(resp, 0, cert);
	if (ret < 0) {
		ret = gnutls_assert_val(0);
		_gnutls_audit_log(session,
				  "Got OCSP response with an unrelated certificate.\n");
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	/* Attempt to verify against our trusted list */
	ret = gnutls_ocsp_resp_verify(resp, tl, &status, verify_flags);
	if ((ret < 0 || status != 0) && cand_issuers_size > 0) {
		/* Attempt to verify against the certificate list provided by the server */

		ret = gnutls_ocsp_resp_verify_direct(resp, cand_issuers[0], &status, verify_flags);
		/* if verification fails attempt to find whether any of the other
		 * bundled CAs is an issuer of the OCSP response */
		if ((ret < 0 || status != 0) && cand_issuers_size > 1) {
			int ret2;
			unsigned status2, i;

			for (i=1;i<cand_issuers_size;i++) {
				ret2 = gnutls_ocsp_resp_verify_direct(resp, cand_issuers[i], &status2, verify_flags);
				if (ret2 >= 0 && status2 == 0) {
					status = status2;
					ret = ret2;
					break;
				}
			}
		}
	}

	if (ret < 0) {
		ret = gnutls_assert_val(0);
		gnutls_assert();
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	/* do not consider revocation data if response was not verified */
	if (status != 0) {
		char buf[MAX_OCSP_MSG_SIZE];

		_gnutls_debug_log("OCSP rejection reason: %s\n",
				  _gnutls_ocsp_verify_status_to_str(status, buf));

		ret = gnutls_assert_val(0);
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	ret = gnutls_ocsp_resp_get_single(resp, 0, NULL, NULL, NULL, NULL,
					  &cert_status, &vtime, &ntime,
					  &rtime, NULL);
	if (ret < 0) {
		_gnutls_audit_log(session,
				  "There was an error parsing the OCSP response: %s.\n",
				  gnutls_strerror(ret));
		ret = gnutls_assert_val(0);
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	if (cert_status == GNUTLS_OCSP_CERT_REVOKED) {
		_gnutls_audit_log(session,
				  "The certificate was revoked via OCSP\n");
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_REVOKED;
		ret = gnutls_assert_val(0);
		goto cleanup;
	}

	/* Report but do not fail on the following errors. That is
	 * because including the OCSP response in the handshake shouldn't 
	 * cause more problems that not including it.
	 */
	if (ntime == -1) {
		if (now - vtime > MAX_OCSP_VALIDITY_SECS) {
			_gnutls_audit_log(session,
					  "The OCSP response is old\n");
			check_failed = 1;
			*ostatus |= GNUTLS_CERT_REVOCATION_DATA_SUPERSEDED;
			goto cleanup;
		}
	} else {
		/* there is a newer OCSP answer, don't trust this one */
		if (ntime < now) {
			_gnutls_audit_log(session,
					  "There is a newer OCSP response but was not provided by the server\n");
			check_failed = 1;
			*ostatus |= GNUTLS_CERT_REVOCATION_DATA_SUPERSEDED;
			goto cleanup;
		}
	}

	ret = 0;
      cleanup:
	if (check_failed == 0)
		session->internals.ocsp_check_ok = 1;

	gnutls_ocsp_resp_deinit(resp);

	return ret;
}

static int
_gnutls_ocsp_verify_mandatory_stapling(gnutls_session_t session,
				       gnutls_x509_crt_t cert,
				       unsigned int * ocsp_status)
{
	gnutls_x509_tlsfeatures_t tlsfeatures;
	int i, ret;
	unsigned feature;

	/* RFC 7633: If cert has TLS feature GNUTLS_EXTENSION_STATUS_REQUEST, stapling is mandatory.
	 *
	 * At this point, we know that we did not get the certificate status.
	 *
	 * To proceed, first check whether we have requested the certificate status
	 */
	if (!_gnutls_hello_ext_is_present(session, GNUTLS_EXTENSION_STATUS_REQUEST)) {
		return 0;
	}

	ret = gnutls_x509_tlsfeatures_init(&tlsfeatures);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* We have requested the status, now check whether the certificate mandates a response */
	if (gnutls_x509_crt_get_tlsfeatures(cert, tlsfeatures, 0, NULL) == 0) {
		for (i = 0;; ++i) {
			ret = gnutls_x509_tlsfeatures_get(tlsfeatures, i, &feature);
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				break;
			}

			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			if (feature == 5 /* TLS ID for status request */) {
				/* We sent a status request, the certificate mandates a reply, but we did not get any. */
				*ocsp_status |= GNUTLS_CERT_MISSING_OCSP_STATUS;
				break;
			}
		}
	}

	ret = 0;
 cleanup:
	gnutls_x509_tlsfeatures_deinit(tlsfeatures);
	return ret;
}
#endif

#define CLEAR_CERTS for(x=0;x<peer_certificate_list_size;x++) { \
	if (peer_certificate_list[x]) \
		gnutls_x509_crt_deinit(peer_certificate_list[x]); \
	} \
	gnutls_free( peer_certificate_list)

/*-
 * _gnutls_x509_cert_verify_peers - return the peer's certificate status
 * @session: is a gnutls session
 *
 * This function will try to verify the peer's certificate and return its status (TRUSTED, REVOKED etc.).
 * The return value (status) should be one of the gnutls_certificate_status_t enumerated elements.
 * However you must also check the peer's name in order to check if the verified certificate belongs to the
 * actual peer. Returns a negative error code in case of an error, or GNUTLS_E_NO_CERTIFICATE_FOUND if no certificate was sent.
 -*/
int
_gnutls_x509_cert_verify_peers(gnutls_session_t session,
			       gnutls_typed_vdata_st * data,
			       unsigned int elements,
			       unsigned int *status)
{
	cert_auth_info_t info;
	gnutls_certificate_credentials_t cred;
	gnutls_x509_crt_t *peer_certificate_list;
	gnutls_datum_t resp;
	int peer_certificate_list_size, i, x, ret;
	gnutls_x509_crt_t *cand_issuers;
	unsigned cand_issuers_size;
	unsigned int ocsp_status = 0;
	unsigned int verify_flags;

	/* No OCSP check so far */
	session->internals.ocsp_check_ok = 0;

	CHECK_AUTH_TYPE(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0)
		return GNUTLS_E_NO_CERTIFICATE_FOUND;

	if (info->ncerts > cred->verify_depth && cred->verify_depth > 0) {
		gnutls_assert();
		return GNUTLS_E_CONSTRAINT_ERROR;
	}

	verify_flags =
	    cred->verify_flags | session->internals.additional_verify_flags;
	/* generate a list of gnutls_certs based on the auth info
	 * raw certs.
	 */
	peer_certificate_list_size = info->ncerts;
	peer_certificate_list =
	    gnutls_calloc(peer_certificate_list_size,
			  sizeof(gnutls_x509_crt_t));
	if (peer_certificate_list == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	for (i = 0; i < peer_certificate_list_size; i++) {
		ret = gnutls_x509_crt_init(&peer_certificate_list[i]);
		if (ret < 0) {
			gnutls_assert();
			CLEAR_CERTS;
			return ret;
		}

		ret =
		    gnutls_x509_crt_import(peer_certificate_list[i],
					   &info->raw_certificate_list[i],
					   GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			gnutls_assert();
			CLEAR_CERTS;
			return ret;
		}
	}

	/* Use the OCSP extension if any */
#ifdef ENABLE_OCSP
	if (verify_flags & GNUTLS_VERIFY_DISABLE_CRL_CHECKS)
		goto skip_ocsp;

	for (i=0;i<peer_certificate_list_size;i++) {
		ret = gnutls_ocsp_status_request_get2(session, i, &resp);
		if (ret < 0) {
			ret = _gnutls_ocsp_verify_mandatory_stapling(session, peer_certificate_list[i], &ocsp_status);
			if (ret < 0) {
				gnutls_assert();
				CLEAR_CERTS;
				return ret;
			}

			continue;
		}

		cand_issuers = NULL;
		cand_issuers_size = 0;
		if (peer_certificate_list_size > i+1) {
			cand_issuers = &peer_certificate_list[i+1];
			cand_issuers_size = peer_certificate_list_size-i-1;
		}

		ret =
			check_ocsp_response(session,
					    peer_certificate_list[i],
					    cred->tlist, 
					    verify_flags, cand_issuers,
					    cand_issuers_size,
					    &resp, &ocsp_status);

		if (ret < 0) {
			CLEAR_CERTS;
			return gnutls_assert_val(ret);
		}
	}
#endif

      skip_ocsp:
	/* Verify certificate 
	 */
	ret =
	    gnutls_x509_trust_list_verify_crt2(cred->tlist,
					       peer_certificate_list,
					       peer_certificate_list_size,
					       data, elements,
					       verify_flags, status, NULL);

	if (ret < 0) {
		gnutls_assert();
		CLEAR_CERTS;
		return ret;
	}

	CLEAR_CERTS;

	*status |= ocsp_status;

	return 0;
}

/**
 * gnutls_certificate_verify_peers2:
 * @session: is a gnutls session
 * @status: is the output of the verification
 *
 * This function will verify the peer's certificate and store
 * the status in the @status variable as a bitwise OR of gnutls_certificate_status_t
 * values or zero if the certificate is trusted. Note that value in @status
 * is set only when the return value of this function is success (i.e, failure 
 * to trust a certificate does not imply a negative return value).
 * The default verification flags used by this function can be overridden
 * using gnutls_certificate_set_verify_flags().
 *
 * This function will take into account the stapled OCSP responses sent by the server,
 * as well as the following X.509 certificate extensions: Name Constraints,
 * Key Usage, and Basic Constraints (pathlen).
 * 
 * Note that you must also check the peer's name in order to check if
 * the verified certificate belongs to the actual peer, see gnutls_x509_crt_check_hostname(),
 * or use gnutls_certificate_verify_peers3().
 *
 * To avoid denial of service attacks some
 * default upper limits regarding the certificate key size and chain
 * size are set. To override them use gnutls_certificate_set_verify_limits().
 *
 * Note that when using raw public-keys verification will not work because there is
 * no corresponding certificate body belonging to the raw key that can be verified. In that
 * case this function will return %GNUTLS_E_INVALID_REQUEST.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) when the validation is performed, or a negative error code otherwise.
 * A successful error code means that the @status parameter must be checked to obtain the validation status.
 **/
int
gnutls_certificate_verify_peers2(gnutls_session_t session,
				 unsigned int *status)
{
	return gnutls_certificate_verify_peers(session, NULL, 0, status);
}

/**
 * gnutls_certificate_verify_peers3:
 * @session: is a gnutls session
 * @hostname: is the expected name of the peer; may be %NULL
 * @status: is the output of the verification
 *
 * This function will verify the peer's certificate and store the
 * the status in the @status variable as a bitwise OR of gnutls_certificate_status_t
 * values or zero if the certificate is trusted. Note that value in @status
 * is set only when the return value of this function is success (i.e, failure 
 * to trust a certificate does not imply a negative return value).
 * The default verification flags used by this function can be overridden
 * using gnutls_certificate_set_verify_flags(). See the documentation
 * of gnutls_certificate_verify_peers2() for details in the verification process.
 *
 * This function will take into account the stapled OCSP responses sent by the server,
 * as well as the following X.509 certificate extensions: Name Constraints,
 * Key Usage, and Basic Constraints (pathlen).
 * 
 * If the @hostname provided is non-NULL then this function will compare
 * the hostname in the certificate against it. The comparison will follow
 * the RFC6125 recommendations. If names do not match the
 * %GNUTLS_CERT_UNEXPECTED_OWNER status flag will be set.
 *
 * In order to verify the purpose of the end-certificate (by checking the extended
 * key usage), use gnutls_certificate_verify_peers().
 *
 * To avoid denial of service attacks some
 * default upper limits regarding the certificate key size and chain
 * size are set. To override them use gnutls_certificate_set_verify_limits().
 *
 * Note that when using raw public-keys verification will not work because there is
 * no corresponding certificate body belonging to the raw key that can be verified. In that
 * case this function will return %GNUTLS_E_INVALID_REQUEST.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) when the validation is performed, or a negative error code otherwise.
 * A successful error code means that the @status parameter must be checked to obtain the validation status.
 *
 * Since: 3.1.4
 **/
int
gnutls_certificate_verify_peers3(gnutls_session_t session,
				 const char *hostname,
				 unsigned int *status)
{
gnutls_typed_vdata_st data;

	data.type = GNUTLS_DT_DNS_HOSTNAME;
	data.size = 0;
	data.data = (void*)hostname;

	return gnutls_certificate_verify_peers(session, &data, 1, status);
}

/**
 * gnutls_certificate_verify_peers:
 * @session: is a gnutls session
 * @data: an array of typed data
 * @elements: the number of data elements
 * @status: is the output of the verification
 *
 * This function will verify the peer's certificate and store the
 * the status in the @status variable as a bitwise OR of gnutls_certificate_status_t
 * values or zero if the certificate is trusted. Note that value in @status
 * is set only when the return value of this function is success (i.e, failure 
 * to trust a certificate does not imply a negative return value).
 * The default verification flags used by this function can be overridden
 * using gnutls_certificate_set_verify_flags(). See the documentation
 * of gnutls_certificate_verify_peers2() for details in the verification process.
 *
 * This function will take into account the stapled OCSP responses sent by the server,
 * as well as the following X.509 certificate extensions: Name Constraints,
 * Key Usage, and Basic Constraints (pathlen).
 * 
 * The acceptable @data types are %GNUTLS_DT_DNS_HOSTNAME, %GNUTLS_DT_RFC822NAME and %GNUTLS_DT_KEY_PURPOSE_OID.
 * The former two accept as data a null-terminated hostname or email address, and the latter a null-terminated
 * object identifier (e.g., %GNUTLS_KP_TLS_WWW_SERVER).
 *
 * If a DNS hostname is provided then this function will compare
 * the hostname in the certificate against the given. If names do not match the 
 * %GNUTLS_CERT_UNEXPECTED_OWNER status flag will be set.
 * If a key purpose OID is provided and the end-certificate contains the extended key
 * usage PKIX extension, it will be required to be have the provided key purpose 
 * or be marked for any purpose, otherwise verification status will have the
 * %GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE flag set.
 *
 * To avoid denial of service attacks some
 * default upper limits regarding the certificate key size and chain
 * size are set. To override them use gnutls_certificate_set_verify_limits().
 *
 * Note that when using raw public-keys verification will not work because there is
 * no corresponding certificate body belonging to the raw key that can be verified. In that
 * case this function will return %GNUTLS_E_INVALID_REQUEST.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) when the validation is performed, or a negative error code otherwise.
 * A successful error code means that the @status parameter must be checked to obtain the validation status.
 *
 * Since: 3.3.0
 **/
int
gnutls_certificate_verify_peers(gnutls_session_t session,
				gnutls_typed_vdata_st * data,
				unsigned int elements,
				unsigned int *status)
{
	cert_auth_info_t info;

	CHECK_AUTH_TYPE(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0)
		return GNUTLS_E_NO_CERTIFICATE_FOUND;


	switch (get_certificate_type(session, GNUTLS_CTYPE_PEERS)) {
		case GNUTLS_CRT_X509:
			return _gnutls_x509_cert_verify_peers(session, data, elements,
										status);
		default:
			return GNUTLS_E_INVALID_REQUEST;
	}
}

/*-
 * _gnutls_x509_extract_certificate_activation_time - return the peer's certificate activation time
 * @cert: should contain an X.509 DER encoded certificate
 *
 * This function will return the certificate's activation time in UNIX time
 * (ie seconds since 00:00:00 UTC January 1, 1970).
 *
 * Returns a (time_t) -1 in case of an error.
 *
 -*/
static time_t
_gnutls_x509_get_raw_crt_activation_time(const gnutls_datum_t * cert)
{
	gnutls_x509_crt_t xcert;
	time_t result;

	result = gnutls_x509_crt_init(&xcert);
	if (result < 0)
		return (time_t) - 1;

	result = gnutls_x509_crt_import(xcert, cert, GNUTLS_X509_FMT_DER);
	if (result < 0) {
		gnutls_x509_crt_deinit(xcert);
		return (time_t) - 1;
	}

	result = gnutls_x509_crt_get_activation_time(xcert);

	gnutls_x509_crt_deinit(xcert);

	return result;
}

/*-
 * gnutls_x509_extract_certificate_expiration_time:
 * @cert: should contain an X.509 DER encoded certificate
 *
 * This function will return the certificate's expiration time in UNIX
 * time (ie seconds since 00:00:00 UTC January 1, 1970).  Returns a
 *
 * (time_t) -1 in case of an error.
 *
 -*/
static time_t
_gnutls_x509_get_raw_crt_expiration_time(const gnutls_datum_t * cert)
{
	gnutls_x509_crt_t xcert;
	time_t result;

	result = gnutls_x509_crt_init(&xcert);
	if (result < 0)
		return (time_t) - 1;

	result = gnutls_x509_crt_import(xcert, cert, GNUTLS_X509_FMT_DER);
	if (result < 0) {
		gnutls_x509_crt_deinit(xcert);
		return (time_t) - 1;
	}

	result = gnutls_x509_crt_get_expiration_time(xcert);

	gnutls_x509_crt_deinit(xcert);

	return result;
}

/**
 * gnutls_certificate_expiration_time_peers:
 * @session: is a gnutls session
 *
 * This function will return the peer's certificate expiration time.
 *
 * Returns: (time_t)-1 on error.
 *
 * Deprecated: gnutls_certificate_verify_peers2() now verifies expiration times.
 **/
time_t gnutls_certificate_expiration_time_peers(gnutls_session_t session)
{
	cert_auth_info_t info;

	CHECK_AUTH_TYPE(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		return (time_t) - 1;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0) {
		gnutls_assert();
		return (time_t) - 1;
	}

	switch (get_certificate_type(session, GNUTLS_CTYPE_PEERS)) {
		case GNUTLS_CRT_X509:
			return
					_gnutls_x509_get_raw_crt_expiration_time(&info->
										 raw_certificate_list[0]);
		default:
			return (time_t) - 1;
	}
}

/**
 * gnutls_certificate_activation_time_peers:
 * @session: is a gnutls session
 *
 * This function will return the peer's certificate activation time.
 *
 * Returns: (time_t)-1 on error.
 *
 * Deprecated: gnutls_certificate_verify_peers2() now verifies activation times.
 **/
time_t gnutls_certificate_activation_time_peers(gnutls_session_t session)
{
	cert_auth_info_t info;

	CHECK_AUTH_TYPE(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		return (time_t) - 1;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0) {
		gnutls_assert();
		return (time_t) - 1;
	}

	switch (get_certificate_type(session, GNUTLS_CTYPE_PEERS)) {
		case GNUTLS_CRT_X509:
			return
					_gnutls_x509_get_raw_crt_activation_time(&info->
										 raw_certificate_list[0]);
		default:
			return (time_t) - 1;
	}
}
