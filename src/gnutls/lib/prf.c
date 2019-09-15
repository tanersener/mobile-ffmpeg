/*
 * Copyright (C) 2002-2015 Free Software Foundation, Inc.
 * Copyright (C) 2014-2015 Nikos Mavrogiannopoulos
 * Copyright (C) 2016-2017 Red Hat, Inc.
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

#include "gnutls_int.h"
#include "errors.h"
#include "handshake.h"
#include "secrets.h"
#include <num.h>
#include <state.h>
#include <algorithms.h>

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
 * Note: This function will only operate under TLS versions prior to 1.3.
 * In TLS1.3 the use of PRF is replaced with HKDF and the generic
 * exporters like gnutls_prf_rfc5705() should be used instead. Under
 * TLS1.3 this function returns %GNUTLS_E_INVALID_REQUEST.
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
	const version_entry_st *vers = get_version(session);

	if (vers && vers->tls13_sem)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = _gnutls_prf_raw(session->security_parameters.prf->id,
			  GNUTLS_MASTER_SIZE, session->security_parameters.master_secret,
			  label_size, label,
			  seed_size, (uint8_t *) seed,
			  outsize, out);

	return ret;
}

static int
_tls13_derive_exporter(const mac_entry_st *prf,
		       gnutls_session_t session,
		       size_t label_size, const char *label,
		       size_t context_size, const char *context,
		       size_t outsize, char *out,
		       bool early)
{
	uint8_t secret[MAX_HASH_SIZE];
	uint8_t digest[MAX_HASH_SIZE];
	unsigned digest_size = prf->output_size;
	int ret;

	ret = _tls13_derive_secret2(prf, label, label_size, NULL, 0,
				    session->key.proto.tls13.ap_expkey,
				    secret);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_hash_fast((gnutls_digest_algorithm_t)prf->id,
			       context, context_size, digest);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _tls13_expand_secret2(prf,
				     EXPORTER_LABEL, sizeof(EXPORTER_LABEL)-1,
				     digest, digest_size,
				     secret, outsize, out);
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
 * Exports keying material from TLS/DTLS session to an application, as
 * specified in RFC5705.
 *
 * In the TLS versions prior to 1.3, it applies the TLS
 * Pseudo-Random-Function (PRF) on the master secret and the provided
 * data, seeded with the client and server random fields.
 *
 * In TLS 1.3, it applies HKDF on the exporter master secret derived
 * from the master secret.
 *
 * The @label variable usually contains a string denoting the purpose
 * for the generated data.
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
	const version_entry_st *vers = get_version(session);
	int ret;

	if (vers && vers->tls13_sem) {
		ret = _tls13_derive_exporter(session->security_parameters.prf,
					     session,
					     label_size, label,
					     context_size, context,
					     outsize, out,
					     0);
	} else {
		char *pctx = NULL;

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
	}

	return ret;
}

/**
 * gnutls_prf_early:
 * @session: is a #gnutls_session_t type.
 * @label_size: length of the @label variable.
 * @label: label used in PRF computation, typically a short string.
 * @context_size: length of the @extra variable.
 * @context: optional extra data to seed the PRF with.
 * @outsize: size of pre-allocated output buffer to hold the output.
 * @out: pre-allocated buffer to hold the generated data.
 *
 * This function is similar to gnutls_prf_rfc5705(), but only works in
 * TLS 1.3 or later to export early keying material.
 *
 * Note that the keying material is only available after the
 * ClientHello message is processed and before the application traffic
 * keys are established.  Therefore this function shall be called in a
 * handshake hook function for %GNUTLS_HANDSHAKE_CLIENT_HELLO.
 *
 * The @label variable usually contains a string denoting the purpose
 * for the generated data.
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
 * Since: 3.6.8
 **/
int
gnutls_prf_early(gnutls_session_t session,
		 size_t label_size, const char *label,
		 size_t context_size, const char *context,
		 size_t outsize, char *out)
{
	if (session->internals.initial_negotiation_completed ||
	    session->key.binders[0].prf == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	return _tls13_derive_exporter(session->key.binders[0].prf, session,
				      label_size, label,
				      context_size, context,
				      outsize, out,
				      1);
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
 * when @server_random_first is set to 0 and @extra is %NULL. Under TLS1.3
 * this function will only operate when these conditions are true, or otherwise
 * return %GNUTLS_E_INVALID_REQUEST.
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
	const version_entry_st *vers = get_version(session);
	size_t seedsize = 2 * GNUTLS_RANDOM_SIZE + extra_size;

	if (vers && vers->tls13_sem) {
		if (extra == NULL && server_random_first == 0)
			return gnutls_prf_rfc5705(session, label_size, label,
						  extra_size, extra, outsize, out);
		else
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

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
	    _gnutls_prf_raw(session->security_parameters.prf->id,
			GNUTLS_MASTER_SIZE, session->security_parameters.master_secret,
			label_size, label,
			seedsize, seed,
			outsize, out);

	gnutls_free(seed);

	return ret;
}

