/*
 * GnuTLS PIN support for PKCS#11 or TPM
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * 
 * Authors: Nikos Mavrogiannopoulos
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
 */

#include "gnutls_int.h"
#include <gnutls/pkcs11.h>
#include <pin.h>
#include "errors.h"

gnutls_pin_callback_t _gnutls_pin_func;
void *_gnutls_pin_data;

/**
 * gnutls_pkcs11_set_pin_function:
 * @fn: The PIN callback, a gnutls_pin_callback_t() function.
 * @userdata: data to be supplied to callback
 *
 * This function will set a callback function to be used when a PIN is
 * required for PKCS 11 operations.  See
 * gnutls_pin_callback_t() on how the callback should behave.
 *
 * Since: 2.12.0
 **/
void
gnutls_pkcs11_set_pin_function(gnutls_pin_callback_t fn, void *userdata)
{
	_gnutls_pin_func = fn;
	_gnutls_pin_data = userdata;
}

/**
 * gnutls_pkcs11_get_pin_function:
 * @userdata: data to be supplied to callback
 *
 * This function will return the callback function set using
 * gnutls_pkcs11_set_pin_function().
 *
 * Returns: The function set or NULL otherwise.
 * 
 * Since: 3.1.0
 **/
gnutls_pin_callback_t gnutls_pkcs11_get_pin_function(void **userdata)
{
	if (_gnutls_pin_func != NULL) {
		*userdata = _gnutls_pin_data;
		return _gnutls_pin_func;
	}
	return NULL;
}

int
_gnutls_retrieve_pin(struct pin_info_st *pin_info, const char *url, const char *label,
		     unsigned flags,
		     char pin[GNUTLS_PKCS11_MAX_PIN_LEN], unsigned pin_size)
{
	int ret;

	if (pin_info && pin_info->cb)
		ret =
		    pin_info->cb(pin_info->data, 0,
				 (char *) url, label, flags,
				 pin, pin_size);
	else if (_gnutls_pin_func)
		ret =
		    _gnutls_pin_func(_gnutls_pin_data, 0,
				     (char *) url, label, flags,
				     pin, pin_size);
	else
		ret = gnutls_assert_val(GNUTLS_E_PKCS11_PIN_ERROR);

	return ret;
}
