/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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

#ifndef GNUTLS_LIB_EXT_STATUS_REQUEST_H
#define GNUTLS_LIB_EXT_STATUS_REQUEST_H

#include <hello_ext.h>

#define STATUS_REQUEST_TLS_ID 5

extern const hello_ext_entry_st ext_mod_status_request;

int
_gnutls_send_server_certificate_status(gnutls_session_t session,
				       int again);
int _gnutls_recv_server_certificate_status(gnutls_session_t session);

int _gnutls_parse_ocsp_response(gnutls_session_t session, const uint8_t *data,
				ssize_t data_size,
				gnutls_datum_t *resp);

#endif /* GNUTLS_LIB_EXT_STATUS_REQUEST_H */
