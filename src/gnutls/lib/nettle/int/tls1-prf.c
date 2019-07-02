/*
 * Copyright (C) 2017 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* Functions for the TLS PRF handling.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <gnutls_int.h>

#include <stdlib.h>
#include <string.h>

#include <nettle/hmac.h>
#include <nettle/memxor.h>
#include "int/tls1-prf.h"
#include <nettle/sha1.h>
#include <nettle/md5.h>


/* The RFC2246 P_hash() function. The mac_ctx is expected to
 * be initialized and key set to be the secret key.
 */
static void
P_hash( void *mac_ctx,
	nettle_hash_update_func *update,
	nettle_hash_digest_func *digest,
	size_t digest_size,
	size_t seed_size, const uint8_t *seed,
	size_t dst_length,
	uint8_t *dst)
{
	uint8_t Atmp[MAX_HASH_SIZE];
	ssize_t left;
	unsigned started = 0;

	/* round up */
	left = dst_length;

	while(left > 0) {
		if (started == 0) { /* A(0) */
			update(mac_ctx, seed_size, seed);
			started = 1;
		} else {
			update(mac_ctx, digest_size, Atmp);
		}
		digest(mac_ctx, digest_size, Atmp); /* store A(i) */

		update(mac_ctx, digest_size, Atmp); /* hash A(i) */
		update(mac_ctx, seed_size, seed); /* hash seed */

		if (left < (ssize_t)digest_size)
			digest_size = left;

		digest(mac_ctx, digest_size, dst);

		left -= digest_size;
		dst += digest_size;
	}

	return;
}

int
tls10_prf(size_t secret_size, const uint8_t *secret,
	  size_t label_size, const char *label,
	  size_t seed_size, const uint8_t *seed,
	  size_t length, uint8_t *dst)
{
	int l_s, cseed_size = seed_size + label_size;
	const uint8_t *s1, *s2;
	struct hmac_md5_ctx md5_ctx;
	struct hmac_sha1_ctx sha1_ctx;
	uint8_t o1[MAX_PRF_BYTES];
	uint8_t cseed[MAX_SEED_SIZE];

	if (cseed_size > MAX_SEED_SIZE || length > MAX_PRF_BYTES)
		return 0;

	memcpy(cseed, label, label_size);
	memcpy(&cseed[label_size], seed, seed_size);

	l_s = secret_size / 2;
	s1 = &secret[0];
	s2 = &secret[l_s];
	if (secret_size % 2 != 0) {
		l_s++;
	}

	hmac_md5_set_key(&md5_ctx, l_s, s1);

	P_hash(&md5_ctx, (nettle_hash_update_func*)hmac_md5_update,
		(nettle_hash_digest_func*)hmac_md5_digest,
		MD5_DIGEST_SIZE,
		cseed_size, cseed, length, o1);

	hmac_sha1_set_key(&sha1_ctx, l_s, s2);

	P_hash(&sha1_ctx, (nettle_hash_update_func*)hmac_sha1_update,
		(nettle_hash_digest_func*)hmac_sha1_digest,
		SHA1_DIGEST_SIZE,
		cseed_size, cseed, length, dst);

	memxor(dst, o1, length);

	return 1;
}

/*-
 * tls12_prf:
 * @mac_ctx: a MAC context initialized with key being the secret
 * @update: a MAC update function
 * @digest: a MAC digest function
 * @digest_size: the MAC output size
 * @label_size: the size of the label
 * @label: the label to apply
 * @seed_size: the seed size
 * @seed: the seed
 * @length: size of desired PRF output
 * @dst: the location to store output
 *
 * The TLS 1.2 Pseudo-Random-Function (PRF).
 *
 * Returns: zero on failure, non zero on success.
 -*/
int
tls12_prf(void *mac_ctx,
	  nettle_hash_update_func *update,
	  nettle_hash_digest_func *digest,
	  size_t digest_size,
	  size_t label_size, const char *label,
	  size_t seed_size, const uint8_t *seed,
	  size_t length, uint8_t *dst)
{
	size_t cseed_size = seed_size + label_size;
	uint8_t cseed[MAX_SEED_SIZE];

	if (cseed_size > MAX_SEED_SIZE)
		return 0;

	memcpy(cseed, label, label_size);
	memcpy(&cseed[label_size], seed, seed_size);

	P_hash(mac_ctx, update, digest, digest_size,
		cseed_size, cseed, length, dst);

	return 1;
}
