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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/*
  Status Request (OCSP) TLS extension.  See RFC 6066 section 8:
  https://tools.ietf.org/html/rfc6066#section-8
*/

#include "gnutls_int.h"
#include "errors.h"
#include <extensions.h>
#include <ext/status_request.h>
#include <mbuffers.h>
#include <auth.h>
#include <auth/cert.h>
#include <handshake.h>

#ifdef ENABLE_OCSP

typedef struct {
	gnutls_datum_t *responder_id;
	size_t responder_id_size;
	gnutls_datum_t request_extensions;
	gnutls_datum_t response;

	unsigned int expect_cstatus;
} status_request_ext_st;

/*
  From RFC 6066.  Client sends:

      struct {
	  CertificateStatusType status_type;
	  select (status_type) {
	      case ocsp: OCSPStatusRequest;
	  } request;
      } CertificateStatusRequest;

      enum { ocsp(1), (255) } CertificateStatusType;

      struct {
	  ResponderID responder_id_list<0..2^16-1>;
	  Extensions  request_extensions;
      } OCSPStatusRequest;

      opaque ResponderID<1..2^16-1>;
      opaque Extensions<0..2^16-1>;
*/


static int
client_send(gnutls_session_t session,
	    gnutls_buffer_st * extdata, status_request_ext_st * priv)
{
	int ret_len = 1 + 2;
	int ret;
	size_t i;

	ret = _gnutls_buffer_append_prefix(extdata, 8, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_buffer_append_prefix(extdata, 16,
					 priv->responder_id_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	for (i = 0; i < priv->responder_id_size; i++) {
		if (priv->responder_id[i].size <= 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		ret = _gnutls_buffer_append_data_prefix(extdata, 16,
							priv->
							responder_id[i].
							data,
							priv->
							responder_id[i].
							size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret_len += 2 + priv->responder_id[i].size;
	}

	ret = _gnutls_buffer_append_data_prefix(extdata, 16,
						priv->request_extensions.
						data,
						priv->request_extensions.
						size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret_len += 2 + priv->request_extensions.size;

	return ret_len;
}

static int
server_recv(gnutls_session_t session,
	    status_request_ext_st * priv,
	    const uint8_t * data, size_t size)
{
	ssize_t data_size = size;
	unsigned rid_bytes = 0;

	/* minimum message is type (1) + responder_id_list (2) +
	   request_extension (2) = 5 */
	if (data_size < 5)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	/* We ignore non-ocsp CertificateStatusType.  The spec is unclear
	   what should be done. */
	if (data[0] != 0x01) {
		gnutls_assert();
		_gnutls_handshake_log("EXT[%p]: unknown status_type %d\n",
				      session, data[0]);
		return 0;
	}
	DECR_LEN(data_size, 1);
	data++;

	rid_bytes = _gnutls_read_uint16(data);

	DECR_LEN(data_size, 2);
	/*data += 2;*/

	/* sanity check only, we don't use any of the data below */

	if (data_size < (ssize_t)rid_bytes)
		return
		    gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	return 0;
}


static int
client_recv(gnutls_session_t session,
	    status_request_ext_st * priv,
	    const uint8_t * data, size_t size)
{
	if (size != 0)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
	else {
		priv->expect_cstatus = 1;
		return 0;
	}
}


/*
 * Servers return a certificate response along with their certificate
 * by sending a "CertificateStatus" message immediately after the
 * "Certificate" message (and before any "ServerKeyExchange" or
 * "CertificateRequest" messages).  If a server returns a
 * "CertificateStatus" message, then the server MUST have included an
 * extension of type "status_request" with empty "extension_data" in
 * the extended server hello.
 *
 * According to the description above, as a server we could simply 
 * return GNUTLS_E_INT_RET_0 on this function. In that case we would
 * only need to use the callbacks at the time we need to send the data,
 * and skip the status response packet if no such data are there.
 * However, that behavior would break gnutls 3.3.x which expects the status 
 * response to be always send if the extension is present.
 *
 * Instead we ensure that this extension is parsed after the CS/certificate
 * are selected (with the _GNUTLS_EXT_TLS_POST_CS type), and we discover
 * (or not) the response to send early.
 */
static int
server_send(gnutls_session_t session,
	    gnutls_buffer_st * extdata, status_request_ext_st * priv)
{
	int ret;
	gnutls_certificate_credentials_t cred;
	gnutls_status_request_ocsp_func func;
	void *func_ptr;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL)	/* no certificate authentication */
		return gnutls_assert_val(0);

	if (session->internals.selected_ocsp_func) {
		func = session->internals.selected_ocsp_func;
		func_ptr = session->internals.selected_ocsp_func_ptr;
	} else if (cred->glob_ocsp_func) {
		func = cred->glob_ocsp_func;
		func_ptr = cred->glob_ocsp_func_ptr;
	} else {
		return 0;
	}

	if (func == NULL)
		return 0;

	ret = func(session, func_ptr, &priv->response);
	if (ret == GNUTLS_E_NO_CERTIFICATE_STATUS)
		return 0;
	else if (ret < 0)
		return gnutls_assert_val(ret);

	return GNUTLS_E_INT_RET_0;
}

static int
_gnutls_status_request_send_params(gnutls_session_t session,
				   gnutls_buffer_st * extdata)
{
	extension_priv_data_t epriv;
	status_request_ext_st *priv;
	int ret;

	/* Do not bother sending the OCSP status request extension
	 * if we are not using certificate authentication */
	if (_gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE) == NULL)
		return 0;

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_STATUS_REQUEST,
					   &epriv);

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (ret < 0 || epriv == NULL)	/* it is ok not to have it */
			return 0;
		priv = epriv;

		return client_send(session, extdata, priv);
	} else {
		epriv = priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		_gnutls_ext_set_session_data(session,
					     GNUTLS_EXTENSION_STATUS_REQUEST,
					     epriv);

		return server_send(session, extdata, priv);
	}
}

static int
_gnutls_status_request_recv_params(gnutls_session_t session,
				   const uint8_t * data, size_t size)
{
	extension_priv_data_t epriv;
	status_request_ext_st *priv;
	int ret;

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_STATUS_REQUEST,
					   &epriv);
	if (ret < 0 || epriv == NULL)	/* it is ok not to have it */
		return 0;

	priv = epriv;

	if (session->security_parameters.entity == GNUTLS_CLIENT)
		return client_recv(session, priv, data, size);
	else
		return server_recv(session, priv, data, size);
}

/**
 * gnutls_ocsp_status_request_enable_client:
 * @session: is a #gnutls_session_t type.
 * @responder_id: array with #gnutls_datum_t with DER data of responder id
 * @responder_id_size: number of members in @responder_id array
 * @extensions: a #gnutls_datum_t with DER encoded OCSP extensions
 *
 * This function is to be used by clients to request OCSP response
 * from the server, using the "status_request" TLS extension.  Only
 * OCSP status type is supported.
 *
 * The @responder_id array, its containing elements as well as
 * the data of @extensions, must be allocated using gnutls_malloc(). They
 * will be deinitialized on session cleanup.
 *
 * Due to the difficult semantics of the @responder_id and @extensions
 * parameters, it is recommended to only call this function with these
 * parameters set to %NULL.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since: 3.1.3
 **/
int
gnutls_ocsp_status_request_enable_client(gnutls_session_t session,
					 gnutls_datum_t * responder_id,
					 size_t responder_id_size,
					 gnutls_datum_t * extensions)
{
	status_request_ext_st *priv;
	extension_priv_data_t epriv;

	if (session->security_parameters.entity == GNUTLS_SERVER)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	epriv = priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	priv->responder_id = responder_id;
	priv->responder_id_size = responder_id_size;
	if (extensions) {
		priv->request_extensions.data = extensions->data;
		priv->request_extensions.size = extensions->size;
	}

	_gnutls_ext_set_session_data(session,
				     GNUTLS_EXTENSION_STATUS_REQUEST,
				     epriv);

	return 0;
}

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
	status_request_ext_st *priv;
	extension_priv_data_t epriv;
	int ret;

	if (session->security_parameters.entity == GNUTLS_SERVER)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_STATUS_REQUEST,
					   &epriv);
	if (ret < 0)
		return gnutls_assert_val(ret);

	priv = epriv;

	if (priv == NULL || priv->response.data == NULL)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	response->data = priv->response.data;
	response->size = priv->response.size;

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

static int file_ocsp_func(gnutls_session_t session, void *ptr,
			  gnutls_datum_t * ocsp_response)
{
	int ret;
	const char *file = ptr;

	ret = gnutls_load_file(file, ocsp_response);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_NO_CERTIFICATE_STATUS);

	return 0;
}

/**
 * gnutls_certificate_set_ocsp_status_request_file:
 * @sc: is a credentials structure.
 * @response_file: a filename of the OCSP response
 * @idx: is a certificate index as returned by gnutls_certificate_set_key() and friends
 *
 * This function sets the filename of an OCSP response, that will be
 * sent to the client if requests an OCSP certificate status for
 * the certificate chain specified by @idx.
 *
 * This is a convenience function which may be inefficient on busy servers since
 * the file is opened on every access. Use 
 * gnutls_certificate_set_ocsp_status_request_function2() to fine-tune
 * file accesses.
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
 * Since: 3.1.3
 **/
int
gnutls_certificate_set_ocsp_status_request_file(gnutls_certificate_credentials_t sc,
						const char *response_file,
						unsigned idx)
{
	if (idx >= sc->ncerts)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	gnutls_free(sc->certs[idx].ocsp_response_file);
	sc->certs[idx].ocsp_response_file = gnutls_strdup(response_file);
	if (sc->certs[idx].ocsp_response_file == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	gnutls_certificate_set_ocsp_status_request_function2(sc, idx, file_ocsp_func, sc->certs[idx].ocsp_response_file);

	return 0;
}

static void _gnutls_status_request_deinit_data(extension_priv_data_t epriv)
{
	status_request_ext_st *priv = epriv;
	unsigned i;

	if (priv == NULL)
		return;

	if (priv->responder_id != NULL) {
		for (i = 0; i < priv->responder_id_size; i++)
			gnutls_free(priv->responder_id[i].data);

		gnutls_free(priv->responder_id);
	}

	gnutls_free(priv->request_extensions.data);
	gnutls_free(priv->response.data);
	gnutls_free(priv);
}

static int
_gnutls_status_request_pack(extension_priv_data_t epriv,
			    gnutls_buffer_st * ps)
{
	status_request_ext_st *priv = epriv;
	int ret;

	BUFFER_APPEND_PFX4(ps, priv->response.data, priv->response.size);

	return 0;

}

static int
_gnutls_status_request_unpack(gnutls_buffer_st * ps,
			      extension_priv_data_t * epriv)
{
	status_request_ext_st *priv;
	int ret;

	priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	BUFFER_POP_DATUM(ps, &priv->response);

	*epriv = priv;

	return 0;

      error:
	gnutls_free(priv);
	return ret;
}

const extension_entry_st ext_mod_status_request = {
	.name = "OCSP Status Request",
	.type = GNUTLS_EXTENSION_STATUS_REQUEST,
	.parse_type = _GNUTLS_EXT_TLS_POST_CS,
	.recv_func = _gnutls_status_request_recv_params,
	.send_func = _gnutls_status_request_send_params,
	.pack_func = _gnutls_status_request_pack,
	.unpack_func = _gnutls_status_request_unpack,
	.deinit_func = _gnutls_status_request_deinit_data
};

/* Functions to be called from handshake */

int
_gnutls_send_server_certificate_status(gnutls_session_t session, int again)
{
	mbuffer_st *bufel = NULL;
	uint8_t *data;
	int data_size = 0;
	int ret;
	extension_priv_data_t epriv;
	status_request_ext_st *priv;

	if (again == 0) {
		ret =
		    _gnutls_ext_get_session_data(session,
						 GNUTLS_EXTENSION_STATUS_REQUEST,
						 &epriv);
		if (ret < 0)
			return 0;

		priv = epriv;

		if (!priv->response.size)
			return 0;

		data_size = priv->response.size + 4;
		bufel =
		    _gnutls_handshake_alloc(session, data_size);
		if (!bufel) {
			_gnutls_free_datum(&priv->response);
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}

		data = _mbuffer_get_udata_ptr(bufel);

		data[0] = 0x01;
		_gnutls_write_uint24(priv->response.size, &data[1]);
		memcpy(&data[4], priv->response.data, priv->response.size);

		_gnutls_free_datum(&priv->response);
	}
	return _gnutls_send_handshake(session, data_size ? bufel : NULL,
				      GNUTLS_HANDSHAKE_CERTIFICATE_STATUS);
}

int _gnutls_recv_server_certificate_status(gnutls_session_t session)
{
	uint8_t *data;
	ssize_t data_size;
	ssize_t r_size;
	gnutls_buffer_st buf;
	int ret;
	status_request_ext_st *priv = NULL;
	extension_priv_data_t epriv;

	ret =
	    _gnutls_ext_get_session_data(session,
					 GNUTLS_EXTENSION_STATUS_REQUEST,
					 &epriv);
	if (ret < 0)
		return 0;

	priv = epriv;

	if (!priv->expect_cstatus)
		return 0;

	ret = _gnutls_recv_handshake(session,
				     GNUTLS_HANDSHAKE_CERTIFICATE_STATUS,
				     1, &buf);
	if (ret < 0)
		return gnutls_assert_val_fatal(ret);

	priv->expect_cstatus = 0;

	data = buf.data;
	data_size = buf.length;

	/* minimum message is type (1) + response (3) + data */
	if (data_size == 0) {
		ret = 0;
		goto error;
	} else if (data_size < 4) {
		ret =
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		goto error;
	}

	if (data[0] != 0x01) {
		gnutls_assert();
		_gnutls_handshake_log("EXT[%p]: unknown status_type %d\n",
				      session, data[0]);
		ret = 0;
		goto error;
	}

	DECR_LENGTH_COM(data_size, 1, ret =
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			goto error);
	data++;

	DECR_LENGTH_COM(data_size, 3, ret =
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			goto error);
	r_size = _gnutls_read_uint24(data);
	data += 3;

	DECR_LENGTH_COM(data_size, r_size, ret =
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			goto error);

	ret = _gnutls_set_datum(&priv->response, data, r_size);
	if (ret < 0)
		goto error;

	ret = 0;

      error:
	_gnutls_buffer_clear(&buf);

	return ret;
}

/**
 * gnutls_ocsp_status_request_is_checked:
 * @session: is a gnutls session
 * @flags: should be zero or %GNUTLS_OCSP_SR_IS_AVAIL
 *
 * When flags are zero this function returns non-zero if a valid OCSP status
 * response was included in the TLS handshake. That is, an OCSP status response
 * which is not too old or superseded. It returns zero otherwise.
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
 * Returns: non zero if the response was valid, or a zero if it wasn't sent,
 * or sent and was invalid.
 *
 * Since: 3.1.4
 **/
int
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
