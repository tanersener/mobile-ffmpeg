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

#ifndef GNUTLS_LIB_SECRETS_H
#define GNUTLS_LIB_SECRETS_H

#include "gnutls_int.h"

int _tls13_init_secret(gnutls_session_t session, const uint8_t *psk, size_t psk_size);
int _tls13_update_secret(gnutls_session_t session, const uint8_t *key, size_t key_size);

int _tls13_init_secret2(const mac_entry_st *prf,
		const uint8_t *psk, size_t psk_size,
		void *out);

int _tls13_derive_secret(gnutls_session_t session,
			 const char *label, unsigned label_size,
			 const uint8_t *msg, size_t msg_size,
			 const uint8_t secret[MAX_HASH_SIZE],
			 void *out /* of enough length to hold PRF MAC */);
int _tls13_derive_secret2(const mac_entry_st *prf,
			  const char *label, unsigned label_size,
			  const uint8_t *tbh, size_t tbh_size,
			  const uint8_t secret[MAX_CIPHER_KEY_SIZE],
			  void *out);

int _tls13_expand_secret(gnutls_session_t session,
			 const char *label, unsigned label_size,
			 const uint8_t *msg, size_t msg_size,
			 const uint8_t secret[MAX_HASH_SIZE],
			 unsigned out_size,
			 void *out);
int _tls13_expand_secret2(const mac_entry_st *prf,
			  const char *label, unsigned label_size,
			  const uint8_t *msg, size_t msg_size,
			  const uint8_t secret[MAX_CIPHER_KEY_SIZE],
			  unsigned out_size, void *out);

#endif /* GNUTLS_LIB_SECRETS_H */
