/*
 * Copyright (C) 2017 Red Hat, Inc.
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

/* Internal API functions to be used by extension handlers.
 */

#include "gnutls_int.h"
#include "hello_ext.h"
#include "hello_ext_lib.h"

void _gnutls_hello_ext_default_deinit(gnutls_ext_priv_data_t priv)
{
	gnutls_free(priv);
}

/* When this is used, the deinitialization function must be set to default:
 * _gnutls_hello_ext_default_deinit.
 *
 * This also prevents and errors on duplicate entries.
 */
int
_gnutls_hello_ext_set_datum(gnutls_session_t session,
			    extensions_t id, const gnutls_datum_t *data)
{
	gnutls_ext_priv_data_t epriv;

	if (_gnutls_hello_ext_get_priv(session, id, &epriv) >= 0)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	if (data->size >= UINT16_MAX)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	epriv = gnutls_malloc(data->size+2);
	if (epriv == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	_gnutls_write_uint16(data->size, epriv);
	memcpy(((uint8_t*)epriv)+2, data->data, data->size);

	_gnutls_hello_ext_set_priv(session, id, epriv);

	return 0;
}

int
_gnutls_hello_ext_get_datum(gnutls_session_t session,
			    extensions_t id, gnutls_datum_t *data /* constant contents */)
{
	gnutls_ext_priv_data_t epriv;
	int ret;

	ret = _gnutls_hello_ext_get_priv(session, id, &epriv);
	if (ret < 0 || epriv == NULL)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

	data->size = _gnutls_read_uint16(epriv);
	data->data = ((uint8_t*)epriv)+2;

	return 0;
}

int
_gnutls_hello_ext_get_resumed_datum(gnutls_session_t session,
				    extensions_t id, gnutls_datum_t *data /* constant contents */)
{
	gnutls_ext_priv_data_t epriv;
	int ret;

	ret = _gnutls_hello_ext_get_resumed_priv(session, id, &epriv);
	if (ret < 0 || epriv == NULL)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

	data->size = _gnutls_read_uint16(epriv);
	data->data = ((uint8_t*)epriv)+2;

	return 0;
}

int
_gnutls_hello_ext_default_pack(gnutls_ext_priv_data_t epriv, gnutls_buffer_st *ps)
{
	size_t size;

	size = _gnutls_read_uint16(epriv);

	return _gnutls_buffer_append_data(ps, epriv, size+2);
}

int
_gnutls_hello_ext_default_unpack(gnutls_buffer_st *ps, gnutls_ext_priv_data_t *epriv)
{
	gnutls_datum_t data;
	uint8_t *store;
	int ret;

	ret = _gnutls_buffer_pop_datum_prefix16(ps, &data);
	if (ret < 0)
		return gnutls_assert_val(ret);

	store = gnutls_calloc(1, data.size+2);
	if (store == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	_gnutls_write_uint16(data.size, store);
	memcpy(store+2, data.data, data.size);

	*epriv = store;
	return 0;
}
