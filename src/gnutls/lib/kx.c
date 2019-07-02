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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* This file contains functions which are wrappers for the key exchange
 * part of TLS. They are called by the handshake functions (gnutls_handshake)
 */

#include "gnutls_int.h"
#include "handshake.h"
#include "kx.h"
#include "dh.h"
#include "errors.h"
#include "algorithms.h"
#include "debug.h"
#include "locks.h"
#include "mpi.h"
#include <state.h>
#include <datum.h>
#include <mbuffers.h>

/* This file contains important thing for the TLS handshake procedure.
 */

#define MASTER_SECRET "master secret"
#define MASTER_SECRET_SIZE (sizeof(MASTER_SECRET)-1)

#define EXT_MASTER_SECRET "extended master secret"
#define EXT_MASTER_SECRET_SIZE (sizeof(EXT_MASTER_SECRET)-1)

GNUTLS_STATIC_MUTEX(keylog_mutex);
static FILE *keylog;

static int generate_normal_master(gnutls_session_t session,
				  gnutls_datum_t *, int);

int _gnutls_generate_master(gnutls_session_t session, int keep_premaster)
{
	if (session->internals.resumed == RESUME_FALSE)
		return generate_normal_master(session, &session->key.key,
					      keep_premaster);
	else if (session->internals.premaster_set) {
		gnutls_datum_t premaster;
		premaster.size =
		    sizeof(session->internals.resumed_security_parameters.
			   master_secret);
		premaster.data =
		    session->internals.resumed_security_parameters.
		    master_secret;
		return generate_normal_master(session, &premaster, 1);
	}
	return 0;
}

void _gnutls_nss_keylog_write(gnutls_session_t session,
			      const char *label,
			      const uint8_t *secret, size_t secret_size)
{
	static const char *keylogfile = NULL;
	static unsigned checked_env = 0;

	if (!checked_env) {
		checked_env = 1;
		keylogfile = secure_getenv("SSLKEYLOGFILE");
		if (keylogfile != NULL)
			keylog = fopen(keylogfile, "a");
	}

	if (keylog) {
		char client_random_hex[2*GNUTLS_RANDOM_SIZE+1];
		char secret_hex[2*MAX_HASH_SIZE+1];

		GNUTLS_STATIC_MUTEX_LOCK(keylog_mutex);
		fprintf(keylog, "%s %s %s\n",
			label,
			_gnutls_bin2hex(session->security_parameters.
					client_random, GNUTLS_RANDOM_SIZE,
					client_random_hex,
					sizeof(client_random_hex), NULL),
			_gnutls_bin2hex(secret, secret_size,
					secret_hex, sizeof(secret_hex), NULL));
		fflush(keylog);
		GNUTLS_STATIC_MUTEX_UNLOCK(keylog_mutex);
	}
}

void _gnutls_nss_keylog_deinit(void)
{
	if (keylog) {
		fclose(keylog);
		keylog = NULL;
	}
}

/* here we generate the TLS Master secret.
 */
static int
generate_normal_master(gnutls_session_t session,
		       gnutls_datum_t * premaster, int keep_premaster)
{
	int ret = 0;
	char buf[512];

	_gnutls_hard_log("INT: PREMASTER SECRET[%d]: %s\n",
			 premaster->size, _gnutls_bin2hex(premaster->data,
							  premaster->size,
							  buf, sizeof(buf),
							  NULL));
	_gnutls_hard_log("INT: CLIENT RANDOM[%d]: %s\n", 32,
			 _gnutls_bin2hex(session->security_parameters.
					 client_random, 32, buf,
					 sizeof(buf), NULL));
	_gnutls_hard_log("INT: SERVER RANDOM[%d]: %s\n", 32,
			 _gnutls_bin2hex(session->security_parameters.
					 server_random, 32, buf,
					 sizeof(buf), NULL));

	if (session->security_parameters.ext_master_secret == 0) {
		uint8_t rnd[2 * GNUTLS_RANDOM_SIZE + 1];
		memcpy(rnd, session->security_parameters.client_random,
		       GNUTLS_RANDOM_SIZE);
		memcpy(&rnd[GNUTLS_RANDOM_SIZE],
		       session->security_parameters.server_random,
		       GNUTLS_RANDOM_SIZE);

#ifdef ENABLE_SSL3
		if (get_num_version(session) == GNUTLS_SSL3) {
			ret =
			    _gnutls_ssl3_generate_random(premaster->data,
							 premaster->size, rnd,
							 2 * GNUTLS_RANDOM_SIZE,
							 GNUTLS_MASTER_SIZE,
							 session->security_parameters.
							 master_secret);
		} else
#endif
			ret =
			    _gnutls_PRF(session, premaster->data, premaster->size,
					MASTER_SECRET, MASTER_SECRET_SIZE,
					rnd, 2 * GNUTLS_RANDOM_SIZE,
					GNUTLS_MASTER_SIZE,
					session->security_parameters.
					master_secret);
	} else {
		gnutls_datum_t shash = {NULL, 0};

		/* draft-ietf-tls-session-hash-02 */
		ret = _gnutls_handshake_get_session_hash(session, &shash);
		if (ret < 0)
			return gnutls_assert_val(ret);
#ifdef ENABLE_SSL3
		if (get_num_version(session) == GNUTLS_SSL3)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
#endif

		ret =
		    _gnutls_PRF(session, premaster->data, premaster->size,
				EXT_MASTER_SECRET, EXT_MASTER_SECRET_SIZE,
				shash.data, shash.size,
				GNUTLS_MASTER_SIZE,
				session->security_parameters.
				master_secret);

		gnutls_free(shash.data);
	}

	_gnutls_nss_keylog_write(session, "CLIENT_RANDOM",
				 session->security_parameters.master_secret,
				 GNUTLS_MASTER_SIZE);

	if (!keep_premaster)
		_gnutls_free_temp_key_datum(premaster);

	if (ret < 0)
		return ret;

	_gnutls_hard_log("INT: MASTER SECRET[%d]: %s\n",
			 GNUTLS_MASTER_SIZE,
			 _gnutls_bin2hex(session->security_parameters.
					 master_secret, GNUTLS_MASTER_SIZE,
					 buf, sizeof(buf), NULL));

	return ret;
}

/* This is called when we want to receive the key exchange message of the
 * server. It does nothing if this type of message is not required
 * by the selected ciphersuite. 
 */
int _gnutls_send_server_kx_message(gnutls_session_t session, int again)
{
	gnutls_buffer_st buf;
	int ret = 0;
	mbuffer_st *bufel = NULL;

	if (session->internals.auth_struct->gnutls_generate_server_kx ==
	    NULL)
		return 0;


	if (again == 0) {
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    session->internals.auth_struct->
		    gnutls_generate_server_kx(session, &buf);

		if (ret == GNUTLS_E_INT_RET_0) {
			gnutls_assert();
			ret = 0;
			goto cleanup;
		}

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}

/* This function sends a certificate request message to the
 * client.
 */
int _gnutls_send_server_crt_request(gnutls_session_t session, int again)
{
	gnutls_buffer_st buf;
	int ret = 0;
	mbuffer_st *bufel = NULL;

	if (session->internals.auth_struct->
	    gnutls_generate_server_crt_request == NULL)
		return 0;

	if (session->internals.send_cert_req <= 0)
		return 0;


	if (again == 0) {
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    session->internals.auth_struct->
		    gnutls_generate_server_crt_request(session, &buf);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}


/* This is the function for the client to send the key
 * exchange message 
 */
int _gnutls_send_client_kx_message(gnutls_session_t session, int again)
{
	gnutls_buffer_st buf;
	int ret = 0;
	mbuffer_st *bufel = NULL;

	if (session->internals.auth_struct->gnutls_generate_client_kx ==
	    NULL)
		return 0;

	if (again == 0) {
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    session->internals.auth_struct->
		    gnutls_generate_client_kx(session, &buf);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}


/* This is the function for the client to send the certificate
 * verify message
 */
int
_gnutls_send_client_certificate_verify(gnutls_session_t session, int again)
{
	gnutls_buffer_st buf;
	int ret = 0;
	mbuffer_st *bufel = NULL;

	/* This is a packet that is only sent by the client
	 */
	if (session->security_parameters.entity == GNUTLS_SERVER)
		return 0;

	/* if certificate verify is not needed just exit 
	 */
	if (!(session->internals.hsk_flags & HSK_CRT_ASKED))
		return 0;


	if (session->internals.auth_struct->
	    gnutls_generate_client_crt_vrfy == NULL) {
		gnutls_assert();
		return 0;	/* this algorithm does not support cli_crt_vrfy 
				 */
	}

	if (again == 0) {
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    session->internals.auth_struct->
		    gnutls_generate_client_crt_vrfy(session, &buf);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (ret == 0)
			goto cleanup;


		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}

/* This is called when we want send our certificate
 */
int _gnutls_send_client_certificate(gnutls_session_t session, int again)
{
	gnutls_buffer_st buf;
	int ret = 0;
	mbuffer_st *bufel = NULL;

	if (!(session->internals.hsk_flags & HSK_CRT_ASKED))
		return 0;

	if (session->internals.auth_struct->
	    gnutls_generate_client_certificate == NULL)
		return 0;

	if (again == 0) {
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

#ifdef ENABLE_SSL3
		if (get_num_version(session) != GNUTLS_SSL3 ||
		    session->internals.selected_cert_list_length > 0)
#endif
		{
			/* TLS 1.x or SSL 3.0 with a valid certificate 
			 */
			ret =
			    session->internals.auth_struct->
			    gnutls_generate_client_certificate(session,
							       &buf);

			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

#ifdef ENABLE_SSL3
	/* In the SSL 3.0 protocol we need to send a
	 * no certificate alert instead of an
	 * empty certificate.
	 */
	if (get_num_version(session) == GNUTLS_SSL3 &&
	    session->internals.selected_cert_list_length == 0) {
		_mbuffer_xfree(&bufel);
		return
		    gnutls_alert_send(session, GNUTLS_AL_WARNING,
				      GNUTLS_A_SSL3_NO_CERTIFICATE);

	} else		/* TLS 1.0 or SSL 3.0 with a valid certificate 
			 */
#endif
		return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CERTIFICATE_PKT);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}


/* This is called when we want send our certificate
 */
int _gnutls_send_server_certificate(gnutls_session_t session, int again)
{
	gnutls_buffer_st buf;
	int ret = 0;
	mbuffer_st *bufel = NULL;

	if (session->internals.auth_struct->
	    gnutls_generate_server_certificate == NULL)
		return 0;

	if (again == 0) {
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    session->internals.auth_struct->
		    gnutls_generate_server_certificate(session, &buf);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_CERTIFICATE_PKT);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}


int _gnutls_recv_server_kx_message(gnutls_session_t session)
{
	gnutls_buffer_st buf;
	int ret = 0;
	unsigned int optflag = 0;

	if (session->internals.auth_struct->gnutls_process_server_kx !=
	    NULL) {
		/* Server key exchange packet is optional for PSK. */
		if (_gnutls_session_is_psk(session))
			optflag = 1;

		ret =
		    _gnutls_recv_handshake(session,
					   GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE,
					   optflag, &buf);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		ret =
		    session->internals.auth_struct->
		    gnutls_process_server_kx(session, buf.data,
					     buf.length);
		_gnutls_buffer_clear(&buf);

		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

	}
	return ret;
}

int _gnutls_recv_server_crt_request(gnutls_session_t session)
{
	gnutls_buffer_st buf;
	int ret = 0;

	if (session->internals.auth_struct->
	    gnutls_process_server_crt_request != NULL) {

		ret =
		    _gnutls_recv_handshake(session,
					   GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST,
					   1, &buf);
		if (ret < 0)
			return ret;

		if (ret == 0 && buf.length == 0) {
			_gnutls_buffer_clear(&buf);
			return 0;	/* ignored */
		}

		ret =
		    session->internals.auth_struct->
		    gnutls_process_server_crt_request(session, buf.data,
						      buf.length);
		_gnutls_buffer_clear(&buf);
		if (ret < 0)
			return ret;

	}
	return ret;
}

int _gnutls_recv_client_kx_message(gnutls_session_t session)
{
	gnutls_buffer_st buf;
	int ret = 0;


	/* Do key exchange only if the algorithm permits it */
	if (session->internals.auth_struct->gnutls_process_client_kx !=
	    NULL) {

		ret =
		    _gnutls_recv_handshake(session,
					   GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE,
					   0, &buf);
		if (ret < 0)
			return ret;

		ret =
		    session->internals.auth_struct->
		    gnutls_process_client_kx(session, buf.data,
					     buf.length);
		_gnutls_buffer_clear(&buf);
		if (ret < 0)
			return ret;

	}

	return ret;
}


int _gnutls_recv_client_certificate(gnutls_session_t session)
{
	gnutls_buffer_st buf;
	int ret = 0;
	int optional;

	if (session->internals.auth_struct->
	    gnutls_process_client_certificate == NULL)
		return 0;

	/* if we have not requested a certificate then just return
	 */
	if (session->internals.send_cert_req == 0) {
		return 0;
	}

	if (session->internals.send_cert_req == GNUTLS_CERT_REQUIRE)
		optional = 0;
	else
		optional = 1;

	ret =
	    _gnutls_recv_handshake(session,
				   GNUTLS_HANDSHAKE_CERTIFICATE_PKT,
				   optional, &buf);

	if (ret < 0) {
		/* Handle the case of old SSL3 clients who send
		 * a warning alert instead of an empty certificate to indicate
		 * no certificate.
		 */
#ifdef ENABLE_SSL3
		if (optional != 0 &&
		    ret == GNUTLS_E_WARNING_ALERT_RECEIVED &&
		    get_num_version(session) == GNUTLS_SSL3 &&
		    gnutls_alert_get(session) ==
		    GNUTLS_A_SSL3_NO_CERTIFICATE) {

			/* SSL3 does not send an empty certificate,
			 * but this alert. So we just ignore it.
			 */
			gnutls_assert();
			return 0;
		}
#endif

		/* certificate was required 
		 */
		if ((ret == GNUTLS_E_WARNING_ALERT_RECEIVED
		     || ret == GNUTLS_E_FATAL_ALERT_RECEIVED)
		    && optional == 0) {
			gnutls_assert();
			return GNUTLS_E_NO_CERTIFICATE_FOUND;
		}

		return ret;
	}

	if (ret == 0 && buf.length == 0 && optional != 0) {
		/* Client has not sent the certificate message.
		 * well I'm not sure we should accept this
		 * behaviour.
		 */
		gnutls_assert();
		ret = 0;
		goto cleanup;
	}
	ret =
	    session->internals.auth_struct->
	    gnutls_process_client_certificate(session, buf.data,
					      buf.length);

	if (ret < 0 && ret != GNUTLS_E_NO_CERTIFICATE_FOUND) {
		gnutls_assert();
		goto cleanup;
	}

	/* ok we should expect a certificate verify message now 
	 */
	if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND && optional != 0)
		ret = 0;
	else
		session->internals.hsk_flags |= HSK_CRT_VRFY_EXPECTED;

      cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}

int _gnutls_recv_server_certificate(gnutls_session_t session)
{
	gnutls_buffer_st buf;
	int ret = 0;

	if (session->internals.auth_struct->
	    gnutls_process_server_certificate != NULL) {

		ret =
		    _gnutls_recv_handshake(session,
					   GNUTLS_HANDSHAKE_CERTIFICATE_PKT,
					   0, &buf);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		ret =
		    session->internals.auth_struct->
		    gnutls_process_server_certificate(session, buf.data,
						      buf.length);
		_gnutls_buffer_clear(&buf);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	return ret;
}


/* Recv the client certificate verify. This packet may not
 * arrive if the peer did not send us a certificate.
 */
int
_gnutls_recv_client_certificate_verify_message(gnutls_session_t session)
{
	gnutls_buffer_st buf;
	int ret = 0;


	if (session->internals.auth_struct->
	    gnutls_process_client_crt_vrfy == NULL)
		return 0;

	if (session->internals.send_cert_req == 0 ||
	    (!(session->internals.hsk_flags & HSK_CRT_VRFY_EXPECTED))) {
		return 0;
	}

	ret =
	    _gnutls_recv_handshake(session,
				   GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY,
				   1, &buf);
	if (ret < 0)
		return ret;

	if (ret == 0 && buf.length == 0
	    && session->internals.send_cert_req == GNUTLS_CERT_REQUIRE) {
		/* certificate was required */
		gnutls_assert();
		ret = GNUTLS_E_NO_CERTIFICATE_FOUND;
		goto cleanup;
	}

	ret =
	    session->internals.auth_struct->
	    gnutls_process_client_crt_vrfy(session, buf.data, buf.length);

      cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}
