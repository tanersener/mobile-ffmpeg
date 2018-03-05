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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* This file contains the code the Certificate Type TLS extension.
 * This extension is currently gnutls specific.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include <ext/ecc.h>
#include <state.h>
#include <num.h>
#include <algorithms.h>

static int _gnutls_supported_ecc_recv_params(gnutls_session_t session,
					     const uint8_t * data,
					     size_t data_size);
static int _gnutls_supported_ecc_send_params(gnutls_session_t session,
					     gnutls_buffer_st * extdata);

static int _gnutls_supported_ecc_pf_recv_params(gnutls_session_t session,
						const uint8_t * data,
						size_t data_size);
static int _gnutls_supported_ecc_pf_send_params(gnutls_session_t session,
						gnutls_buffer_st *
						extdata);

const extension_entry_st ext_mod_supported_ecc = {
	.name = "Supported curves",
	.type = GNUTLS_EXTENSION_SUPPORTED_ECC,
	.parse_type = GNUTLS_EXT_TLS,

	.recv_func = _gnutls_supported_ecc_recv_params,
	.send_func = _gnutls_supported_ecc_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL
};

const extension_entry_st ext_mod_supported_ecc_pf = {
	.name = "Supported ECC Point Formats",
	.type = GNUTLS_EXTENSION_SUPPORTED_ECC_PF,
	.parse_type = GNUTLS_EXT_TLS,

	.recv_func = _gnutls_supported_ecc_pf_recv_params,
	.send_func = _gnutls_supported_ecc_pf_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL
};

/* 
 * In case of a server: if a SUPPORTED_ECC extension type is received then it stores
 * into the session security parameters the new value. The server may use gnutls_session_certificate_type_get(),
 * to access it.
 *
 * In case of a client: If a supported_eccs have been specified then we send the extension.
 *
 */
static int
_gnutls_supported_ecc_recv_params(gnutls_session_t session,
				  const uint8_t * data, size_t _data_size)
{
	int new_type = -1, ret, i;
	ssize_t data_size = _data_size;
	uint16_t len;
	const uint8_t *p = data;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		/* A client shouldn't receive this extension, but of course
		 * there are servers out there that send it. Just ignore it. */
		_gnutls_debug_log("received SUPPORTED ECC extension on client side!!!\n");
		return 0;
	} else {		/* SERVER SIDE - we must check if the sent supported ecc type is the right one 
				 */
		if (data_size < 2)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

		DECR_LEN(data_size, 2);
		len = _gnutls_read_uint16(p);
		p += 2;

		if (len % 2 != 0)
			return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		DECR_LEN(data_size, len);

		for (i = 0; i < len; i += 2) {
			new_type =
			    _gnutls_tls_id_to_ecc_curve(_gnutls_read_uint16
							(&p[i]));
			if (new_type < 0)
				continue;

			/* Check if we support this supported_ecc */
			if ((ret =
			     _gnutls_session_supports_ecc_curve(session,
								new_type))
			    < 0) {
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
		     _gnutls_session_supports_ecc_curve(session,
							new_type)) < 0) {
			/* The peer has requested unsupported ecc
			 * types. Instead of failing, procceed normally.
			 * (the ciphersuite selection would fail, or a
			 * non certificate ciphersuite will be selected).
			 */
			return gnutls_assert_val(0);
		}

		_gnutls_session_ecc_curve_set(session, new_type);
	}

	return 0;
}


/* returns data_size or a negative number on failure
 */
static int
_gnutls_supported_ecc_send_params(gnutls_session_t session,
				  gnutls_buffer_st * extdata)
{
	unsigned len, i;
	int ret;
	uint16_t p;

	/* this extension is only being sent on client side */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {

		if (session->internals.priorities.supported_ecc.
		    algorithms > 0) {

			len =
			    session->internals.priorities.supported_ecc.
			    algorithms;

			/* this is a vector!
			 */
			ret =
			    _gnutls_buffer_append_prefix(extdata, 16,
							 len * 2);
			if (ret < 0)
				return gnutls_assert_val(ret);

			for (i = 0; i < len; i++) {
				p = _gnutls_ecc_curve_get_tls_id(session->
								 internals.
								 priorities.supported_ecc.
								 priority
								 [i]);
				ret =
				    _gnutls_buffer_append_prefix(extdata,
								 16, p);
				if (ret < 0)
					return gnutls_assert_val(ret);
			}
			return (len + 1) * 2;
		}

	}

	return 0;
}

/* 
 * In case of a server: if a SUPPORTED_ECC extension type is received then it stores
 * into the session security parameters the new value. The server may use gnutls_session_certificate_type_get(),
 * to access it.
 *
 * In case of a client: If a supported_eccs have been specified then we send the extension.
 *
 */
static int
_gnutls_supported_ecc_pf_recv_params(gnutls_session_t session,
				     const uint8_t * data,
				     size_t _data_size)
{
	int len, i;
	int uncompressed = 0;
	int data_size = _data_size;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (data_size < 1)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

		len = data[0];
		if (len < 1)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

		DECR_LEN(data_size, len + 1);

		for (i = 1; i <= len; i++)
			if (data[i] == 0) {	/* uncompressed */
				uncompressed = 1;
				break;
			}

		if (uncompressed == 0)
			return
			    gnutls_assert_val
			    (GNUTLS_E_UNKNOWN_PK_ALGORITHM);
	} else {
		/* only sanity check here. We only support uncompressed points
		 * and a client must support it thus nothing to check.
		 */
		if (_data_size < 1)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_supported_ecc_pf_send_params(gnutls_session_t session,
				     gnutls_buffer_st * extdata)
{
	const uint8_t p[2] = { 0x01, 0x00 };	/* only support uncompressed point format */
	int ret;

	if (session->security_parameters.entity == GNUTLS_SERVER
	    && !_gnutls_session_is_ecc(session))
		return 0;

	if (session->internals.priorities.supported_ecc.algorithms > 0) {
		ret = _gnutls_buffer_append_data(extdata, p, 2);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return 2;
	}
	return 0;
}


/* Returns 0 if the given ECC curve is allowed in the current
 * session. A negative error value is returned otherwise.
 */
int
_gnutls_session_supports_ecc_curve(gnutls_session_t session,
				   unsigned int ecc_type)
{
	unsigned i;

	if (session->internals.priorities.supported_ecc.algorithms > 0) {
		for (i = 0;
		     i <
		     session->internals.priorities.supported_ecc.
		     algorithms; i++) {
			if (session->internals.priorities.supported_ecc.
			    priority[i] == ecc_type)
				return 0;
		}
	}

	return GNUTLS_E_ECC_UNSUPPORTED_CURVE;
}
