/*
 * Copyright (C) 2012-2017 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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

/*
 *  Status Request (OCSP) API.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <auth.h>
#include <auth/cert.h>
#include <handshake.h>
#include <minmax.h>

#ifdef ENABLE_OCSP

#include <gnutls/ocsp.h>
#include "x509/ocsp.h"

/**
 * gnutls_ocsp_status_request_get:
 * @session: is a #gnutls_session_t type.
 * @response: a #gnutls_datum_t with DER encoded OCSP response
 *
 * This function returns the OCSP status response received
 * from the TLS server. The @response should be treated as
 * constant. If no OCSP response is available then
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since: 3.1.3
 **/
int
gnutls_ocsp_status_request_get(gnutls_session_t session,
			       gnutls_datum_t * response)
{
	return gnutls_ocsp_status_request_get2(session, 0, response);
}

/**
 * gnutls_ocsp_status_request_get2:
 * @session: is a #gnutls_session_t type.
 * @idx: the index of peer's certificate
 * @response: a #gnutls_datum_t with DER encoded OCSP response
 *
 * This function returns the OCSP status response received
 * from the TLS server for the certificate index provided.
 * The index corresponds to certificates as returned by
 * gnutls_certificate_get_peers. When index is zero this
 * function operates identically to gnutls_ocsp_status_request_get().
 *
 * The returned @response should be treated as
 * constant. If no OCSP response is available for the
 * given index then %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since: 3.6.3
 **/
int
gnutls_ocsp_status_request_get2(gnutls_session_t session,
			        unsigned idx,
			        gnutls_datum_t * response)
{
	const version_entry_st *ver = get_version(session);
	cert_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);

	if (!ver->tls13_sem && session->security_parameters.entity == GNUTLS_SERVER)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (info == NULL || info->raw_ocsp_list == NULL ||
	    info->nocsp <= idx || info->raw_ocsp_list[idx].size == 0)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	response->data = info->raw_ocsp_list[idx].data;
	response->size = info->raw_ocsp_list[idx].size;

	return 0;
}

/**
 * gnutls_certificate_set_ocsp_status_request_function:
 * @sc: is a #gnutls_certificate_credentials_t type.
 * @ocsp_func: function pointer to OCSP status request callback.
 * @ptr: opaque pointer passed to callback function
 *
 * This function is to be used by server to register a callback to
 * handle OCSP status requests from the client.  The callback will be
 * invoked if the client supplied a status-request OCSP extension.
 * The callback function prototype is:
 *
 * typedef int (*gnutls_status_request_ocsp_func)
 *    (gnutls_session_t session, void *ptr, gnutls_datum_t *ocsp_response);
 *
 * The callback will be invoked if the client requests an OCSP certificate
 * status.  The callback may return %GNUTLS_E_NO_CERTIFICATE_STATUS, if
 * there is no recent OCSP response. If the callback returns %GNUTLS_E_SUCCESS,
 * it is expected to have the @ocsp_response field set with a valid (DER-encoded)
 * OCSP response. The response must be a value allocated using gnutls_malloc(),
 * and will be deinitialized by the caller.
 *
 * It is possible to set a specific callback for each provided certificate
 * using gnutls_certificate_set_ocsp_status_request_function2().
 *
 * Since: 3.1.3
 **/
void
gnutls_certificate_set_ocsp_status_request_function
(gnutls_certificate_credentials_t sc,
gnutls_status_request_ocsp_func ocsp_func, void *ptr)
{

	sc->glob_ocsp_func = ocsp_func;
	sc->glob_ocsp_func_ptr = ptr;
}

/**
 * gnutls_certificate_set_ocsp_status_request_function2:
 * @sc: is a #gnutls_certificate_credentials_t type.
 * @idx: is a certificate index as returned by gnutls_certificate_set_key() and friends
 * @ocsp_func: function pointer to OCSP status request callback.
 * @ptr: opaque pointer passed to callback function
 *
 * This function is to be used by server to register a callback to
 * provide OCSP status requests that correspond to the indexed certificate chain
 * from the client.  The callback will be invoked if the client supplied a
 * status-request OCSP extension.
 *
 * The callback function prototype is:
 *
 * typedef int (*gnutls_status_request_ocsp_func)
 *    (gnutls_session_t session, void *ptr, gnutls_datum_t *ocsp_response);
 *
 * The callback will be invoked if the client requests an OCSP certificate
 * status.  The callback may return %GNUTLS_E_NO_CERTIFICATE_STATUS, if
 * there is no recent OCSP response. If the callback returns %GNUTLS_E_SUCCESS,
 * it is expected to have the @ocsp_response field set with a valid (DER-encoded)
 * OCSP response. The response must be a value allocated using gnutls_malloc(),
 * and will be deinitialized by the caller.
 *
 * Note: the ability to set multiple OCSP responses per credential
 * structure via the index @idx was added in version 3.5.6. To keep
 * backwards compatibility, it requires using gnutls_certificate_set_flags()
 * with the %GNUTLS_CERTIFICATE_API_V2 flag to make the set certificate
 * functions return an index usable by this function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since: 3.5.5
 **/
int
gnutls_certificate_set_ocsp_status_request_function2
(gnutls_certificate_credentials_t sc, unsigned idx, gnutls_status_request_ocsp_func ocsp_func, void *ptr)
{
	if (idx >= sc->ncerts)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	sc->certs[idx].ocsp_func = ocsp_func;
	sc->certs[idx].ocsp_func_ptr = ptr;

	return 0;
}

static
unsigned resp_matches_pcert(gnutls_ocsp_resp_t resp, const gnutls_pcert_st *cert)
{
	gnutls_x509_crt_t crt;
	int ret;
	unsigned retval;

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0)
		return 0;

	ret = gnutls_x509_crt_import(crt, &cert->cert, GNUTLS_X509_FMT_DER);
	if (ret < 0) {
		gnutls_assert();
		retval = 0;
		goto cleanup;
	}

	ret = gnutls_ocsp_resp_check_crt(resp, 0, crt);
	if (ret == 0)
		retval = 1;
	else
		retval = 0;

 cleanup:
	gnutls_x509_crt_deinit(crt);
	return retval;
}

/**
 * gnutls_certificate_set_ocsp_status_request_file:
 * @sc: is a credentials structure.
 * @response_file: a filename of the OCSP response
 * @idx: is a certificate index as returned by gnutls_certificate_set_key() and friends
 *
 * This function loads the provided OCSP response. It will be
 * sent to the client if requests an OCSP certificate status for
 * the certificate chain specified by @idx.
 *
 * Note: the ability to set multiple OCSP responses per credential
 * structure via the index @idx was added in version 3.5.6. To keep
 * backwards compatibility, it requires using gnutls_certificate_set_flags()
 * with the %GNUTLS_CERTIFICATE_API_V2 flag to make the set certificate
 * functions return an index usable by this function.
 *
 * This function can be called multiple times since GnuTLS 3.6.3
 * when multiple responses which apply to the chain are available.
 * If the response provided does not match any certificates present
 * in the chain, the code %GNUTLS_E_OCSP_MISMATCH_WITH_CERTS is returned.
 * To revert to the previous behavior set the flag %GNUTLS_CERTIFICATE_SKIP_OCSP_RESPONSE_CHECK
 * in the certificate credentials structure. In that case, only the
 * end-certificate's OCSP response can be set.
 * If the response is already expired at the time of loading the code
 * %GNUTLS_E_EXPIRED is returned.
 *
 * To revert to the previous behavior of this function which does not return
 * any errors, set the flag %GNUTLS_CERTIFICATE_SKIP_OCSP_RESPONSE_CHECK
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since: 3.1.3
 **/
int
gnutls_certificate_set_ocsp_status_request_file(gnutls_certificate_credentials_t sc,
						const char *response_file,
						unsigned idx)
{
	int ret;

	ret = gnutls_certificate_set_ocsp_status_request_file2(sc, response_file,
							       idx, GNUTLS_X509_FMT_DER);
	if (ret >= 0)
		return 0;
	else
		return ret;
}

static int append_response(gnutls_certificate_credentials_t sc, unsigned idx,
			   gnutls_ocsp_resp_t resp, const gnutls_datum_t *der)
{
	int ret;
	unsigned i, found = 0;
	unsigned try_already_set = 0;
	time_t t;

 retry:

	/* iterate through all certificates in chain, and add the response
	 * to the certificate that it matches with.
	 */
	for (i=0;i<MIN(sc->certs[idx].cert_list_length, MAX_OCSP_RESPONSES);i++) {
		if (!try_already_set && sc->certs[idx].ocsp_data[i].response.data)
			continue;

		if (!resp_matches_pcert(resp, &sc->certs[idx].cert_list[i]))
			continue;

		t = _gnutls_ocsp_get_validity(resp);
		/* if already invalid */
		if (t == (time_t)-1) {
			_gnutls_debug_log("the OCSP response associated with chain %d on pos %d, is invalid/expired\n", idx, i);
			return GNUTLS_E_EXPIRED;
		} else if (t == (time_t)-2) {
			_gnutls_debug_log("the OCSP response associated with chain %d on pos %d, is too old (ignoring)\n", idx, i);
			return 0;
		}

		if (t >= 0)
			sc->certs[idx].ocsp_data[i].exptime = t;
		else
			sc->certs[idx].ocsp_data[i].exptime = 0;

		_gnutls_debug_log("associating OCSP response with chain %d on pos %d\n", idx, i);

		gnutls_free(sc->certs[idx].ocsp_data[i].response.data);

		ret = _gnutls_set_datum(&sc->certs[idx].ocsp_data[i].response,
					der->data,
					der->size);
		if (ret < 0) {
			gnutls_assert();
			sc->certs[idx].ocsp_data[i].response.data = NULL;
			sc->certs[idx].ocsp_data[i].response.size = 0;
			return ret;
		}

		if (sc->certs[idx].ocsp_data_length <= i)
			sc->certs[idx].ocsp_data_length = i+1;

		found = 1;
		break;
	}

	if (!found) {
		/* slow path; if we found no matching certificate for the OCSP
		 * response, try all the existing, even if a response is already
		 * given. */
		if (!try_already_set) {
			try_already_set = 1;
			goto retry;
		}
		ret = GNUTLS_E_OCSP_MISMATCH_WITH_CERTS;
	} else {
		ret = 0;
	}

	return ret;
}

/**
 * gnutls_certificate_set_ocsp_status_request_file2:
 * @sc: is a credentials structure.
 * @response_file: a filename of the OCSP response
 * @idx: is a certificate index as returned by gnutls_certificate_set_key() and friends
 * @fmt: is PEM or DER
 *
 * This function loads the OCSP responses to be sent to the
 * peer for the certificate chain specified by @idx. When @fmt is
 * set to PEM, multiple responses can be loaded.
 *
 * This function must be called after setting any certificates, and
 * cannot be used for certificates that are provided via a callback --
 * that is when gnutls_certificate_set_retrieve_function() is used. In
 * that case consider using gnutls_certificate_set_retrieve_function3().
 *
 * This function can be called multiple times when multiple responses
 * applicable to the certificate chain are available.
 * If the response provided does not match any certificates present
 * in the chain, the code %GNUTLS_E_OCSP_MISMATCH_WITH_CERTS is returned.
 * If the response is already expired at the time of loading the code
 * %GNUTLS_E_EXPIRED is returned.
 *
 * Returns: On success, the number of loaded responses is returned,
 *   otherwise a negative error code.
 *
 * Since: 3.1.3
 **/
int
gnutls_certificate_set_ocsp_status_request_file2(gnutls_certificate_credentials_t sc,
						const char *response_file,
						unsigned idx,
						gnutls_x509_crt_fmt_t fmt)
{
	gnutls_datum_t raw = {NULL, 0};
	int ret;

	if (idx >= sc->ncerts)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	ret = gnutls_load_file(response_file, &raw);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_FILE_ERROR);

	ret = gnutls_certificate_set_ocsp_status_request_mem(sc, &raw, idx, fmt);
	gnutls_free(raw.data);
	return ret;
}

#define PEM_OCSP_RESPONSE "OCSP RESPONSE"
#define FULL_PEM_OCSP_RESPONSE "-----BEGIN OCSP RESPONSE"

/**
 * gnutls_certificate_set_ocsp_status_request_mem:
 * @sc: is a credentials structure.
 * @resp_data: a memory buffer holding an OCSP response
 * @idx: is a certificate index as returned by gnutls_certificate_set_key() and friends
 * @fmt: is PEM or DER
 *
 * This function sets the OCSP responses to be sent to the
 * peer for the certificate chain specified by @idx. When @fmt is set
 * to PEM, multiple responses can be loaded.
 *
 * Note: the ability to set multiple OCSP responses per credential
 * structure via the index @idx was added in version 3.5.6. To keep
 * backwards compatibility, it requires using gnutls_certificate_set_flags()
 * with the %GNUTLS_CERTIFICATE_API_V2 flag to make the set certificate
 * functions return an index usable by this function.
 *
 * This function must be called after setting any certificates, and
 * cannot be used for certificates that are provided via a callback --
 * that is when gnutls_certificate_set_retrieve_function() is used.
 *
 * This function can be called multiple times when multiple responses which
 * apply to the certificate chain are available.
 * If the response provided does not match any certificates present
 * in the chain, the code %GNUTLS_E_OCSP_MISMATCH_WITH_CERTS is returned.
 * If the response is already expired at the time of loading the code
 * %GNUTLS_E_EXPIRED is returned.
 *
 * Returns: On success, the number of loaded responses is returned,
 *   otherwise a negative error code.
 *
 * Since: 3.6.3
 **/
int
gnutls_certificate_set_ocsp_status_request_mem(gnutls_certificate_credentials_t sc,
					       const gnutls_datum_t *resp_data,
					       unsigned idx,
					       gnutls_x509_crt_fmt_t fmt)

{
	gnutls_datum_t der = {NULL, 0};
	gnutls_ocsp_resp_t resp = NULL;
	int ret;
	unsigned int nresp = 0;

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	if (fmt == GNUTLS_X509_FMT_PEM) {
		/* load multiple responses */
		gnutls_datum_t p = {resp_data->data, resp_data->size};

		p.data = memmem(p.data, p.size, FULL_PEM_OCSP_RESPONSE,
				sizeof(FULL_PEM_OCSP_RESPONSE)-1);
		if (p.data == NULL) {
			ret = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
			goto cleanup;
		}

		p.size -= p.data - resp_data->data;
		if (p.size <= 0) {
			ret = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
			goto cleanup;
		}

		do {
			ret = gnutls_pem_base64_decode2(PEM_OCSP_RESPONSE, &p, &der);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret = gnutls_certificate_set_ocsp_status_request_mem(sc, &der, idx,
									     GNUTLS_X509_FMT_DER);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			nresp++;

			gnutls_free(der.data);

			p.data++;
			p.size--;

			p.data = memmem(p.data, p.size, FULL_PEM_OCSP_RESPONSE,
					sizeof(FULL_PEM_OCSP_RESPONSE)-1);
			if (p.data == NULL)
				break;
			p.size = resp_data->size - (p.data - resp_data->data);
		} while(p.size > 0);

		ret = nresp;
	} else {
		/* DER: load a single response */
		if (sc->flags & GNUTLS_CERTIFICATE_SKIP_OCSP_RESPONSE_CHECK) {
			ret = gnutls_ocsp_resp_import2(resp, resp_data, GNUTLS_X509_FMT_DER);
			if (ret >= 0) {
				sc->certs[idx].ocsp_data[0].exptime = _gnutls_ocsp_get_validity(resp);
				if (sc->certs[idx].ocsp_data[0].exptime <= 0)
					sc->certs[idx].ocsp_data[0].exptime = 0;
			}

			/* quick load of first response */
			gnutls_free(sc->certs[idx].ocsp_data[0].response.data);

			ret = _gnutls_set_datum(&sc->certs[idx].ocsp_data[0].response,
						resp_data->data,
						resp_data->size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			sc->certs[idx].ocsp_data_length = 1;
			goto cleanup;
		}

		ret = gnutls_ocsp_resp_import2(resp, resp_data, GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = append_response(sc, idx, resp, resp_data);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = 1;
	}
  cleanup:
	gnutls_free(der.data);
	if (resp)
		gnutls_ocsp_resp_deinit(resp);

	return ret;
}

/**
 * gnutls_certificate_get_ocsp_expiration:
 * @sc: is a credentials structure.
 * @idx: is a certificate chain index as returned by gnutls_certificate_set_key() and friends
 * @oidx: is an OCSP response index
 * @flags: should be zero
 *
 * This function returns the validity of the loaded OCSP responses,
 * to provide information on when to reload/refresh them.
 *
 * Note that the credentials structure should be read-only when in
 * use, thus when reloading, either the credentials structure must not
 * be in use by any sessions, or a new credentials structure should be
 * allocated for new sessions.
 *
 * When @oidx is (-1) then the minimum refresh time for all responses
 * is returned. Otherwise the index specifies the response corresponding
 * to the @odix certificate in the certificate chain.
 *
 * Returns: On success, the expiration time of the OCSP response. Otherwise
 *   (time_t)(-1) on error, or (time_t)-2 on out of bounds.
 *
 * Since: 3.6.3
 **/
time_t
gnutls_certificate_get_ocsp_expiration(gnutls_certificate_credentials_t sc,
				       unsigned idx,
				       int oidx,
				       unsigned flags)
{
	unsigned j;

	if (idx >= sc->ncerts)
		return (time_t)-2;

	if (oidx == -1) {
		time_t min = 0;

		for (j=0;j<MIN(sc->certs[idx].cert_list_length, MAX_OCSP_RESPONSES);j++) {
			if (min <= 0)
				min = sc->certs[idx].ocsp_data[j].exptime;
			else
				if (sc->certs[idx].ocsp_data[j].exptime > 0 &&
				    min >= sc->certs[idx].ocsp_data[j].exptime)
					min = sc->certs[idx].ocsp_data[j].exptime;
		}
		return min;
	}

	if (oidx >= MAX_OCSP_RESPONSES || (unsigned)oidx >= sc->certs[idx].cert_list_length)
		return (time_t)-2;

	if (sc->certs[idx].ocsp_data[oidx].response.data == NULL)
		return (time_t)-1;

	return sc->certs[idx].ocsp_data[oidx].exptime;
}

/**
 * gnutls_ocsp_status_request_is_checked:
 * @session: is a gnutls session
 * @flags: should be zero or %GNUTLS_OCSP_SR_IS_AVAIL
 *
 * When flags are zero this function returns non-zero if a valid OCSP status
 * response was included in the TLS handshake. That is, an OCSP status response
 * which is not too old, superseded or marks the certificate as revoked.
 * It returns zero otherwise.
 *
 * When the flag %GNUTLS_OCSP_SR_IS_AVAIL is specified, the function
 * returns non-zero if an OCSP status response was included in the handshake
 * even if it was invalid. Otherwise, if no OCSP status response was included,
 * it returns zero. The %GNUTLS_OCSP_SR_IS_AVAIL flag was introduced in GnuTLS 3.4.0.
 *
 * This is a helper function when needing to decide whether to perform an
 * explicit OCSP validity check on the peer's certificate. Should be called after
 * any of gnutls_certificate_verify_peers*() are called.
 *
 * This function is always usable on client side, but on server side only
 * under TLS 1.3, which is the first version of TLS that allows cliend-side OCSP
 * responses.
 *
 * Returns: Non-zero if the response was valid, or a zero if it wasn't sent,
 * or sent and was invalid.
 *
 * Since: 3.1.4
 **/
unsigned
gnutls_ocsp_status_request_is_checked(gnutls_session_t session,
				      unsigned int flags)
{
	int ret;
	gnutls_datum_t data;

	if (flags & GNUTLS_OCSP_SR_IS_AVAIL) {
		ret = gnutls_ocsp_status_request_get(session, &data);
		if (ret < 0)
			return gnutls_assert_val(0);

		if (data.data == NULL)
			return gnutls_assert_val(0);
		return 1;
	}
	return session->internals.ocsp_check_ok;
}

#endif
