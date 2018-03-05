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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_EXTENSIONS_H
#define GNUTLS_EXTENSIONS_H

#include <gnutls/gnutls.h>

int _gnutls_parse_extensions(gnutls_session_t session,
			     gnutls_ext_parse_type_t parse_type,
			     const uint8_t * data, int data_size);
int _gnutls_gen_extensions(gnutls_session_t session,
			   gnutls_buffer_st * extdata,
			   gnutls_ext_parse_type_t);
int _gnutls_ext_init(void);
void _gnutls_ext_deinit(void);

void _gnutls_extension_list_add(gnutls_session_t session, uint16_t type);
int _gnutls_extension_list_check(gnutls_session_t session, uint16_t type);

void _gnutls_ext_free_session_data(gnutls_session_t session);

/* functions to be used by extensions internally
 */
void _gnutls_ext_unset_session_data(gnutls_session_t session,
				    uint16_t type);
void _gnutls_ext_set_session_data(gnutls_session_t session, uint16_t type,
				  gnutls_ext_priv_data_t);
int _gnutls_ext_get_session_data(gnutls_session_t session, uint16_t type,
				 gnutls_ext_priv_data_t *);
int _gnutls_ext_get_resumed_session_data(gnutls_session_t session,
					 uint16_t type,
					 gnutls_ext_priv_data_t * data);

void _gnutls_ext_restore_resumed_session(gnutls_session_t session);

/* for session packing */
int _gnutls_ext_pack(gnutls_session_t session, gnutls_buffer_st * packed);
int _gnutls_ext_unpack(gnutls_session_t session,
		       gnutls_buffer_st * packed);

typedef gnutls_ext_priv_data_t extension_priv_data_t;

typedef struct extension_entry_st {
	const char *name; /* const overriden when free_struct is set */
	unsigned free_struct;

	uint16_t type;
	gnutls_ext_parse_type_t parse_type;

	/* this function must return 0 when Not Applicable
	 * size of extension data if ok
	 * < 0 on other error.
	 */
	gnutls_ext_recv_func recv_func;

	/* this function must return 0 when Not Applicable
	 * size of extension data if ok
	 * GNUTLS_E_INT_RET_0 if extension data size is zero
	 * < 0 on other error.
	 */
	gnutls_ext_send_func send_func;

	gnutls_ext_deinit_data_func deinit_func;	/* this will be called to deinitialize
							 * internal data 
							 */
	gnutls_ext_pack_func pack_func;	/* packs internal data to machine independent format */
	gnutls_ext_unpack_func unpack_func;	/* unpacks internal data */
} extension_entry_st;

int _gnutls_ext_register(extension_entry_st *);

#endif
