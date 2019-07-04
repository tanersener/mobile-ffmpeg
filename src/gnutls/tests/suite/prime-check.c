/*
 * Copyright (C) 2016 Nikos Mavrogiannopoulos
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <nettle/bignum.h>
#include <gmp.h>
#include <gnutls/gnutls.h>
#include <assert.h>

/* Tests whether the included parameters are indeed prime */

static void test_prime(const gnutls_datum_t * prime, const gnutls_datum_t * _q)
{
	mpz_t p;
	unsigned bits = prime->size * 8;

	nettle_mpz_init_set_str_256_u(p, prime->size, prime->data);

	assert(mpz_sizeinbase(p, 2) == bits);
	assert(mpz_probab_prime_p(p, 18));

	if (_q) {
		mpz_t q;

		nettle_mpz_init_set_str_256_u(q, _q->size, _q->data);
		mpz_mul_ui(q, q, 2);
		mpz_add_ui(q, q, 1);
		assert(mpz_cmp(p, q) == 0);
		mpz_clear(q);
	}

	mpz_clear(p);
}

int main(int argc, char **argv)
{
	test_prime(&gnutls_srp_8192_group_prime, NULL);
	test_prime(&gnutls_srp_4096_group_prime, NULL);
	test_prime(&gnutls_srp_3072_group_prime, NULL);
	test_prime(&gnutls_srp_2048_group_prime, NULL);
	test_prime(&gnutls_srp_1536_group_prime, NULL);
	test_prime(&gnutls_srp_1024_group_prime, NULL);

	test_prime(&gnutls_ffdhe_8192_group_prime, &gnutls_ffdhe_8192_group_q);
	test_prime(&gnutls_ffdhe_6144_group_prime, &gnutls_ffdhe_6144_group_q);
	test_prime(&gnutls_ffdhe_4096_group_prime, &gnutls_ffdhe_4096_group_q);
	test_prime(&gnutls_ffdhe_3072_group_prime, &gnutls_ffdhe_3072_group_q);
	test_prime(&gnutls_ffdhe_2048_group_prime, &gnutls_ffdhe_2048_group_q);

	return 0;
}
