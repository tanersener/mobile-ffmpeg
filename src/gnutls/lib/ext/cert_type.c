/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
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

/* This file contains the code the Certificate Type TLS extension.
 * This extension is currently gnutls specific.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include <ext/cert_type.h>
#include <state.h>
#include <num.h>

#ifdef ENABLE_OPENPGP

/* Maps record size to numbers according to the
 * extensions draft.
 */
inline static int _gnutls_num2cert_type(int num);
inline static int _gnutls_cert_type2num(int record_size);
static int _gnutls_cert_type_recv_params(gnutls_session_t session,
					 const uint8_t * data,
					 size_t data_size);
static int _gnutls_cert_type_send_params(gnutls_session_t session,
					 gnutls_buffer_st * extdata);

const extension_entry_st ext_mod_cert_type = {
	.name = "Certificate Type",
	.type = GNUTLS_EXTENSION_CERT_TYPE,
	.parse_type = GNUTLS_EXT_TLS,

	.recv_func = _gnutls_cert_type_recv_params,
	.send_func = _gnutls_cert_type_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL
};

/* 
 * In case of a server: if a CERT_TYPE extension type is received then it stores
 * into the session security parameters the new value. The server may use gnutls_session_certificate_type_get(),
 * to access it.
 *
 * In case of a client: If a cert_types have been specified then we send the extension.
 *
 */

static int
_gnutls_cert_type_recv_params(gnutls_session_t session,
			      const uint8_t * data, size_t _data_size)
{
	int new_type = -1, ret, i;
	ssize_t data_size = _data_size;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (data_size > 0) {
			if (data_size != 1) {
				gnutls_assert();
				return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			}

			new_type = _gnutls_num2cert_type(data[0]);

			if (new_type < 0) {
				gnutls_assert();
				return new_type;
			}

			/* Check if we support this cert_type */
			if ((ret =
			     _gnutls_session_cert_type_supported(session,
								 new_type))
			    < 0) {
				gnutls_assert();
				return ret;
			}

			_gnutls_session_cert_type_set(session, new_type);
		}
	} else {		/* SERVER SIDE - we must check if the sent cert type is the right one 
				 */
		if (data_size > 1) {
			uint8_t len;

			DECR_LEN(data_size, 1);
			len = data[0];
			DECR_LEN(data_size, len);

			for (i = 0; i < len; i++) {
				new_type =
				    _gnutls_num2cert_type(data[i + 1]);

				if (new_type < 0)
					continue;

				/* Check if we support this cert_type */
				if ((ret =
				     _gnutls_session_cert_type_supported
				     (session, new_type)) < 0) {
					gnutls_assert();
					continue;
				} else
					break;
				/* new_type is ok */
			}

			if (new_type < 0) {
				gnutls_assert();
				return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			}

			if ((ret =
			     _gnutls_session_cert_type_supported(session,
								 new_type))
			    < 0) {
				gnutls_assert();
				/* The peer has requested unsupported certificate
				 * types. Instead of failing, procceed normally.
				 * (the ciphersuite selection would fail, or a
				 * non certificate ciphersuite will be selected).
				 */
				return 0;
			}

			_gnutls_session_cert_type_set(session, new_type);
		}
	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_cert_type_send_params(gnutls_session_t session,
			      gnutls_buffer_st * extdata)
{
	unsigned len, i;
	int ret;
	uint8_t p;

	/* this function sends the client extension data (dnsname) */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {

		if (session->internals.priorities.cert_type.algorithms > 0) {

			len =
			    session->internals.priorities.cert_type.
			    algorithms;

			if (len == 1 &&
			    session->internals.priorities.cert_type.
			    priority[0] == GNUTLS_CRT_X509) {
				/* We don't use this extension if X.509 certificates
				 * are used.
				 */
				return 0;
			}

			/* this is a vector!
			 */
			p = (uint8_t) len;
			ret = _gnutls_buffer_append_data(extdata, &p, 1);
			if (ret < 0)
				return gnutls_assert_val(ret);

			for (i = 0; i < len; i++) {
				p = _gnutls_cert_type2num(session->
							  internals.
							  priorities.cert_type.
							  priority[i]);
				ret =
				    _gnutls_buffer_append_data(extdata, &p,
							       1);
				if (ret < 0)
					return gnutls_assert_val(ret);
			}
			return len + 1;
		}

	} else {		/* server side */
		if (session->security_parameters.cert_type !=
		    DEFAULT_CERT_TYPE) {
			len = 1;

			p = _gnutls_cert_type2num(session->
						  security_parameters.
						  cert_type);
			ret = _gnutls_buffer_append_data(extdata, &p, 1);
			if (ret < 0)
				return gnutls_assert_val(ret);

			return len;
		}


	}

	return 0;
}

/* Maps numbers to record sizes according to the
 * extensions draft.
 */
inline static int _gnutls_num2cert_type(int num)
{
	switch (num) {
	case 0:
		return GNUTLS_CRT_X509;
	case 1:
		return GNUTLS_CRT_OPENPGP;
	default:
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}
}

/* Maps record size to numbers according to the
 * extensions draft.
 */
inline static int _gnutls_cert_type2num(int cert_type)
{
	switch (cert_type) {
	case GNUTLS_CRT_X509:
		return 0;
	case GNUTLS_CRT_OPENPGP:
		return 1;
	default:
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}

}

#endif
