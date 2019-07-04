/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_DB_H
#define GNUTLS_LIB_DB_H

int _gnutls_server_register_current_session(gnutls_session_t session);
int _gnutls_server_restore_session(gnutls_session_t session,
				   uint8_t * session_id,
				   int session_id_size);

int _gnutls_check_resumed_params(gnutls_session_t session);

#define PACKED_SESSION_MAGIC ((0xfadebadd)+(_gnutls_global_version))

#endif /* GNUTLS_LIB_DB_H */
