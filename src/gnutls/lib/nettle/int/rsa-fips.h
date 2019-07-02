/* rsa-fips.h
 *
 * The RSA publickey algorithm.
 */

/* Copyright (C) 2014 Red Hat
 *  
 * The gnutls library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02111-1301, USA.
 */

#ifndef GNUTLS_LIB_NETTLE_INT_RSA_FIPS_H
#define GNUTLS_LIB_NETTLE_INT_RSA_FIPS_H

#include <nettle/rsa.h>

int
_rsa_generate_fips186_4_keypair(struct rsa_public_key *pub,
				struct rsa_private_key *key,
				unsigned seed_length, uint8_t * seed,
				void *progress_ctx,
				nettle_progress_func * progress,
				/* Desired size of modulo, in bits */
				unsigned n_size);

int
rsa_generate_fips186_4_keypair(struct rsa_public_key *pub,
			       struct rsa_private_key *key,
			       void *random_ctx, nettle_random_func * random,
			       void *progress_ctx,
			       nettle_progress_func * progress,
			       unsigned *rseed_size,
			       void *rseed,
			       /* Desired size of modulo, in bits */
			       unsigned n_size);

#endif /* GNUTLS_LIB_NETTLE_INT_RSA_FIPS_H */
