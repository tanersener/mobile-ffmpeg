/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

#include "gnutls_int.h"
#include "errors.h"
#include <x509_b64.h>
#include <auth/cert.h>
#include <algorithms.h>
#include <datum.h>
#include <mpi.h>
#include <global.h>
#include <pk.h>
#include <debug.h>
#include <buffers.h>
#include <tls-sig.h>
#include <kx.h>
#include <libtasn1.h>
#include <ext/signature.h>
#include <state.h>
#include <x509/common.h>
#include <abstract_int.h>

int _gnutls_check_key_usage_for_sig(gnutls_session_t session, unsigned key_usage, unsigned our_cert)
{
	const char *lstr;
	unsigned allow_key_usage_violation;

	if (our_cert) {
		lstr = "Local";
		allow_key_usage_violation = session->internals.priorities->allow_server_key_usage_violation;
	} else {
		lstr = "Peer's";
		allow_key_usage_violation = session->internals.allow_key_usage_violation;
	}

	if (key_usage != 0) {
		if (!(key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE)) {
			gnutls_assert();
			if (likely(allow_key_usage_violation == 0)) {
				_gnutls_audit_log(session,
					  "%s certificate does not allow digital signatures. Key usage violation detected.\n", lstr);
				return GNUTLS_E_KEY_USAGE_VIOLATION;
			} else {
				_gnutls_audit_log(session,
					  "%s certificate does not allow digital signatures. Key usage violation detected (ignored).\n", lstr);
			}
		}
	}
	return 0;
}

/* Generates a signature of all the random data and the parameters.
 * Used in *DHE_* ciphersuites for TLS 1.2.
 */
static int
_gnutls_handshake_sign_data12(gnutls_session_t session,
			    gnutls_pcert_st * cert, gnutls_privkey_t pkey,
			    gnutls_datum_t * params,
			    gnutls_datum_t * signature,
			    gnutls_sign_algorithm_t sign_algo)
{
	gnutls_datum_t dconcat;
	int ret;

	_gnutls_handshake_log
	    ("HSK[%p]: signing TLS 1.2 handshake data: using %s\n", session,
	     gnutls_sign_algorithm_get_name(sign_algo));

	if (unlikely(gnutls_sign_supports_pk_algorithm(sign_algo, pkey->pk_algorithm) == 0))
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	dconcat.size = GNUTLS_RANDOM_SIZE*2 + params->size;
	dconcat.data = gnutls_malloc(dconcat.size);
	if (dconcat.data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	memcpy(dconcat.data, session->security_parameters.client_random, GNUTLS_RANDOM_SIZE);
	memcpy(dconcat.data+GNUTLS_RANDOM_SIZE, session->security_parameters.server_random, GNUTLS_RANDOM_SIZE);
	memcpy(dconcat.data+GNUTLS_RANDOM_SIZE*2, params->data, params->size);

	ret = gnutls_privkey_sign_data2(pkey, sign_algo,
					0, &dconcat, signature);
	if (ret < 0) {
		gnutls_assert();
	}
	gnutls_free(dconcat.data);

	return ret;

}

static int
_gnutls_handshake_sign_data10(gnutls_session_t session,
			    gnutls_pcert_st * cert, gnutls_privkey_t pkey,
			    gnutls_datum_t * params,
			    gnutls_datum_t * signature,
			    gnutls_sign_algorithm_t sign_algo)
{
	gnutls_datum_t dconcat;
	int ret;
	digest_hd_st td_sha;
	uint8_t concat[MAX_SIG_SIZE];
	const mac_entry_st *me;
	gnutls_pk_algorithm_t pk_algo;

	pk_algo = gnutls_privkey_get_pk_algorithm(pkey, NULL);
	if (pk_algo == GNUTLS_PK_RSA)
		me = hash_to_entry(GNUTLS_DIG_MD5_SHA1);
	else
		me = hash_to_entry(
				gnutls_sign_get_hash_algorithm(sign_algo));
	if (me == NULL)
		return gnutls_assert_val(GNUTLS_E_UNKNOWN_HASH_ALGORITHM);

	if (unlikely(gnutls_sign_supports_pk_algorithm(sign_algo, pk_algo) == 0))
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	pk_algo = gnutls_sign_get_pk_algorithm(sign_algo);
	if (pk_algo == GNUTLS_PK_UNKNOWN)
		return gnutls_assert_val(GNUTLS_E_UNKNOWN_PK_ALGORITHM);

	_gnutls_handshake_log
	    ("HSK[%p]: signing handshake data: using %s\n", session,
	     gnutls_sign_algorithm_get_name(sign_algo));

	ret = _gnutls_hash_init(&td_sha, me);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_hash(&td_sha, session->security_parameters.client_random,
		     GNUTLS_RANDOM_SIZE);
	_gnutls_hash(&td_sha, session->security_parameters.server_random,
		     GNUTLS_RANDOM_SIZE);
	_gnutls_hash(&td_sha, params->data, params->size);

	_gnutls_hash_deinit(&td_sha, concat);

	dconcat.data = concat;
	dconcat.size = _gnutls_hash_get_algo_len(me);

	ret = gnutls_privkey_sign_hash(pkey, me->id, GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
				       &dconcat, signature);
	if (ret < 0) {
		gnutls_assert();
	}

	return ret;
}

/* Generates a signature of all the random data and the parameters.
 * Used in DHE_* ciphersuites.
 */
int
_gnutls_handshake_sign_data(gnutls_session_t session,
			    gnutls_pcert_st * cert, gnutls_privkey_t pkey,
			    gnutls_datum_t * params,
			    gnutls_datum_t * signature,
			    gnutls_sign_algorithm_t * sign_algo)
{
	const version_entry_st *ver = get_version(session);
	unsigned key_usage = 0;
	int ret;

	*sign_algo = session->security_parameters.server_sign_algo;
	if (*sign_algo == GNUTLS_SIGN_UNKNOWN) {
		gnutls_assert();
		return GNUTLS_E_UNWANTED_ALGORITHM;
	}

	gnutls_pubkey_get_key_usage(cert->pubkey, &key_usage);

	ret = _gnutls_check_key_usage_for_sig(session, key_usage, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (_gnutls_version_has_selectable_sighash(ver))
		return _gnutls_handshake_sign_data12(session, cert, pkey, params, signature, *sign_algo);
	else
		return _gnutls_handshake_sign_data10(session, cert, pkey, params, signature, *sign_algo);
}

/* Generates a signature of all the random data and the parameters.
 * Used in DHE_* ciphersuites.
 */
static int
_gnutls_handshake_verify_data10(gnutls_session_t session,
			      unsigned verify_flags,
			      gnutls_pcert_st * cert,
			      const gnutls_datum_t * params,
			      gnutls_datum_t * signature,
			      gnutls_sign_algorithm_t sign_algo)
{
	gnutls_datum_t dconcat;
	int ret;
	digest_hd_st td_sha;
	uint8_t concat[MAX_SIG_SIZE];
	gnutls_digest_algorithm_t hash_algo;
	const mac_entry_st *me;
	gnutls_pk_algorithm_t pk_algo;

	pk_algo = gnutls_pubkey_get_pk_algorithm(cert->pubkey, NULL);
	if (pk_algo == GNUTLS_PK_RSA) {
		hash_algo = GNUTLS_DIG_MD5_SHA1;
		verify_flags |= GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA;
	} else {
		hash_algo = GNUTLS_DIG_SHA1;
		if (sign_algo == GNUTLS_SIGN_UNKNOWN) {
			sign_algo = gnutls_pk_to_sign(pk_algo, hash_algo);
		}
	}

	me = hash_to_entry(hash_algo);

	ret = _gnutls_hash_init(&td_sha, me);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_hash(&td_sha, session->security_parameters.client_random,
		     GNUTLS_RANDOM_SIZE);
	_gnutls_hash(&td_sha, session->security_parameters.server_random,
		     GNUTLS_RANDOM_SIZE);
	_gnutls_hash(&td_sha, params->data, params->size);

	_gnutls_hash_deinit(&td_sha, concat);

	dconcat.data = concat;
	dconcat.size = _gnutls_hash_get_algo_len(me);

	ret = gnutls_pubkey_verify_hash2(cert->pubkey, sign_algo,
					 GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1|verify_flags,
					 &dconcat, signature);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return ret;
}

static int
_gnutls_handshake_verify_data12(gnutls_session_t session,
			      unsigned verify_flags,
			      gnutls_pcert_st * cert,
			      const gnutls_datum_t * params,
			      gnutls_datum_t * signature,
			      gnutls_sign_algorithm_t sign_algo)
{
	gnutls_datum_t dconcat;
	int ret;
	const version_entry_st *ver = get_version(session);
	const gnutls_sign_entry_st *se = _gnutls_sign_to_entry(sign_algo);

	_gnutls_handshake_log
	    ("HSK[%p]: verify TLS 1.2 handshake data: using %s\n", session,
	     se->name);

	ret =
	    _gnutls_pubkey_compatible_with_sig(session,
					       cert->pubkey, ver,
					       sign_algo);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (unlikely(sign_supports_cert_pk_algorithm(se, cert->pubkey->params.algo) == 0)) {
		_gnutls_handshake_log("HSK[%p]: certificate of %s cannot be combined with %s sig\n",
				      session, gnutls_pk_get_name(cert->pubkey->params.algo), se->name);
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	ret =
	    _gnutls_session_sign_algo_enabled(session, sign_algo);
	if (ret < 0)
		return gnutls_assert_val(ret);

	dconcat.size = GNUTLS_RANDOM_SIZE*2+params->size;
	dconcat.data = gnutls_malloc(dconcat.size);
	if (dconcat.data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	memcpy(dconcat.data, session->security_parameters.client_random, GNUTLS_RANDOM_SIZE);
	memcpy(dconcat.data+GNUTLS_RANDOM_SIZE, session->security_parameters.server_random, GNUTLS_RANDOM_SIZE);
	memcpy(dconcat.data+GNUTLS_RANDOM_SIZE*2, params->data, params->size);

	/* Here we intentionally enable flag GNUTLS_VERIFY_ALLOW_BROKEN
	 * because we have checked whether the currently used signature
	 * algorithm is allowed in the session. */
	ret = gnutls_pubkey_verify_data2(cert->pubkey, sign_algo, verify_flags|GNUTLS_VERIFY_ALLOW_BROKEN,
					 &dconcat, signature);
	if (ret < 0)
		gnutls_assert();

	gnutls_free(dconcat.data);

	return ret;
}

int
_gnutls_handshake_verify_data(gnutls_session_t session,
			      unsigned verify_flags,
			      gnutls_pcert_st * cert,
			      const gnutls_datum_t * params,
			      gnutls_datum_t * signature,
			      gnutls_sign_algorithm_t sign_algo)
{
	unsigned key_usage;
	int ret;
	const version_entry_st *ver = get_version(session);

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	gnutls_pubkey_get_key_usage(cert->pubkey, &key_usage);

	ret = _gnutls_check_key_usage_for_sig(session, key_usage, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	gnutls_sign_algorithm_set_server(session, sign_algo);

	if (_gnutls_version_has_selectable_sighash(ver))
		return _gnutls_handshake_verify_data12(session, verify_flags, cert, params, signature, sign_algo);
	else
		return _gnutls_handshake_verify_data10(session, verify_flags, cert, params, signature, sign_algo);
}


/* Client certificate verify calculations
 */

static void
_gnutls_reverse_datum(gnutls_datum_t * d)
{
	unsigned i;

	for (i = 0; i < d->size / 2; i ++) {
		uint8_t t = d->data[i];
		d->data[i] = d->data[d->size - 1 - i];
		d->data[d->size - 1 - i] = t;
	}
}

static int
_gnutls_create_reverse(const gnutls_datum_t *src, gnutls_datum_t *dst)
{
	unsigned int i;

	dst->size = src->size;
	dst->data = gnutls_malloc(dst->size);
	if (!dst->data)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	for (i = 0; i < dst->size; i++)
		dst->data[i] = src->data[dst->size - 1 - i];

	return 0;
}

/* this is _gnutls_handshake_verify_crt_vrfy for TLS 1.2
 */
static int
_gnutls_handshake_verify_crt_vrfy12(gnutls_session_t session,
				    unsigned verify_flags,
				    gnutls_pcert_st * cert,
				    gnutls_datum_t * signature,
				    gnutls_sign_algorithm_t sign_algo)
{
	int ret;
	gnutls_datum_t dconcat;
	const gnutls_sign_entry_st *se = _gnutls_sign_to_entry(sign_algo);
	gnutls_datum_t sig_rev = {NULL, 0};

	ret = _gnutls_session_sign_algo_enabled(session, sign_algo);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (unlikely(sign_supports_cert_pk_algorithm(se, cert->pubkey->params.algo) == 0)) {
		_gnutls_handshake_log("HSK[%p]: certificate of %s cannot be combined with %s sig\n",
				      session, gnutls_pk_get_name(cert->pubkey->params.algo), se->name);
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	if (se->flags & GNUTLS_SIGN_FLAG_CRT_VRFY_REVERSE) {
		ret = _gnutls_create_reverse(signature, &sig_rev);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	dconcat.data = session->internals.handshake_hash_buffer.data;
	dconcat.size = session->internals.handshake_hash_buffer_prev_len;

	/* Here we intentionally enable flag GNUTLS_VERIFY_ALLOW_BROKEN
	 * because we have checked whether the currently used signature
	 * algorithm is allowed in the session. */
	ret = gnutls_pubkey_verify_data2(cert->pubkey, sign_algo, verify_flags|GNUTLS_VERIFY_ALLOW_BROKEN,
					 &dconcat,
					 sig_rev.data ? &sig_rev : signature);
	_gnutls_free_datum(&sig_rev);
	if (ret < 0)
		gnutls_assert();

	return ret;

}

/* Verifies a SSL 3.0 signature (like the one in the client certificate
 * verify message).
 */
#ifdef ENABLE_SSL3
static int
_gnutls_handshake_verify_crt_vrfy3(gnutls_session_t session,
				   unsigned verify_flags,
				   gnutls_pcert_st * cert,
				   gnutls_datum_t * signature,
				   gnutls_sign_algorithm_t sign_algo)
{
	int ret;
	uint8_t concat[MAX_SIG_SIZE];
	digest_hd_st td_sha;
	gnutls_datum_t dconcat;
	gnutls_pk_algorithm_t pk =
	    gnutls_pubkey_get_pk_algorithm(cert->pubkey, NULL);

	ret = _gnutls_generate_master(session, 1);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	dconcat.data = concat;
	dconcat.size = 0;

	if (pk == GNUTLS_PK_RSA) {
		digest_hd_st td_md5;

		ret = _gnutls_hash_init(&td_md5,
					hash_to_entry(GNUTLS_DIG_MD5));
		if (ret < 0)
			return gnutls_assert_val(ret);

		_gnutls_hash(&td_md5,
				session->internals.handshake_hash_buffer.data,
				session->internals.handshake_hash_buffer_prev_len);

		ret = _gnutls_mac_deinit_ssl3_handshake(&td_md5, concat,
						session->security_parameters.
						master_secret,
						GNUTLS_MASTER_SIZE);
		if (ret < 0)
			return gnutls_assert_val(ret);

		verify_flags |= GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA;
		dconcat.size = 16;
	}

	ret = _gnutls_hash_init(&td_sha, hash_to_entry(GNUTLS_DIG_SHA1));
	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_HASH_FAILED;
	}

	_gnutls_hash(&td_sha,
		     session->internals.handshake_hash_buffer.data,
		     session->internals.handshake_hash_buffer_prev_len);

	ret =
	    _gnutls_mac_deinit_ssl3_handshake(&td_sha,
					      dconcat.data + dconcat.size,
					      session->security_parameters.
					      master_secret,
					      GNUTLS_MASTER_SIZE);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	dconcat.size += 20;

	ret = gnutls_pubkey_verify_hash2(cert->pubkey, GNUTLS_SIGN_UNKNOWN,
					 GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1|verify_flags,
					 &dconcat, signature);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return ret;
}
#endif

static int
_gnutls_handshake_verify_crt_vrfy10(gnutls_session_t session,
				    unsigned verify_flags,
				    gnutls_pcert_st * cert,
				    gnutls_datum_t * signature,
				    gnutls_sign_algorithm_t sign_algo)
{
	int ret;
	uint8_t concat[MAX_SIG_SIZE];
	digest_hd_st td_sha;
	gnutls_datum_t dconcat;
	gnutls_pk_algorithm_t pk_algo;
	const mac_entry_st *me;

	/* TLS 1.0 and TLS 1.1 */
	pk_algo = gnutls_pubkey_get_pk_algorithm(cert->pubkey, NULL);
	if (pk_algo == GNUTLS_PK_RSA) {
		me = hash_to_entry(GNUTLS_DIG_MD5_SHA1);
		verify_flags |= GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA;
		sign_algo = GNUTLS_SIGN_UNKNOWN;
	} else {
		me = hash_to_entry(GNUTLS_DIG_SHA1);
		sign_algo = gnutls_pk_to_sign(pk_algo, GNUTLS_DIG_SHA1);
	}
	ret = _gnutls_hash_init(&td_sha, me);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_hash(&td_sha,
		     session->internals.handshake_hash_buffer.data,
		     session->internals.handshake_hash_buffer_prev_len);

	_gnutls_hash_deinit(&td_sha, concat);

	dconcat.data = concat;
	dconcat.size = _gnutls_hash_get_algo_len(me);

	ret = gnutls_pubkey_verify_hash2(cert->pubkey, sign_algo,
					 GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1|verify_flags,
					 &dconcat, signature);
	if (ret < 0)
		gnutls_assert();

	return ret;
}

/* Verifies a TLS signature (like the one in the client certificate
 * verify message). 
 */
int
_gnutls_handshake_verify_crt_vrfy(gnutls_session_t session,
				  unsigned verify_flags,
				  gnutls_pcert_st * cert,
				  gnutls_datum_t * signature,
				  gnutls_sign_algorithm_t sign_algo)
{
	int ret;
	const version_entry_st *ver = get_version(session);
	unsigned key_usage;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	gnutls_pubkey_get_key_usage(cert->pubkey, &key_usage);

	ret = _gnutls_check_key_usage_for_sig(session, key_usage, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_handshake_log("HSK[%p]: verify cert vrfy: using %s\n",
			      session,
			      gnutls_sign_algorithm_get_name(sign_algo));

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	gnutls_sign_algorithm_set_client(session, sign_algo);

	/* TLS 1.2 */
	if (_gnutls_version_has_selectable_sighash(ver))
		return _gnutls_handshake_verify_crt_vrfy12(session, 
							   verify_flags,
							   cert,
							   signature,
							   sign_algo);
#ifdef ENABLE_SSL3
	if (ver->id == GNUTLS_SSL3)
		return _gnutls_handshake_verify_crt_vrfy3(session,
							  verify_flags,
							  cert,
							  signature,
							  sign_algo);
#endif

	/* TLS 1.0 and TLS 1.1 */
	return _gnutls_handshake_verify_crt_vrfy10(session,
						   verify_flags,
						   cert,
						   signature,
						   sign_algo);
}

/* the same as _gnutls_handshake_sign_crt_vrfy except that it is made for TLS 1.2.
 * Returns the used signature algorithm, or a negative error code.
 */
static int
_gnutls_handshake_sign_crt_vrfy12(gnutls_session_t session,
				  gnutls_pcert_st * cert,
				  gnutls_privkey_t pkey,
				  gnutls_datum_t * signature)
{
	gnutls_datum_t dconcat;
	gnutls_sign_algorithm_t sign_algo;
	const gnutls_sign_entry_st *se;
	int ret;

	sign_algo = _gnutls_session_get_sign_algo(session, cert, pkey, 1);
	if (sign_algo == GNUTLS_SIGN_UNKNOWN) {
		gnutls_assert();
		return GNUTLS_E_UNWANTED_ALGORITHM;
	}

	se = _gnutls_sign_to_entry(sign_algo);
	if (se == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	gnutls_sign_algorithm_set_client(session, sign_algo);

	if (unlikely(gnutls_sign_supports_pk_algorithm(sign_algo, pkey->pk_algorithm) == 0))
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	_gnutls_debug_log("sign handshake cert vrfy: picked %s\n",
			  gnutls_sign_algorithm_get_name(sign_algo));

	dconcat.data = session->internals.handshake_hash_buffer.data;
	dconcat.size = session->internals.handshake_hash_buffer.length;

	ret = gnutls_privkey_sign_data2(pkey, sign_algo,
					0, &dconcat, signature);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (se->flags & GNUTLS_SIGN_FLAG_CRT_VRFY_REVERSE)
		_gnutls_reverse_datum(signature);

	return sign_algo;
}

#ifdef ENABLE_SSL3
static int
_gnutls_handshake_sign_crt_vrfy3(gnutls_session_t session,
				 gnutls_pcert_st * cert,
				 const version_entry_st *ver,
				 gnutls_privkey_t pkey,
				 gnutls_datum_t * signature)
{
	gnutls_datum_t dconcat;
	int ret;
	uint8_t concat[MAX_SIG_SIZE];
	digest_hd_st td_sha;
	gnutls_pk_algorithm_t pk =
	    gnutls_privkey_get_pk_algorithm(pkey, NULL);

	/* ensure 1024 bit DSA keys are used */
	ret =
	    _gnutls_pubkey_compatible_with_sig(session, cert->pubkey, ver,
					       GNUTLS_SIGN_UNKNOWN);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_generate_master(session, 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	dconcat.data = concat;
	dconcat.size = 0;

	if (pk == GNUTLS_PK_RSA) {
		digest_hd_st td_md5;
		ret =
		    _gnutls_hash_init(&td_md5,
				      hash_to_entry(GNUTLS_DIG_MD5));
		if (ret < 0)
			return gnutls_assert_val(ret);

		_gnutls_hash(&td_md5,
			     session->internals.handshake_hash_buffer.data,
			     session->internals.handshake_hash_buffer.
			     length);

		ret = _gnutls_mac_deinit_ssl3_handshake(&td_md5,
							dconcat.data,
							session->security_parameters.
							master_secret,
							GNUTLS_MASTER_SIZE);
		if (ret < 0)
			return gnutls_assert_val(ret);

		dconcat.size = 16;
	}

	ret = _gnutls_hash_init(&td_sha, hash_to_entry(GNUTLS_DIG_SHA1));
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_hash(&td_sha,
		     session->internals.handshake_hash_buffer.data,
		     session->internals.handshake_hash_buffer.length);
	ret =
		_gnutls_mac_deinit_ssl3_handshake(&td_sha,
				dconcat.data + dconcat.size,
				session->security_parameters.
				master_secret,
				GNUTLS_MASTER_SIZE);
	if (ret < 0)
		return gnutls_assert_val(ret);

	dconcat.size += 20;

	ret = gnutls_privkey_sign_hash(pkey, GNUTLS_DIG_SHA1,
				       GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
				       &dconcat, signature);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return GNUTLS_SIGN_UNKNOWN;
}
#endif

static int
_gnutls_handshake_sign_crt_vrfy10(gnutls_session_t session,
				  gnutls_pcert_st * cert,
				  const version_entry_st *ver,
				  gnutls_privkey_t pkey,
				  gnutls_datum_t * signature)
{
	gnutls_datum_t dconcat;
	int ret;
	uint8_t concat[MAX_SIG_SIZE];
	digest_hd_st td_sha;
	gnutls_pk_algorithm_t pk =
	    gnutls_privkey_get_pk_algorithm(pkey, NULL);
	const mac_entry_st *me;

	/* ensure 1024 bit DSA keys are used */
	ret =
	    _gnutls_pubkey_compatible_with_sig(session, cert->pubkey, ver,
					       GNUTLS_SIGN_UNKNOWN);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (pk == GNUTLS_PK_RSA)
		me = hash_to_entry(GNUTLS_DIG_MD5_SHA1);
	else
		me = hash_to_entry(GNUTLS_DIG_SHA1);

	ret = _gnutls_hash_init(&td_sha, me);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_hash(&td_sha,
		     session->internals.handshake_hash_buffer.data,
		     session->internals.handshake_hash_buffer.length);

	_gnutls_hash_deinit(&td_sha, concat);

	dconcat.data = concat;
	dconcat.size = _gnutls_hash_get_algo_len(me);

	ret = gnutls_privkey_sign_hash(pkey, me->id, GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
				       &dconcat, signature);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return GNUTLS_SIGN_UNKNOWN;
}

/* Generates a signature of all the previous sent packets in the
 * handshake procedure.
 * 20040227: now it works for SSL 3.0 as well
 * 20091031: works for TLS 1.2 too!
 *
 * For TLS1.x, x<2 returns negative for failure and zero or unspecified for success.
 * For TLS1.2 returns the signature algorithm used on success, or a negative error code;
 *
 * Returns the used signature algorithm, or a negative error code.
 */
int
_gnutls_handshake_sign_crt_vrfy(gnutls_session_t session,
				gnutls_pcert_st * cert,
				gnutls_privkey_t pkey,
				gnutls_datum_t * signature)
{
	int ret;
	const version_entry_st *ver = get_version(session);
	unsigned key_usage = 0;

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	gnutls_pubkey_get_key_usage(cert->pubkey, &key_usage);

	ret = _gnutls_check_key_usage_for_sig(session, key_usage, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* TLS 1.2 */
	if (_gnutls_version_has_selectable_sighash(ver))
		return _gnutls_handshake_sign_crt_vrfy12(session, cert,
							 pkey, signature);

	/* TLS 1.1 or earlier */
#ifdef ENABLE_SSL3
	if (ver->id == GNUTLS_SSL3)
		return _gnutls_handshake_sign_crt_vrfy3(session, cert, ver,
							pkey, signature);
#endif

	return _gnutls_handshake_sign_crt_vrfy10(session, cert, ver,
						 pkey, signature);
}
