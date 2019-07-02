/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
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

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include <crypto-backend.h>
#include <crypto.h>
#include <mpi.h>
#include <pk.h>
#include <random.h>
#include <cipher_int.h>

/* default values for priorities */
int crypto_mac_prio = INT_MAX;
int crypto_digest_prio = INT_MAX;
int crypto_cipher_prio = INT_MAX;

typedef struct algo_list {
	int algorithm;
	int priority;
	void *alg_data;
	int free_alg_data;
	struct algo_list *next;
} algo_list;

#define cipher_list algo_list
#define mac_list algo_list
#define digest_list algo_list

static int
_algo_register(algo_list * al, int algorithm, int priority, void *s, int free_s)
{
	algo_list *cl;
	algo_list *last_cl = al;
	int ret;

	if (al == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	/* look if there is any cipher with lowest priority. In that case do not add.
	 */
	cl = al;
	while (cl && cl->alg_data) {
		if (cl->algorithm == algorithm) {
			if (cl->priority < priority) {
				gnutls_assert();
				ret = GNUTLS_E_CRYPTO_ALREADY_REGISTERED;
				goto cleanup;
			} else {
				/* the current has higher priority -> overwrite */
				cl->algorithm = algorithm;
				cl->priority = priority;
				cl->alg_data = s;
				cl->free_alg_data = free_s;
				return 0;
			}
		}
		cl = cl->next;
		if (cl)
			last_cl = cl;
	}

	cl = gnutls_calloc(1, sizeof(cipher_list));

	if (cl == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	last_cl->algorithm = algorithm;
	last_cl->priority = priority;
	last_cl->alg_data = s;
	last_cl->free_alg_data = free_s;
	last_cl->next = cl;

	return 0;
 cleanup:
	if (free_s) gnutls_free(s);
	return ret;
}

static const void *_get_algo(algo_list * al, int algo)
{
	cipher_list *cl;

	/* look if there is any cipher with lowest priority. In that case do not add.
	 */
	cl = al;
	while (cl && cl->alg_data) {
		if (cl->algorithm == algo) {
			return cl->alg_data;
		}
		cl = cl->next;
	}

	return NULL;
}

static cipher_list glob_cl = { GNUTLS_CIPHER_NULL, 0, NULL, 0, NULL };
static mac_list glob_ml = { GNUTLS_MAC_NULL, 0, NULL, 0, NULL };
static digest_list glob_dl = { GNUTLS_MAC_NULL, 0, NULL, 0, NULL };

static void _deregister(algo_list * cl)
{
	algo_list *next;

	next = cl->next;
	cl->next = NULL;
	cl = next;

	while (cl) {
		next = cl->next;
		if (cl->free_alg_data)
			gnutls_free(cl->alg_data);
		gnutls_free(cl);
		cl = next;
	}
}

void _gnutls_crypto_deregister(void)
{
	_deregister(&glob_cl);
	_deregister(&glob_ml);
	_deregister(&glob_dl);
}

/*-
 * gnutls_crypto_single_cipher_register:
 * @algorithm: is the gnutls algorithm identifier
 * @priority: is the priority of the algorithm
 * @s: is a structure holding new cipher's data
 *
 * This function will register a cipher algorithm to be used by
 * gnutls.  Any algorithm registered will override the included
 * algorithms and by convention kernel implemented algorithms have
 * priority of 90 and CPU-assisted of 80.  The algorithm with the lowest priority will be
 * used by gnutls.
 *
 * In the case the registered init or setkey functions return %GNUTLS_E_NEED_FALLBACK,
 * GnuTLS will attempt to use the next in priority registered cipher.
 *
 * This function should be called before gnutls_global_init().
 *
 * For simplicity you can use the convenience
 * gnutls_crypto_single_cipher_register() macro.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 2.6.0
 -*/
int
gnutls_crypto_single_cipher_register(gnutls_cipher_algorithm_t algorithm,
				     int priority,
				     const gnutls_crypto_cipher_st * s,
				     int free_s)
{
	/* we override const in case free_s is set */
	return _algo_register(&glob_cl, algorithm, priority, (void*)s, free_s);
}

const gnutls_crypto_cipher_st
    *_gnutls_get_crypto_cipher(gnutls_cipher_algorithm_t algo)
{
	return _get_algo(&glob_cl, algo);
}

/**
 * gnutls_crypto_register_cipher:
 * @algorithm: is the gnutls algorithm identifier
 * @priority: is the priority of the algorithm
 * @init: A function which initializes the cipher
 * @setkey: A function which sets the key of the cipher
 * @setiv: A function which sets the nonce/IV of the cipher (non-AEAD)
 * @encrypt: A function which performs encryption (non-AEAD)
 * @decrypt: A function which performs decryption (non-AEAD)
 * @deinit: A function which deinitializes the cipher
 *
 * This function will register a cipher algorithm to be used by
 * gnutls.  Any algorithm registered will override the included
 * algorithms and by convention kernel implemented algorithms have
 * priority of 90 and CPU-assisted of 80.  The algorithm with the lowest priority will be
 * used by gnutls.
 *
 * In the case the registered init or setkey functions return %GNUTLS_E_NEED_FALLBACK,
 * GnuTLS will attempt to use the next in priority registered cipher.
 *
 * The functions which are marked as non-AEAD they are not required when
 * registering a cipher to be used with the new AEAD API introduced in
 * GnuTLS 3.4.0. Internally GnuTLS uses the new AEAD API.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_crypto_register_cipher(gnutls_cipher_algorithm_t algorithm,
			      int priority,
			      gnutls_cipher_init_func init,
			      gnutls_cipher_setkey_func setkey,
			      gnutls_cipher_setiv_func setiv,
			      gnutls_cipher_encrypt_func encrypt,
			      gnutls_cipher_decrypt_func decrypt,
			      gnutls_cipher_deinit_func deinit)
{
	gnutls_crypto_cipher_st *s = gnutls_calloc(1, sizeof(gnutls_crypto_cipher_st));
	if (s == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	s->init = init;
	s->setkey = setkey;
	s->setiv = setiv;
	s->encrypt = encrypt;
	s->decrypt = decrypt;
	s->deinit = deinit;

	return gnutls_crypto_single_cipher_register(algorithm, priority, s, 1);
}

/**
 * gnutls_crypto_register_aead_cipher:
 * @algorithm: is the gnutls AEAD cipher identifier
 * @priority: is the priority of the algorithm
 * @init: A function which initializes the cipher
 * @setkey: A function which sets the key of the cipher
 * @aead_encrypt: Perform the AEAD encryption
 * @aead_decrypt: Perform the AEAD decryption
 * @deinit: A function which deinitializes the cipher
 *
 * This function will register a cipher algorithm to be used by
 * gnutls.  Any algorithm registered will override the included
 * algorithms and by convention kernel implemented algorithms have
 * priority of 90 and CPU-assisted of 80.  The algorithm with the lowest priority will be
 * used by gnutls.
 *
 * In the case the registered init or setkey functions return %GNUTLS_E_NEED_FALLBACK,
 * GnuTLS will attempt to use the next in priority registered cipher.
 *
 * The functions registered will be used with the new AEAD API introduced in
 * GnuTLS 3.4.0. Internally GnuTLS uses the new AEAD API.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_crypto_register_aead_cipher(gnutls_cipher_algorithm_t algorithm,
			      int priority,
			      gnutls_cipher_init_func init,
			      gnutls_cipher_setkey_func setkey,
			      gnutls_cipher_aead_encrypt_func aead_encrypt,
			      gnutls_cipher_aead_decrypt_func aead_decrypt,
			      gnutls_cipher_deinit_func deinit)
{
	gnutls_crypto_cipher_st *s = gnutls_calloc(1, sizeof(gnutls_crypto_cipher_st));
	if (s == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	s->init = init;
	s->setkey = setkey;
	s->aead_encrypt = aead_encrypt;
	s->aead_decrypt = aead_decrypt;
	s->deinit = deinit;

	return gnutls_crypto_single_cipher_register(algorithm, priority, s, 1);
}

/*-
 * gnutls_crypto_rnd_register:
 * @priority: is the priority of the generator
 * @s: is a structure holding new generator's data
 *
 * This function will register a random generator to be used by
 * gnutls.  Any generator registered will override the included
 * generator and by convention kernel implemented generators have
 * priority of 90 and CPU-assisted of 80. The generator with the lowest priority will be
 * used by gnutls.
 *
 * This function should be called before gnutls_global_init().
 *
 * For simplicity you can use the convenience
 * gnutls_crypto_rnd_register() macro.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 2.6.0
 -*/
int
gnutls_crypto_rnd_register(int priority, const gnutls_crypto_rnd_st * s)
{
	if (crypto_rnd_prio >= priority) {
		memcpy(&_gnutls_rnd_ops, s, sizeof(*s));
		crypto_rnd_prio = priority;
		return 0;
	}

	return GNUTLS_E_CRYPTO_ALREADY_REGISTERED;
}

/*-
 * gnutls_crypto_single_mac_register:
 * @algorithm: is the gnutls algorithm identifier
 * @priority: is the priority of the algorithm
 * @s: is a structure holding new algorithms's data
 *
 * This function will register a MAC algorithm to be used by gnutls.
 * Any algorithm registered will override the included algorithms and
 * by convention kernel implemented algorithms have priority of 90
 *  and CPU-assisted of 80.
 * The algorithm with the lowest priority will be used by gnutls.
 *
 * This function should be called before gnutls_global_init().
 *
 * For simplicity you can use the convenience
 * gnutls_crypto_single_mac_register() macro.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 2.6.0
 -*/
int
gnutls_crypto_single_mac_register(gnutls_mac_algorithm_t algorithm,
				  int priority,
				  const gnutls_crypto_mac_st * s,
				  int free_s)
{
	return _algo_register(&glob_ml, algorithm, priority, (void*)s, free_s);
}

const gnutls_crypto_mac_st *_gnutls_get_crypto_mac(gnutls_mac_algorithm_t
						   algo)
{
	return _get_algo(&glob_ml, algo);
}

/*-
 * gnutls_crypto_single_digest_register:
 * @algorithm: is the gnutls algorithm identifier
 * @priority: is the priority of the algorithm
 * @s: is a structure holding new algorithms's data
 *
 * This function will register a digest (hash) algorithm to be used by
 * gnutls.  Any algorithm registered will override the included
 * algorithms and by convention kernel implemented algorithms have
 * priority of 90  and CPU-assisted of 80.  The algorithm with the lowest priority will be
 * used by gnutls.
 *
 * This function should be called before gnutls_global_init().
 *
 * For simplicity you can use the convenience
 * gnutls_crypto_single_digest_register() macro.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 2.6.0
 -*/
int
gnutls_crypto_single_digest_register(gnutls_digest_algorithm_t algorithm,
				     int priority,
				     const gnutls_crypto_digest_st * s,
				     int free_s)
{
	return _algo_register(&glob_dl, algorithm, priority, (void*)s, free_s);
}

const gnutls_crypto_digest_st
    *_gnutls_get_crypto_digest(gnutls_digest_algorithm_t algo)
{
	return _get_algo(&glob_dl, algo);
}

/**
 * gnutls_crypto_register_mac:
 * @algorithm: is the gnutls MAC identifier
 * @priority: is the priority of the algorithm
 * @init: A function which initializes the MAC
 * @setkey: A function which sets the key of the MAC
 * @setnonce: A function which sets the nonce for the mac (may be %NULL for common MAC algorithms)
 * @hash: Perform the hash operation
 * @output: Provide the output of the MAC
 * @deinit: A function which deinitializes the MAC
 * @hash_fast: Perform the MAC operation in one go
 *
 * This function will register a MAC algorithm to be used by gnutls.
 * Any algorithm registered will override the included algorithms and
 * by convention kernel implemented algorithms have priority of 90
 *  and CPU-assisted of 80.
 * The algorithm with the lowest priority will be used by gnutls.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_crypto_register_mac(gnutls_mac_algorithm_t algorithm,
			   int priority,
			   gnutls_mac_init_func init,
			   gnutls_mac_setkey_func setkey,
			   gnutls_mac_setnonce_func setnonce,
			   gnutls_mac_hash_func hash,
			   gnutls_mac_output_func output,
			   gnutls_mac_deinit_func deinit,
			   gnutls_mac_fast_func hash_fast)
{
	gnutls_crypto_mac_st *s = gnutls_calloc(1, sizeof(gnutls_crypto_mac_st));
	if (s == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	s->init = init;
	s->setkey = setkey;
	s->setnonce = setnonce;
	s->hash = hash;
	s->output = output;
	s->fast = hash_fast;
	s->deinit = deinit;

	return gnutls_crypto_single_mac_register(algorithm, priority, s, 1);
}

/**
 * gnutls_crypto_register_digest:
 * @algorithm: is the gnutls digest identifier
 * @priority: is the priority of the algorithm
 * @init: A function which initializes the digest
 * @hash: Perform the hash operation
 * @output: Provide the output of the digest
 * @deinit: A function which deinitializes the digest
 * @hash_fast: Perform the digest operation in one go
 *
 * This function will register a digest algorithm to be used by gnutls.
 * Any algorithm registered will override the included algorithms and
 * by convention kernel implemented algorithms have priority of 90
 *  and CPU-assisted of 80.
 * The algorithm with the lowest priority will be used by gnutls.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_crypto_register_digest(gnutls_digest_algorithm_t algorithm,
			   int priority,
			   gnutls_digest_init_func init,
			   gnutls_digest_hash_func hash,
			   gnutls_digest_output_func output,
			   gnutls_digest_deinit_func deinit,
			   gnutls_digest_fast_func hash_fast)
{
	gnutls_crypto_digest_st *s = gnutls_calloc(1, sizeof(gnutls_crypto_digest_st));
	if (s == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	s->init = init;
	s->hash = hash;
	s->output = output;
	s->fast = hash_fast;
	s->deinit = deinit;

	return gnutls_crypto_single_digest_register(algorithm, priority, s, 1);
}
