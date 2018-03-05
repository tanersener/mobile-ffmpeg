/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

/* This file contains the RSA key exchange part of the certificate
 * authentication.
 */

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "dh.h"
#include "num.h"
#include "datum.h"
#include <auth/cert.h>
#include <pk.h>
#include <algorithms.h>
#include <global.h>
#include "debug.h"
#include <tls-sig.h>
#include <x509.h>
#include <random.h>
#include <mpi.h>
#include <abstract_int.h>
#include <auth/rsa_common.h>

int _gnutls_gen_rsa_client_kx(gnutls_session_t, gnutls_buffer_st *);
static int proc_rsa_client_kx(gnutls_session_t, uint8_t *, size_t);

const mod_auth_st rsa_auth_struct = {
	"RSA",
	_gnutls_gen_cert_server_crt,
	_gnutls_gen_cert_client_crt,
	NULL,			/* gen server kx */
	_gnutls_gen_rsa_client_kx,
	_gnutls_gen_cert_client_crt_vrfy,	/* gen client cert vrfy */
	_gnutls_gen_cert_server_cert_req,	/* server cert request */

	_gnutls_proc_crt,
	_gnutls_proc_crt,
	NULL,			/* proc server kx */
	proc_rsa_client_kx,	/* proc client kx */
	_gnutls_proc_cert_client_crt_vrfy,	/* proc client cert vrfy */
	_gnutls_proc_cert_cert_req	/* proc server cert request */
};

static
int check_key_usage_for_enc(gnutls_session_t session, unsigned key_usage)
{
	if (key_usage != 0) {
		if (!(key_usage & GNUTLS_KEY_KEY_ENCIPHERMENT) && !(key_usage & GNUTLS_KEY_KEY_AGREEMENT)) {
			gnutls_assert();
			if (session->internals.priorities.allow_key_usage_violation == 0) {
				_gnutls_audit_log(session,
					  "Peer's certificate does not allow encryption. Key usage violation detected.\n");
				return GNUTLS_E_KEY_USAGE_VIOLATION;
			} else {
				_gnutls_audit_log(session,
					  "Peer's certificate does not allow encryption. Key usage violation detected (ignored).\n");
			}
		}
	}
	return 0;
}

/* This function reads the RSA parameters from peer's certificate;
 */
int
_gnutls_get_public_rsa_params(gnutls_session_t session,
			      gnutls_pk_params_st * params)
{
	int ret;
	cert_auth_info_t info;
	unsigned key_usage;
	gnutls_pcert_st peer_cert;

	/* normal non export case */

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);

	if (info == NULL || info->ncerts == 0) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	ret =
	    _gnutls_get_auth_info_pcert(&peer_cert,
					session->security_parameters.
					cert_type, info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	gnutls_pubkey_get_key_usage(peer_cert.pubkey, &key_usage);

	ret = check_key_usage_for_enc(session, key_usage);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup2;
	}

	gnutls_pk_params_init(params);

	ret = _gnutls_pubkey_get_mpis(peer_cert.pubkey, params);
	if (ret < 0) {
		ret = gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		goto cleanup2;
	}

	gnutls_pcert_deinit(&peer_cert);
	return 0;

      cleanup2:
	gnutls_pcert_deinit(&peer_cert);

	return ret;
}

static int
proc_rsa_client_kx(gnutls_session_t session, uint8_t * data,
		   size_t _data_size)
{
	gnutls_datum_t plaintext = {NULL, 0};
	gnutls_datum_t ciphertext;
	int ret, dsize;
	int use_rnd_key = 0;
	ssize_t data_size = _data_size;
	gnutls_datum_t rndkey = {NULL, 0};

#ifdef ENABLE_SSL3
	if (get_num_version(session) == GNUTLS_SSL3) {
		/* SSL 3.0 
		 */
		ciphertext.data = data;
		ciphertext.size = data_size;
	} else
#endif
	{
		/* TLS 1.0+
		 */
		DECR_LEN(data_size, 2);
		ciphertext.data = &data[2];
		dsize = _gnutls_read_uint16(data);

		if (dsize != data_size) {
			gnutls_assert();
			return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
		}
		ciphertext.size = dsize;
	}

	rndkey.size = GNUTLS_MASTER_SIZE;
	rndkey.data = gnutls_malloc(rndkey.size);
	if (rndkey.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	/* we do not need strong random numbers here.
	 */
	ret = gnutls_rnd(GNUTLS_RND_NONCE, rndkey.data,
			  rndkey.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_privkey_decrypt_data(session->internals.selected_key, 0,
					&ciphertext, &plaintext);

	if (ret < 0 || plaintext.size != GNUTLS_MASTER_SIZE) {
		/* In case decryption fails then don't inform
		 * the peer. Just use a random key. (in order to avoid
		 * attack against pkcs-1 formating).
		 */
		_gnutls_debug_log("auth_rsa: Possible PKCS #1 format attack\n");
		if (ret >= 0) {
			gnutls_free(plaintext.data);
			plaintext.data = NULL;
		}
		use_rnd_key = 1;
	} else {
		/* If the secret was properly formatted, then
		 * check the version number.
		 */
		if (_gnutls_get_adv_version_major(session) !=
		    plaintext.data[0]
		    || (session->internals.priorities.allow_wrong_pms == 0
			&& _gnutls_get_adv_version_minor(session) !=
			plaintext.data[1])) {
			/* No error is returned here, if the version number check
			 * fails. We proceed normally.
			 * That is to defend against the attack described in the paper
			 * "Attacking RSA-based sessions in SSL/TLS" by Vlastimil Klima,
			 * Ondej Pokorny and Tomas Rosa.
			 */
			_gnutls_debug_log("auth_rsa: Possible PKCS #1 version check format attack\n");
		}
	}

	if (use_rnd_key != 0) {
		session->key.key.data = rndkey.data;
		session->key.key.size = rndkey.size;
		rndkey.data = NULL;
	} else {
		session->key.key.data = plaintext.data;
		session->key.key.size = plaintext.size;
	}

	/* This is here to avoid the version check attack
	 * discussed above.
	 */
	session->key.key.data[0] = _gnutls_get_adv_version_major(session);
	session->key.key.data[1] = _gnutls_get_adv_version_minor(session);

	ret = 0;
 cleanup:
	gnutls_free(rndkey.data);
	return ret;
}



/* return RSA(random) using the peers public key 
 */
int
_gnutls_gen_rsa_client_kx(gnutls_session_t session,
			  gnutls_buffer_st * data)
{
	cert_auth_info_t auth = session->key.auth_info;
	gnutls_datum_t sdata;	/* data to send */
	gnutls_pk_params_st params;
	int ret;

	if (auth == NULL) {
		/* this shouldn't have happened. The proc_certificate
		 * function should have detected that.
		 */
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	session->key.key.size = GNUTLS_MASTER_SIZE;
	session->key.key.data = gnutls_malloc(session->key.key.size);

	if (session->key.key.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = gnutls_rnd(GNUTLS_RND_RANDOM, session->key.key.data,
			  session->key.key.size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (session->internals.rsa_pms_version[0] == 0) {
		session->key.key.data[0] =
		    _gnutls_get_adv_version_major(session);
		session->key.key.data[1] =
		    _gnutls_get_adv_version_minor(session);
	} else {		/* use the version provided */
		session->key.key.data[0] =
		    session->internals.rsa_pms_version[0];
		session->key.key.data[1] =
		    session->internals.rsa_pms_version[1];
	}

	/* move RSA parameters to key (session).
	 */
	if ((ret = _gnutls_get_public_rsa_params(session, &params)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_pk_encrypt(GNUTLS_PK_RSA, &sdata, &session->key.key,
			       &params);

	gnutls_pk_params_release(&params);

	if (ret < 0)
		return gnutls_assert_val(ret);


#ifdef ENABLE_SSL3
	if (get_num_version(session) == GNUTLS_SSL3) {
		/* SSL 3.0 */
		_gnutls_buffer_replace_data(data, &sdata);

		return data->length;
	} else
#endif
	{		/* TLS 1.x */
		ret =
		    _gnutls_buffer_append_data_prefix(data, 16, sdata.data,
						      sdata.size);

		_gnutls_free_datum(&sdata);
		return ret;
	}
}
