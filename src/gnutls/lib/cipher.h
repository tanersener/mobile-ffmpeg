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

#ifndef GNUTLS_LIB_CIPHER_H
#define GNUTLS_LIB_CIPHER_H

int _gnutls_encrypt(gnutls_session_t session,
		    const uint8_t * data,
		    size_t data_size, size_t min_pad,
		    mbuffer_st * bufel,
		    content_type_t type, record_parameters_st * params);

int _gnutls_decrypt(gnutls_session_t session,
		    gnutls_datum_t * ciphertext, gnutls_datum_t * output,
		    content_type_t *type, record_parameters_st * params,
		    gnutls_uint64 * sequence);

#define MAX_PREAMBLE_SIZE 16

int
_gnutls_make_preamble(uint8_t * uint64_data, uint8_t type, unsigned int length,
		      const version_entry_st * ver, uint8_t preamble[MAX_PREAMBLE_SIZE]);

int cbc_mac_verify(gnutls_session_t session, record_parameters_st *params,
		   uint8_t preamble[MAX_PREAMBLE_SIZE],
		   content_type_t type,
		   gnutls_uint64 *sequence,
		   const uint8_t *data, size_t data_size,
		   size_t tag_size);

#endif /* GNUTLS_LIB_CIPHER_H */
