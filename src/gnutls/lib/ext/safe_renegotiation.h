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

#ifndef EXT_SAFE_RENEGOTIATION_H
#define EXT_SAFE_RENEGOTIATION_H

#include <extensions.h>

typedef struct {
	uint8_t client_verify_data[MAX_VERIFY_DATA_SIZE];
	size_t client_verify_data_len;
	uint8_t server_verify_data[MAX_VERIFY_DATA_SIZE];
	size_t server_verify_data_len;
	uint8_t ri_extension_data[MAX_VERIFY_DATA_SIZE * 2];	/* max signal is 72 bytes in s->c sslv3 */
	size_t ri_extension_data_len;

	unsigned int safe_renegotiation_received:1;
	unsigned int initial_negotiation_completed:1;
	unsigned int connection_using_safe_renegotiation:1;
} sr_ext_st;

extern const extension_entry_st ext_mod_sr;

int _gnutls_ext_sr_finished(gnutls_session_t session, void *vdata,
			    size_t vdata_size, int dir);
int _gnutls_ext_sr_recv_cs(gnutls_session_t session);
int _gnutls_ext_sr_verify(gnutls_session_t session);
int _gnutls_ext_sr_send_cs(gnutls_session_t);

#endif				/* EXT_SAFE_RENEGOTIATION_H */
