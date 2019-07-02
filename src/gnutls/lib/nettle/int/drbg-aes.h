/* drbg-aes.h
 *
 * The CTR-AES-256-based random-number generator from SP800-90A.
 */

/* Copyright (C) 2013 Red Hat
 *  
 * The nettle library is free software; you can redistribute it and/or modify
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

#ifndef GNUTLS_LIB_NETTLE_INT_DRBG_AES_H
#define GNUTLS_LIB_NETTLE_INT_DRBG_AES_H

#include <config.h>
#include <nettle/aes.h>

/* This is nettle's increment macro */
/* Requires that size > 0 */
#define INCREMENT(size, ctr)                    \
  do {                                          \
    unsigned increment_i = (size) - 1;          \
    if (++(ctr)[increment_i] == 0)              \
      while (increment_i > 0                    \
             && ++(ctr)[--increment_i] == 0 )   \
        ;                                       \
  } while (0)

#define DRBG_AES_KEY_SIZE AES256_KEY_SIZE
#define DRBG_AES_SEED_SIZE (AES_BLOCK_SIZE+DRBG_AES_KEY_SIZE)

/* This is the CTR-AES-256-based random-number generator from SP800-90A.
 */
struct drbg_aes_ctx {
	unsigned seeded;
	/* The current key */
	struct aes256_ctx key;

	uint8_t v[AES_BLOCK_SIZE];

	unsigned reseed_counter;
};

/* max_number_of_bits_per_request */
#define MAX_DRBG_AES_GENERATE_SIZE 65536 /* 2^19 */

/* This DRBG should be reseeded if reseed_counter exceeds
 * that number. Otherwise drbg_aes_random() will fail.
 */
#define DRBG_AES_RESEED_TIME 16777216

/* The entropy provided in these functions should be of
 * size DRBG_AES_SEED_SIZE. Additional data and pers.
 * string may be <= DRBG_AES_SEED_SIZE.
 */
int
drbg_aes_init(struct drbg_aes_ctx *ctx, 
	unsigned entropy_size, const uint8_t *entropy, 
	unsigned pstring_size, const uint8_t* pstring);

int
drbg_aes_reseed(struct drbg_aes_ctx *ctx, 
	unsigned entropy_size, const uint8_t *entropy, 
	unsigned add_size, const uint8_t* add);

/* our wrapper for the low-level drbg_aes_generate */
int
drbg_aes_random(struct drbg_aes_ctx *ctx, unsigned length,
		uint8_t * dst);

int
drbg_aes_generate(struct drbg_aes_ctx *ctx, unsigned length,
		uint8_t * dst, unsigned add_size, const uint8_t* add);

/* For deinitialization use zeroize_key() on the context */

int drbg_aes_is_seeded(struct drbg_aes_ctx *ctx);

int drbg_aes_self_test(void);

#endif /* GNUTLS_LIB_NETTLE_INT_DRBG_AES_H */
