/*
 * Copyright (C) 2009-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * Written by Nikos Mavrogiannopoulos <nmav@gnutls.org>.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "benchmark.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static unsigned page_size = 4096;

#define ALLOC(x) {x=malloc(step+64);assert(x!=NULL);}
#define ALLOCM(x, mem) {x=malloc(mem); assert(x!=NULL); assert(gnutls_rnd(GNUTLS_RND_NONCE, x, mem) >= 0);}
#define FREE(x) free(x)
#define INC(orig, x, s) x+=page_size; if ((x+step) >= (((unsigned char*)orig) + MAX_MEM)) { x = orig; }

#define MAX_MEM 64*1024*1024

static void cipher_mac_bench(int algo, int mac_algo, int size)
{
	int ret;
	gnutls_cipher_hd_t ctx;
	gnutls_hmac_hd_t mac_ctx;
	void *_key, *_iv;
	gnutls_datum_t key, iv;
	int ivsize = gnutls_cipher_get_iv_size(algo);
	int keysize = gnutls_cipher_get_key_size(algo);
	int step = size * 1024;
	struct benchmark_st st;
	void *output, *input;
	unsigned char c, *i;

	_key = malloc(keysize);
	if (_key == NULL)
		return;
	memset(_key, 0xf0, keysize);

	_iv = malloc(ivsize);
	if (_iv == NULL) {
		free(_key);
		return;
	}
	memset(_iv, 0xf0, ivsize);

	iv.data = _iv;
	iv.size = ivsize;

	key.data = _key;
	key.size = keysize;

	assert(gnutls_rnd(GNUTLS_RND_NONCE, &c, 1) >= 0);

	printf("%19s-%s ", gnutls_cipher_get_name(algo),
	       gnutls_mac_get_name(mac_algo));
	fflush(stdout);

	ALLOCM(input, MAX_MEM);
	ALLOC(output);
	i = input;

	start_benchmark(&st);

	ret = gnutls_hmac_init(&mac_ctx, mac_algo, key.data, key.size);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		goto leave;
	}

	ret = gnutls_cipher_init(&ctx, algo, &key, &iv);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		goto leave;
	}

	do {
		gnutls_hmac(mac_ctx, i, step);
		gnutls_cipher_encrypt2(ctx, i, step, output, step + 64);
		st.size += step;
		INC(input, i, step);
	}
	while (benchmark_must_finish == 0);

	gnutls_cipher_deinit(ctx);
	gnutls_hmac_deinit(mac_ctx, NULL);

	stop_benchmark(&st, NULL, 1);

      leave:
	FREE(input);
	FREE(output);
	free(_key);
	free(_iv);
}

static void force_memcpy(void *dest, const void *src, size_t n)
{
	volatile unsigned volatile_zero = 0;
	volatile char *vdest = (volatile char*)dest;

	if (n > 0) {
		do {
			memcpy(dest, src, n);
		} while(vdest[volatile_zero] != ((char*)src)[volatile_zero]);
	}
}

static void cipher_bench(int algo, int size, int aead)
{
	int ret;
	gnutls_cipher_hd_t ctx;
	void *_key, *_iv;
	gnutls_aead_cipher_hd_t actx;
	gnutls_datum_t key, iv;
	int ivsize = gnutls_cipher_get_iv_size(algo);
	int keysize = gnutls_cipher_get_key_size(algo);
	int step = size * 1024;
	void *input, *output;
	struct benchmark_st st;
	unsigned char c, *i;

	_key = malloc(keysize);
	if (_key == NULL)
		return;
	memset(_key, 0xf0, keysize);

	_iv = malloc(ivsize);
	if (_iv == NULL) {
		free(_key);
		return;
	}
	memset(_iv, 0xf0, ivsize);

	iv.data = _iv;
	iv.size = ivsize;

	key.data = _key;
	key.size = keysize;

	printf("%24s ", gnutls_cipher_get_name(algo));
	fflush(stdout);
	assert(gnutls_rnd(GNUTLS_RND_NONCE, &c, 1) >= 0);

	ALLOCM(input, MAX_MEM);
	ALLOCM(output, step+64);
	i = input;

	start_benchmark(&st);

	if (algo == GNUTLS_CIPHER_NULL) {
		do {
			force_memcpy(output, i, step);
			st.size += step;
			INC(input, i, step);
		}
		while (benchmark_must_finish == 0);
	} else if (aead != 0) {
		unsigned tag_size = gnutls_cipher_get_tag_size(algo);
		size_t out_size;

		ret = gnutls_aead_cipher_init(&actx, algo, &key);
		if (ret < 0) {
			fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
			goto leave;
		}

		do {
			out_size = step+64;
			assert(gnutls_aead_cipher_encrypt(actx, iv.data, iv.size, NULL, 0, tag_size,
				i, step, output, &out_size) >= 0);
			st.size += step;
			INC(input, i, step);
		}
		while (benchmark_must_finish == 0);

		gnutls_aead_cipher_deinit(actx);
	} else {
		ret = gnutls_cipher_init(&ctx, algo, &key, &iv);
		if (ret < 0) {
			fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
			goto leave;
		}

		do {
			gnutls_cipher_encrypt2(ctx, i, step, output, step + 64);
			st.size += step;
			INC(input, i, step);
		}
		while (benchmark_must_finish == 0);

		gnutls_cipher_deinit(ctx);
	}
	stop_benchmark(&st, NULL, 1);

	FREE(input);
	FREE(output);
      leave:
	free(_key);
	free(_iv);
}

static void mac_bench(int algo, int size)
{
	void *_key;
	int key_size = gnutls_hmac_get_key_size(algo);
	int step = size * 1024;
	struct benchmark_st st;
	void *input;
	unsigned char c, *i;

	ALLOCM(input, MAX_MEM);
	i = input;

	_key = malloc(key_size);
	if (_key == NULL)
		return;
	memset(_key, 0xf0, key_size);

	printf("%16s ", gnutls_mac_get_name(algo));
	fflush(stdout);

	assert(gnutls_rnd(GNUTLS_RND_NONCE, &c, 1) >= 0);

	start_benchmark(&st);

	do {
		gnutls_hmac_fast(algo, _key, key_size, i, step, _key);
		st.size += step;
		INC(input, i, step);
	}
	while (benchmark_must_finish == 0);

	stop_benchmark(&st, NULL, 1);
	FREE(input);

	free(_key);
}

void benchmark_cipher(int debug_level)
{
	int size = 16;
	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(debug_level);

#ifdef _SC_PAGESIZE
	page_size = sysconf(_SC_PAGESIZE);
#endif

	printf("Checking AEAD ciphers, payload size: %u\n", size * 1024);
	cipher_bench(GNUTLS_CIPHER_AES_128_GCM, size, 1);
	cipher_bench(GNUTLS_CIPHER_AES_128_CCM, size, 1);
	cipher_bench(GNUTLS_CIPHER_CHACHA20_POLY1305, size, 1);

	printf("\nChecking cipher-MAC combinations, payload size: %u\n", size * 1024);
	cipher_mac_bench(GNUTLS_CIPHER_SALSA20_256, GNUTLS_MAC_SHA1, size);
	cipher_mac_bench(GNUTLS_CIPHER_AES_128_CBC, GNUTLS_MAC_SHA1, size);
	cipher_mac_bench(GNUTLS_CIPHER_AES_128_CBC, GNUTLS_MAC_SHA256,
			 size);
#ifdef ENABLE_GOST
	cipher_mac_bench(GNUTLS_CIPHER_GOST28147_TC26Z_CNT, GNUTLS_MAC_GOST28147_TC26Z_IMIT,
			 size);
#endif

	printf("\nChecking MAC algorithms, payload size: %u\n", size * 1024);
	mac_bench(GNUTLS_MAC_SHA1, size);
	mac_bench(GNUTLS_MAC_SHA256, size);
	mac_bench(GNUTLS_MAC_SHA512, size);
#ifdef ENABLE_GOST
	mac_bench(GNUTLS_MAC_GOST28147_TC26Z_IMIT, size);
	mac_bench(GNUTLS_MAC_GOSTR_94, size);
	mac_bench(GNUTLS_MAC_STREEBOG_512, size);
#endif

	printf("\nChecking ciphers, payload size: %u\n", size * 1024);
	cipher_bench(GNUTLS_CIPHER_3DES_CBC, size, 0);
	cipher_bench(GNUTLS_CIPHER_AES_128_CBC, size, 0);
	cipher_bench(GNUTLS_CIPHER_SALSA20_256, size, 0);
	cipher_bench(GNUTLS_CIPHER_NULL, size, 1);
#ifdef ENABLE_GOST
	cipher_bench(GNUTLS_CIPHER_GOST28147_TC26Z_CNT, size, 0);
#endif

	gnutls_global_deinit();
}
