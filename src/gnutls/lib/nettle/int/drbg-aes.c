/* drbg-aes.c */

/* Copyright (C) 2013, 2014 Red Hat
 *
 * This file is part of GnuTLS.
 *  
 * The GnuTLS library is free software; you can redistribute it and/or modify
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

#include <config.h>
#include <drbg-aes.h>
#include <nettle/memxor.h>
#include <nettle/aes.h>
#include <minmax.h>
#include <string.h>
#include <stdio.h>
#include <fips.h>

int
drbg_aes_init(struct drbg_aes_ctx *ctx,
	      unsigned entropy_size, const uint8_t * entropy,
	      unsigned pstring_size, const uint8_t * pstring)
{
	uint8_t tmp[DRBG_AES_KEY_SIZE];

	memset(ctx, 0, sizeof(*ctx));
	memset(tmp, 0, sizeof(tmp));

	aes_set_encrypt_key(&ctx->key, DRBG_AES_KEY_SIZE, tmp);

	return drbg_aes_reseed(ctx, entropy_size, entropy,
			       pstring_size, pstring);
}

/* Sets V and key based on pdata */
static void
drbg_aes_update(struct drbg_aes_ctx *ctx,
		const uint8_t pdata[DRBG_AES_SEED_SIZE])
{
	unsigned len = 0;
	uint8_t tmp[DRBG_AES_SEED_SIZE];
	uint8_t *t = tmp;

	while (len < DRBG_AES_SEED_SIZE) {
		INCREMENT(sizeof(ctx->v), ctx->v);
		aes_encrypt(&ctx->key, AES_BLOCK_SIZE, t, ctx->v);
		t += AES_BLOCK_SIZE;
		len += AES_BLOCK_SIZE;
	}

	memxor(tmp, pdata, DRBG_AES_SEED_SIZE);

	aes_set_encrypt_key(&ctx->key, DRBG_AES_KEY_SIZE, tmp);

	memcpy(ctx->v, &tmp[DRBG_AES_KEY_SIZE], AES_BLOCK_SIZE);

	ctx->seeded = 1;
}

int
drbg_aes_reseed(struct drbg_aes_ctx *ctx,
		unsigned entropy_size, const uint8_t * entropy,
		unsigned add_size, const uint8_t * add)
{
	uint8_t tmp[DRBG_AES_SEED_SIZE];
	unsigned len = 0;

	if (add_size > DRBG_AES_SEED_SIZE || entropy_size != DRBG_AES_SEED_SIZE)
		return 0;

	if (add_size <= DRBG_AES_SEED_SIZE && add_size > 0) {
		memcpy(tmp, add, add_size);
		len = add_size;
	}

	if (len != DRBG_AES_SEED_SIZE)
		memset(&tmp[len], 0, DRBG_AES_SEED_SIZE - len);

	memxor(tmp, entropy, entropy_size);

	drbg_aes_update(ctx, tmp);
	ctx->reseed_counter = 1;

	return 1;
}

int drbg_aes_random(struct drbg_aes_ctx *ctx, unsigned length, uint8_t * dst)
{
	unsigned p_len;
	int left = length;
	uint8_t *p = dst;
	int ret;

	while(left > 0) {
		p_len = MIN(MAX_DRBG_AES_GENERATE_SIZE, left);
		ret = drbg_aes_generate(ctx, p_len, p, 0, 0);
		if (ret == 0)
			return ret;

		p += p_len;
		left -= p_len;
	}

	return 1;
}

/* we don't use additional input */
int drbg_aes_generate(struct drbg_aes_ctx *ctx, unsigned length, uint8_t * dst,
			unsigned add_size, const uint8_t *add)
{
	uint8_t tmp[AES_BLOCK_SIZE];
	uint8_t seed[DRBG_AES_SEED_SIZE];
	unsigned left;

	if (ctx->seeded == 0)
		return gnutls_assert_val(0);

	if (length > MAX_DRBG_AES_GENERATE_SIZE)
		return gnutls_assert_val(0);

	if (add_size > 0) {
		if (add_size > DRBG_AES_SEED_SIZE)
			return gnutls_assert_val(0);
		memcpy(seed, add, add_size);
		if (add_size != DRBG_AES_SEED_SIZE)
			memset(&seed[add_size], 0, DRBG_AES_SEED_SIZE - add_size);

		drbg_aes_update(ctx, seed);
	} else {
		memset(seed, 0, DRBG_AES_SEED_SIZE);
	}

	/* Throw the first block generated. FIPS 140-2 requirement (see 
	 * the continuous random number generator test in 4.9.2)
	 */
	if (ctx->prev_block_present == 0) {
		INCREMENT(sizeof(ctx->v), ctx->v);
		aes_encrypt(&ctx->key, AES_BLOCK_SIZE, ctx->prev_block, ctx->v);

		ctx->prev_block_present = 1;
	}

	/* Perform the actual encryption */
	for (left = length; left >= AES_BLOCK_SIZE;
	     left -= AES_BLOCK_SIZE, dst += AES_BLOCK_SIZE) {

		INCREMENT(sizeof(ctx->v), ctx->v);
		aes_encrypt(&ctx->key, AES_BLOCK_SIZE, dst, ctx->v);

		/* if detected loop */
		if (memcmp(dst, ctx->prev_block, AES_BLOCK_SIZE) == 0) {
			_gnutls_switch_lib_state(LIB_STATE_ERROR);
			return gnutls_assert_val(0);
		}

		memcpy(ctx->prev_block, dst, AES_BLOCK_SIZE);
	}

	if (left > 0) {		/* partial fill */

		INCREMENT(sizeof(ctx->v), ctx->v);
		aes_encrypt(&ctx->key, AES_BLOCK_SIZE, tmp, ctx->v);

		/* if detected loop */
		if (memcmp(tmp, ctx->prev_block, AES_BLOCK_SIZE) == 0) {
			_gnutls_switch_lib_state(LIB_STATE_ERROR);
			return gnutls_assert_val(0);
		}

		memcpy(ctx->prev_block, tmp, AES_BLOCK_SIZE);
		memcpy(dst, tmp, left);
	}

	if (ctx->reseed_counter > DRBG_AES_RESEED_TIME)
		return gnutls_assert_val(0);
	ctx->reseed_counter++;

	drbg_aes_update(ctx, seed);

	return 1;
}

int drbg_aes_is_seeded(struct drbg_aes_ctx *ctx)
{
	return ctx->seeded;
}
