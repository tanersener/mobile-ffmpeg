/*
 * Copyright (C) 2018 Free Software Foundation, Inc.
 *
 * Author: Ander Juaristi
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

#ifndef GNUTLS_LIB_STEK_H
#define GNUTLS_LIB_STEK_H

#include "gnutls_int.h"

int _gnutls_get_session_ticket_encryption_key(gnutls_session_t session,
					      gnutls_datum_t *key_name,
					      gnutls_datum_t *mac_key,
					      gnutls_datum_t *enc_key);
int _gnutls_get_session_ticket_decryption_key(gnutls_session_t session,
					      const gnutls_datum_t *ticket_data,
					      gnutls_datum_t *key_name,
					      gnutls_datum_t *mac_key,
					      gnutls_datum_t *enc_key);

void _gnutls_set_session_ticket_key_rotation_callback(gnutls_session_t session,
		gnutls_stek_rotation_callback_t cb);

int _gnutls_initialize_session_ticket_key_rotation(gnutls_session_t session,
		const gnutls_datum_t *key);

#endif /* GNUTLS_LIB_STEK_H */
