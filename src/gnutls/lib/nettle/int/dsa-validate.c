/* dsa-keygen.c
 *
 * Generation of DSA keypairs
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2013 Red Hat
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
#include <string.h>

#include <nettle/dsa.h>
#include <dsa-fips.h>

#include <nettle/bignum.h>

/* This validates the given p, q, g params using the provided
 * dss_params_validation_seeds.
 * 
 * The hash function used is SHA384.
 * 
 * pub: The output public key
 * key: The output private key
 * cert: A certificate that can be used to verify the generated parameters
 * index: 1 for digital signatures (DSA), 2 for key establishment (DH)
 * 
 */
int
dsa_validate_dss_pqg(struct dsa_params *pub,
		     struct dss_params_validation_seeds *cert, unsigned index)
{
	int ret;
	uint8_t domain_seed[MAX_PVP_SEED_SIZE*3];
	unsigned domain_seed_size = 0;

	ret = _dsa_validate_dss_pq(pub, cert);
	if (ret == 0)
		return 0;

	domain_seed_size = cert->seed_length + cert->qseed_length + cert->pseed_length;
	memcpy(domain_seed, cert->seed, cert->seed_length);
	memcpy(&domain_seed[cert->seed_length], cert->pseed, cert->pseed_length);
	memcpy(&domain_seed[cert->seed_length+cert->pseed_length], cert->qseed, cert->qseed_length);

	ret = _dsa_validate_dss_g(pub, domain_seed_size, domain_seed, index);
	if (ret == 0)
		return 0;

	return 1;
}

int
_dsa_validate_dss_g(struct dsa_params *pub,
		    unsigned domain_seed_size, const uint8_t *domain_seed, unsigned index)
{
	int ret;
	unsigned p_bits, q_bits;
	struct dsa_params pub2;
	mpz_t r;

	p_bits = mpz_sizeinbase(pub->p, 2);
	q_bits = mpz_sizeinbase(pub->q, 2);

	ret = _dsa_check_qp_sizes(q_bits, p_bits, 0);
	if (ret == 0) {
		return 0;
	}

	mpz_init(r);
	dsa_params_init(&pub2);

	mpz_set(pub2.p, pub->p);
	mpz_set(pub2.q, pub->q);

	/* verify g */
	if (index > 255) {
		goto fail;
	}

	/* 2<= g <= p-1 */
	mpz_set(r, pub->p);
	mpz_sub_ui(r, r, 1);
	if (mpz_cmp_ui(pub->g, 2) < 0 || mpz_cmp(pub->g, r) >= 0) {
		goto fail;
	}

	/* g^q == 1 mod p */
	mpz_powm(r, pub->g, pub->q, pub->p);
	if (mpz_cmp_ui(r, 1) != 0) {
		goto fail;
	}

	/* repeat g generation */
	ret = _dsa_generate_dss_g(&pub2,
				  domain_seed_size, domain_seed,
				  NULL, NULL, index);
	if (ret == 0) {
		goto fail;
	}

	if (mpz_cmp(pub->g, pub2.g) != 0) {
		goto fail;
	}

	/* everything looks ok */
	ret = 1;
	goto finish;

 fail:
	ret = 0;

 finish:
	dsa_params_clear(&pub2);
	mpz_clear(r);

	return ret;
}

int
_dsa_validate_dss_pq(struct dsa_params *pub,
		     struct dss_params_validation_seeds *cert)
{
	int ret;
	unsigned p_bits, q_bits;
	struct dsa_params pub2;
	struct dss_params_validation_seeds cert2;
	mpz_t r, s;

	p_bits = mpz_sizeinbase(pub->p, 2);
	q_bits = mpz_sizeinbase(pub->q, 2);

	ret = _dsa_check_qp_sizes(q_bits, p_bits, 0);
	if (ret == 0) {
		return 0;
	}

	mpz_init(r);
	mpz_init(s);
	dsa_params_init(&pub2);

	nettle_mpz_set_str_256_u(s, cert->seed_length, cert->seed);

	/* firstseed < 2^(N-1) */
	mpz_set_ui(r, 1);
	mpz_mul_2exp(r, r, q_bits - 1);

	if (mpz_cmp(s, r) < 0) {
		goto fail;
	}

	/* 2^N <= q */
	mpz_set_ui(r, 1);
	mpz_mul_2exp(r, r, q_bits);

	if (mpz_cmp(r, pub->q) <= 0) {
		goto fail;
	}

	/* 2^L <= p */
	mpz_set_ui(r, 1);
	mpz_mul_2exp(r, r, p_bits);

	if (mpz_cmp(r, pub->p) <= 0) {
		goto fail;
	}

	/* p-1 mod q != 0 */
	mpz_set(r, pub->p);
	mpz_sub_ui(r, r, 1);

	mpz_mod(r, r, pub->q);
	if (mpz_cmp_ui(r, 0) != 0) {
		goto fail;
	}

	/* replay the construction */
	ret = _dsa_generate_dss_pq(&pub2, &cert2, cert->seed_length, cert->seed,
				   NULL, NULL, p_bits, q_bits);
	if (ret == 0) {
		goto fail;
	}

	if ((cert->pseed_length > 0 && cert->pseed_length != cert2.pseed_length)
	    || (cert->qseed_length > 0
		&& cert->qseed_length != cert2.qseed_length)
	    || (cert->pgen_counter > 0
		&& cert->pgen_counter != cert2.pgen_counter)
	    || (cert->qgen_counter > 0
		&& cert->qgen_counter != cert2.qgen_counter)
	    || (cert->qseed_length > 0
		&& memcmp(cert->qseed, cert2.qseed, cert2.qseed_length) != 0)
	    || (cert->pseed_length > 0
		&& memcmp(cert->pseed, cert2.pseed, cert2.pseed_length) != 0)) {
		goto fail;
	}

	if (mpz_cmp(pub->q, pub2.q) != 0) {
		goto fail;
	}

	if (mpz_cmp(pub->p, pub2.p) != 0) {
		goto fail;
	}

	if (mpz_sizeinbase(s, 2) < q_bits - 1) {
		goto fail;
	}

	ret = 1;
	goto finish;

 fail:
	ret = 0;

 finish:
	dsa_params_clear(&pub2);
	mpz_clear(r);
	mpz_clear(s);

	return ret;
}
