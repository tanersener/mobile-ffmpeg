/*
 * Generation of RSA keypairs.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2002 Niels Möller
 * Copyright (C) 2014 Red Hat
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nettle/rsa.h>
#include <dsa-fips.h>
#include <rsa-fips.h>

#include <nettle/bignum.h>

static int
rsa_provable_prime (mpz_t p,
			  unsigned *prime_seed_length, void *prime_seed,
			  unsigned bits,
			  unsigned seed_length, const void *seed,
			  mpz_t e,
			  void *progress_ctx, nettle_progress_func * progress)
{
mpz_t x, t, s, r1, r2, p0, sq;
int ret;
unsigned pcounter = 0;
unsigned iterations;
unsigned storage_length = 0, i;
uint8_t *storage = NULL;
uint8_t pseed[MAX_PVP_SEED_SIZE+1];
unsigned pseed_length = sizeof(pseed), tseed_length;
unsigned max = bits*5;

	mpz_init(p0);
	mpz_init(sq);
	mpz_init(x);
	mpz_init(t);
	mpz_init(s);
	mpz_init(r1);
	mpz_init(r2);

	/* p1 = p2 = 1 */

	ret = st_provable_prime(p0, &pseed_length, pseed,
				NULL, 1+div_ceil(bits,2), seed_length,
				seed, progress_ctx, progress);
	if (ret == 0) {
		goto cleanup;
	}

	iterations = div_ceil(bits, DIGEST_SIZE*8);
	mpz_set_ui(x, 0);

	if (iterations > 0) {
		storage_length = iterations * DIGEST_SIZE;
		storage = malloc(storage_length);
		if (storage == NULL) {
			goto fail;
		}

		nettle_mpz_set_str_256_u(s, pseed_length, pseed);
		for (i = 0; i < iterations; i++) {
			tseed_length = mpz_seed_sizeinbase_256_u(s, pseed_length);
			if (tseed_length > sizeof(pseed))
				goto fail;
			nettle_mpz_get_str_256(tseed_length, pseed, s);

			hash(&storage[(iterations - i - 1) * DIGEST_SIZE],
			     tseed_length, pseed);
			mpz_add_ui(s, s, 1);
		}

		nettle_mpz_set_str_256_u(x, storage_length, storage);
	}

	/* x = sqrt(2)*2^(bits-1) + (x mod 2^(bits) - sqrt(2)*2(bits-1)) */

	/* sq = sqrt(2)*2^(bits-1) */
	mpz_set_ui(r1, 1);
	mpz_mul_2exp(r1, r1, 2*bits-1);
	mpz_sqrt(sq, r1);

	/* r2 = 2^bits - sq */
	mpz_set_ui(r2, 1);
	mpz_mul_2exp(r2, r2, bits);
	mpz_sub(r2, r2, sq);

	/* x =  sqrt(2)*2^(bits-1) + (x mod (2^L - sqrt(2)*2^(bits-1)) */
	mpz_mod(x, x, r2);
	mpz_add(x, x, sq);

	/* y = p2 = p1 = 1 */

	/* r1 = (2 y p0 p1) */
	mpz_mul_2exp(r1, p0, 1);

	/* r2 = 2 p0 p1 p2 (p2=y=1) */
	mpz_set(r2, r1);

	/* r1 = (2 y p0 p1) + x */
	mpz_add(r1, r1, x);

	/* t = ((2 y p0 p1) + x) / (2 p0 p1 p2) */
	mpz_cdiv_q(t, r1, r2);

 retry:
	/* p = t p2 - y = t - 1 */
	mpz_sub_ui(p, t, 1);

	/* p = 2(tp2-y)p0p1 */
	mpz_mul(p, p, p0);
	mpz_mul_2exp(p, p, 1);

	/* p = 2(tp2-y)p0p1 + 1*/
	mpz_add_ui(p, p, 1);

	mpz_set_ui(r2, 1);
	mpz_mul_2exp(r2, r2, bits);

	if (mpz_cmp(p, r2) > 0) {
		/* t = (2 y p0 p1) + sqrt(2)*2^(bits-1) / (2p0p1p2) */
		mpz_set(r1, p0);
		/* r1 = (2 y p0 p1) */
		mpz_mul_2exp(r1, r1, 1);

		/* sq =  sqrt(2)*2^(bits-1) */

		/* r1 = (2 y p0 p1) + sq */
		mpz_add(r1, r1, sq);

		/* r2 = 2 p0 p1 p2 */
		mpz_mul_2exp(r2, p0, 1);

		/* t = ((2 y p0 p1) + sq) / (2 p0 p1 p2) */
		mpz_cdiv_q(t, r1, r2);
	}

	pcounter++;

	/* r2 = p - 1 */
	mpz_sub_ui(r2, p, 1);

	/* r1 = GCD(p1, e) */
	mpz_gcd(r1, e, r2);

	if (mpz_cmp_ui(r1, 1) == 0) {
		mpz_set_ui(x, 0); /* a = 0 */
		if (iterations > 0) {
			for (i = 0; i < iterations; i++) {
				tseed_length = mpz_seed_sizeinbase_256_u(s, pseed_length);
				if (tseed_length > sizeof(pseed))
					goto fail;
				nettle_mpz_get_str_256(tseed_length, pseed, s);

				hash(&storage[(iterations - i - 1) * DIGEST_SIZE],
				     tseed_length, pseed);
				mpz_add_ui(s, s, 1);
			}

			nettle_mpz_set_str_256_u(x, storage_length, storage);
		}

		/* a = 2 + a mod p-3 */
		mpz_sub_ui(r1, p, 3);	/* p is too large to worry about negatives */
		mpz_mod(x, x, r1);
		mpz_add_ui(x, x, 2);

		/* z = a^(2(tp2-y)p1) mod p */

		/* r1 = (tp2-y) */
		mpz_sub_ui(r1, t, 1);
		/* r1 = 2(tp2-y)p1 */
		mpz_mul_2exp(r1, r1, 1);

		/* z = r2 = a^r1 mod p */
		mpz_powm(r2, x, r1, p);

		mpz_sub_ui(r1, r2, 1);
		mpz_gcd(x, r1, p);

		if (mpz_cmp_ui(x, 1) == 0) {
			mpz_powm(r1, r2, p0, p);
			if (mpz_cmp_ui(r1, 1) == 0) {
				if (prime_seed_length != NULL) {
					tseed_length = mpz_seed_sizeinbase_256_u(s, pseed_length);
					if (tseed_length > sizeof(pseed))
						goto fail;

					nettle_mpz_get_str_256(tseed_length, pseed, s);

					if (*prime_seed_length < tseed_length) {
						*prime_seed_length = tseed_length;
						goto fail;
					}
					*prime_seed_length = tseed_length;
					if (prime_seed != NULL)
						memcpy(prime_seed, pseed, tseed_length);
				}
				ret = 1;
				goto cleanup;
			}
		}
	}

	if (pcounter >= max) {
		goto fail;
	}

	mpz_add_ui(t, t, 1);
	goto retry;

fail:
	ret = 0;
cleanup:
	free(storage);
	mpz_clear(p0);
	mpz_clear(sq);
	mpz_clear(r1);
	mpz_clear(r2);
	mpz_clear(x);
	mpz_clear(t);
	mpz_clear(s);

	return ret;
}

/* This generates p,q params using the B.3.2.2 algorithm in FIPS 186-4.
 * 
 * The hash function used is SHA384.
 * The exponent e used is the value in pub->e.
 */
int
_rsa_generate_fips186_4_keypair(struct rsa_public_key *pub,
				struct rsa_private_key *key,
				unsigned seed_length, uint8_t * seed,
				void *progress_ctx,
				nettle_progress_func * progress,
				/* Desired size of modulo, in bits */
				unsigned n_size)
{
	mpz_t t, r, p1, q1, lcm;
	int ret;
	struct dss_params_validation_seeds cert;
	unsigned l = n_size / 2;

	FIPS_RULE(n_size == 2048 && seed_length != 14 * 2, 0, "seed length other than 28 bytes\n");
	FIPS_RULE(n_size == 3072 && seed_length != 16 * 2, 0, "seed length other than 32 bytes\n");
	FIPS_RULE(n_size != 2048 && n_size != 3072, 0, "unsupported size for modulus\n");

	if (!mpz_tstbit(pub->e, 0)) {
		_gnutls_debug_log("Unacceptable e (it is even)\n");
		return 0;
	}

	if (mpz_cmp_ui(pub->e, 65536) <= 0) {
		_gnutls_debug_log("Unacceptable e\n");
		return 0;
	}

	mpz_init(p1);
	mpz_init(q1);
	mpz_init(lcm);
	mpz_init(t);
	mpz_init(r);

	mpz_set_ui(t, 1);
	mpz_mul_2exp(t, t, 256);

	if (mpz_cmp(pub->e, t) >= 0) {
		ret = 0;
		goto cleanup;
	}

	cert.pseed_length = sizeof(cert.pseed);
	ret = rsa_provable_prime(key->p, &cert.pseed_length, cert.pseed,
				l, seed_length,
				seed, pub->e, progress_ctx, progress);
	if (ret == 0) {
		goto cleanup;
	}

	mpz_set_ui(r, 1);
	mpz_mul_2exp(r, r, (l) - 100);

	do {
		cert.qseed_length = sizeof(cert.qseed);
		ret = rsa_provable_prime(key->q, &cert.qseed_length, cert.qseed,
					l, cert.pseed_length, cert.pseed,
					pub->e,
					progress_ctx, progress);
		if (ret == 0) {
			goto cleanup;
		}


		cert.pseed_length = cert.qseed_length;
		memcpy(cert.pseed, cert.qseed, cert.qseed_length);


		if (mpz_cmp(key->p, key->q) > 0)
			mpz_sub(t, key->p, key->q);
		else
			mpz_sub(t, key->q, key->p);
	} while (mpz_cmp(t, r) <= 0);

	memset(&cert, 0, sizeof(cert));

	mpz_mul(pub->n, key->p, key->q);

	if (mpz_sizeinbase(pub->n, 2) != n_size) {
		ret = 0;
		goto cleanup;
	}

	/* c = q^{-1} (mod p) */
	if (mpz_invert(key->c, key->q, key->p) == 0) {
		ret = 0;
		goto cleanup;
	}

	mpz_sub_ui(p1, key->p, 1);
	mpz_sub_ui(q1, key->q, 1);

	mpz_lcm(lcm, p1, q1);

	if (mpz_invert(key->d, pub->e, lcm) == 0) {
		ret = 0;
		goto cleanup;
	}

	/* check whether d > 2^(nlen/2) -- FIPS186-4 5.3.1 */
	if (mpz_sizeinbase(key->d, 2) < n_size/2) {
		ret = 0;
		goto cleanup;
	}

	/* Done! Almost, we must compute the auxiliary private values. */
	/* a = d % (p-1) */
	mpz_fdiv_r(key->a, key->d, p1);

	/* b = d % (q-1) */
	mpz_fdiv_r(key->b, key->d, q1);

	/* c was computed earlier */

	pub->size = key->size = (n_size + 7) / 8;
	if (pub->size < RSA_MINIMUM_N_OCTETS) {
		ret = 0;
		goto cleanup;
	}

	ret = 1;
 cleanup:
	mpz_clear(p1);
	mpz_clear(q1);
	mpz_clear(lcm);
	mpz_clear(t);
	mpz_clear(r);
	return ret;
}

/* Not entirely accurate but a good precision
 */
#define SEED_LENGTH(bits) (_gnutls_pk_bits_to_subgroup_bits(bits)/8)

/* This generates p,q params using the B.3.2.2 algorithm in FIPS 186-4.
 * 
 * The hash function used is SHA384.
 * The exponent e used is the value in pub->e.
 */
int
rsa_generate_fips186_4_keypair(struct rsa_public_key *pub,
			       struct rsa_private_key *key,
			       void *random_ctx, nettle_random_func * random,
			       void *progress_ctx,
			       nettle_progress_func * progress,
			       unsigned *rseed_size,
			       void *rseed,
			       /* Desired size of modulo, in bits */
			       unsigned n_size)
{
	uint8_t seed[128];
	unsigned seed_length;
	int ret;

	FIPS_RULE(n_size != 2048 && n_size != 3072, 0, "size of prime of other than 2048 or 3072\n");

	seed_length = SEED_LENGTH(n_size);
	if (seed_length > sizeof(seed))
		return 0;

	random(random_ctx, seed_length, seed);

	if (rseed && rseed_size) {
		if (*rseed_size < seed_length) {
			return 0;
		}
		memcpy(rseed, seed, seed_length);
		*rseed_size = seed_length;
	}

	ret = _rsa_generate_fips186_4_keypair(pub, key, seed_length, seed,
					       progress_ctx, progress, n_size);
	gnutls_memset(seed, 0, seed_length);
	return ret;
}
