/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

#ifndef AUTH_ECDH_COMMON
#define AUTH_ECDH_COMMON

#include <auth.h>

int
_gnutls_gen_ecdh_common_client_kx(gnutls_session_t session,
				  gnutls_buffer_st * data);

int
_gnutls_gen_ecdh_common_client_kx_int(gnutls_session_t session,
				      gnutls_buffer_st * data,
				      gnutls_datum_t * psk_key);

int
_gnutls_proc_ecdh_common_client_kx(gnutls_session_t session,
				   uint8_t * data, size_t _data_size,
				   gnutls_ecc_curve_t curve,
				   gnutls_datum_t * psk_key);

int _gnutls_ecdh_common_print_server_kx(gnutls_session_t,
					gnutls_buffer_st * data,
					gnutls_ecc_curve_t curve);
int _gnutls_proc_ecdh_common_server_kx(gnutls_session_t session,
				       uint8_t * data, size_t _data_size);



#endif
