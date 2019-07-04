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

#ifndef GNUTLS_LIB_HELLO_EXT_LIB_H
#define GNUTLS_LIB_HELLO_EXT_LIB_H

#include <gnutls/gnutls.h>
#include "hello_ext.h"

/* Functions to use at the send() or recv() extension function to temporarily
 * store and retrieve data related to the extension.
 */
int
_gnutls_hello_ext_set_datum(gnutls_session_t session,
			    extensions_t id, const gnutls_datum_t *data);
int
_gnutls_hello_ext_get_datum(gnutls_session_t session,
			    extensions_t id, gnutls_datum_t *data /* constant contents */);

int
_gnutls_hello_ext_get_resumed_datum(gnutls_session_t session,
				    extensions_t id, gnutls_datum_t *data /* constant contents */);

/* clear up any set data for the extension */
#if 0 /* defined in hello_ext.h */
void
_gnutls_hello_ext_unset_priv(gnutls_session_t session,
                              extensions_t id);
#endif

/* Function that will deinitialize the temporal data. Must be set
 * as the deinit_func in the hello_ext_entry_st if the functions above
 * are used.
 */
void _gnutls_hello_ext_default_deinit(gnutls_ext_priv_data_t priv);

/* Functions to pack and unpack data if they need to be stored at
 * session resumption data. Must be set as the pack_func and unpack_func
 * of hello_ext_entry_st if the set and get functions above are used,
 * and data must be accessible on resumed sessions.
 */
int
_gnutls_hello_ext_default_pack(gnutls_ext_priv_data_t epriv, gnutls_buffer_st *ps);

int
_gnutls_hello_ext_default_unpack(gnutls_buffer_st *ps, gnutls_ext_priv_data_t *epriv);

#endif /* GNUTLS_LIB_HELLO_EXT_LIB_H */
