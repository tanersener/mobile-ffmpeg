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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
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
	const char *pstring;
	const uint8_t res[4][16];
};

struct priv_st {
	struct drbg_aes_ctx *ctx;
};

/* Run a Know-Answer-Test using a dedicated test context. */
int drbg_aes_self_test(void)
{
	static const struct self_test_st tv[] = {
		{
		 .entropy = {0xb9, 0xca, 0x7f, 0xd6, 0xa0, 0xf5, 0xd3, 0x42,
			     0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
			     0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
			     0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
			     0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
			     0x00, 0x00, 0x28, 0x18, 0x00, 0x00, 0x25, 0x00},
		 .pstring = "test test test",
		 .res = {
			 {0xa4, 0xae, 0xb4, 0x51, 0xd0, 0x0d, 0x97, 0xcc, 0x46,
			  0xbb, 0xc0, 0xec, 0x5c, 0xa1, 0xf0, 0x34},
			 {0x68, 0xc4, 0x04, 0x63, 0x3d, 0x9e, 0x2c, 0x05, 0x18,
			  0xcf, 0xde, 0x2a, 0x4c, 0x49, 0xc8, 0x2b},
			 {0x60, 0x5a, 0xd6, 0x71, 0x5e, 0xb3, 0x86, 0x22, 0xd5,
			  0x21, 0x7f, 0xd7, 0x1d, 0xa3, 0xff, 0xa6},
			 {0xe0, 0xf8, 0x77, 0x2c, 0xcb, 0xa4, 0x52, 0xa5, 0x35,
			  0xf5, 0x21, 0xb9, 0x20, 0x4e, 0xff, 0x3e},
			 }
		 },
		{
		 .entropy = {
			     0xb9, 0xca, 0x7f, 0xd6, 0xa0, 0xf5, 0xd3, 0x42,
			     0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
			     0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
			     0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
			     0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
			     0x00, 0x00, 0x28, 0x18, 0x00, 0x00, 0x25, 0x00},
		 .pstring = "tost tost test",
		 .res = {
			 {0x47, 0x2d, 0x1e, 0xa9, 0xe9, 0xed, 0x02, 0xba, 0x0b,
			  0x8f, 0xc7, 0x59, 0x84, 0xe0, 0x7d, 0x6e},
			 {0x4c, 0x63, 0xfd, 0xc9, 0x17, 0x1e, 0x09, 0xca, 0x62,
			  0x72, 0x45, 0x4f, 0xeb, 0x5b, 0xd0, 0x02},
			 {0x3e, 0x29, 0x1c, 0xde, 0xd9, 0xdd, 0x65, 0x4f, 0xfe,
			  0xcd, 0x17, 0xa3, 0xa0, 0x23, 0x3b, 0xd5},
			 {0x2b, 0x45, 0xd2, 0x4a, 0xf9, 0xd4, 0x91, 0xa4, 0x2e,
			  0xaf, 0xe6, 0xb5, 0x40, 0xb4, 0xf5, 0xd7},
			 }
		 },
		{
		 .entropy = {
			     0x42, 0x9c, 0x08, 0x3d, 0x82, 0xf4, 0x8a, 0x40,
			     0x66, 0xb5, 0x49, 0x27, 0xab, 0x42, 0xc7, 0xc3,
			     0x0e, 0xb7, 0x61, 0x3c, 0xfe, 0xb0, 0xbe, 0x73,
			     0xf7, 0x6e, 0x6d, 0x6f, 0x1d, 0xa3, 0x14, 0xfa,
			     0xbb, 0x4b, 0xc1, 0x0e, 0xc5, 0xfb, 0xcd, 0x46,
			     0xbe, 0x28, 0x61, 0xe7, 0x03, 0x2b, 0x37, 0x7d},
		 .pstring = "one two",
		 .res = {
			 {0x6c, 0x29, 0x75, 0xdc, 0xd3, 0xaf, 0xfa, 0xf0, 0xe9,
			  0xa8, 0xa4, 0xd8, 0x60, 0x62, 0xc9, 0xaa},
			 {0x2b, 0xac, 0x71, 0x36, 0x42, 0xbf, 0x2a, 0xff, 0xa7,
			  0xc7, 0xf6, 0x08, 0xa4, 0x3b, 0xe6, 0x00},
			 {0x1d, 0x2c, 0x18, 0xbc, 0xc4, 0xbe, 0x64, 0x4b, 0x9a,
			  0x6c, 0x45, 0xcb, 0x6b, 0xf2, 0xed, 0xc3},
			 {0xe3, 0x41, 0x58, 0x24, 0x57, 0xa0, 0x60, 0xad, 0xb6,
			  0x45, 0x8d, 0x8f, 0x32, 0x81, 0x77, 0xa9},
			 }
		 },
	};
	unsigned i, j;
	struct drbg_aes_ctx test_ctx;
	struct drbg_aes_ctx test_ctx2;
	struct priv_st priv;
	int ret, saved;
	uint8_t *tmp;
	unsigned char result[16];

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
		/* Setup the key.  */
		ret =
		    drbg_aes_init(&test_ctx, DRBG_AES_SEED_SIZE, tv[i].entropy,
				  strlen(tv[i].pstring), (void *)tv[i].pstring);
		if (ret == 0)
			goto fail;

		if (drbg_aes_is_seeded(&test_ctx) == 0)
			goto fail;

		/* Get and compare the first three results.  */
		for (j = 0; j < 3; j++) {
			/* Compute the next value.  */
			if (drbg_aes_random(&test_ctx, 16, result) == 0)
				goto fail;

			/* Compare it to the known value.  */
			if (memcmp(result, tv[i].res[j], 16) != 0) {
				goto fail;
			}
		}

		ret =
		    drbg_aes_reseed(&test_ctx, DRBG_AES_SEED_SIZE,
				    tv[i].entropy, 0, NULL);
		if (ret == 0)
			goto fail;

		if (drbg_aes_random(&test_ctx, 16, result) == 0)
			goto fail;

		if (memcmp(result, tv[i].res[3], 16) != 0) {
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
