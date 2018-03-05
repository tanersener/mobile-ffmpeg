/*
 * Copyright (C) 2002-2015 Free Software Foundation, Inc.
 * Copyright (C) 2014-2015 Nikos Mavrogiannopoulos
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

/* Functions for the TLS PRF handling.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <num.h>
#include <state.h>
#include <algorithms.h>

#define MAX_PRF_BYTES 200
#define MAX_SEED_SIZE 200

inline static int
_gnutls_cal_PRF_A(const mac_entry_st * me,
		  const void *secret, int secret_size,
		  const void *seed, int seed_size, void *result)
{
	int ret;

	ret =
	    _gnutls_mac_fast(me->id, secret, secret_size, seed, seed_size,
			     result);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/* Produces "total_bytes" bytes using the hash algorithm specified.
 * (used in the PRF function)
 */
static int
P_hash(gnutls_mac_algorithm_t algorithm,
       const uint8_t * secret, int secret_size,
       const uint8_t * seed, int seed_size, int total_bytes, uint8_t * ret)
{

	mac_hd_st td2;
	int i, times, how, blocksize, A_size;
	uint8_t final[MAX_HASH_SIZE], Atmp[MAX_SEED_SIZE];
	int output_bytes, result;
	const mac_entry_st *me = mac_to_entry(algorithm);

	blocksize = _gnutls_mac_get_algo_len(me);

	if (seed_size > MAX_SEED_SIZE || total_bytes <= 0 || blocksize == 0) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	output_bytes = 0;
	do {
		output_bytes += blocksize;
	}
	while (output_bytes < total_bytes);

	/* calculate A(0) */

	memcpy(Atmp, seed, seed_size);
	A_size = seed_size;

	times = output_bytes / blocksize;

	for (i = 0; i < times; i++) {
		result = _gnutls_mac_init(&td2, me, secret, secret_size);
		if (result < 0) {
			gnutls_assert();
			return result;
		}

		/* here we calculate A(i+1) */
		if ((result =
		     _gnutls_cal_PRF_A(me, secret, secret_size, Atmp,
				       A_size, Atmp)) < 0) {
			gnutls_assert();
			_gnutls_mac_deinit(&td2, final);
			return result;
		}

		A_size = blocksize;

		_gnutls_mac(&td2, Atmp, A_size);
		_gnutls_mac(&td2, seed, seed_size);
		_gnutls_mac_deinit(&td2, final);

		if ((1 + i) * blocksize < total_bytes) {
			how = blocksize;
		} else {
			how = total_bytes - (i) * blocksize;
		}

		if (how > 0) {
			memcpy(&ret[i * blocksize], final, how);
		}
	}

	return 0;
}

/* This function operates as _gnutls_PRF(), but does not require
 * a pointer to the current session. It takes the @mac algorithm
 * explicitly. For legacy TLS/SSL sessions before TLS 1.2 the MAC
 * must be set to %GNUTLS_MAC_UNKNOWN.
 */
static int
_gnutls_PRF_raw(gnutls_mac_algorithm_t mac,
		const uint8_t * secret, unsigned int secret_size,
		const char *label, int label_size, const uint8_t * seed,
		int seed_size, int total_bytes, void *ret)
{
	int l_s, s_seed_size;
	const uint8_t *s1, *s2;
	uint8_t s_seed[MAX_SEED_SIZE];
	uint8_t o1[MAX_PRF_BYTES], o2[MAX_PRF_BYTES];
	int result;

	if (total_bytes > MAX_PRF_BYTES) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}
	/* label+seed = s_seed */
	s_seed_size = seed_size + label_size;

	if (s_seed_size > MAX_SEED_SIZE) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	memcpy(s_seed, label, label_size);
	memcpy(&s_seed[label_size], seed, seed_size);

	if (mac != GNUTLS_MAC_UNKNOWN) {
		result =
		    P_hash(mac, secret, secret_size,
			   s_seed, s_seed_size,
			   total_bytes, ret);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	} else {
		l_s = secret_size / 2;

		s1 = &secret[0];
		s2 = &secret[l_s];

		if (secret_size % 2 != 0) {
			l_s++;
		}

		result =
		    P_hash(GNUTLS_MAC_MD5, s1, l_s, s_seed, s_seed_size,
			   total_bytes, o1);
		if (result < 0) {
			gnutls_assert();
			return result;
		}

		result =
		    P_hash(GNUTLS_MAC_SHA1, s2, l_s, s_seed, s_seed_size,
			   total_bytes, o2);
		if (result < 0) {
			gnutls_assert();
			return result;
		}

		memxor(o1, o2, total_bytes);

		memcpy(ret, o1, total_bytes);
	}

	return 0;		/* ok */
}


/* The PRF function expands a given secret 
 * needed by the TLS specification. ret must have a least total_bytes
 * available.
 */
int
_gnutls_PRF(gnutls_session_t session,
	    const uint8_t * secret, unsigned int secret_size,
	    const char *label, int label_size, const uint8_t * seed,
	    int seed_size, int total_bytes, void *ret)
{
	const version_entry_st *ver = get_version(session);

	if (_gnutls_version_has_selectable_prf(ver)) {
		return _gnutls_PRF_raw(
			_gnutls_cipher_suite_get_prf(session->security_parameters.cipher_suite),
			secret, secret_size,
			label, label_size,
			seed, seed_size,
			total_bytes,
			ret);
	} else {
		return _gnutls_PRF_raw(
			GNUTLS_MAC_UNKNOWN,
			secret, secret_size,
			label, label_size,
			seed, seed_size,
			total_bytes,
			ret);
	}
}

#ifdef ENABLE_FIPS140
int
_gnutls_prf_raw(gnutls_mac_algorithm_t mac,
		size_t master_size, const void *master,
		size_t label_size, const char *label,
		size_t seed_size, const char *seed, size_t outsize,
		char *out);

/*-
 * _gnutls_prf_raw:
 * @mac: the MAC algorithm to use, set to %GNUTLS_MAC_UNKNOWN for the TLS1.0 mac
 * @master_size: length of the @master variable.
 * @master: the master secret used in PRF computation
 * @label_size: length of the @label variable.
 * @label: label used in PRF computation, typically a short string.
 * @seed_size: length of the @seed variable.
 * @seed: optional extra data to seed the PRF with.
 * @outsize: size of pre-allocated output buffer to hold the output.
 * @out: pre-allocated buffer to hold the generated data.
 *
 * Apply the TLS Pseudo-Random-Function (PRF) on the master secret
 * and the provided data.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 -*/
int
_gnutls_prf_raw(gnutls_mac_algorithm_t mac,
		size_t master_size, const void *master,
		size_t label_size, const char *label,
		size_t seed_size, const char *seed, size_t outsize,
		char *out)
{
	return _gnutls_PRF_raw(mac,
			  master, master_size,
			  label, label_size,
			  (uint8_t *) seed, seed_size,
			  outsize, out);

}
#endif

/**
 * gnutls_prf_raw:
 * @session: is a #gnutls_session_t type.
 * @label_size: length of the @label variable.
 * @label: label used in PRF computation, typically a short string.
 * @seed_size: length of the @seed variable.
 * @seed: optional extra data to seed the PRF with.
 * @outsize: size of pre-allocated output buffer to hold the output.
 * @out: pre-allocated buffer to hold the generated data.
 *
 * Apply the TLS Pseudo-Random-Function (PRF) on the master secret
 * and the provided data.
 *
 * The @label variable usually contains a string denoting the purpose
 * for the generated data.  The @seed usually contains data such as the
 * client and server random, perhaps together with some additional
 * data that is added to guarantee uniqueness of the output for a
 * particular purpose.
 *
 * Because the output is not guaranteed to be unique for a particular
 * session unless @seed includes the client random and server random
 * fields (the PRF would output the same data on another connection
 * resumed from the first one), it is not recommended to use this
 * function directly.  The gnutls_prf() function seeds the PRF with the
 * client and server random fields directly, and is recommended if you
 * want to generate pseudo random data unique for each session.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_prf_raw(gnutls_session_t session,
	       size_t label_size,
	       const char *label,
	       size_t seed_size, const char *seed, size_t outsize,
	       char *out)
{
	int ret;

	ret = _gnutls_PRF(session,
			  session->security_parameters.master_secret,
			  GNUTLS_MASTER_SIZE,
			  label,
			  label_size, (uint8_t *) seed, seed_size, outsize,
			  out);

	return ret;
}

/**
 * gnutls_prf_rfc5705:
 * @session: is a #gnutls_session_t type.
 * @label_size: length of the @label variable.
 * @label: label used in PRF computation, typically a short string.
 * @context_size: length of the @extra variable.
 * @context: optional extra data to seed the PRF with.
 * @outsize: size of pre-allocated output buffer to hold the output.
 * @out: pre-allocated buffer to hold the generated data.
 *
 * Applies the TLS Pseudo-Random-Function (PRF) on the master secret
 * and the provided data, seeded with the client and server random fields,
 * as specified in RFC5705.
 *
 * The @label variable usually contains a string denoting the purpose
 * for the generated data.  The @server_random_first indicates whether
 * the client random field or the server random field should be first
 * in the seed.  Non-zero indicates that the server random field is first,
 * 0 that the client random field is first.
 *
 * The @context variable can be used to add more data to the seed, after
 * the random variables.  It can be used to make sure the
 * generated output is strongly connected to some additional data
 * (e.g., a string used in user authentication). 
 *
 * The output is placed in @out, which must be pre-allocated.
 *
 * Note that, to provide the RFC5705 context, the @context variable
 * must be non-null.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 *
 * Since: 3.4.4
 **/
int
gnutls_prf_rfc5705(gnutls_session_t session,
		   size_t label_size, const char *label,
		   size_t context_size, const char *context,
		   size_t outsize, char *out)
{
	char *pctx = NULL;
	int ret;

	if (context != NULL && context_size > 65535)  {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (context != NULL) {
		pctx = gnutls_malloc(context_size+2);
		if (!pctx) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		memcpy(pctx+2, context, context_size);
		_gnutls_write_uint16(context_size, (void*)pctx);
		context_size += 2;
	}

	ret = gnutls_prf(session, label_size, label, 0,
			 context_size, pctx, outsize, out);

	gnutls_free(pctx);
	return ret;
}

/**
 * gnutls_prf:
 * @session: is a #gnutls_session_t type.
 * @label_size: length of the @label variable.
 * @label: label used in PRF computation, typically a short string.
 * @server_random_first: non-zero if server random field should be first in seed
 * @extra_size: length of the @extra variable.
 * @extra: optional extra data to seed the PRF with.
 * @outsize: size of pre-allocated output buffer to hold the output.
 * @out: pre-allocated buffer to hold the generated data.
 *
 * Applies the TLS Pseudo-Random-Function (PRF) on the master secret
 * and the provided data, seeded with the client and server random fields.
 * For the key expansion specified in RFC5705 see gnutls_prf_rfc5705().
 *
 * The @label variable usually contains a string denoting the purpose
 * for the generated data.  The @server_random_first indicates whether
 * the client random field or the server random field should be first
 * in the seed.  Non-zero indicates that the server random field is first,
 * 0 that the client random field is first.
 *
 * The @extra variable can be used to add more data to the seed, after
 * the random variables.  It can be used to make sure the
 * generated output is strongly connected to some additional data
 * (e.g., a string used in user authentication).
 *
 * The output is placed in @out, which must be pre-allocated.
 *
 * Note: This function produces identical output with gnutls_prf_rfc5705()
 * when @server_random_first is set to 0 and @extra is %NULL.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_prf(gnutls_session_t session,
	   size_t label_size,
	   const char *label,
	   int server_random_first,
	   size_t extra_size, const char *extra,
	   size_t outsize, char *out)
{
	int ret;
	uint8_t *seed;
	size_t seedsize = 2 * GNUTLS_RANDOM_SIZE + extra_size;

	seed = gnutls_malloc(seedsize);
	if (!seed) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	memcpy(seed, server_random_first ?
	       session->security_parameters.server_random :
	       session->security_parameters.client_random,
	       GNUTLS_RANDOM_SIZE);
	memcpy(seed + GNUTLS_RANDOM_SIZE,
	       server_random_first ? session->security_parameters.
	       client_random : session->security_parameters.server_random,
	       GNUTLS_RANDOM_SIZE);

	if (extra && extra_size) {
		memcpy(seed + 2 * GNUTLS_RANDOM_SIZE, extra, extra_size);
	}

	ret =
	    _gnutls_PRF(session,
			session->security_parameters.master_secret,
			GNUTLS_MASTER_SIZE, label, label_size, seed,
			seedsize, outsize, out);

	gnutls_free(seed);

	return ret;
}

