/*
 * Copyright (C) 2009-2012 Free Software Foundation, Inc.
 *
 * Author: Steve Dispensa (<dispensa@phonefactor.com>)
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
#include <ext/safe_renegotiation.h>
#include "errors.h"


static int _gnutls_sr_recv_params(gnutls_session_t state,
				  const uint8_t * data, size_t data_size);
static int _gnutls_sr_send_params(gnutls_session_t state,
				  gnutls_buffer_st *);
static void _gnutls_sr_deinit_data(extension_priv_data_t priv);

const extension_entry_st ext_mod_sr = {
	.name = "Safe Renegotiation",
	.type = GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
	.parse_type = GNUTLS_EXT_MANDATORY,

	.recv_func = _gnutls_sr_recv_params,
	.send_func = _gnutls_sr_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = _gnutls_sr_deinit_data,
};

int
_gnutls_ext_sr_finished(gnutls_session_t session, void *vdata,
			size_t vdata_size, int dir)
{
	int ret;
	sr_ext_st *priv;
	extension_priv_data_t epriv;

	if (session->internals.priorities.sr == SR_DISABLED) {
		return 0;
	}

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					   &epriv);
	if (ret < 0) {
		gnutls_assert();
		/* if a client didn't advertise safe renegotiation, we treat
		 * it as disabled. */
		if (session->security_parameters.entity == GNUTLS_SERVER)
			return 0;
		return ret;
	}
	priv = epriv;

	/* Save data for safe renegotiation. 
	 */
	if (vdata_size > MAX_VERIFY_DATA_SIZE) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if ((session->security_parameters.entity == GNUTLS_CLIENT
	     && dir == 0)
	    || (session->security_parameters.entity == GNUTLS_SERVER
		&& dir == 1)) {
		priv->client_verify_data_len = vdata_size;
		memcpy(priv->client_verify_data, vdata, vdata_size);
	} else {
		priv->server_verify_data_len = vdata_size;
		memcpy(priv->server_verify_data, vdata, vdata_size);
	}

	return 0;
}

int _gnutls_ext_sr_verify(gnutls_session_t session)
{
	int ret;
	sr_ext_st *priv = NULL;
	extension_priv_data_t epriv;

	if (session->internals.priorities.sr == SR_DISABLED) {
		gnutls_assert();
		return 0;
	}

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					   &epriv);
	if (ret >= 0)
		priv = epriv;

	/* Safe renegotiation */

	if (priv && priv->safe_renegotiation_received) {
		if ((priv->ri_extension_data_len <
		     priv->client_verify_data_len)
		    ||
		    (memcmp
		     (priv->ri_extension_data, priv->client_verify_data,
		      priv->client_verify_data_len))) {
			gnutls_assert();
			_gnutls_handshake_log
			    ("HSK[%p]: Safe renegotiation failed [1]\n",
			     session);
			return GNUTLS_E_SAFE_RENEGOTIATION_FAILED;
		}

		if (session->security_parameters.entity == GNUTLS_CLIENT) {
			if ((priv->ri_extension_data_len !=
			     priv->client_verify_data_len +
			     priv->server_verify_data_len)
			    || memcmp(priv->ri_extension_data +
				      priv->client_verify_data_len,
				      priv->server_verify_data,
				      priv->server_verify_data_len) != 0) {
				gnutls_assert();
				_gnutls_handshake_log
				    ("HSK[%p]: Safe renegotiation failed [2]\n",
				     session);
				return GNUTLS_E_SAFE_RENEGOTIATION_FAILED;
			}
		} else {	/* Make sure there are 0 extra bytes */

			if (priv->ri_extension_data_len !=
			    priv->client_verify_data_len) {
				gnutls_assert();
				_gnutls_handshake_log
				    ("HSK[%p]: Safe renegotiation failed [3]\n",
				     session);
				return GNUTLS_E_SAFE_RENEGOTIATION_FAILED;
			}
		}

		_gnutls_handshake_log
		    ("HSK[%p]: Safe renegotiation succeeded\n", session);
	} else {		/* safe renegotiation not received... */

		if (priv && priv->connection_using_safe_renegotiation) {
			gnutls_assert();
			_gnutls_handshake_log
			    ("HSK[%p]: Peer previously asked for safe renegotiation\n",
			     session);
			return GNUTLS_E_SAFE_RENEGOTIATION_FAILED;
		}

		/* Clients can't tell if it's an initial negotiation */
		if (session->internals.initial_negotiation_completed) {
			if (session->internals.priorities.sr < SR_PARTIAL) {
				_gnutls_handshake_log
				    ("HSK[%p]: Allowing unsafe (re)negotiation\n",
				     session);
			} else {
				gnutls_assert();
				_gnutls_handshake_log
				    ("HSK[%p]: Denying unsafe (re)negotiation\n",
				     session);
				return
				    GNUTLS_E_UNSAFE_RENEGOTIATION_DENIED;
			}
		} else {
			if (session->internals.priorities.sr < SR_SAFE) {
				_gnutls_handshake_log
				    ("HSK[%p]: Allowing unsafe initial negotiation\n",
				     session);
			} else {
				gnutls_assert();
				_gnutls_handshake_log
				    ("HSK[%p]: Denying unsafe initial negotiation\n",
				     session);
				return GNUTLS_E_SAFE_RENEGOTIATION_FAILED;
			}
		}
	}

	return 0;
}

/* if a server received the special ciphersuite.
 */
int _gnutls_ext_sr_recv_cs(gnutls_session_t session)
{
	int ret, set = 0;
	sr_ext_st *priv;
	extension_priv_data_t epriv;

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					   &epriv);
	if (ret < 0) {
		set = 1;
	}

	if (set != 0) {
		priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		epriv = priv;
	} else
		priv = epriv;

	priv->safe_renegotiation_received = 1;
	priv->connection_using_safe_renegotiation = 1;
	_gnutls_extension_list_add(session, GNUTLS_EXTENSION_SAFE_RENEGOTIATION);

	if (set != 0)
		_gnutls_ext_set_session_data(session,
					     GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					     epriv);

	return 0;
}

int _gnutls_ext_sr_send_cs(gnutls_session_t session)
{
	int ret, set = 0;
	sr_ext_st *priv;
	extension_priv_data_t epriv;

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					   &epriv);
	if (ret < 0) {
		set = 1;
	}

	if (set != 0) {
		priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		epriv = priv;
	}

	if (set != 0)
		_gnutls_ext_set_session_data(session,
					     GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					     epriv);

	return 0;
}

static int
_gnutls_sr_recv_params(gnutls_session_t session,
		       const uint8_t * data, size_t _data_size)
{
	unsigned int len;
	ssize_t data_size = _data_size;
	sr_ext_st *priv;
	extension_priv_data_t epriv;
	int set = 0, ret;

	if (data_size == 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	len = data[0];
	DECR_LEN(data_size,
		 len + 1 /* count the first byte and payload */ );

	if (session->internals.priorities.sr == SR_DISABLED) {
		gnutls_assert();
		return 0;
	}

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					   &epriv);
	if (ret < 0
	    && session->security_parameters.entity == GNUTLS_SERVER) {
		set = 1;
	} else if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (set != 0) {
		priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		epriv = priv;

		_gnutls_ext_set_session_data(session,
					     GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					     epriv);
	} else {
		priv = epriv;
	}

	/* It is not legal to receive this extension on a renegotiation and
	 * not receive it on the initial negotiation.
	 */
	if (session->internals.initial_negotiation_completed != 0 &&
	    priv->connection_using_safe_renegotiation == 0) {
		gnutls_assert();
		return GNUTLS_E_SAFE_RENEGOTIATION_FAILED;
	}

	if (len > sizeof(priv->ri_extension_data)) {
		gnutls_assert();
		return GNUTLS_E_SAFE_RENEGOTIATION_FAILED;
	}

	if (len > 0)
		memcpy(priv->ri_extension_data, &data[1], len);
	priv->ri_extension_data_len = len;

	/* "safe renegotiation received" means on *this* handshake; "connection using
	 * safe renegotiation" means that the initial hello received on the connection
	 * indicated safe renegotiation.
	 */
	priv->safe_renegotiation_received = 1;
	priv->connection_using_safe_renegotiation = 1;

	return 0;
}

static int
_gnutls_sr_send_params(gnutls_session_t session,
		       gnutls_buffer_st * extdata)
{
	/* The format of this extension is a one-byte length of verify data followed
	 * by the verify data itself. Note that the length byte does not include
	 * itself; IOW, empty verify data is represented as a length of 0. That means
	 * the minimum extension is one byte: 0x00.
	 */
	sr_ext_st *priv;
	int ret, set = 0, len;
	extension_priv_data_t epriv;
	size_t init_length = extdata->length;

	if (session->internals.priorities.sr == SR_DISABLED) {
		gnutls_assert();
		return 0;
	}

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					   &epriv);
	if (ret < 0) {
		set = 1;
	}

	if (set != 0) {
		priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		epriv = priv;

		_gnutls_ext_set_session_data(session,
					     GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					     epriv);
	} else
		priv = epriv;

	/* Always offer the extension if we're a client */
	if (priv->connection_using_safe_renegotiation ||
	    session->security_parameters.entity == GNUTLS_CLIENT) {
		len = priv->client_verify_data_len;
		if (session->security_parameters.entity == GNUTLS_SERVER)
			len += priv->server_verify_data_len;

		ret = _gnutls_buffer_append_prefix(extdata, 8, len);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    _gnutls_buffer_append_data(extdata,
					       priv->client_verify_data,
					       priv->
					       client_verify_data_len);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (session->security_parameters.entity == GNUTLS_SERVER) {
			ret =
			    _gnutls_buffer_append_data(extdata,
						       priv->
						       server_verify_data,
						       priv->
						       server_verify_data_len);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
	} else
		return 0;

	return extdata->length - init_length;
}

static void _gnutls_sr_deinit_data(extension_priv_data_t priv)
{
	gnutls_free(priv);
}

/**
 * gnutls_safe_renegotiation_status:
 * @session: is a #gnutls_session_t type.
 *
 * Can be used to check whether safe renegotiation is being used
 * in the current session.
 *
 * Returns: 0 when safe renegotiation is not used and non (0) when
 *   safe renegotiation is used.
 *
 * Since: 2.10.0
 **/
unsigned gnutls_safe_renegotiation_status(gnutls_session_t session)
{
	int ret;
	sr_ext_st *priv;
	extension_priv_data_t epriv;

	ret = _gnutls_ext_get_session_data(session,
					   GNUTLS_EXTENSION_SAFE_RENEGOTIATION,
					   &epriv);
	if (ret < 0) {
		gnutls_assert();
		return 0;
	}
	priv = epriv;

	return priv->connection_using_safe_renegotiation;
}
