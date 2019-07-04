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

#ifndef GNUTLS_LIB_NETTLE_INT_TLS1_PRF_H
#define GNUTLS_LIB_NETTLE_INT_TLS1_PRF_H

#include <nettle/nettle-meta.h>

#define MAX_SEED_SIZE 200
#define MAX_PRF_BYTES 200

/* Namespace mangling */
#define tls10_prf nettle_tls10_prf
#define tls12_prf nettle_tls12_prf

int
tls10_prf(size_t secret_size, const uint8_t *secret,
	  size_t label_size, const char *label,
	  size_t seed_size, const uint8_t *seed,
	  size_t length, uint8_t *dst);

int
tls12_prf(void *mac_ctx,
	  nettle_hash_update_func *update,
	  nettle_hash_digest_func *digest,
	  size_t digest_size,
	  size_t label_size, const char *label,
	  size_t seed_size, const uint8_t *seed,
	  size_t length, uint8_t *dst);

#endif /* GNUTLS_LIB_NETTLE_INT_TLS1_PRF_H */
