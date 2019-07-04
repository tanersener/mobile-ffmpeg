/* dsa.h
 *
 * The DSA publickey algorithm.
 */

/* Copyright (C) 2013 Red Hat
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
 
#ifndef GNUTLS_LIB_NETTLE_INT_DSA_FIPS_H
#define GNUTLS_LIB_NETTLE_INT_DSA_FIPS_H

#include <nettle/bignum.h> /* includes gmp.h */
#include <nettle/dsa.h>
#include <nettle/sha2.h>
#include <fips.h>

#define div_ceil(x,y) ((x+(y)-1)/(y))

struct dss_params_validation_seeds {
	unsigned seed_length; /* first seed */
	uint8_t seed[MAX_PVP_SEED_SIZE+1];
	
	unsigned pseed_length;
	uint8_t pseed[MAX_PVP_SEED_SIZE+1];
	unsigned qseed_length;
	uint8_t qseed[MAX_PVP_SEED_SIZE+1];
	unsigned pgen_counter;
	unsigned qgen_counter;
};

int
st_provable_prime (mpz_t p,
			  unsigned *prime_seed_length, void *prime_seed,
			  unsigned *prime_gen_counter,
			  unsigned bits,
			  unsigned seed_length, const void *seed,
			  void *progress_ctx, nettle_progress_func * progress);

int
dsa_generate_dss_pqg(struct dsa_params *params,
		     struct dss_params_validation_seeds* cert,
		     unsigned index,
		     void *random_ctx, nettle_random_func *random,
		     void *progress_ctx, nettle_progress_func *progress,
		     unsigned p_bits /* = L */, unsigned q_bits /* = N */);

int
_dsa_generate_dss_pqg(struct dsa_params *params,
			 struct dss_params_validation_seeds *cert,
			 unsigned index,
			 unsigned seed_size, void *seed,
			 void *progress_ctx, nettle_progress_func * progress,
			 unsigned p_bits /* = L */ , unsigned q_bits /* = N */ );

int
dsa_generate_dss_keypair(struct dsa_params *params,
		     mpz_t y,
		     mpz_t x,
		     void *random_ctx, nettle_random_func *random,
		     void *progress_ctx, nettle_progress_func *progress);

int
dsa_validate_dss_pqg(struct dsa_params *pub,
		     struct dss_params_validation_seeds* cert,
		     unsigned index);

int
_dsa_validate_dss_pq(struct dsa_params *pub,
		     struct dss_params_validation_seeds* cert);

int
_dsa_validate_dss_g(struct dsa_params *pub,
		    unsigned domain_seed_size, const uint8_t *domain_seed, unsigned index);

unsigned _dsa_check_qp_sizes(unsigned q_bits, unsigned p_bits, unsigned generate);

/* The following low-level functions can be used for DH key exchange as well 
 */
int
_dsa_generate_dss_pq(struct dsa_params *pub,
		     struct dss_params_validation_seeds* cert,
		     unsigned seed_length, void* seed,
		     void *progress_ctx, nettle_progress_func *progress,
		     unsigned p_bits, unsigned q_bits);

int
_dsa_generate_dss_g(struct dsa_params *pub,
		    unsigned domain_seed_size, const uint8_t* domain_seed,
		    void *progress_ctx, nettle_progress_func * progress,
		    unsigned index);

void
_dsa_generate_dss_xy(struct dsa_params *pub,
		     mpz_t y,
		     mpz_t x,
		     void *random_ctx, nettle_random_func *random);

#define DIGEST_SIZE SHA384_DIGEST_SIZE
inline static void
hash (uint8_t digest[DIGEST_SIZE], unsigned length, void *data)
{
  struct sha384_ctx ctx;

  sha384_init (&ctx);
  sha384_update (&ctx, length, data);
  sha384_digest (&ctx, DIGEST_SIZE, digest);

  return;
}

unsigned mpz_seed_sizeinbase_256_u(mpz_t s, unsigned nominal);

#endif /* GNUTLS_LIB_NETTLE_INT_DSA_FIPS_H */
