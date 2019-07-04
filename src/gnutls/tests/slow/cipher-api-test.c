/*
 * Copyright (C) 2018 Red Hat, Inc.
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

#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>

#if defined(WIN32)
int main(int argc, char **argv)
{
	exit(77);
}
#else

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <utils.h>

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

/* Test whether an invalid call to gnutls_cipher_encrypt() is caught */
static void test_cipher(int algo)
{
	int ret;
	gnutls_cipher_hd_t ch;
	uint8_t key16[64];
	uint8_t iv16[32];
	uint8_t data[128];
	gnutls_datum_t key, iv;

	key.data = key16;
	key.size = gnutls_cipher_get_key_size(algo);
	assert(key.size <= sizeof(key16));

	iv.data = iv16;
	iv.size = gnutls_cipher_get_iv_size(algo);
	assert(iv.size <= sizeof(iv16));

	memset(iv.data, 0xff, iv.size);
	memset(key.data, 0xfe, key.size);
	memset(data, 0xfa, sizeof(data));

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = global_init();
	if (ret < 0) {
		fail("Cannot initialize library\n"); /*errcode 1 */
	}

	ret =
	    gnutls_cipher_init(&ch, algo, &key, &iv);
	if (ret < 0)
		fail("gnutls_cipher_init failed\n"); /*errcode 1 */

	/* try encrypting in a way that violates nettle's block conventions */
	ret = gnutls_cipher_encrypt(ch, data, sizeof(data)-1);
	if (ret >= 0)
		fail("succeeded in encrypting partial data on block cipher\n");

	gnutls_cipher_deinit(ch);

	gnutls_global_deinit();
}

/* Test whether an invalid gnutls_cipher_add_auth() is caught */
static void test_aead_cipher1(int algo)
{
	int ret;
	gnutls_cipher_hd_t ch;
	uint8_t key16[64];
	uint8_t iv16[32];
	uint8_t data[128];
	gnutls_datum_t key, iv;

	if (algo == GNUTLS_CIPHER_CHACHA20_POLY1305)
		return;

	key.data = key16;
	key.size = gnutls_cipher_get_key_size(algo);
	assert(key.size <= sizeof(key16));

	iv.data = iv16;
	iv.size = gnutls_cipher_get_iv_size(algo);
	assert(iv.size <= sizeof(iv16));

	memset(iv.data, 0xff, iv.size);
	memset(key.data, 0xfe, key.size);
	memset(data, 0xfa, sizeof(data));

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = global_init();
	if (ret < 0) {
		fail("Cannot initialize library\n"); /*errcode 1 */
	}

	ret =
	    gnutls_cipher_init(&ch, algo, &key, &iv);
	if (ret < 0)
		fail("gnutls_cipher_init failed\n"); /*errcode 1 */

	ret = gnutls_cipher_add_auth(ch, data, sizeof(data)-1);
	if (ret < 0)
		fail("could not add auth data\n");

	ret = gnutls_cipher_add_auth(ch, data, 16);
	if (ret >= 0)
		fail("succeeded in adding auth data data after partial data were given\n");

	gnutls_cipher_deinit(ch);

	gnutls_global_deinit();
	return;
}

/* Test whether an invalid call to gnutls_cipher_encrypt() is caught */
static void test_aead_cipher2(int algo)
{
	int ret;
	gnutls_cipher_hd_t ch;
	uint8_t key16[64];
	uint8_t iv16[32];
	uint8_t data[128];
	gnutls_datum_t key, iv;

	key.data = key16;
	key.size = gnutls_cipher_get_key_size(algo);
	assert(key.size <= sizeof(key16));

	iv.data = iv16;
	iv.size = gnutls_cipher_get_iv_size(algo);
	assert(iv.size <= sizeof(iv16));

	memset(iv.data, 0xff, iv.size);
	memset(key.data, 0xfe, key.size);
	memset(data, 0xfa, sizeof(data));

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	ret = global_init();
	if (ret < 0) {
		fail("Cannot initialize library\n"); /*errcode 1 */
	}

	ret =
	    gnutls_cipher_init(&ch, algo, &key, &iv);
	if (ret < 0)
		fail("gnutls_cipher_init failed\n"); /*errcode 1 */

	/* try encrypting in a way that violates nettle's AEAD conventions */
	ret = gnutls_cipher_encrypt(ch, data, sizeof(data)-1);
	if (ret < 0)
		fail("could not encrypt data\n");

	ret = gnutls_cipher_encrypt(ch, data, sizeof(data));
	if (ret >= 0)
		fail("succeeded in encrypting partial data after partial data were given\n");

	gnutls_cipher_deinit(ch);

	gnutls_global_deinit();
	return;
}

static void check_status(int status)
{
	if (WEXITSTATUS(status) != 0 ||
	    (WIFSIGNALED(status) && WTERMSIG(status) != SIGABRT)) {
		if (WIFSIGNALED(status)) {
			fail("Child died with signal %d\n", WTERMSIG(status));
		} else {
			fail("Child died with status %d\n",
			     WEXITSTATUS(status));
		}
	}
}

static
void start(const char *name, int algo, unsigned aead)
{
	pid_t child;

	success("trying %s\n", name);

	signal(SIGPIPE, SIG_IGN);

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status;
		/* parent */
		wait(&status);
		check_status(status);
	} else {
		if (!aead)
			test_cipher(algo);
		else
			test_aead_cipher1(algo);
		exit(0);
	}

	if (!aead)
		return;

	/* check test_aead_cipher2 */

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status;
		/* parent */
		wait(&status);
		check_status(status);
	} else {
		test_aead_cipher2(algo);
		exit(0);
	}
}

void doit(void)
{
	start("aes128-gcm", GNUTLS_CIPHER_AES_128_GCM, 1);
	start("aes256-gcm", GNUTLS_CIPHER_AES_256_GCM, 1);
	start("aes128-cbc", GNUTLS_CIPHER_AES_128_CBC, 0);
	start("aes256-cbc", GNUTLS_CIPHER_AES_256_CBC, 0);
	start("3des-cbc", GNUTLS_CIPHER_3DES_CBC, 0);
	if (!gnutls_fips140_mode_enabled()) {
		start("camellia128-gcm", GNUTLS_CIPHER_CAMELLIA_128_GCM, 1);
		start("camellia256-gcm", GNUTLS_CIPHER_CAMELLIA_256_GCM, 1);
		start("chacha20-poly1305", GNUTLS_CIPHER_CHACHA20_POLY1305, 1);
	}
}

#endif
