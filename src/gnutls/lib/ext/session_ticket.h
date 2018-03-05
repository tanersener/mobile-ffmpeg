/*
 * Copyright (C) 2009-2012 Free Software Foundation, Inc.
 *
 * Author: Daiki Ueno
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

#ifndef EXT_SESSION_TICKET_H
#define EXT_SESSION_TICKET_H

#include <extensions.h>

extern const extension_entry_st ext_mod_session_ticket;

int _gnutls_send_new_session_ticket(gnutls_session_t session, int again);
int _gnutls_recv_new_session_ticket(gnutls_session_t session);

#endif
