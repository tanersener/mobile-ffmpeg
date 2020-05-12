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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include <algorithms.h>
#include "errors.h"
#include <x509/common.h>
#include "c-strcase.h"


/* Note that all algorithms are in CBC or STREAM modes.
 * Do not add any algorithms in other modes (avoid modified algorithms).
 * View first: "The order of encryption and authentication for
 * protecting communications" by Hugo Krawczyk - CRYPTO 2001
 *
 * On update, make sure to update MAX_CIPHER_BLOCK_SIZE, MAX_CIPHER_IV_SIZE,
 * and MAX_CIPHER_KEY_SIZE as well.
 * If any ciphers are removed, remove them from the back-end but
 * keep them in that list to allow backwards compatibility with applications
 * that specify them (they will be a no-op).
 */
static const cipher_entry_st algorithms[] = {
	{ .name = "AES-256-CBC",
	  .id = GNUTLS_CIPHER_AES_256_CBC,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "AES-192-CBC",
	  .id = GNUTLS_CIPHER_AES_192_CBC,
	  .blocksize = 16,
	  .keysize = 24,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "AES-128-CBC",
	  .id = GNUTLS_CIPHER_AES_128_CBC,
	  .blocksize = 16,
	  .keysize = 16,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "AES-128-GCM",
	  .id = GNUTLS_CIPHER_AES_128_GCM,
	  .blocksize = 16,
	  .keysize = 16,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 4,
	  .explicit_iv = 8,
	  .cipher_iv = 12,
	  .tagsize = 16},
	{ .name = "AES-256-GCM",
	  .id = GNUTLS_CIPHER_AES_256_GCM,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 4,
	  .explicit_iv = 8,
	  .cipher_iv = 12,
	  .tagsize = 16},
	{ .name = "AES-128-CCM",
	  .id = GNUTLS_CIPHER_AES_128_CCM,
	  .blocksize = 16,
	  .keysize = 16,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 4,
	  .explicit_iv = 8,
	  .cipher_iv = 12,
	  .flags = GNUTLS_CIPHER_FLAG_ONLY_AEAD,
	  .tagsize = 16},
	{ .name = "AES-256-CCM",
	  .id = GNUTLS_CIPHER_AES_256_CCM,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 4,
	  .explicit_iv = 8,
	  .cipher_iv = 12,
	  .flags = GNUTLS_CIPHER_FLAG_ONLY_AEAD,
	  .tagsize = 16},
	{ .name = "AES-128-CCM-8",
	  .id = GNUTLS_CIPHER_AES_128_CCM_8,
	  .blocksize = 16,
	  .keysize = 16,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 4,
	  .explicit_iv = 8,
	  .cipher_iv = 12,
	  .flags = GNUTLS_CIPHER_FLAG_ONLY_AEAD,
	  .tagsize = 8},
	{ .name = "AES-256-CCM-8",
	  .id = GNUTLS_CIPHER_AES_256_CCM_8,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 4,
	  .explicit_iv = 8,
	  .cipher_iv = 12,
	  .flags = GNUTLS_CIPHER_FLAG_ONLY_AEAD,
	  .tagsize = 8},
	{ .name = "ARCFOUR-128",
	  .id = GNUTLS_CIPHER_ARCFOUR_128,
	  .blocksize = 1,
	  .keysize = 16,
	  .type = CIPHER_STREAM,
	 0, 0, 0, 0},
	{ .name = "ESTREAM-SALSA20-256",
	  .id = GNUTLS_CIPHER_ESTREAM_SALSA20_256,
	  .blocksize = 64,
	  .keysize = 32,
	 .type = CIPHER_STREAM, 0, 0, 8, 0},
	{ .name = "SALSA20-256",
	  .id = GNUTLS_CIPHER_SALSA20_256,
	  .blocksize = 64,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .explicit_iv = 0,
	  .cipher_iv = 8},
	{ .name = "CHACHA20-32",
	  .id = GNUTLS_CIPHER_CHACHA20_32,
	  .blocksize = 64,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .explicit_iv = 0,
	  /* IV includes counter */
	  .cipher_iv = 16},
	{ .name = "CHACHA20-64",
	  .id = GNUTLS_CIPHER_CHACHA20_64,
	  .blocksize = 64,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .explicit_iv = 0,
	  /* IV includes counter */
	  .cipher_iv = 16},
	{ .name = "CAMELLIA-256-CBC",
	  .id = GNUTLS_CIPHER_CAMELLIA_256_CBC,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "CAMELLIA-192-CBC",
	  .id = GNUTLS_CIPHER_CAMELLIA_192_CBC,
	  .blocksize = 16,
	  .keysize = 24,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "CAMELLIA-128-CBC",
	  .id = GNUTLS_CIPHER_CAMELLIA_128_CBC,
	  .blocksize = 16,
	  .keysize = 16,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "CHACHA20-POLY1305",
	  .id = GNUTLS_CIPHER_CHACHA20_POLY1305,
	  .blocksize = 64,
	  .keysize = 32,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 12,
	  .explicit_iv = 0,
	  /* in chacha20 we don't need a rekey after 2^24 messages */
	  .flags = GNUTLS_CIPHER_FLAG_XOR_NONCE | GNUTLS_CIPHER_FLAG_NO_REKEY,
	  .cipher_iv = 12,
	  .tagsize = 16
	},
	{ .name = "CAMELLIA-128-GCM",
	  .id = GNUTLS_CIPHER_CAMELLIA_128_GCM,
	  .blocksize = 16,
	  .keysize = 16,
	  .type = CIPHER_AEAD, 4, 8, 12, 16},
	{ .name = "CAMELLIA-256-GCM",
	  .id = GNUTLS_CIPHER_CAMELLIA_256_GCM,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_AEAD,
	  .implicit_iv = 4,
	  .explicit_iv = 8,
	  .cipher_iv = 12,
	  .tagsize = 16},
	{ .name = "GOST28147-TC26Z-CFB",
	  .id = GNUTLS_CIPHER_GOST28147_TC26Z_CFB,
	  .blocksize = 8,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .implicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "GOST28147-CPA-CFB",
	  .id = GNUTLS_CIPHER_GOST28147_CPA_CFB,
	  .blocksize = 8,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .implicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "GOST28147-CPB-CFB",
	  .id = GNUTLS_CIPHER_GOST28147_CPB_CFB,
	  .blocksize = 8,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .implicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "GOST28147-CPC-CFB",
	  .id = GNUTLS_CIPHER_GOST28147_CPC_CFB,
	  .blocksize = 8,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .implicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "GOST28147-CPD-CFB",
	  .id = GNUTLS_CIPHER_GOST28147_CPD_CFB,
	  .blocksize = 8,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .implicit_iv = 8,
	  .cipher_iv = 8},

	{ .name = "AES-128-CFB8",
	  .id = GNUTLS_CIPHER_AES_128_CFB8,
	  .blocksize = 16,
	  .keysize = 16,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "AES-192-CFB8",
	  .id = GNUTLS_CIPHER_AES_192_CFB8,
	  .blocksize = 16,
	  .keysize = 24,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "AES-256-CFB8",
	  .id = GNUTLS_CIPHER_AES_256_CFB8,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "AES-128-XTS",
	  .id = GNUTLS_CIPHER_AES_128_XTS,
	  .blocksize = 16,
	  .keysize = 32,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "AES-256-XTS",
	  .id = GNUTLS_CIPHER_AES_256_XTS,
	  .blocksize = 16,
	  .keysize = 64,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 16,
	  .cipher_iv = 16},
	{ .name = "GOST28147-TC26Z-CNT",
	  .id = GNUTLS_CIPHER_GOST28147_TC26Z_CNT,
	  .blocksize = 8,
	  .keysize = 32,
	  .type = CIPHER_STREAM,
	  .implicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "3DES-CBC",
	  .id = GNUTLS_CIPHER_3DES_CBC,
	  .blocksize = 8,
	  .keysize = 24,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "DES-CBC",
	  .id = GNUTLS_CIPHER_DES_CBC,
	  .blocksize = 8,
	  .keysize = 8,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "ARCFOUR-40",
	  .id = GNUTLS_CIPHER_ARCFOUR_40,
	  .blocksize = 1,
	  .keysize = 5,
	  .type = CIPHER_STREAM},
	{ .name = "RC2-40",
	  .id = GNUTLS_CIPHER_RC2_40_CBC,
	  .blocksize = 8,
	  .keysize = 5,
	  .type = CIPHER_BLOCK,
	  .explicit_iv = 8,
	  .cipher_iv = 8},
	{ .name = "NULL",
	  .id = GNUTLS_CIPHER_NULL,
	  .blocksize = 1,
	  .keysize = 0,
	  .type = CIPHER_STREAM
	},
	{0, 0, 0, 0, 0, 0, 0}
};

#define GNUTLS_CIPHER_LOOP(b) \
	const cipher_entry_st *p; \
		for(p = algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_ALG_LOOP(a) \
			GNUTLS_CIPHER_LOOP( if(p->id == algorithm) { a; break; } )

/* CIPHER functions */

const cipher_entry_st *_gnutls_cipher_to_entry(gnutls_cipher_algorithm_t c)
{
	GNUTLS_CIPHER_LOOP(if (c == p->id) return p);

	return NULL;
}

/* Returns cipher entry even for ciphers that are not supported,
 * but are listed (e.g., deprecated ciphers).
 */
const cipher_entry_st *cipher_name_to_entry(const char *name)
{
	GNUTLS_CIPHER_LOOP(
		if (c_strcasecmp(p->name, name) == 0) {
			return p;
		}
	);

	return NULL;
}

/**
 * gnutls_cipher_get_block_size:
 * @algorithm: is an encryption algorithm
 *
 * Returns: the block size of the encryption algorithm.
 *
 * Since: 2.10.0
 **/
unsigned gnutls_cipher_get_block_size(gnutls_cipher_algorithm_t algorithm)
{
	size_t ret = 0;
	GNUTLS_ALG_LOOP(ret = p->blocksize);
	return ret;

}

/**
 * gnutls_cipher_get_tag_size:
 * @algorithm: is an encryption algorithm
 *
 * This function returns the tag size of an authenticated encryption
 * algorithm. For non-AEAD algorithms, it returns zero.
 *
 * Returns: the tag size of the authenticated encryption algorithm.
 *
 * Since: 3.2.2
 **/
unsigned gnutls_cipher_get_tag_size(gnutls_cipher_algorithm_t algorithm)
{
	return _gnutls_cipher_get_tag_size(cipher_to_entry(algorithm));
}

/**
 * gnutls_cipher_get_iv_size:
 * @algorithm: is an encryption algorithm
 *
 * This function returns the size of the initialization vector (IV) for the
 * provided algorithm. For algorithms with variable size IV (e.g., AES-CCM),
 * the returned size will be the one used by TLS.
 *
 * Returns: block size for encryption algorithm.
 *
 * Since: 3.2.0
 **/
unsigned gnutls_cipher_get_iv_size(gnutls_cipher_algorithm_t algorithm)
{
	size_t ret = 0;
	GNUTLS_ALG_LOOP(ret = p->cipher_iv);
	return ret;
}

/**
 * gnutls_cipher_get_key_size:
 * @algorithm: is an encryption algorithm
 *
 * This function returns the key size of the provided algorithm.
 *
 * Returns: length (in bytes) of the given cipher's key size, or 0 if
 *   the given cipher is invalid.
 **/
size_t gnutls_cipher_get_key_size(gnutls_cipher_algorithm_t algorithm)
{				/* In bytes */
	size_t ret = 0;
	GNUTLS_ALG_LOOP(ret = p->keysize);
	return ret;

}

/**
 * gnutls_cipher_get_name:
 * @algorithm: is an encryption algorithm
 *
 * Convert a #gnutls_cipher_algorithm_t type to a string.
 *
 * Returns: a pointer to a string that contains the name of the
 *   specified cipher, or %NULL.
 **/
const char *gnutls_cipher_get_name(gnutls_cipher_algorithm_t algorithm)
{
	const char *ret = NULL;

	/* avoid prefix */
	GNUTLS_ALG_LOOP(ret = p->name);

	return ret;
}

/**
 * gnutls_cipher_get_id:
 * @name: is a cipher algorithm name
 *
 * The names are compared in a case insensitive way.
 *
 * Returns: return a #gnutls_cipher_algorithm_t value corresponding to
 *   the specified cipher, or %GNUTLS_CIPHER_UNKNOWN on error.
 **/
gnutls_cipher_algorithm_t gnutls_cipher_get_id(const char *name)
{
	gnutls_cipher_algorithm_t ret = GNUTLS_CIPHER_UNKNOWN;

	GNUTLS_CIPHER_LOOP(
		if (c_strcasecmp(p->name, name) == 0) {
			if (p->id == GNUTLS_CIPHER_NULL || _gnutls_cipher_exists(p->id))
				ret = p->id;
			break;
		}
	);

	return ret;
}

/**
 * gnutls_cipher_list:
 *
 * Get a list of supported cipher algorithms.  Note that not
 * necessarily all ciphers are supported as TLS cipher suites.  For
 * example, DES is not supported as a cipher suite, but is supported
 * for other purposes (e.g., PKCS#8 or similar).
 *
 * This function is not thread safe.
 *
 * Returns: a (0)-terminated list of #gnutls_cipher_algorithm_t
 *   integers indicating the available ciphers.
 *
 **/
const gnutls_cipher_algorithm_t *gnutls_cipher_list(void)
{
	static gnutls_cipher_algorithm_t supported_ciphers[MAX_ALGOS] =
	    { 0 };

	if (supported_ciphers[0] == 0) {
		int i = 0;

		GNUTLS_CIPHER_LOOP(
			if (p->id == GNUTLS_CIPHER_NULL || _gnutls_cipher_exists(p->id))
				supported_ciphers[i++] = p->id;
		);
		supported_ciphers[i++] = 0;
	}

	return supported_ciphers;
}
