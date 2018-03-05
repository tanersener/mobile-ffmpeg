/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

#include "gnutls_int.h"
#include <algorithms.h>
#include "errors.h"
#include <x509/common.h>

typedef struct {
	const char *name;
	gnutls_sec_param_t sec_param;
	unsigned int bits;	/* security level */
	unsigned int pk_bits;	/* DH, RSA, SRP */
	unsigned int dsa_bits;	/* bits for DSA. Handled differently since
				 * choice of key size in DSA is political.
				 */
	unsigned int subgroup_bits;	/* subgroup bits */
	unsigned int ecc_bits;	/* bits for ECC keys */
} gnutls_sec_params_entry;

static const gnutls_sec_params_entry sec_params[] = {
	{"Insecure", GNUTLS_SEC_PARAM_INSECURE, 0, 0, 0, 0, 0},
	{"Export", GNUTLS_SEC_PARAM_EXPORT, 42, 512, 0, 84, 0},
	{"Very weak", GNUTLS_SEC_PARAM_VERY_WEAK, 64, 767, 0, 128, 0},
	{"Weak", GNUTLS_SEC_PARAM_WEAK, 72, 1008, 1008, 160, 160},
#ifdef ENABLE_FIPS140
	{"Low", GNUTLS_SEC_PARAM_LOW, 80, 1024, 1024, 160, 160},
	{"Legacy", GNUTLS_SEC_PARAM_LEGACY, 96, 1024, 1024, 192, 192},
	{"Medium", GNUTLS_SEC_PARAM_MEDIUM, 112, 2048, 2048, 224, 224},
	{"High", GNUTLS_SEC_PARAM_HIGH, 128, 3072, 3072, 256, 256},
#else
	{"Low", GNUTLS_SEC_PARAM_LOW, 80, 1024, 1024, 160, 160}, /* ENISA-LEGACY */
	{"Legacy", GNUTLS_SEC_PARAM_LEGACY, 96, 1776, 2048, 192, 192},
	{"Medium", GNUTLS_SEC_PARAM_MEDIUM, 112, 2048, 2048, 256, 224},
	{"High", GNUTLS_SEC_PARAM_HIGH, 128, 3072, 3072, 256, 256},
#endif
	{"Ultra", GNUTLS_SEC_PARAM_ULTRA, 192, 8192, 8192, 384, 384},
	{"Future", GNUTLS_SEC_PARAM_FUTURE, 256, 15360, 15360, 512, 512},
	{NULL, 0, 0, 0, 0, 0}
};

#define GNUTLS_SEC_PARAM_LOOP(b) \
	{ const gnutls_sec_params_entry *p; \
		for(p = sec_params; p->name != NULL; p++) { b ; } }

/**
 * gnutls_sec_param_to_pk_bits:
 * @algo: is a public key algorithm
 * @param: is a security parameter
 *
 * When generating private and public key pairs a difficult question
 * is which size of "bits" the modulus will be in RSA and the group size
 * in DSA. The easy answer is 1024, which is also wrong. This function
 * will convert a human understandable security parameter to an
 * appropriate size for the specific algorithm.
 *
 * Returns: The number of bits, or (0).
 *
 * Since: 2.12.0
 **/
unsigned int
gnutls_sec_param_to_pk_bits(gnutls_pk_algorithm_t algo,
			    gnutls_sec_param_t param)
{
	unsigned int ret = 0;

	/* handle DSA differently */
	GNUTLS_SEC_PARAM_LOOP(
	if (p->sec_param == param) {
		if (algo == GNUTLS_PK_DSA)
			ret = p->dsa_bits;
		else if (IS_EC(algo))
			ret = p->ecc_bits;
		else
			ret = p->pk_bits; break;
	}
	);
	return ret;
}

/**
 * gnutls_sec_param_to_symmetric_bits:
 * @algo: is a public key algorithm
 * @param: is a security parameter
 *
 * This function will return the number of bits that correspond to
 * symmetric cipher strength for the given security parameter.
 *
 * Returns: The number of bits, or (0).
 *
 * Since: 3.3.0
 **/
unsigned int
gnutls_sec_param_to_symmetric_bits(gnutls_sec_param_t param)
{
	unsigned int ret = 0;

	/* handle DSA differently */
	GNUTLS_SEC_PARAM_LOOP(
	if (p->sec_param == param) {
		ret = p->bits; break;
	}
	);
	return ret;
}

/* Returns the corresponding size for subgroup bits (q),
 * given the group bits (p).
 */
unsigned int _gnutls_pk_bits_to_subgroup_bits(unsigned int pk_bits)
{
	unsigned int ret = 0;

	GNUTLS_SEC_PARAM_LOOP(
		ret = p->subgroup_bits; 
		if (p->pk_bits >= pk_bits)
			break;
	);
	return ret;
}

/**
 * gnutls_sec_param_get_name:
 * @param: is a security parameter
 *
 * Convert a #gnutls_sec_param_t value to a string.
 *
 * Returns: a pointer to a string that contains the name of the
 *   specified security level, or %NULL.
 *
 * Since: 2.12.0
 **/
const char *gnutls_sec_param_get_name(gnutls_sec_param_t param)
{
	const char *ret = "Unknown";

	GNUTLS_SEC_PARAM_LOOP(
		if (p->sec_param == param) {
			ret = p->name; 
			break;
		}
	);

	return ret;
}

/**
 * gnutls_pk_bits_to_sec_param:
 * @algo: is a public key algorithm
 * @bits: is the number of bits
 *
 * This is the inverse of gnutls_sec_param_to_pk_bits(). Given an algorithm
 * and the number of bits, it will return the security parameter. This is
 * a rough indication.
 *
 * Returns: The security parameter.
 *
 * Since: 2.12.0
 **/
gnutls_sec_param_t
gnutls_pk_bits_to_sec_param(gnutls_pk_algorithm_t algo, unsigned int bits)
{
	gnutls_sec_param_t ret = GNUTLS_SEC_PARAM_INSECURE;

	if (bits == 0)
		return GNUTLS_SEC_PARAM_UNKNOWN;

	if (IS_EC(algo)) {
		GNUTLS_SEC_PARAM_LOOP(
			if (p->ecc_bits > bits) {
				break;
			}
			ret = p->sec_param;
		);
	} else {
		GNUTLS_SEC_PARAM_LOOP(
			if (p->pk_bits > bits) {
			      break;
			}
			ret = p->sec_param;
		);
	}

	return ret;
}
