/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2013 Red Hat
 * Copyright (C) 2008  Free Software Foundation, Inc.
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

/* This is a known-answer test for the DRBG-CTR-AES. 
 */

#include <config.h>
#include "errors.h"
#include <drbg-aes.h>
#include <string.h>
#include <stdio.h>

struct self_test_st {
	const uint8_t entropy[DRBG_AES_SEED_SIZE];
	const uint8_t pstring[32];
	const uint8_t reseed[DRBG_AES_SEED_SIZE];
	const uint8_t addtl[3][32];
	const uint8_t res[64];
};

struct priv_st {
	struct drbg_aes_ctx *ctx;
};

/* Run a Know-Answer-Test using a dedicated test context. */
int drbg_aes_self_test(void)
{
	static const struct self_test_st tv[] = {
		/*
		 * Test vector from NIST ACVP test framework that was
		 * successfully validated by ACVP server.
		 */
		{
		 .entropy = { 0xBE, 0x36, 0xDA, 0x22, 0xC5, 0xEE, 0xC2, 0x46,
			      0x88, 0xAF, 0xD5, 0xFB, 0xC7, 0x12, 0x98, 0x58,
			      0x32, 0xD0, 0x35, 0x89, 0x33, 0xF0, 0xFA, 0x2B,
			      0x1B, 0x0D, 0x02, 0xE9, 0x3A, 0x28, 0x5F, 0x06,
			      0x04, 0x3B, 0x97, 0x5F, 0xED, 0xD6, 0x2D, 0xC5,
			      0xD9, 0x76, 0x42, 0x06, 0xEC, 0x80, 0x55, 0xFB },
		 .pstring = { 0x50, 0xF9, 0x47, 0x14, 0x27, 0xF4, 0xA0, 0xAF,
			      0x30, 0x08, 0x74, 0x85, 0xC7, 0x94, 0xA3, 0x5D,
			      0x8F, 0x4F, 0x43, 0x52, 0x0C, 0xC0, 0x64, 0x47,
			      0xF8, 0xAD, 0xC7, 0xB2, 0x6C, 0x7F, 0x26, 0x6E },
		 .reseed = {  0x64, 0xDB, 0x9E, 0xC3, 0x45, 0x88, 0xED, 0x33,
			      0xC8, 0x4C, 0xE2, 0x87, 0x12, 0x9C, 0xCA, 0x02,
			      0x16, 0x41, 0xB5, 0x3B, 0xCB, 0x5F, 0x01, 0xAE,
			      0xA0, 0x01, 0xBB, 0x16, 0x44, 0x1B, 0x99, 0x82,
			      0x97, 0x84, 0x5B, 0x16, 0x58, 0xF3, 0xBD, 0xBE,
			      0x9A, 0xAB, 0x9F, 0xB7, 0xB2, 0x93, 0xBE, 0xA5 },
		 .addtl = {
			   {  0x10, 0xDD, 0xBC, 0x33, 0x29, 0x10, 0x53, 0x4C,
			      0xA0, 0x10, 0x72, 0xBF, 0x4C, 0x55, 0xDD, 0x7C,
			      0x08, 0x5F, 0xDF, 0x40, 0xB6, 0x03, 0xF2, 0xBC,
			      0xEA, 0xAE, 0x08, 0x46, 0x61, 0x68, 0x91, 0xC9 },
			   {  0x00, 0xB6, 0x84, 0xF7, 0xF3, 0x14, 0xC7, 0x80,
			      0x57, 0xA4, 0x8F, 0x48, 0xE5, 0xC9, 0x7F, 0x8D,
			      0x54, 0x88, 0x96, 0xDF, 0x94, 0x55, 0xB1, 0x1C,
			      0xFA, 0xCF, 0xE0, 0x4D, 0xAA, 0x01, 0xFA, 0x25 },
			   {  0x97, 0x02, 0xDB, 0xCB, 0x85, 0x2A, 0xAA, 0x55,
			      0x96, 0xC7, 0xF8, 0xF3, 0xB3, 0x9B, 0xBC, 0xCA,
			      0xB5, 0xC1, 0x7C, 0x1C, 0x0D, 0x2F, 0x5B, 0x0E,
			      0x9B, 0xBA, 0xB4, 0xDD, 0x45, 0x90, 0xF2, 0x14 },
			  },
		 .res = {     0xfe, 0x78, 0x3c, 0x64, 0x98, 0xb8, 0x69, 0x1d,
			      0xb7, 0xd4, 0xfb, 0x71, 0xdb, 0x58, 0xd2, 0xee,
			      0x32, 0x63, 0xfd, 0xed, 0x78, 0xe7, 0x93, 0x13,
			      0x65, 0xd7, 0xf8, 0x6b, 0x71, 0x90, 0xfc, 0xf4,
			      0xa3, 0x29, 0xae, 0x0b, 0xca, 0x40, 0x23, 0x61,
			      0x6c, 0xa3, 0xf8, 0xc6, 0x75, 0x15, 0x38, 0x36,
			      0x11, 0x5c, 0xc0, 0x87, 0x8a, 0x9b, 0x91, 0xdb,
			      0x56, 0xb9, 0x06, 0x98, 0xc5, 0x78, 0x1a, 0x3a }
		 },
	};
	unsigned i, j;
	struct drbg_aes_ctx test_ctx;
	struct drbg_aes_ctx test_ctx2;
	struct priv_st priv;
	int ret, saved;
	uint8_t *tmp;
	unsigned char result[64];

	memset(&priv, 0, sizeof(priv));
	priv.ctx = &test_ctx;

	/* Test the error handling of drbg_aes_init */
	ret =
	    drbg_aes_init(&test_ctx, DRBG_AES_SEED_SIZE, tv[0].entropy,
			  DRBG_AES_SEED_SIZE*2, (void*)tv);
	if (ret != 0) {
		gnutls_assert();
		return 0;
	}

	tmp = gnutls_malloc(MAX_DRBG_AES_GENERATE_SIZE+1);
	if (tmp == NULL) {
		gnutls_assert();
		return 0;
	}

	for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
		/* CAVP test step 1: initialization with perso string */
		ret = drbg_aes_init(&test_ctx,
				    sizeof(tv[i].entropy), tv[i].entropy,
				    sizeof(tv[i].pstring), tv[i].pstring);
		if (ret == 0)
			goto fail;

		if (drbg_aes_is_seeded(&test_ctx) == 0)
			goto fail;

		/* CAVP test step 2: reseed with addtl information */
		ret = drbg_aes_reseed(&test_ctx,
				      sizeof(tv[i].reseed), tv[i].reseed,
				      sizeof(tv[i].addtl[0]), tv[i].addtl[0]);
		if (ret == 0)
			goto fail;

		/* CAVP test step 3: generate with addtl info, discard result */
		if (drbg_aes_generate(&test_ctx, sizeof(result), result,
				      sizeof(tv[i].addtl[1]),
				      tv[i].addtl[1]) == 0)
			goto fail;

		/* CAVP test step 4: generate with addtl info */
		if (drbg_aes_generate(&test_ctx, sizeof(result), result,
				      sizeof(tv[i].addtl[2]),
				      tv[i].addtl[2]) == 0)
			goto fail;

		if (memcmp(result, tv[i].res, sizeof(result)) != 0) {
			goto fail;
		}

		/* test the error handling of drbg_aes_random() */
		saved = test_ctx.reseed_counter;
		test_ctx.reseed_counter = DRBG_AES_RESEED_TIME+1;
		if (drbg_aes_random(&test_ctx, 16, result) != 0) {
			gnutls_assert();
			goto fail;
		}
		test_ctx.reseed_counter = saved;

		ret = drbg_aes_random(&test_ctx, MAX_DRBG_AES_GENERATE_SIZE+1, tmp);
		if (ret == 0) {
			gnutls_assert();
			goto fail;
		}

		/* test the low-level function */
		ret = drbg_aes_generate(&test_ctx, MAX_DRBG_AES_GENERATE_SIZE+1, tmp, 0, NULL);
		if (ret != 0) {
			gnutls_assert();
			goto fail;
		}

		/* Test of the reseed function for error handling */
		ret =
		    drbg_aes_reseed(&test_ctx, DRBG_AES_SEED_SIZE*2,
				    (uint8_t*)tv, 0, NULL);
		if (ret != 0)
			goto fail;

		ret =
		    drbg_aes_reseed(&test_ctx, DRBG_AES_SEED_SIZE,
				    tv[i].entropy, DRBG_AES_SEED_SIZE*2, (uint8_t*)tv);
		if (ret != 0)
			goto fail;

		/* check whether reseed detection works */
		if (i==0) {
			ret =
			    drbg_aes_reseed(&test_ctx, DRBG_AES_SEED_SIZE,
					    tv[i].entropy, 0, NULL);
			if (ret == 0)
				goto fail;

			saved = test_ctx.reseed_counter;
			test_ctx.reseed_counter = DRBG_AES_RESEED_TIME-4;
			for (j=0;j<5;j++) {
				if (drbg_aes_random(&test_ctx, 1, result) == 0) {
					gnutls_assert();
					goto fail;
				}
			}
			/* that should fail */
			if (drbg_aes_random(&test_ctx, 1, result) != 0) {
				gnutls_assert();
				goto fail;
			}
			test_ctx.reseed_counter = saved;
		}

		/* test deinit, which is zeroize_key() */
		memcpy(&test_ctx2, &test_ctx, sizeof(test_ctx));
		zeroize_key(&test_ctx, sizeof(test_ctx));
		if (memcmp(&test_ctx, &test_ctx2, sizeof(test_ctx)) == 0) {
			gnutls_assert();
			goto fail;
		}


	}

	gnutls_free(tmp);
	return 1;
 fail:
	free(tmp);
	return 0;
}
