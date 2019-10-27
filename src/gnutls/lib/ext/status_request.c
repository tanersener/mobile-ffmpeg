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
  Status Request (OCSP) TLS extension.  See RFC 6066 section 8:
  https://tools.ietf.org/html/rfc6066#section-8
*/

#include "gnutls_int.h"
#include "errors.h"
#include <hello_ext.h>
#include <ext/status_request.h>
#include <mbuffers.h>
#include <auth.h>
#include <auth/cert.h>
#include <handshake.h>

#ifdef ENABLE_OCSP

typedef struct {
	/* server response */
	gnutls_datum_t sresp;

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
	const uint8_t data[5] = "\x01\x00\x00\x00\x00";
	const int len = 5;
	int ret;

	/* We do not support setting either ResponderID or Extensions */

	ret = _gnutls_buffer_append_data(extdata, data, len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return len;
}

static int
server_recv(gnutls_session_t session,
	    status_request_ext_st * priv,
	    const uint8_t * data, size_t data_size)
{
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

	if (session->internals.selected_ocsp_length > 0) {
		if (session->internals.selected_ocsp[0].response.data) {
			if (session->internals.selected_ocsp[0].exptime != 0 &&
			    (gnutls_time(0) >= session->internals.selected_ocsp[0].exptime)) {
				gnutls_assert();
				return 0;
			}

			ret = _gnutls_set_datum(&priv->sresp,
						session->internals.selected_ocsp[0].response.data,
						session->internals.selected_ocsp[0].response.size);
			if (ret < 0)
				return gnutls_assert_val(ret);
			return GNUTLS_E_INT_RET_0;
		} else {
			return 0;
		}
	} else if (session->internals.selected_ocsp_func) {
		func = session->internals.selected_ocsp_func;
		func_ptr = session->internals.selected_ocsp_func_ptr;
	} else {
		return 0;
	}

	if (func == NULL)
		return 0;

	ret = func(session, func_ptr, &priv->sresp);
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
	gnutls_ext_priv_data_t epriv;
	status_request_ext_st *priv;
	int ret;

	/* Do not bother sending the OCSP status request extension
	 * if we are not using certificate authentication */
	if (_gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE) == NULL)
		return 0;

	ret = _gnutls_hello_ext_get_priv(session,
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

		_gnutls_hello_ext_set_priv(session,
					     GNUTLS_EXTENSION_STATUS_REQUEST,
					     epriv);

		return server_send(session, extdata, priv);
	}
}

static int
_gnutls_status_request_recv_params(gnutls_session_t session,
				   const uint8_t * data, size_t size)
{
	gnutls_ext_priv_data_t epriv;
	status_request_ext_st *priv;
	int ret;

	ret = _gnutls_hello_ext_get_priv(session,
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
 * @responder_id: ignored, must be %NULL
 * @responder_id_size: ignored, must be zero
 * @extensions: ignored, must be %NULL
 *
 * This function is to be used by clients to request OCSP response
 * from the server, using the "status_request" TLS extension.  Only
 * OCSP status type is supported.
 *
 * Previous versions of GnuTLS supported setting @responder_id and
 * @extensions fields, but due to the difficult semantics of the
 * parameter usage, and other issues, this support was removed
 * since 3.6.0 and these parameters must be set to %NULL.
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
	gnutls_ext_priv_data_t epriv;

	if (session->security_parameters.entity == GNUTLS_SERVER)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	epriv = priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	_gnutls_hello_ext_set_priv(session,
				     GNUTLS_EXTENSION_STATUS_REQUEST,
				     epriv);

	return 0;
}


static void _gnutls_status_request_deinit_data(gnutls_ext_priv_data_t epriv)
{
	status_request_ext_st *priv = epriv;

	if (priv == NULL)
		return;

	gnutls_free(priv->sresp.data);
	gnutls_free(priv);
}

const hello_ext_entry_st ext_mod_status_request = {
	.name = "OCSP Status Request",
	.tls_id = STATUS_REQUEST_TLS_ID,
	.gid = GNUTLS_EXTENSION_STATUS_REQUEST,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.parse_type = _GNUTLS_EXT_TLS_POST_CS,
	.recv_func = _gnutls_status_request_recv_params,
	.send_func = _gnutls_status_request_send_params,
	.deinit_func = _gnutls_status_request_deinit_data,
	.cannot_be_overriden = 1
};

/* Functions to be called from handshake */

int
_gnutls_send_server_certificate_status(gnutls_session_t session, int again)
{
	mbuffer_st *bufel = NULL;
	uint8_t *data;
	int data_size = 0;
	int ret;
	gnutls_ext_priv_data_t epriv;
	status_request_ext_st *priv;

	if (again == 0) {
		ret =
		    _gnutls_hello_ext_get_priv(session,
						 GNUTLS_EXTENSION_STATUS_REQUEST,
						 &epriv);
		if (ret < 0)
			return 0;

		priv = epriv;

		if (!priv->sresp.size)
			return 0;

		data_size = priv->sresp.size + 4;
		bufel =
		    _gnutls_handshake_alloc(session, data_size);
		if (!bufel) {
			_gnutls_free_datum(&priv->sresp);
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}

		data = _mbuffer_get_udata_ptr(bufel);

		data[0] = 0x01;
		_gnutls_write_uint24(priv->sresp.size, &data[1]);
		memcpy(&data[4], priv->sresp.data, priv->sresp.size);

		_gnutls_free_datum(&priv->sresp);
	}
	return _gnutls_send_handshake(session, data_size ? bufel : NULL,
				      GNUTLS_HANDSHAKE_CERTIFICATE_STATUS);
}

int _gnutls_parse_ocsp_response(gnutls_session_t session, const uint8_t *data, ssize_t data_size, gnutls_datum_t *resp)
{
	int ret;
	ssize_t r_size;

	resp->data = 0;
	resp->size = 0;

	/* minimum message is type (1) + response (3) + data */
	if (data_size < 4)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (data[0] != 0x01) {
		gnutls_assert();
		_gnutls_handshake_log("EXT[%p]: unknown status_type %d\n",
				      session, data[0]);
		return 0;
	}

	DECR_LENGTH_RET(data_size, 1,
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
	data++;

	DECR_LENGTH_RET(data_size, 3,
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	r_size = _gnutls_read_uint24(data);
	data += 3;

	DECR_LENGTH_RET(data_size, r_size,
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (r_size < 1)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	ret = _gnutls_set_datum(resp, data, r_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

int _gnutls_recv_server_certificate_status(gnutls_session_t session)
{
	uint8_t *data;
	ssize_t data_size;
	gnutls_buffer_st buf;
	int ret;
	gnutls_datum_t resp;
	status_request_ext_st *priv = NULL;
	gnutls_ext_priv_data_t epriv;
	cert_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);

	if (info == NULL)
		return 0;

	ret =
	    _gnutls_hello_ext_get_priv(session,
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

	if (data_size == 0) {
		ret = 0;
		goto error;
	}

	ret = _gnutls_parse_ocsp_response(session, data, data_size, &resp);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	if (resp.data && resp.size > 0) {
		info->raw_ocsp_list = gnutls_malloc(sizeof(gnutls_datum_t));
		if (info->raw_ocsp_list == NULL) {
			ret = GNUTLS_E_MEMORY_ERROR;
			goto error;
		}
		info->raw_ocsp_list[0].data = resp.data;
		info->raw_ocsp_list[0].size = resp.size;
		info->nocsp = 1;
	}

	ret = 0;

      error:
	_gnutls_buffer_clear(&buf);

	return ret;
}

#endif
