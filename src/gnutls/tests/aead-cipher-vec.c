/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Daiki Ueno
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnutls/crypto.h>

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

/* Test whether gnutls_aead_cipher_{en,de}crypt_vec works */
static void start(const char *name, int algo)
{
	int ret;
	gnutls_aead_cipher_hd_t ch;
	uint8_t key16[64];
	uint8_t iv16[32];
	uint8_t auth[128];
	uint8_t data[128+64];
	gnutls_datum_t key, iv;
	giovec_t iov[2];
	giovec_t auth_iov[2];
	uint8_t tag[64];
	size_t tag_size = 0;

	key.data = key16;
	key.size = gnutls_cipher_get_key_size(algo);
	assert(key.size <= sizeof(key16));

	iv.data = iv16;
	iv.size = gnutls_cipher_get_iv_size(algo);
	assert(iv.size <= sizeof(iv16));

	memset(iv.data, 0xff, iv.size);
	memset(key.data, 0xfe, key.size);
	memset(data, 0xfa, 128);
	memset(auth, 0xaa, sizeof(auth));

	iov[0].iov_base = data;
	iov[0].iov_len = 64;
	iov[1].iov_base = data + 64;
	iov[1].iov_len = 64;

	auth_iov[0].iov_base = auth;
	auth_iov[0].iov_len = 64;
	auth_iov[1].iov_base = auth + 64;
	auth_iov[1].iov_len = 64;

	success("trying %s\n", name);

	ret =
	    gnutls_aead_cipher_init(&ch, algo, &key);
	if (ret < 0)
		fail("gnutls_cipher_init: %s\n", gnutls_strerror(ret));

	ret = gnutls_aead_cipher_encryptv2(ch,
					   iv.data, iv.size,
					   auth_iov, 2,
					   iov, 2,
					   tag, &tag_size);
	if (ret < 0)
		fail("could not encrypt data: %s\n", gnutls_strerror(ret));

	ret = gnutls_aead_cipher_decryptv2(ch,
					   iv.data, iv.size,
					   auth_iov, 2,
					   iov, 2,
					   tag, tag_size);
	if (ret < 0)
		fail("could not decrypt data: %s\n", gnutls_strerror(ret));

	gnutls_aead_cipher_deinit(ch);
}

void
doit(void)
{
	int ret;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = global_init();
	if (ret < 0) {
		fail("Cannot initialize library\n"); /*errcode 1 */
	}

	start("aes-128-gcm", GNUTLS_CIPHER_AES_128_GCM);
	start("aes-256-gcm", GNUTLS_CIPHER_AES_256_GCM);
	start("aes-128-ccm", GNUTLS_CIPHER_AES_128_CCM);
	if (!gnutls_fips140_mode_enabled())
		start("chacha20-poly1305", GNUTLS_CIPHER_CHACHA20_POLY1305);

	gnutls_global_deinit();
}
