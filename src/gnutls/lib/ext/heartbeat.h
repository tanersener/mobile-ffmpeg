/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
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

#ifndef EXT_HEARTBEAT_H
#define EXT_HEARTBEAT_H

#include <extensions.h>

#define HEARTBEAT_REQUEST 1
#define HEARTBEAT_RESPONSE 2

#define MAX_HEARTBEAT_LENGTH DEFAULT_MAX_RECORD_SIZE

#define LOCAL_ALLOWED_TO_SEND (1<<2)
#define LOCAL_NOT_ALLOWED_TO_SEND (1<<3)

#define HEARTBEAT_DEFAULT_POLICY PEER_NOT_ALLOWED_TO_SEND

extern const extension_entry_st ext_mod_heartbeat;

int _gnutls_heartbeat_handle(gnutls_session_t session, mbuffer_st * bufel);
int _gnutls_heartbeat_enabled(gnutls_session_t session, int local);
#endif
