/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

/* Functions that relate to the TLS handshake procedure.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "dh.h"
#include "debug.h"
#include "algorithms.h"
#include "compress.h"
#include "cipher.h"
#include "buffers.h"
#include "mbuffers.h"
#include "kx.h"
#include "handshake.h"
#include "num.h"
#include "hash_int.h"
#include "db.h"
#include "extensions.h"
#include "supplemental.h"
#include "auth.h"
#include "sslv2_compat.h"
#include <auth/cert.h>
#include "constate.h"
#include <record.h>
#include <state.h>
#include <ext/srp.h>
#include <ext/session_ticket.h>
#include <ext/status_request.h>
#include <ext/safe_renegotiation.h>
#include <auth/anon.h>		/* for gnutls_anon_server_credentials_t */
#include <auth/psk.h>		/* for gnutls_psk_server_credentials_t */
#include <random.h>
#include <dtls.h>

#ifdef HANDSHAKE_DEBUG
#define ERR(x, y) _gnutls_handshake_log("HSK[%p]: %s (%d)\n", session, x,y)
#else
#define ERR(x, y)
#endif

#define TRUE 1
#define FALSE 0

static int server_select_comp_method(gnutls_session_t session,
					     uint8_t * data, int datalen);
static int handshake_client(gnutls_session_t session);
static int handshake_server(gnutls_session_t session);

static int
recv_hello(gnutls_session_t session, uint8_t * data, int datalen);

static int
recv_handshake_final(gnutls_session_t session, int init);
static int
send_handshake_final(gnutls_session_t session, int init);

/* Empties but does not free the buffer
 */
static inline void
handshake_hash_buffer_empty(gnutls_session_t session)
{

	_gnutls_buffers_log("BUF[HSK]: Emptied buffer\n");

	session->internals.handshake_hash_buffer_prev_len = 0;
	session->internals.handshake_hash_buffer.length = 0;
	return;
}

static int
handshake_hash_add_recvd(gnutls_session_t session,
				 gnutls_handshake_description_t recv_type,
				 uint8_t * header, uint16_t header_size,
				 uint8_t * dataptr, uint32_t datalen);

static int
handshake_hash_add_sent(gnutls_session_t session,
				gnutls_handshake_description_t type,
				uint8_t * dataptr, uint32_t datalen);

static int
recv_hello_verify_request(gnutls_session_t session,
				  uint8_t * data, int datalen);


/* Clears the handshake hash buffers and handles.
 */
void _gnutls_handshake_hash_buffers_clear(gnutls_session_t session)
{
	session->internals.handshake_hash_buffer_prev_len = 0;
	session->internals.handshake_hash_buffer_client_kx_len = 0;
	_gnutls_buffer_clear(&session->internals.handshake_hash_buffer);
}

/* this will copy the required values for resuming to
 * internals, and to security_parameters.
 * this will keep as less data to security_parameters.
 */
static int resume_copy_required_values(gnutls_session_t session)
{
	int ret;

	/* get the new random values */
	memcpy(session->internals.resumed_security_parameters.
	       server_random, session->security_parameters.server_random,
	       GNUTLS_RANDOM_SIZE);
	memcpy(session->internals.resumed_security_parameters.
	       client_random, session->security_parameters.client_random,
	       GNUTLS_RANDOM_SIZE);

	/* keep the ciphersuite and compression 
	 * That is because the client must see these in our
	 * hello message.
	 */
	memcpy(session->security_parameters.cipher_suite,
	       session->internals.resumed_security_parameters.cipher_suite,
	       2);
	session->security_parameters.compression_method =
	    session->internals.resumed_security_parameters.
	    compression_method;

	ret = _gnutls_epoch_set_cipher_suite(session, EPOCH_NEXT,
					     session->internals.
					     resumed_security_parameters.
					     cipher_suite);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_epoch_set_compression(session, EPOCH_NEXT,
					    session->internals.
					    resumed_security_parameters.
					    compression_method);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* or write_compression_algorithm
	 * they are the same
	 */

	session->security_parameters.entity =
	    session->internals.resumed_security_parameters.entity;

	if (session->internals.resumed_security_parameters.pversion ==
	    NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (_gnutls_set_current_version(session,
				    session->internals.
				    resumed_security_parameters.pversion->
				    id) < 0)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	session->security_parameters.cert_type =
	    session->internals.resumed_security_parameters.cert_type;

	memcpy(session->security_parameters.session_id,
	       session->internals.resumed_security_parameters.session_id,
	       sizeof(session->security_parameters.session_id));
	session->security_parameters.session_id_size =
	    session->internals.resumed_security_parameters.session_id_size;

	return 0;
}


/* this function will produce GNUTLS_RANDOM_SIZE==32 bytes of random data
 * and put it to dst.
 */
static int create_tls_random(uint8_t * dst)
{
	uint32_t tim;
	int ret;

	/* Use weak random numbers for the most of the
	 * buffer except for the first 4 that are the
	 * system's time.
	 */

	tim = gnutls_time(NULL);
	/* generate server random value */
	_gnutls_write_uint32(tim, dst);

	ret =
	    gnutls_rnd(GNUTLS_RND_NONCE, &dst[3], GNUTLS_RANDOM_SIZE - 3);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

int _gnutls_set_client_random(gnutls_session_t session, uint8_t * rnd)
{
	int ret;

	if (rnd != NULL)
		memcpy(session->security_parameters.client_random, rnd,
		       GNUTLS_RANDOM_SIZE);
	else {
		/* no random given, we generate. */
		if (session->internals.sc_random_set != 0) {
			memcpy(session->security_parameters.client_random,
			       session->internals.
			       resumed_security_parameters.client_random,
			       GNUTLS_RANDOM_SIZE);
		} else {
			ret =
			    create_tls_random(session->
					      security_parameters.
					      client_random);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
	}
	return 0;
}

int _gnutls_set_server_random(gnutls_session_t session, uint8_t * rnd)
{
	int ret;

	if (rnd != NULL)
		memcpy(session->security_parameters.server_random, rnd,
		       GNUTLS_RANDOM_SIZE);
	else {
		/* no random given, we generate. */
		if (session->internals.sc_random_set != 0) {
			memcpy(session->security_parameters.server_random,
			       session->internals.
			       resumed_security_parameters.server_random,
			       GNUTLS_RANDOM_SIZE);
		} else {
			ret =
			    create_tls_random(session->
					      security_parameters.
					      server_random);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
	}
	return 0;
}

#ifdef ENABLE_SSL3
/* Calculate The SSL3 Finished message
 */
#define SSL3_CLIENT_MSG "CLNT"
#define SSL3_SERVER_MSG "SRVR"
#define SSL_MSG_LEN 4
static int
_gnutls_ssl3_finished(gnutls_session_t session, int type, uint8_t * ret,
		      int sending)
{
	digest_hd_st td_md5;
	digest_hd_st td_sha;
	const char *mesg;
	int rc, len;

	if (sending)
		len = session->internals.handshake_hash_buffer.length;
	else
		len = session->internals.handshake_hash_buffer_prev_len;

	rc = _gnutls_hash_init(&td_sha, hash_to_entry(GNUTLS_DIG_SHA1));
	if (rc < 0)
		return gnutls_assert_val(rc);

	rc = _gnutls_hash_init(&td_md5, hash_to_entry(GNUTLS_DIG_MD5));
	if (rc < 0) {
		_gnutls_hash_deinit(&td_sha, NULL);
		return gnutls_assert_val(rc);
	}

	_gnutls_hash(&td_sha,
		     session->internals.handshake_hash_buffer.data, len);
	_gnutls_hash(&td_md5,
		     session->internals.handshake_hash_buffer.data, len);

	if (type == GNUTLS_SERVER)
		mesg = SSL3_SERVER_MSG;
	else
		mesg = SSL3_CLIENT_MSG;

	_gnutls_hash(&td_md5, mesg, SSL_MSG_LEN);
	_gnutls_hash(&td_sha, mesg, SSL_MSG_LEN);

	rc = _gnutls_mac_deinit_ssl3_handshake(&td_md5, ret,
					       session->security_parameters.
					       master_secret,
					       GNUTLS_MASTER_SIZE);
	if (rc < 0) {
		_gnutls_hash_deinit(&td_md5, NULL);
		_gnutls_hash_deinit(&td_sha, NULL);
		return gnutls_assert_val(rc);
	}

	rc = _gnutls_mac_deinit_ssl3_handshake(&td_sha, &ret[16],
					       session->security_parameters.
					       master_secret,
					       GNUTLS_MASTER_SIZE);
	if (rc < 0) {
		_gnutls_hash_deinit(&td_sha, NULL);
		return gnutls_assert_val(rc);
	}

	return 0;
}
#endif

/* Hash the handshake messages as required by TLS 1.0
 */
#define SERVER_MSG "server finished"
#define CLIENT_MSG "client finished"
#define TLS_MSG_LEN 15
static int
_gnutls_finished(gnutls_session_t session, int type, void *ret,
		 int sending)
{
	const int siz = TLS_MSG_LEN;
	uint8_t concat[MAX_HASH_SIZE + 16 /*MD5 */ ];
	size_t hash_len;
	const char *mesg;
	int rc, len;

	if (sending)
		len = session->internals.handshake_hash_buffer.length;
	else
		len = session->internals.handshake_hash_buffer_prev_len;

	if (!_gnutls_version_has_selectable_prf(get_version(session))) {
		rc = _gnutls_hash_fast(GNUTLS_DIG_SHA1,
				       session->internals.
				       handshake_hash_buffer.data, len,
				       &concat[16]);
		if (rc < 0)
			return gnutls_assert_val(rc);

		rc = _gnutls_hash_fast(GNUTLS_DIG_MD5,
				       session->internals.
				       handshake_hash_buffer.data, len,
				       concat);
		if (rc < 0)
			return gnutls_assert_val(rc);

		hash_len = 20 + 16;
	} else {
		int algorithm =
		    _gnutls_cipher_suite_get_prf(session->
						 security_parameters.
						 cipher_suite);

		rc = _gnutls_hash_fast(algorithm,
				       session->internals.
				       handshake_hash_buffer.data, len,
				       concat);
		if (rc < 0)
			return gnutls_assert_val(rc);

		hash_len =
		    _gnutls_hash_get_algo_len(mac_to_entry(algorithm));
	}

	if (type == GNUTLS_SERVER) {
		mesg = SERVER_MSG;
	} else {
		mesg = CLIENT_MSG;
	}

	return _gnutls_PRF(session,
			   session->security_parameters.master_secret,
			   GNUTLS_MASTER_SIZE, mesg, siz, concat, hash_len,
			   12, ret);
}


/* returns the 0 on success or a negative error code.
 */
int
_gnutls_negotiate_version(gnutls_session_t session,
			  gnutls_protocol_t adv_version, uint8_t major, uint8_t minor)
{
	int ret;


	/* if we do not support that version  */
	if (adv_version == GNUTLS_VERSION_UNKNOWN || _gnutls_version_is_supported(session, adv_version) == 0) {
		/* if we get an unknown/unsupported version, then fail if the version we
		 * got is too low to be supported */
		if (!_gnutls_version_is_too_high(session, major, minor))
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

		/* If he requested something we do not support
		 * then we send him the highest we support.
		 */
		ret = _gnutls_version_max(session);
		if (ret == GNUTLS_VERSION_UNKNOWN) {
			/* this check is not really needed.
			 */
			gnutls_assert();
			return GNUTLS_E_UNKNOWN_CIPHER_SUITE;
		}
	} else {
		ret = adv_version;
	}

	if (_gnutls_set_current_version(session, ret) < 0)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	return ret;
}

/* This function returns:
 *  - zero on success
 *  - GNUTLS_E_INT_RET_0 if GNUTLS_E_AGAIN || GNUTLS_E_INTERRUPTED were returned by the callback
 *  - a negative error code on other error
 */
int
_gnutls_user_hello_func(gnutls_session_t session,
			gnutls_protocol_t adv_version, uint8_t major, uint8_t minor)
{
	int ret, sret = 0;

	if (session->internals.user_hello_func != NULL) {
		ret = session->internals.user_hello_func(session);

		if (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED) {
			gnutls_assert();
			sret = GNUTLS_E_INT_RET_0;
		} else if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* Here we need to renegotiate the version since the callee might
		 * have disabled some TLS versions.
		 */
		ret = _gnutls_negotiate_version(session, adv_version, major, minor);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}
	return sret;
}

/* Read a client hello packet. 
 * A client hello must be a known version client hello
 * or version 2.0 client hello (only for compatibility
 * since SSL version 2.0 is not supported).
 */
static int
read_client_hello(gnutls_session_t session, uint8_t * data,
			  int datalen)
{
	uint8_t session_id_len;
	int pos = 0, ret;
	uint16_t suite_size, comp_size, ext_size;
	gnutls_protocol_t adv_version;
	int neg_version, sret = 0;
	int len = datalen;
	uint8_t major, minor;
	uint8_t *suite_ptr, *comp_ptr, *session_id, *ext_ptr;

	DECR_LEN(len, 2);
	_gnutls_handshake_log("HSK[%p]: Client's version: %d.%d\n",
			      session, data[pos], data[pos + 1]);

	adv_version = _gnutls_version_get(data[pos], data[pos + 1]);

	major = data[pos];
	minor = data[pos+1];
	set_adv_version(session, major, minor);

	neg_version = _gnutls_negotiate_version(session, adv_version, major, minor);
	if (neg_version < 0) {
		gnutls_assert();
		return neg_version;
	}

	pos += 2;

	_gnutls_handshake_log("HSK[%p]: Selected version %s\n",
		     session, gnutls_protocol_get_name(neg_version));

	/* Read client random value.
	 */
	DECR_LEN(len, GNUTLS_RANDOM_SIZE);
	ret = _gnutls_set_client_random(session, &data[pos]);
	if (ret < 0)
		return gnutls_assert_val(ret);

	pos += GNUTLS_RANDOM_SIZE;

	ret = _gnutls_set_server_random(session, NULL);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->security_parameters.timestamp = gnutls_time(NULL);

	DECR_LEN(len, 1);
	session_id_len = data[pos++];

	/* RESUME SESSION 
	 */
	if (session_id_len > GNUTLS_MAX_SESSION_ID_SIZE) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}
	DECR_LEN(len, session_id_len);
	session_id = &data[pos];
	pos += session_id_len;

	if (IS_DTLS(session)) {
		int cookie_size;

		DECR_LEN(len, 1);
		cookie_size = data[pos++];
		DECR_LEN(len, cookie_size);
		pos += cookie_size;
	}

	/* move forward to extensions and store other vals */
	DECR_LEN(len, 2);
	suite_size = _gnutls_read_uint16(&data[pos]);
	pos += 2;

	suite_ptr = &data[pos];
	DECR_LEN(len, suite_size);
	pos += suite_size;

	DECR_LEN(len, 1);
	comp_size = data[pos++]; /* the number of compression methods */

	comp_ptr = &data[pos];
	DECR_LEN(len, comp_size);
	pos += comp_size;

	ext_ptr = &data[pos];
	ext_size = len;

	/* Parse only the mandatory to read extensions for resumption.
	 * We don't want to parse any other extensions since
	 * we don't want new extension values to override the
	 * resumed ones.
	 */
	ret =
	    _gnutls_parse_extensions(session, GNUTLS_EXT_MANDATORY,
				     ext_ptr, ext_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_server_restore_session(session, session_id,
					   session_id_len);

	if (session_id_len > 0)
		session->internals.resumption_requested = 1;

	if (ret == 0) {		/* resumed using default TLS resumption! */
		ret = _gnutls_server_select_suite(session, suite_ptr, suite_size, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = resume_copy_required_values(session);
		if (ret < 0)
			return gnutls_assert_val(ret);

		session->internals.resumed = RESUME_TRUE;

		return _gnutls_user_hello_func(session, adv_version, major, minor);
	} else {
		_gnutls_generate_session_id(session->security_parameters.
					    session_id,
					    &session->security_parameters.
					    session_id_size);

		session->internals.resumed = RESUME_FALSE;
	}

	/* Parse the extensions (if any)
	 *
	 * Unconditionally try to parse extensions; safe renegotiation uses them in
	 * sslv3 and higher, even though sslv3 doesn't officially support them.
	 */
	ret = _gnutls_parse_extensions(session, GNUTLS_EXT_APPLICATION,
				       ext_ptr, ext_size);
	/* len is the rest of the parsed length */
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* we cache this error code */
	sret = _gnutls_user_hello_func(session, adv_version, major, minor);
	if (sret < 0 && sret != GNUTLS_E_INT_RET_0) {
		gnutls_assert();
		return sret;
	}

	/* Session tickets are parsed in this point */
	ret =
	    _gnutls_parse_extensions(session, GNUTLS_EXT_TLS, ext_ptr, ext_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* resumed by session_ticket extension */
	if (session->internals.resumed != RESUME_FALSE) {
		/* to indicate the client that the current session is resumed */
		memcpy(session->internals.resumed_security_parameters.
		       session_id, session_id, session_id_len);
		session->internals.resumed_security_parameters.
		    session_id_size = session_id_len;

		session->internals.resumed_security_parameters.
		    max_record_recv_size =
		    session->security_parameters.max_record_recv_size;
		session->internals.resumed_security_parameters.
		    max_record_send_size =
		    session->security_parameters.max_record_send_size;

		ret = _gnutls_server_select_suite(session, suite_ptr, suite_size, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = resume_copy_required_values(session);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return 0;
	}

	/* select an appropriate cipher suite (as well as certificate)
	 */
	ret = _gnutls_server_select_suite(session, suite_ptr, suite_size, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* select appropriate compression method */
	ret =
	    server_select_comp_method(session, comp_ptr,
					      comp_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* call extensions that are intended to be parsed after the ciphersuite/cert
	 * are known. */
	ret =
	    _gnutls_parse_extensions(session, _GNUTLS_EXT_TLS_POST_CS, ext_ptr, ext_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return sret;
}

/* This is to be called after sending CHANGE CIPHER SPEC packet
 * and initializing encryption. This is the first encrypted message
 * we send.
 */
static int _gnutls_send_finished(gnutls_session_t session, int again)
{
	mbuffer_st *bufel;
	uint8_t *data;
	int ret;
	size_t vdata_size = 0;
	const version_entry_st *vers;

	if (again == 0) {
		bufel =
		    _gnutls_handshake_alloc(session, 
					    MAX_VERIFY_DATA_SIZE);
		if (bufel == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		data = _mbuffer_get_udata_ptr(bufel);

		vers = get_version(session);
		if (unlikely(vers == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

#ifdef ENABLE_SSL3
		if (vers->id == GNUTLS_SSL3) {
			ret =
			    _gnutls_ssl3_finished(session,
						  session->
						  security_parameters.
						  entity, data, 1);
			_mbuffer_set_udata_size(bufel, 36);
		} else {	/* TLS 1.0+ */
#endif
			ret = _gnutls_finished(session,
					       session->
					       security_parameters.entity,
					       data, 1);
			_mbuffer_set_udata_size(bufel, 12);
#ifdef ENABLE_SSL3
		}
#endif

		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		vdata_size = _mbuffer_get_udata_size(bufel);

		ret =
		    _gnutls_ext_sr_finished(session, data, vdata_size, 0);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		if ((session->internals.resumed == RESUME_FALSE
		     && session->security_parameters.entity ==
		     GNUTLS_CLIENT)
		    || (session->internals.resumed != RESUME_FALSE
			&& session->security_parameters.entity ==
			GNUTLS_SERVER)) {
			/* if we are a client not resuming - or we are a server resuming */
			_gnutls_handshake_log
			    ("HSK[%p]: recording tls-unique CB (send)\n",
			     session);
			memcpy(session->internals.cb_tls_unique, data,
			       vdata_size);
			session->internals.cb_tls_unique_len = vdata_size;
		}

		ret =
		    _gnutls_send_handshake(session, bufel,
					   GNUTLS_HANDSHAKE_FINISHED);
	} else {
		ret =
		    _gnutls_send_handshake(session, NULL,
					   GNUTLS_HANDSHAKE_FINISHED);
	}

	return ret;
}

/* This is to be called after sending our finished message. If everything
 * went fine we have negotiated a secure connection 
 */
static int _gnutls_recv_finished(gnutls_session_t session)
{
	uint8_t data[MAX_VERIFY_DATA_SIZE], *vrfy;
	gnutls_buffer_st buf;
	int data_size;
	int ret;
	int vrfy_size;
	const version_entry_st *vers = get_version(session);

	if (unlikely(vers == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	ret =
	    _gnutls_recv_handshake(session, GNUTLS_HANDSHAKE_FINISHED,
				   0, &buf);
	if (ret < 0) {
		ERR("recv finished int", ret);
		gnutls_assert();
		return ret;
	}

	vrfy = buf.data;
	vrfy_size = buf.length;

#ifdef ENABLE_SSL3
	if (vers->id == GNUTLS_SSL3)
		data_size = 36;
	else
#endif
		data_size = 12;

	if (vrfy_size != data_size) {
		gnutls_assert();
		ret = GNUTLS_E_ERROR_IN_FINISHED_PACKET;
		goto cleanup;
	}

#ifdef ENABLE_SSL3
	if (vers->id == GNUTLS_SSL3) {
		ret =
		    _gnutls_ssl3_finished(session,
					  (session->security_parameters.
					   entity + 1) % 2, data, 0);
	} else /* TLS 1.0+ */
#endif
		ret =
		    _gnutls_finished(session,
				     (session->security_parameters.entity +
				      1) % 2, data, 0);

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (memcmp(vrfy, data, data_size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_ERROR_IN_FINISHED_PACKET;
		goto cleanup;
	}

	ret = _gnutls_ext_sr_finished(session, data, data_size, 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if ((session->internals.resumed != RESUME_FALSE
	     && session->security_parameters.entity == GNUTLS_CLIENT)
	    || (session->internals.resumed == RESUME_FALSE
		&& session->security_parameters.entity == GNUTLS_SERVER)) {
		/* if we are a client resuming - or we are a server not resuming */
		_gnutls_handshake_log
		    ("HSK[%p]: recording tls-unique CB (recv)\n", session);
		memcpy(session->internals.cb_tls_unique, data, data_size);
		session->internals.cb_tls_unique_len = data_size;
	}


	session->internals.initial_negotiation_completed = 1;

      cleanup:
	_gnutls_buffer_clear(&buf);

	return ret;
}

/* returns PK_RSA if the given cipher suite list only supports,
 * RSA algorithms, PK_DSA if DSS, and PK_ANY for both or PK_NONE for none.
 */
static int
server_find_pk_algos_in_ciphersuites(const uint8_t *
				     data, unsigned int datalen,
				     gnutls_pk_algorithm_t * algos,
				     size_t * algos_size)
{
	unsigned int j, x;
	gnutls_kx_algorithm_t kx;
	gnutls_pk_algorithm_t pk;
	unsigned found;
	unsigned int max = *algos_size;

	if (datalen % 2 != 0) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	*algos_size = 0;
	for (j = 0; j < datalen; j += 2) {
		kx = _gnutls_cipher_suite_get_kx_algo(&data[j]);
		if (_gnutls_map_kx_get_cred(kx, 1) ==
		    GNUTLS_CRD_CERTIFICATE) {
			pk = _gnutls_map_kx_get_pk(kx);
			found = 0;
			for (x = 0; x < *algos_size; x++) {
				if (algos[x] == pk) {
					found = 1;
					break;
				}
			}

			if (found == 0) {
				algos[(*algos_size)++] =
				    _gnutls_map_kx_get_pk(kx);
				if ((*algos_size) >= max)
					return 0;
			}
		}
	}

	return 0;
}

/* This selects the best supported ciphersuite from the given ones. Then
 * it adds the suite to the session and performs some checks.
 *
 * When @scsv_only is non-zero only the available SCSVs are parsed
 * and acted upon.
 */
int
_gnutls_server_select_suite(gnutls_session_t session, uint8_t * data,
			    unsigned int datalen, unsigned scsv_only)
{
	int ret;
	unsigned int i, j, cipher_suites_size;
	size_t pk_algos_size;
	uint8_t cipher_suites[MAX_CIPHERSUITE_SIZE];
	int retval;
	gnutls_pk_algorithm_t pk_algos[MAX_ALGOS];	/* will hold the pk algorithms
							 * supported by the peer.
							 */

	for (i = 0; i < datalen; i += 2) {
		/* we support the TLS renegotiation SCSV, even if we are
		 * not under SSL 3.0, because openssl sends this SCSV
		 * on resumption unconditionally. */
		/* TLS_RENEGO_PROTECTION_REQUEST = { 0x00, 0xff } */
		if (session->internals.priorities.sr != SR_DISABLED &&
		    data[i] == GNUTLS_RENEGO_PROTECTION_REQUEST_MAJOR &&
		    data[i + 1] == GNUTLS_RENEGO_PROTECTION_REQUEST_MINOR) {
			_gnutls_handshake_log
			    ("HSK[%p]: Received safe renegotiation CS\n",
			     session);
			retval = _gnutls_ext_sr_recv_cs(session);
			if (retval < 0) {
				gnutls_assert();
				return retval;
			}
		}

		/* TLS_FALLBACK_SCSV */
		if (data[i] == GNUTLS_FALLBACK_SCSV_MAJOR &&
		    data[i + 1] == GNUTLS_FALLBACK_SCSV_MINOR) {
			unsigned max = _gnutls_version_max(session);
			_gnutls_handshake_log
			    ("HSK[%p]: Received fallback CS\n",
			     session);

			if (gnutls_protocol_get_version(session) != max)
				return gnutls_assert_val(GNUTLS_E_INAPPROPRIATE_FALLBACK);
		}
	}

	if (scsv_only)
		return 0;

	pk_algos_size = MAX_ALGOS;
	ret =
	    server_find_pk_algos_in_ciphersuites(data, datalen, pk_algos,
						 &pk_algos_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_supported_ciphersuites(session, cipher_suites,
					   sizeof(cipher_suites));
	if (ret < 0)
		return gnutls_assert_val(ret);

	cipher_suites_size = ret;

	/* Here we remove any ciphersuite that does not conform
	 * the certificate requested, or to the
	 * authentication requested (e.g. SRP).
	 */
	ret =
	    _gnutls_remove_unwanted_ciphersuites(session, cipher_suites,
						 cipher_suites_size,
						 pk_algos, pk_algos_size);
	if (ret <= 0) {
		gnutls_assert();
		if (ret < 0)
			return ret;
		else
			return GNUTLS_E_UNKNOWN_CIPHER_SUITE;
	}

	cipher_suites_size = ret;

	/* Data length should be zero mod 2 since
	 * every ciphersuite is 2 bytes. (this check is needed
	 * see below).
	 */
	if (datalen % 2 != 0) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	memset(session->security_parameters.cipher_suite, 0, 2);

	retval = GNUTLS_E_UNKNOWN_CIPHER_SUITE;

	_gnutls_handshake_log
	    ("HSK[%p]: Requested cipher suites[size: %d]: \n", session,
	     (int) datalen);

	if (session->internals.priorities.server_precedence == 0) {
		for (j = 0; j < datalen; j += 2) {
			_gnutls_handshake_log("\t0x%.2x, 0x%.2x %s\n",
					      data[j], data[j + 1],
					      _gnutls_cipher_suite_get_name
					      (&data[j]));
			for (i = 0; i < cipher_suites_size; i += 2) {
				if (memcmp(&cipher_suites[i], &data[j], 2)
				    == 0) {
					_gnutls_handshake_log
					    ("HSK[%p]: Selected cipher suite: %s\n",
					     session,
					     _gnutls_cipher_suite_get_name
					     (&data[j]));
					memcpy(session->
					       security_parameters.
					       cipher_suite,
					       &cipher_suites[i], 2);
					_gnutls_epoch_set_cipher_suite
					    (session, EPOCH_NEXT,
					     session->security_parameters.
					     cipher_suite);


					retval = 0;
					goto finish;
				}
			}
		}
	} else {		/* server selects */

		for (i = 0; i < cipher_suites_size; i += 2) {
			for (j = 0; j < datalen; j += 2) {
				if (memcmp(&cipher_suites[i], &data[j], 2)
				    == 0) {
					_gnutls_handshake_log
					    ("HSK[%p]: Selected cipher suite: %s\n",
					     session,
					     _gnutls_cipher_suite_get_name
					     (&data[j]));
					memcpy(session->
					       security_parameters.
					       cipher_suite,
					       &cipher_suites[i], 2);
					_gnutls_epoch_set_cipher_suite
					    (session, EPOCH_NEXT,
					     session->security_parameters.
					     cipher_suite);


					retval = 0;
					goto finish;
				}
			}
		}
	}
      finish:

	if (retval != 0) {
		gnutls_assert();
		return retval;
	}

	/* check if the credentials (username, public key etc.) are ok
	 */
	if (_gnutls_get_kx_cred
	    (session,
	     _gnutls_cipher_suite_get_kx_algo(session->security_parameters.
					      cipher_suite)) == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}


	/* set the mod_auth_st to the appropriate struct
	 * according to the KX algorithm. This is needed since all the
	 * handshake functions are read from there;
	 */
	session->internals.auth_struct =
	    _gnutls_kx_auth_struct(_gnutls_cipher_suite_get_kx_algo
				   (session->security_parameters.
				    cipher_suite));
	if (session->internals.auth_struct == NULL) {

		_gnutls_handshake_log
		    ("HSK[%p]: Cannot find the appropriate handler for the KX algorithm\n",
		     session);
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	return 0;

}


/* This selects the best supported compression method from the ones provided 
 */
static int
server_select_comp_method(gnutls_session_t session,
			  uint8_t * data, int datalen)
{
	int x, i, j;
	uint8_t comps[MAX_ALGOS];

	x = _gnutls_supported_compression_methods(session, comps,
						  MAX_ALGOS);
	if (x < 0) {
		gnutls_assert();
		return x;
	}

	if (session->internals.priorities.server_precedence == 0) {
		for (j = 0; j < datalen; j++) {
			for (i = 0; i < x; i++) {
				if (comps[i] == data[j]) {
					gnutls_compression_method_t method
					    =
					    _gnutls_compression_get_id
					    (comps[i]);

					_gnutls_epoch_set_compression
					    (session, EPOCH_NEXT, method);
					session->security_parameters.
					    compression_method = method;

					_gnutls_handshake_log
					    ("HSK[%p]: Selected Compression Method: %s\n",
					     session,
					     gnutls_compression_get_name
					     (method));
					return 0;
				}
			}
		}
	} else {
		for (i = 0; i < x; i++) {
			for (j = 0; j < datalen; j++) {
				if (comps[i] == data[j]) {
					gnutls_compression_method_t method
					    =
					    _gnutls_compression_get_id
					    (comps[i]);

					_gnutls_epoch_set_compression
					    (session, EPOCH_NEXT, method);
					session->security_parameters.
					    compression_method = method;

					_gnutls_handshake_log
					    ("HSK[%p]: Selected Compression Method: %s\n",
					     session,
					     gnutls_compression_get_name
					     (method));
					return 0;
				}
			}
		}
	}

	/* we were not able to find a compatible compression
	 * algorithm
	 */
	gnutls_assert();
	return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;

}

/* This function sends an empty handshake packet. (like hello request).
 * If the previous _gnutls_send_empty_handshake() returned
 * GNUTLS_E_AGAIN or GNUTLS_E_INTERRUPTED, then it must be called again 
 * (until it returns ok), with NULL parameters.
 */
static int
_gnutls_send_empty_handshake(gnutls_session_t session,
			     gnutls_handshake_description_t type,
			     int again)
{
	mbuffer_st *bufel;

	if (again == 0) {
		bufel = _gnutls_handshake_alloc(session, 0);
		if (bufel == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
	} else
		bufel = NULL;

	return _gnutls_send_handshake(session, bufel, type);
}

inline
    static int call_hook_func(gnutls_session_t session,
			      gnutls_handshake_description_t type,
			      int post, unsigned incoming,
			      const uint8_t *data, unsigned data_size)
{
	gnutls_datum_t msg = {(void*)data, data_size};

	if (session->internals.h_hook != NULL) {
		if ((session->internals.h_type == type
		     || session->internals.h_type == GNUTLS_HANDSHAKE_ANY)
		    && (session->internals.h_post == post
			|| session->internals.h_post == GNUTLS_HOOK_BOTH))
			return session->internals.h_hook(session, type,
							 post, incoming, &msg);
	}
	return 0;
}

/* This function sends a handshake message of type 'type' containing the
 * data specified here. If the previous _gnutls_send_handshake() returned
 * GNUTLS_E_AGAIN or GNUTLS_E_INTERRUPTED, then it must be called again 
 * (until it returns ok), with NULL parameters.
 */
int
_gnutls_send_handshake(gnutls_session_t session, mbuffer_st * bufel,
		       gnutls_handshake_description_t type)
{
	int ret;
	uint8_t *data;
	uint32_t datasize, i_datasize;
	int pos = 0;

	if (bufel == NULL) {
		/* we are resuming a previously interrupted
		 * send.
		 */
		ret = _gnutls_handshake_io_write_flush(session);
		return ret;

	}

	/* first run */
	data = _mbuffer_get_uhead_ptr(bufel);
	i_datasize = _mbuffer_get_udata_size(bufel);
	datasize = i_datasize + _mbuffer_get_uhead_size(bufel);

	data[pos++] = (uint8_t) type;
	_gnutls_write_uint24(_mbuffer_get_udata_size(bufel), &data[pos]);
	pos += 3;

	/* Add DTLS handshake fragment headers.  The message will be
	 * fragmented later by the fragmentation sub-layer. All fields must
	 * be set properly for HMAC. The HMAC requires we pretend that the
	 * message was sent in a single fragment. */
	if (IS_DTLS(session)) {
		_gnutls_write_uint16(session->internals.dtls.
				     hsk_write_seq++, &data[pos]);
		pos += 2;

		/* Fragment offset */
		_gnutls_write_uint24(0, &data[pos]);
		pos += 3;

		/* Fragment length */
		_gnutls_write_uint24(i_datasize, &data[pos]);
		/* pos += 3; */
	}

	_gnutls_handshake_log("HSK[%p]: %s was queued [%ld bytes]\n",
			      session, _gnutls_handshake2str(type),
			      (long) datasize);

	/* Here we keep the handshake messages in order to hash them...
	 */
	if (type != GNUTLS_HANDSHAKE_HELLO_REQUEST)
		if ((ret =
		     handshake_hash_add_sent(session, type, data,
						     datasize)) < 0) {
			gnutls_assert();
			_mbuffer_xfree(&bufel);
			return ret;
		}

	ret = call_hook_func(session, type, GNUTLS_HOOK_PRE, 0,
			     _mbuffer_get_udata_ptr(bufel), _mbuffer_get_udata_size(bufel));
	if (ret < 0) {
		gnutls_assert();
		_mbuffer_xfree(&bufel);
		return ret;
	}

	session->internals.last_handshake_out = type;

	ret = _gnutls_handshake_io_cache_int(session, type, bufel);
	if (ret < 0) {
		_mbuffer_xfree(&bufel);
		gnutls_assert();
		return ret;
	}

	ret = call_hook_func(session, type, GNUTLS_HOOK_POST, 0, 
			      _mbuffer_get_udata_ptr(bufel), _mbuffer_get_udata_size(bufel));
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* The messages which are followed by another are not sent by default
	 * but are cached instead */
	switch (type) {
	case GNUTLS_HANDSHAKE_CERTIFICATE_PKT:	/* this one is followed by ServerHelloDone
						 * or ClientKeyExchange always.
						 */
	case GNUTLS_HANDSHAKE_CERTIFICATE_STATUS:
	case GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE:	/* as above */
	case GNUTLS_HANDSHAKE_SERVER_HELLO:	/* as above */
	case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:	/* as above */
	case GNUTLS_HANDSHAKE_NEW_SESSION_TICKET:	/* followed by ChangeCipherSpec */

		/* now for client Certificate, ClientKeyExchange and
		 * CertificateVerify are always followed by ChangeCipherSpec
		 */
	case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY:
	case GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
		ret = 0;
		break;
	default:
		/* send cached messages */
		ret = _gnutls_handshake_io_write_flush(session);
		break;
	}

	return ret;
}

#define CHECK_SIZE(ll) \
  if ((session->internals.max_handshake_data_buffer_size > 0) && \
      (((ll) + session->internals.handshake_hash_buffer.length) > \
       session->internals.max_handshake_data_buffer_size)) { \
    _gnutls_debug_log("Handshake buffer length is %u (max: %u)\n", (unsigned)((ll) + session->internals.handshake_hash_buffer.length), (unsigned)session->internals.max_handshake_data_buffer_size); \
    return gnutls_assert_val(GNUTLS_E_HANDSHAKE_TOO_LARGE); \
    }

/* This function add the handshake headers and the
 * handshake data to the handshake hash buffers. Needed
 * for the finished messages calculations.
 */
static int
handshake_hash_add_recvd(gnutls_session_t session,
				 gnutls_handshake_description_t recv_type,
				 uint8_t * header, uint16_t header_size,
				 uint8_t * dataptr, uint32_t datalen)
{
	int ret;
	const version_entry_st *vers = get_version(session);

	if (unlikely(vers == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if ((vers->id != GNUTLS_DTLS0_9 &&
	     recv_type == GNUTLS_HANDSHAKE_HELLO_VERIFY_REQUEST) ||
	    recv_type == GNUTLS_HANDSHAKE_HELLO_REQUEST)
		return 0;

	CHECK_SIZE(header_size + datalen);

	session->internals.handshake_hash_buffer_prev_len =
	    session->internals.handshake_hash_buffer.length;

	if (vers->id != GNUTLS_DTLS0_9) {
		ret =
		    _gnutls_buffer_append_data(&session->internals.
					       handshake_hash_buffer,
					       header, header_size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}
	if (datalen > 0) {
		ret =
		    _gnutls_buffer_append_data(&session->internals.
					       handshake_hash_buffer,
					       dataptr, datalen);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	/* save the size until client KX. That is because the TLS
	 * session hash is calculated up to this message.
	 */
	if (recv_type == GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE)
		session->internals.handshake_hash_buffer_client_kx_len =
			session->internals.handshake_hash_buffer.length;

	return 0;
}

/* This function will store the handshake message we sent.
 */
static int
handshake_hash_add_sent(gnutls_session_t session,
				gnutls_handshake_description_t type,
				uint8_t * dataptr, uint32_t datalen)
{
	int ret;
	const version_entry_st *vers = get_version(session);

	if (unlikely(vers == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	/* We don't check for GNUTLS_HANDSHAKE_HELLO_VERIFY_REQUEST because it
	 * is not sent via that channel.
	 */
	if (type != GNUTLS_HANDSHAKE_HELLO_REQUEST) {
		CHECK_SIZE(datalen);

		if (vers->id == GNUTLS_DTLS0_9) {
			/* Old DTLS doesn't include the header in the MAC */
			if (datalen < 12) {
				gnutls_assert();
				return GNUTLS_E_INTERNAL_ERROR;
			}
			dataptr += 12;
			datalen -= 12;

			if (datalen == 0)
				return 0;
		}

		ret =
		    _gnutls_buffer_append_data(&session->internals.
					       handshake_hash_buffer,
					       dataptr, datalen);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (type == GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE)
			session->internals.handshake_hash_buffer_client_kx_len =
				session->internals.handshake_hash_buffer.length;

		return 0;
	}

	return 0;
}

/* This function will receive handshake messages of the given types,
 * and will pass the message to the right place in order to be processed.
 * E.g. for the SERVER_HELLO message (if it is expected), it will be
 * passed to _gnutls_recv_hello().
 */
int
_gnutls_recv_handshake(gnutls_session_t session,
		       gnutls_handshake_description_t type,
		       unsigned int optional, gnutls_buffer_st * buf)
{
	int ret, ret2;
	handshake_buffer_st hsk;

	ret = _gnutls_handshake_io_recv_int(session, type, &hsk, optional);
	if (ret < 0) {
		if (optional != 0
		    && ret == GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET) {
			if (buf)
				_gnutls_buffer_init(buf);
			return 0;
		}

		return gnutls_assert_val_fatal(ret);
	}

	session->internals.last_handshake_in = hsk.htype;

	ret = call_hook_func(session, hsk.htype, GNUTLS_HOOK_PRE, 1, hsk.data.data, hsk.data.length);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = handshake_hash_add_recvd(session, hsk.htype,
				       hsk.header, hsk.header_size,
				       hsk.data.data,
				       hsk.data.length);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	switch (hsk.htype) {
	case GNUTLS_HANDSHAKE_CLIENT_HELLO_V2:
	case GNUTLS_HANDSHAKE_CLIENT_HELLO:
	case GNUTLS_HANDSHAKE_SERVER_HELLO:
#ifdef ENABLE_SSL2
		if (hsk.htype == GNUTLS_HANDSHAKE_CLIENT_HELLO_V2)
			ret =
			    _gnutls_read_client_hello_v2(session,
							 hsk.data.data,
							 hsk.data.length);
		else
#endif
			ret =
			    recv_hello(session, hsk.data.data,
					hsk.data.length);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		break;
	case GNUTLS_HANDSHAKE_HELLO_VERIFY_REQUEST:
		ret =
		    recv_hello_verify_request(session,
					      hsk.data.data,
					      hsk.data.length);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		} else {
			/* Signal our caller we have received a verification cookie
			   and ClientHello needs to be sent again. */
			ret = 1;
		}

		break;
	case GNUTLS_HANDSHAKE_SERVER_HELLO_DONE:
		if (hsk.data.length == 0)
			ret = 0;
		else {
			gnutls_assert();
			ret = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			goto cleanup;
		}
		break;
	case GNUTLS_HANDSHAKE_CERTIFICATE_PKT:
	case GNUTLS_HANDSHAKE_CERTIFICATE_STATUS:
	case GNUTLS_HANDSHAKE_FINISHED:
	case GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
	case GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
	case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:
	case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY:
	case GNUTLS_HANDSHAKE_SUPPLEMENTAL:
	case GNUTLS_HANDSHAKE_NEW_SESSION_TICKET:
		ret = hsk.data.length;
		break;
	default:
		gnutls_assert();
		/* we shouldn't actually arrive here in any case .
		 * unexpected messages should be catched after _gnutls_handshake_io_recv_int()
		 */
		ret = GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET;
		goto cleanup;
	}

	ret2 = call_hook_func(session, hsk.htype, GNUTLS_HOOK_POST, 1, hsk.data.data, hsk.data.length);
	if (ret2 < 0) {
		ret = ret2;
		gnutls_assert();
		goto cleanup;
	}

	if (buf) {
		*buf = hsk.data;
		return ret;
	}

      cleanup:
	_gnutls_handshake_buffer_clear(&hsk);
	return ret;
}

/* This function checks if the given cipher suite is supported, and sets it
 * to the session;
 */
static int
set_client_ciphersuite(gnutls_session_t session, uint8_t suite[2])
{
	uint8_t z;
	uint8_t cipher_suites[MAX_CIPHERSUITE_SIZE];
	int cipher_suite_size;
	int i;

	z = 1;
	cipher_suite_size =
	    _gnutls_supported_ciphersuites(session, cipher_suites,
					   sizeof(cipher_suites));
	if (cipher_suite_size < 0) {
		gnutls_assert();
		return cipher_suite_size;
	}

	for (i = 0; i < cipher_suite_size; i += 2) {
		if (memcmp(&cipher_suites[i], suite, 2) == 0) {
			z = 0;
			break;
		}
	}

	if (z != 0) {
		gnutls_assert();
		_gnutls_handshake_log
		    ("HSK[%p]: unsupported cipher suite %.2X.%.2X\n",
		     session, (unsigned int) suite[0],
		     (unsigned int) suite[1]);
		return GNUTLS_E_UNKNOWN_CIPHER_SUITE;
	}

	memcpy(session->security_parameters.cipher_suite, suite, 2);
	_gnutls_epoch_set_cipher_suite(session, EPOCH_NEXT,
				       session->security_parameters.
				       cipher_suite);

	_gnutls_handshake_log("HSK[%p]: Selected cipher suite: %s\n",
			      session,
			      _gnutls_cipher_suite_get_name
			      (session->security_parameters.cipher_suite));


	/* check if the credentials (username, public key etc.) are ok.
	 * Actually checks if they exist.
	 */
	if (!session->internals.premaster_set &&
	    _gnutls_get_kx_cred
	    (session,
	     _gnutls_cipher_suite_get_kx_algo
	     (session->security_parameters.cipher_suite)) == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}


	/* set the mod_auth_st to the appropriate struct
	 * according to the KX algorithm. This is needed since all the
	 * handshake functions are read from there;
	 */
	session->internals.auth_struct =
	    _gnutls_kx_auth_struct(_gnutls_cipher_suite_get_kx_algo
				   (session->security_parameters.
				    cipher_suite));

	if (session->internals.auth_struct == NULL) {

		_gnutls_handshake_log
		    ("HSK[%p]: Cannot find the appropriate handler for the KX algorithm\n",
		     session);
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}


	return 0;
}

/* This function sets the given comp method to the session.
 */
static int
set_client_comp_method(gnutls_session_t session,
			       uint8_t comp_method)
{
	int comp_methods_num;
	uint8_t compression_methods[MAX_ALGOS];
	int id = _gnutls_compression_get_id(comp_method);
	int i;

	_gnutls_handshake_log
	    ("HSK[%p]: Selected compression method: %s (%d)\n", session,
	     gnutls_compression_get_name(id), (int) comp_method);

	comp_methods_num = _gnutls_supported_compression_methods(session,
								 compression_methods,
								 MAX_ALGOS);
	if (comp_methods_num < 0) {
		gnutls_assert();
		return comp_methods_num;
	}

	for (i = 0; i < comp_methods_num; i++) {
		if (compression_methods[i] == comp_method) {
			comp_methods_num = 0;
			break;
		}
	}

	if (comp_methods_num != 0) {
		gnutls_assert();
		return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
	}

	session->security_parameters.compression_method = id;
	_gnutls_epoch_set_compression(session, EPOCH_NEXT, id);

	return 0;
}

/* This function returns 0 if we are resuming a session or -1 otherwise.
 * This also sets the variables in the session. Used only while reading a server
 * hello.
 */
static int
client_check_if_resuming(gnutls_session_t session,
				 uint8_t * session_id, int session_id_len)
{
	char buf[2 * GNUTLS_MAX_SESSION_ID_SIZE + 1];

	_gnutls_handshake_log("HSK[%p]: SessionID length: %d\n", session,
			      session_id_len);
	_gnutls_handshake_log("HSK[%p]: SessionID: %s\n", session,
			      _gnutls_bin2hex(session_id, session_id_len,
					      buf, sizeof(buf), NULL));

	if ((session->internals.resumption_requested != 0 ||
	     session->internals.premaster_set != 0) &&
	    session_id_len > 0 &&
	    session->internals.resumed_security_parameters.
	    session_id_size == session_id_len
	    && memcmp(session_id,
		      session->internals.resumed_security_parameters.
		      session_id, session_id_len) == 0) {
		/* resume session */
		memcpy(session->internals.resumed_security_parameters.
		       server_random,
		       session->security_parameters.server_random,
		       GNUTLS_RANDOM_SIZE);
		memcpy(session->internals.resumed_security_parameters.
		       client_random,
		       session->security_parameters.client_random,
		       GNUTLS_RANDOM_SIZE);

		memcpy(session->security_parameters.cipher_suite,
			session->internals.resumed_security_parameters.cipher_suite, 2);
		session->security_parameters.compression_method =
			session->internals.resumed_security_parameters.compression_method;

		_gnutls_epoch_set_cipher_suite
		    (session, EPOCH_NEXT,
		     session->internals.resumed_security_parameters.
		     cipher_suite);
		_gnutls_epoch_set_compression(session, EPOCH_NEXT,
					      session->internals.
					      resumed_security_parameters.
					      compression_method);

		session->internals.resumed = RESUME_TRUE;	/* we are resuming */

		return 0;
	} else {
		/* keep the new session id */
		session->internals.resumed = RESUME_FALSE;	/* we are not resuming */
		session->security_parameters.session_id_size =
		    session_id_len;
		if (session_id_len > 0) {
			memcpy(session->security_parameters.session_id, session_id,
			       session_id_len);
		}

		return -1;
	}
}


/* This function reads and parses the server hello handshake message.
 * This function also restores resumed parameters if we are resuming a
 * session.
 */
static int
read_server_hello(gnutls_session_t session,
			  uint8_t * data, int datalen)
{
	uint8_t session_id_len = 0;
	int pos = 0;
	int ret = 0;
	gnutls_protocol_t version;
	int len = datalen;

	if (datalen < 38) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	_gnutls_handshake_log("HSK[%p]: Server's version: %d.%d\n",
			      session, data[pos], data[pos + 1]);

	DECR_LEN(len, 2);
	version = _gnutls_version_get(data[pos], data[pos + 1]);
	if (_gnutls_version_is_supported(session, version) == 0) {
		gnutls_assert();
		return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
	}

	if (_gnutls_set_current_version(session, version) < 0)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	pos += 2;

	DECR_LEN(len, GNUTLS_RANDOM_SIZE);
	ret = _gnutls_set_server_random(session, &data[pos]);
	if (ret < 0)
		return gnutls_assert_val(ret);

	pos += GNUTLS_RANDOM_SIZE;


	/* Read session ID
	 */
	DECR_LEN(len, 1);
	session_id_len = data[pos++];

	if (len < session_id_len || session_id_len > GNUTLS_MAX_SESSION_ID_SIZE) {
		gnutls_assert();
		return GNUTLS_E_ILLEGAL_PARAMETER;
	}
	DECR_LEN(len, session_id_len);

	/* check if we are resuming and set the appropriate
	 * values;
	 */
	if (client_check_if_resuming
	    (session, &data[pos], session_id_len) == 0) {
		pos += session_id_len + 2 + 1;
		DECR_LEN(len, 2 + 1);

		ret =
		    _gnutls_parse_extensions(session, GNUTLS_EXT_MANDATORY,
					     &data[pos], len);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
		return 0;
	}

	pos += session_id_len;

	/* Check if the given cipher suite is supported and copy
	 * it to the session.
	 */

	DECR_LEN(len, 2);
	ret = set_client_ciphersuite(session, &data[pos]);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}
	pos += 2;

	/* move to compression 
	 */
	DECR_LEN(len, 1);

	ret = set_client_comp_method(session, data[pos++]);
	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
	}

	/* Parse extensions.
	 */
	ret =
	    _gnutls_parse_extensions(session, GNUTLS_EXT_ANY, &data[pos],
				     len);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return ret;
}

#define RESERVED_CIPHERSUITES 4
/* This function copies the appropriate ciphersuites to a locally allocated buffer
 * Needed in client hello messages. Returns the new data length. If add_scsv is
 * true, add the special safe renegotiation CS.
 */
static int
copy_ciphersuites(gnutls_session_t session,
		  gnutls_buffer_st * cdata, int add_scsv)
{
	int ret;
	uint8_t cipher_suites[MAX_CIPHERSUITE_SIZE + RESERVED_CIPHERSUITES]; /* allow space for SCSV */
	int cipher_suites_size;
	size_t init_length = cdata->length;

	ret =
	    _gnutls_supported_ciphersuites(session, cipher_suites,
					   sizeof(cipher_suites) - RESERVED_CIPHERSUITES);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Here we remove any ciphersuite that does not conform
	 * the certificate requested, or to the
	 * authentication requested (eg SRP).
	 */
	ret =
	    _gnutls_remove_unwanted_ciphersuites(session, cipher_suites,
						 ret, NULL, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* If no cipher suites were enabled.
	 */
	if (ret == 0)
		return
		    gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);

	cipher_suites_size = ret;
#ifdef ENABLE_SSL3
	if (add_scsv) {
		cipher_suites[cipher_suites_size] = 0x00;
		cipher_suites[cipher_suites_size + 1] = 0xff;
		cipher_suites_size += 2;

		ret = _gnutls_ext_sr_send_cs(session);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}
#endif

	if (session->internals.priorities.fallback) {
		cipher_suites[cipher_suites_size] =
			GNUTLS_FALLBACK_SCSV_MAJOR;
		cipher_suites[cipher_suites_size + 1] =
			GNUTLS_FALLBACK_SCSV_MINOR;
		cipher_suites_size += 2;
	}

	ret =
	    _gnutls_buffer_append_data_prefix(cdata, 16, cipher_suites,
					      cipher_suites_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = cdata->length - init_length;

	return ret;
}


/* This function copies the appropriate compression methods, to a locally allocated buffer 
 * Needed in hello messages. Returns the new data length.
 */
static int
copy_comp_methods(gnutls_session_t session,
			  gnutls_buffer_st * cdata)
{
	int ret;
	uint8_t compression_methods[MAX_ALGOS], comp_num;
	size_t init_length = cdata->length;

	ret =
	    _gnutls_supported_compression_methods(session,
						  compression_methods,
						  MAX_ALGOS);
	if (ret < 0)
		return gnutls_assert_val(ret);

	comp_num = ret;

	/* put the number of compression methods */
	ret = _gnutls_buffer_append_prefix(cdata, 8, comp_num);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_buffer_append_data(cdata, compression_methods,
				       comp_num);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = cdata->length - init_length;

	return ret;
}

/* This function sends the client hello handshake message.
 */
static int send_client_hello(gnutls_session_t session, int again)
{
	mbuffer_st *bufel = NULL;
	int type;
	int ret = 0;
	const version_entry_st *hver;
	uint8_t tver[2];
	gnutls_buffer_st extdata;
	int rehandshake = 0;
	uint8_t session_id_len =
	    session->internals.resumed_security_parameters.session_id_size;

	_gnutls_buffer_init(&extdata);

	/* note that rehandshake is different than resuming
	 */
	if (session->security_parameters.session_id_size)
		rehandshake = 1;

	if (again == 0) {
		/* if we are resuming a session then we set the
		 * version number to the previously established.
		 */
		if (session->internals.resumption_requested == 0 &&
		    session->internals.premaster_set == 0) {
			if (rehandshake)	/* already negotiated version thus version_max == negotiated version */
				hver = get_version(session);
			else	/* new handshake. just get the max */
				hver =
				    version_to_entry(_gnutls_version_max
						     (session));
		} else {
			/* we are resuming a session */
			hver =
			    session->internals.resumed_security_parameters.
			    pversion;
		}

		if (hver == NULL) {
			gnutls_assert();
			return GNUTLS_E_NO_PRIORITIES_WERE_SET;
		}

		if (unlikely(session->internals.default_hello_version[0] != 0)) {
			tver[0] = session->internals.default_hello_version[0];
			tver[1] = session->internals.default_hello_version[1];
		} else {
			tver[0] = hver->major;
			tver[1] = hver->minor;
		}
		ret = _gnutls_buffer_append_data(&extdata, tver, 2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		_gnutls_handshake_log("HSK[%p]: Adv. version: %u.%u\n", session,
				      (unsigned)tver[0], (unsigned)tver[1]);

		/* Set the version we advertized as maximum 
		 * (RSA uses it).
		 */
		set_adv_version(session, hver->major, hver->minor);
		if (_gnutls_set_current_version(session, hver->id) < 0)
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

		if (session->internals.priorities.min_record_version != 0) {
			/* Advertize the lowest supported (SSL 3.0) record packet
			 * version in record packets during the handshake.
			 * That is to avoid confusing implementations
			 * that do not support TLS 1.2 and don't know
			 * how 3,3 version of record packets look like.
			 */
			const version_entry_st *v = _gnutls_version_lowest(session);

			if (v == NULL) {
				gnutls_assert();
				return GNUTLS_E_NO_PRIORITIES_WERE_SET;
			} else {
				_gnutls_record_set_default_version(session,
								   v->major, v->minor);
			}
		}

		/* In order to know when this session was initiated.
		 */
		session->security_parameters.timestamp = gnutls_time(NULL);

		/* Generate random data 
		 */
		if (!IS_DTLS(session)
		    || session->internals.dtls.hsk_hello_verify_requests ==
		    0) {
			ret = _gnutls_set_client_random(session, NULL);
			if (ret < 0)
				return gnutls_assert_val(ret);

		}

		ret = _gnutls_buffer_append_data(&extdata,
					session->security_parameters.client_random,
					GNUTLS_RANDOM_SIZE);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Copy the Session ID 
		 */
		ret = _gnutls_buffer_append_data_prefix(&extdata, 8, 
						session->internals.resumed_security_parameters.session_id,
						session_id_len);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Copy the DTLS cookie
		 */
		if (IS_DTLS(session)) {
			ret = _gnutls_buffer_append_data_prefix(&extdata, 8, session->internals.dtls.cookie,
				session->internals.dtls.cookie_len);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}

		/* Copy the ciphersuites.
		 */
#ifdef ENABLE_SSL3
		/* If using SSLv3 Send TLS_RENEGO_PROTECTION_REQUEST SCSV for MITM
		 * prevention on initial negotiation (but not renegotiation; that's
		 * handled with the RI extension below).
		 */
		if (!session->internals.initial_negotiation_completed &&
		    session->security_parameters.entity == GNUTLS_CLIENT &&
		    (hver->id == GNUTLS_SSL3 &&
		     session->internals.priorities.no_extensions != 0)) {
			ret =
			    copy_ciphersuites(session, &extdata,
					      TRUE);
			if (session->security_parameters.entity == GNUTLS_CLIENT)
				_gnutls_extension_list_add(session,
						   GNUTLS_EXTENSION_SAFE_RENEGOTIATION);
		} else
#endif
			ret =
			    copy_ciphersuites(session, &extdata,
					      FALSE);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Copy the compression methods.
		 */
		ret = copy_comp_methods(session, &extdata);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Generate and copy TLS extensions.
		 */
		if (session->internals.priorities.no_extensions == 0) {
			if (_gnutls_version_has_extensions(hver)) {
				type = GNUTLS_EXT_ANY;
			} else {
				type = GNUTLS_EXT_MANDATORY;
			}

			ret =
			    _gnutls_gen_extensions(session, &extdata,
						   type);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}

		bufel =
		    _gnutls_handshake_alloc(session, extdata.length);
		if (bufel == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto cleanup; 
		}
		_mbuffer_set_udata_size(bufel, 0);

		ret =
		    _mbuffer_append_data(bufel, extdata.data,
					 extdata.length);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	_gnutls_buffer_clear(&extdata);

	return
	    _gnutls_send_handshake(session, bufel,
				   GNUTLS_HANDSHAKE_CLIENT_HELLO);

      cleanup:
	_mbuffer_xfree(&bufel);
	_gnutls_buffer_clear(&extdata);
	return ret;
}

static int send_server_hello(gnutls_session_t session, int again)
{
	mbuffer_st *bufel = NULL;
	uint8_t *data = NULL;
	gnutls_buffer_st extdata;
	int pos = 0;
	int datalen, ret = 0;
	uint8_t comp;
	uint8_t session_id_len =
	    session->security_parameters.session_id_size;
	char buf[2 * GNUTLS_MAX_SESSION_ID_SIZE + 1];
	const version_entry_st *vers;

	_gnutls_buffer_init(&extdata);

	if (again == 0) {
		datalen = 2 + session_id_len + 1 + GNUTLS_RANDOM_SIZE + 3;
		ret =
		    _gnutls_gen_extensions(session, &extdata,
					   (session->internals.resumed ==
					    RESUME_TRUE) ?
					   GNUTLS_EXT_MANDATORY :
					   GNUTLS_EXT_ANY);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		bufel =
		    _gnutls_handshake_alloc(session,
					    datalen + extdata.length);
		if (bufel == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto fail;
		}
		data = _mbuffer_get_udata_ptr(bufel);

		vers = get_version(session);
		if (unlikely(vers == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		data[pos++] = vers->major;
		data[pos++] = vers->minor;

		memcpy(&data[pos],
		       session->security_parameters.server_random,
		       GNUTLS_RANDOM_SIZE);
		pos += GNUTLS_RANDOM_SIZE;

		data[pos++] = session_id_len;
		if (session_id_len > 0) {
			memcpy(&data[pos],
			       session->security_parameters.session_id,
			       session_id_len);
		}
		pos += session_id_len;

		_gnutls_handshake_log("HSK[%p]: SessionID: %s\n", session,
				      _gnutls_bin2hex(session->
						      security_parameters.session_id,
						      session_id_len, buf,
						      sizeof(buf), NULL));

		memcpy(&data[pos],
		       session->security_parameters.cipher_suite, 2);
		pos += 2;

		comp =
		    _gnutls_compression_get_num(session->
						security_parameters.
						compression_method);
		data[pos++] = comp;

		if (extdata.length > 0) {
			memcpy(&data[pos], extdata.data, extdata.length);
		}
	}

	ret =
	    _gnutls_send_handshake(session, bufel,
				   GNUTLS_HANDSHAKE_SERVER_HELLO);

      fail:
	_gnutls_buffer_clear(&extdata);
	return ret;
}

static int send_hello(gnutls_session_t session, int again)
{
	int ret;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		ret = send_client_hello(session, again);

	} else {		/* SERVER */
		ret = send_server_hello(session, again);
	}

	return ret;
}

/* RECEIVE A HELLO MESSAGE. This should be called from gnutls_recv_handshake_int only if a
 * hello message is expected. It uses the security_parameters.cipher_suite
 * and internals.compression_method.
 */
static int
recv_hello(gnutls_session_t session, uint8_t * data, int datalen)
{
	int ret;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		ret = read_server_hello(session, data, datalen);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	} else {		/* Server side reading a client hello */

		ret = read_client_hello(session, data, datalen);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	return 0;
}

static int
recv_hello_verify_request(gnutls_session_t session,
				  uint8_t * data, int datalen)
{
	ssize_t len = datalen;
	size_t pos = 0;
	uint8_t cookie_len;
	unsigned int nb_verifs;

	if (!IS_DTLS(session)
	    || session->security_parameters.entity == GNUTLS_SERVER) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	nb_verifs = ++session->internals.dtls.hsk_hello_verify_requests;
	if (nb_verifs >= MAX_HANDSHAKE_HELLO_VERIFY_REQUESTS) {
		/* The server is either buggy, malicious or changing cookie
		   secrets _way_ too fast. */
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET;
	}

	/* TODO: determine if we need to do anything with the server version field */
	DECR_LEN(len, 2);
	pos += 2;

	DECR_LEN(len, 1);
	cookie_len = data[pos];
	pos++;

	if (cookie_len > DTLS_MAX_COOKIE_SIZE) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	DECR_LEN(len, cookie_len);

	session->internals.dtls.cookie_len = cookie_len;
	memcpy(session->internals.dtls.cookie, &data[pos], cookie_len);

	if (len != 0) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	/* reset handshake hash buffers */
	handshake_hash_buffer_empty(session);

	return 0;
}

/* The packets in gnutls_handshake (it's more broad than original TLS handshake)
 *
 *     Client					       Server
 *
 *     ClientHello		  -------->
 *				  <--------	 ServerHello
 *
 *						    Certificate*
 *					      ServerKeyExchange*
 *				  <--------   CertificateRequest*
 *
 *				  <--------      ServerHelloDone
 *     Certificate*
 *     ClientKeyExchange
 *     CertificateVerify*
 *     [ChangeCipherSpec]
 *     Finished		     -------->
 *						NewSessionTicket
 *					      [ChangeCipherSpec]
 *				  <--------	     Finished
 *
 * (*): means optional packet.
 */

/* Handshake when resumming session:
 *      Client						Server
 *
 *      ClientHello		   -------->
 *						      ServerHello
 *					       [ChangeCipherSpec]
 *				   <--------	     Finished
 *     [ChangeCipherSpec]
 *     Finished		      -------->
 * 
 */

/**
 * gnutls_rehandshake:
 * @session: is a #gnutls_session_t type.
 *
 * This function will renegotiate security parameters with the
 * client.  This should only be called in case of a server.
 *
 * This message informs the peer that we want to renegotiate
 * parameters (perform a handshake).
 *
 * If this function succeeds (returns 0), you must call the
 * gnutls_handshake() function in order to negotiate the new
 * parameters.
 *
 * Since TLS is full duplex some application data might have been
 * sent during peer's processing of this message. In that case
 * one should call gnutls_record_recv() until GNUTLS_E_REHANDSHAKE
 * is returned to clear any pending data. Care must be taken, if
 * rehandshake is mandatory, to terminate if it does not start after
 * some threshold.
 *
 * If the client does not wish to renegotiate parameters he 
 * should reply with an alert message, thus the return code will be
 * %GNUTLS_E_WARNING_ALERT_RECEIVED and the alert will be
 * %GNUTLS_A_NO_RENEGOTIATION.  A client may also choose to ignore
 * this message.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 **/
int gnutls_rehandshake(gnutls_session_t session)
{
	int ret;

	/* only server sends that handshake packet */
	if (session->security_parameters.entity == GNUTLS_CLIENT)
		return GNUTLS_E_INVALID_REQUEST;

	_dtls_async_timer_delete(session);

	ret =
	    _gnutls_send_empty_handshake(session,
					 GNUTLS_HANDSHAKE_HELLO_REQUEST,
					 AGAIN(STATE50));
	STATE = STATE50;

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}
	STATE = STATE0;

	return 0;
}

inline static int
_gnutls_abort_handshake(gnutls_session_t session, int ret)
{
	if (((ret == GNUTLS_E_WARNING_ALERT_RECEIVED) &&
	     (gnutls_alert_get(session) == GNUTLS_A_NO_RENEGOTIATION))
	    || ret == GNUTLS_E_GOT_APPLICATION_DATA)
		return 0;

	/* this doesn't matter */
	return GNUTLS_E_INTERNAL_ERROR;
}



static int _gnutls_send_supplemental(gnutls_session_t session, int again)
{
	mbuffer_st *bufel;
	int ret = 0;

	_gnutls_debug_log("EXT[%p]: Sending supplemental data\n", session);

	if (again)
		ret =
		    _gnutls_send_handshake(session, NULL,
					   GNUTLS_HANDSHAKE_SUPPLEMENTAL);
	else {
		gnutls_buffer_st buf;
		_gnutls_buffer_init(&buf);

		ret = _gnutls_gen_supplemental(session, &buf);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		bufel =
		    _gnutls_handshake_alloc(session, 
					    buf.length);
		if (bufel == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		_mbuffer_set_udata(bufel, buf.data, buf.length);
		_gnutls_buffer_clear(&buf);

		ret = _gnutls_send_handshake(session, bufel,
					     GNUTLS_HANDSHAKE_SUPPLEMENTAL);
	}

	return ret;
}

static int _gnutls_recv_supplemental(gnutls_session_t session)
{
	gnutls_buffer_st buf;
	int ret;

	_gnutls_debug_log("EXT[%p]: Expecting supplemental data\n",
			  session);

	ret =
	    _gnutls_recv_handshake(session, GNUTLS_HANDSHAKE_SUPPLEMENTAL,
				   1, &buf);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_parse_supplemental(session, buf.data, buf.length);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

      cleanup:
	_gnutls_buffer_clear(&buf);

	return ret;
}

/**
 * gnutls_handshake:
 * @session: is a #gnutls_session_t type.
 *
 * This function does the handshake of the TLS/SSL protocol, and
 * initializes the TLS connection.
 *
 * This function will fail if any problem is encountered, and will
 * return a negative error code. In case of a client, if the client
 * has asked to resume a session, but the server couldn't, then a
 * full handshake will be performed.
 *
 * The non-fatal errors expected by this function are:
 * %GNUTLS_E_INTERRUPTED, %GNUTLS_E_AGAIN, 
 * %GNUTLS_E_WARNING_ALERT_RECEIVED, and %GNUTLS_E_GOT_APPLICATION_DATA,
 * the latter only in a case of rehandshake.
 *
 * The former two interrupt the handshake procedure due to the lower
 * layer being interrupted, and the latter because of an alert that
 * may be sent by a server (it is always a good idea to check any
 * received alerts). On these errors call this function again, until it
 * returns 0; cf.  gnutls_record_get_direction() and
 * gnutls_error_is_fatal(). In DTLS sessions the non-fatal error
 * %GNUTLS_E_LARGE_PACKET is also possible, and indicates that
 * the MTU should be adjusted.
 *
 * If this function is called by a server after a rehandshake request
 * then %GNUTLS_E_GOT_APPLICATION_DATA or
 * %GNUTLS_E_WARNING_ALERT_RECEIVED may be returned.  Note that these
 * are non fatal errors, only in the specific case of a rehandshake.
 * Their meaning is that the client rejected the rehandshake request or
 * in the case of %GNUTLS_E_GOT_APPLICATION_DATA it could also mean that
 * some data were pending. A client may receive that error code if
 * it initiates the handshake and the server doesn't agreed.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 **/
int gnutls_handshake(gnutls_session_t session)
{
	int ret;
	record_parameters_st *params;

	/* sanity check. Verify that there are priorities setup.
	 */

	if (STATE == STATE0) {
		/* first call */
		if (session->internals.priorities.protocol.algorithms == 0)
			return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);

		session->internals.extensions_sent_size = 0;
		session->internals.crt_requested = 0;
		session->internals.handshake_in_progress = 1;
		session->internals.vc_status = -1;
		gettime(&session->internals.handshake_start_time);
		if (session->internals.handshake_timeout_ms &&
		    session->internals.handshake_endtime == 0)
			    session->internals.handshake_endtime = session->internals.handshake_start_time.tv_sec +
			    session->internals.handshake_timeout_ms / 1000;
	}

	if (session->internals.recv_state == RECV_STATE_FALSE_START) {
		session_invalidate(session);
		return gnutls_assert_val(GNUTLS_E_HANDSHAKE_DURING_FALSE_START);
	}

	ret =
	    _gnutls_epoch_get(session,
			      session->security_parameters.epoch_next,
			      &params);
	if (ret < 0) {
		/* We assume the epoch is not allocated if _gnutls_epoch_get fails. */
		ret =
		    _gnutls_epoch_alloc(session,
					session->security_parameters.
					epoch_next, NULL);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		do {
			ret = handshake_client(session);
		} while (ret == 1);
	} else {
		ret = handshake_server(session);
	}
	if (ret < 0) {
		/* In the case of a rehandshake abort
		 * we should reset the handshake's internal state.
		 */
		if (_gnutls_abort_handshake(session, ret) == 0)
			STATE = STATE0;

		return ret;
	}

	/* clear handshake buffer */
	if (session->security_parameters.entity != GNUTLS_CLIENT ||
	    !(session->internals.flags & GNUTLS_ENABLE_FALSE_START) ||
	    session->internals.recv_state != RECV_STATE_FALSE_START) {

		_gnutls_handshake_hash_buffers_clear(session);

		if (IS_DTLS(session) == 0) {
			_gnutls_handshake_io_buffer_clear(session);
		} else {
			_dtls_async_timer_init(session);
		}

		_gnutls_handshake_internal_state_clear(session);

		session->security_parameters.epoch_next++;
	}

	return 0;
}

/**
 * gnutls_handshake_set_timeout:
 * @session: is a #gnutls_session_t type.
 * @ms: is a timeout value in milliseconds
 *
 * This function sets the timeout for the TLS handshake process
 * to the provided value. Use an @ms value of zero to disable
 * timeout, or %GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT for a reasonable
 * default value. For the DTLS protocol, the more detailed
 * gnutls_dtls_set_timeouts() is provided.
 *
 * This function requires to set a pull timeout callback. See
 * gnutls_transport_set_pull_timeout_function().
 *
 * Since: 3.1.0
 **/
void
gnutls_handshake_set_timeout(gnutls_session_t session, unsigned int ms)
{
	if (ms == GNUTLS_INDEFINITE_TIMEOUT) {
		session->internals.handshake_timeout_ms = 0;
		return;
	}

	if (ms == GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT)
		ms = DEFAULT_HANDSHAKE_TIMEOUT_MS;

	if (IS_DTLS(session)) {
		gnutls_dtls_set_timeouts(session, DTLS_RETRANS_TIMEOUT, ms);
		return;
	}

	session->internals.handshake_timeout_ms = ms;
}


#define IMED_RET( str, ret, allow_alert) do { \
	if (ret < 0) { \
		/* EAGAIN and INTERRUPTED are always non-fatal */ \
		if (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED) \
			return ret; \
		if (ret == GNUTLS_E_GOT_APPLICATION_DATA && session->internals.initial_negotiation_completed != 0) \
			return ret; \
		if (session->internals.handshake_suspicious_loops < 16) { \
			if (ret == GNUTLS_E_LARGE_PACKET) { \
				session->internals.handshake_suspicious_loops++; \
				return ret; \
			} \
			/* a warning alert might interrupt handshake */ \
			if (allow_alert != 0 && ret==GNUTLS_E_WARNING_ALERT_RECEIVED) { \
				session->internals.handshake_suspicious_loops++; \
				return ret; \
			} \
		} \
		gnutls_assert(); \
		ERR( str, ret); \
		/* do not allow non-fatal errors at this point */ \
		if (gnutls_error_is_fatal(ret) == 0) ret = gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR); \
		session_invalidate(session); \
		_gnutls_handshake_hash_buffers_clear(session); \
		return ret; \
	} } while (0)


/* Runs the certificate verification callback.
 * side is either GNUTLS_CLIENT or GNUTLS_SERVER.
 */
static int run_verify_callback(gnutls_session_t session, unsigned int side)
{
	gnutls_certificate_credentials_t cred;
	int ret, type;

	cred =
	    (gnutls_certificate_credentials_t) _gnutls_get_cred(session,
								GNUTLS_CRD_CERTIFICATE);

	if (side == GNUTLS_CLIENT)
		type = gnutls_auth_server_get_type(session);
	else
		type = gnutls_auth_client_get_type(session);

	if (type != GNUTLS_CRD_CERTIFICATE)
		return 0;

	/* verify whether the certificate of the peer remained the same
	 * as with any previous handshakes */
	if (cred != NULL) {
		ret = _gnutls_check_if_cert_hash_is_same(session, cred);
		if (ret < 0) {
			return gnutls_assert_val(ret);
		}
	}

	if (cred != NULL &&
	    (cred->verify_callback != NULL || session->internals.verify_callback != NULL) &&
	    (session->security_parameters.entity == GNUTLS_CLIENT ||
	     session->internals.send_cert_req != GNUTLS_CERT_IGNORE)) {
		if (session->internals.verify_callback)
			ret = session->internals.verify_callback(session);
		else
			ret = cred->verify_callback(session);
		if (ret < -1)
			return gnutls_assert_val(ret);
		else if (ret != 0)
			return gnutls_assert_val(GNUTLS_E_CERTIFICATE_ERROR);
	}

	return 0;
}

static bool can_send_false_start(gnutls_session_t session)
{
	const version_entry_st *vers;

	vers = get_version(session);
	if (unlikely(vers == NULL || !vers->false_start))
		return 0;

	if (session->internals.selected_cert_list != NULL)
		return 0;

	if (!_gnutls_kx_allows_false_start(session))
		return 0;

	return 1;
}

/*
 * handshake_client 
 * This function performs the client side of the handshake of the TLS/SSL protocol.
 */
static int handshake_client(gnutls_session_t session)
{
	int ret = 0;

#ifdef HANDSHAKE_DEBUG
	char buf[64];

	if (session->internals.resumed_security_parameters.
	    session_id_size > 0)
		_gnutls_handshake_log("HSK[%p]: Ask to resume: %s\n",
				      session,
				      _gnutls_bin2hex(session->internals.
						      resumed_security_parameters.
						      session_id,
						      session->internals.
						      resumed_security_parameters.
						      session_id_size, buf,
						      sizeof(buf), NULL));
#endif
	switch (STATE) {
	case STATE0:
	case STATE1:
		ret = send_hello(session, AGAIN(STATE1));
		STATE = STATE1;
		IMED_RET("send hello", ret, 1);
		/* fall through */
	case STATE2:
		if (IS_DTLS(session)) {
			ret =
			    _gnutls_recv_handshake(session,
						   GNUTLS_HANDSHAKE_HELLO_VERIFY_REQUEST,
						   1, NULL);
			STATE = STATE2;
			IMED_RET("recv hello verify", ret, 1);

			if (ret == 1) {
				STATE = STATE0;
				return 1;
			}
		}
		/* fall through */
	case STATE3:
		/* receive the server hello */
		ret =
		    _gnutls_recv_handshake(session,
					   GNUTLS_HANDSHAKE_SERVER_HELLO,
					   0, NULL);
		STATE = STATE3;
		IMED_RET("recv hello", ret, 1);
		/* fall through */
	case STATE4:
		ret = _gnutls_ext_sr_verify(session);
		STATE = STATE4;
		IMED_RET("recv hello", ret, 0);
		/* fall through */
	case STATE5:
		if (session->security_parameters.do_recv_supplemental) {
			ret = _gnutls_recv_supplemental(session);
			STATE = STATE5;
			IMED_RET("recv supplemental", ret, 1);
		}
		/* fall through */
	case STATE6:
		/* RECV CERTIFICATE */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_server_certificate(session);
		STATE = STATE6;
		IMED_RET("recv server certificate", ret, 1);
		/* fall through */
	case STATE7:
#ifdef ENABLE_OCSP
		/* RECV CERTIFICATE STATUS */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_recv_server_certificate_status
			    (session);
		STATE = STATE7;
		IMED_RET("recv server certificate", ret, 1);
#endif
		/* fall through */
	case STATE8:
		ret = run_verify_callback(session, GNUTLS_CLIENT);
		STATE = STATE8;
		if (ret < 0)
			return gnutls_assert_val(ret);
	case STATE9:
		/* receive the server key exchange */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_server_kx_message(session);
		STATE = STATE9;
		IMED_RET("recv server kx message", ret, 1);
		/* fall through */
	case STATE10:
		/* receive the server certificate request - if any 
		 */

		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_server_crt_request(session);
		STATE = STATE10;
		IMED_RET("recv server certificate request message", ret,
			 1);
		/* fall through */
	case STATE11:
		/* receive the server hello done */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_recv_handshake(session,
						   GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
						   0, NULL);
		STATE = STATE11;
		IMED_RET("recv server hello done", ret, 1);
		/* fall through */
	case STATE12:
		if (session->security_parameters.do_send_supplemental) {
			ret =
			    _gnutls_send_supplemental(session,
						      AGAIN(STATE12));
			STATE = STATE12;
			IMED_RET("send supplemental", ret, 0);
		}
		/* fall through */
	case STATE13:
		/* send our certificate - if any and if requested
		 */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_send_client_certificate(session,
							    AGAIN
							    (STATE13));
		STATE = STATE13;
		IMED_RET("send client certificate", ret, 0);
		/* fall through */
	case STATE14:
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_send_client_kx_message(session,
							   AGAIN(STATE14));
		STATE = STATE14;
		IMED_RET("send client kx", ret, 0);
		/* fall through */
	case STATE15:
		/* send client certificate verify */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_send_client_certificate_verify(session,
								   AGAIN
								   (STATE15));
		STATE = STATE15;
		IMED_RET("send client certificate verify", ret, 1);
		/* fall through */
	case STATE16:
		STATE = STATE16;
		if (session->internals.resumed == RESUME_FALSE) {
			ret = send_handshake_final(session, TRUE);
			IMED_RET("send handshake final 2", ret, 1);
#ifdef ENABLE_SESSION_TICKETS
		} else {
			ret = _gnutls_recv_new_session_ticket(session);
			IMED_RET("recv handshake new session ticket", ret,
				 1);
#endif
		}
		/* fall through */
	case STATE17:
		STATE = STATE17;
		if (session->internals.resumed == RESUME_FALSE && (session->internals.flags & GNUTLS_ENABLE_FALSE_START) && can_send_false_start(session)) {
			session->internals.false_start_used = 1;
			session->internals.recv_state = RECV_STATE_FALSE_START;
			/* complete this phase of the handshake. We
			 * should be called again by gnutls_record_recv()
			 */
			STATE = STATE18;
			gnutls_assert();

			return 0;
		} else {
			session->internals.false_start_used = 0;
		}
		/* fall through */
	case STATE18:
		STATE = STATE18;

		if (session->internals.resumed == RESUME_FALSE) {
#ifdef ENABLE_SESSION_TICKETS
			ret = _gnutls_recv_new_session_ticket(session);
			IMED_RET("recv handshake new session ticket", ret,
				 1);
#endif
		} else {
			ret = recv_handshake_final(session, TRUE);
			IMED_RET("recv handshake final", ret, 1);
		}
		/* fall through */
	case STATE19:
		STATE = STATE19;
		if (session->internals.resumed == RESUME_FALSE) {
			ret = recv_handshake_final(session, FALSE);
			IMED_RET("recv handshake final 2", ret, 1);
		} else {
			ret = send_handshake_final(session, FALSE);
			IMED_RET("send handshake final", ret, 1);
		}

		STATE = STATE0;
		/* fall through */
	default:
		break;
	}

	/* explicitly reset any false start flags */
	session->internals.recv_state = RECV_STATE_0;

	return 0;
}



/* This function is to be called if the handshake was successfully 
 * completed. This sends a Change Cipher Spec packet to the peer.
 */
static ssize_t send_change_cipher_spec(gnutls_session_t session, int again)
{
	uint8_t *data;
	mbuffer_st *bufel;
	int ret;
	const version_entry_st *vers;

	if (again == 0) {
		bufel = _gnutls_handshake_alloc(session, 1);
		if (bufel == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		vers = get_version(session);
		if (unlikely(vers == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (vers->id == GNUTLS_DTLS0_9)
			_mbuffer_set_uhead_size(bufel, 3);
		else
			_mbuffer_set_uhead_size(bufel, 1);
		_mbuffer_set_udata_size(bufel, 0);

		data = _mbuffer_get_uhead_ptr(bufel);

		data[0] = 1;
		if (vers->id == GNUTLS_DTLS0_9) {
			_gnutls_write_uint16(session->internals.dtls.
					     hsk_write_seq, &data[1]);
			session->internals.dtls.hsk_write_seq++;
		}

		ret =
		    _gnutls_handshake_io_cache_int(session,
						   GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC,
						   bufel);
		if (ret < 0) {
			_mbuffer_xfree(&bufel);
			return gnutls_assert_val(ret);
		}

		_gnutls_handshake_log("REC[%p]: Sent ChangeCipherSpec\n",
				      session);
	}

	return 0;
}

/* This function sends the final handshake packets and initializes connection 
 */
static int send_handshake_final(gnutls_session_t session, int init)
{
	int ret = 0;

	/* Send the CHANGE CIPHER SPEC PACKET */

	switch (FINAL_STATE) {
	case STATE0:
	case STATE1:
		ret = send_change_cipher_spec(session, FAGAIN(STATE1));
		FINAL_STATE = STATE0;

		if (ret < 0) {
			ERR("send ChangeCipherSpec", ret);
			gnutls_assert();
			return ret;
		}
		/* Initialize the connection session (start encryption) - in case of client 
		 */
		if (init == TRUE) {
			ret = _gnutls_connection_state_init(session);
			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
		}

		ret = _gnutls_write_connection_state_init(session);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* fall through */
	case STATE2:
		/* send the finished message */
		ret = _gnutls_send_finished(session, FAGAIN(STATE2));
		FINAL_STATE = STATE2;
		if (ret < 0) {
			ERR("send Finished", ret);
			gnutls_assert();
			return ret;
		}

		FINAL_STATE = STATE0;
		/* fall through */
	default:
		break;
	}

	return 0;
}

/* This function receives the final handshake packets 
 * And executes the appropriate function to initialize the
 * read session.
 */
static int recv_handshake_final(gnutls_session_t session, int init)
{
	int ret = 0;
	uint8_t ccs[3];
	unsigned int ccs_len = 1;
	unsigned int tleft;
	const version_entry_st *vers;

	ret = handshake_remaining_time(session);
	if (ret < 0)
		return gnutls_assert_val(ret);
	tleft = ret;

	switch (FINAL_STATE) {
	case STATE0:
	case STATE30:
		FINAL_STATE = STATE30;

		/* This is the last flight and peer cannot be sure
		 * we have received it unless we notify him. So we
		 * wait for a message and retransmit if needed. */
		if (IS_DTLS(session) && !_dtls_is_async(session) &&
		    (gnutls_record_check_pending(session) +
		     record_check_unprocessed(session)) == 0) {
			ret = _dtls_wait_and_retransmit(session);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}

		vers = get_version(session);
		if (unlikely(vers == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (vers->id == GNUTLS_DTLS0_9)
			ccs_len = 3;

		ret =
		    _gnutls_recv_int(session, GNUTLS_CHANGE_CIPHER_SPEC,
				     ccs, ccs_len, NULL, tleft);
		if (ret <= 0) {
			ERR("recv ChangeCipherSpec", ret);
			gnutls_assert();
			return (ret<0)?ret:GNUTLS_E_UNEXPECTED_PACKET;
		}

		if (vers->id == GNUTLS_DTLS0_9)
			session->internals.dtls.hsk_read_seq++;

		/* Initialize the connection session (start encryption) - in case of server */
		if (init == TRUE) {
			ret = _gnutls_connection_state_init(session);
			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
		}

		ret = _gnutls_read_connection_state_init(session);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
		/* fall through */
	case STATE31:
		FINAL_STATE = STATE31;

		if (IS_DTLS(session) && !_dtls_is_async(session) &&
		    (gnutls_record_check_pending(session) +
		     record_check_unprocessed(session)) == 0) {
			ret = _dtls_wait_and_retransmit(session);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}

		ret = _gnutls_recv_finished(session);
		if (ret < 0) {
			ERR("recv finished", ret);
			gnutls_assert();
			return ret;
		}
		FINAL_STATE = STATE0;
		/* fall through */
	default:
		break;
	}


	return 0;
}

/*
 * handshake_server
 * This function does the server stuff of the handshake protocol.
 */
static int handshake_server(gnutls_session_t session)
{
	int ret = 0;

	switch (STATE) {
	case STATE0:
	case STATE1:
		ret =
		    _gnutls_recv_handshake(session,
					   GNUTLS_HANDSHAKE_CLIENT_HELLO,
					   0, NULL);
		if (ret == GNUTLS_E_INT_RET_0) {
			/* this is triggered by post_client_hello, and instructs the
			 * handshake to proceed but be put on hold */
			ret = GNUTLS_E_INTERRUPTED;
			STATE = STATE2; /* hello already parsed -> move on */
		} else {
			STATE = STATE1;
		}
		IMED_RET("recv hello", ret, 1);
		/* fall through */
	case STATE2:
		ret = _gnutls_ext_sr_verify(session);
		STATE = STATE2;
		IMED_RET("recv hello", ret, 0);
		/* fall through */
	case STATE3:
		ret = send_hello(session, AGAIN(STATE3));
		STATE = STATE3;
		IMED_RET("send hello", ret, 1);
		/* fall through */
	case STATE4:
		if (session->security_parameters.do_send_supplemental) {
			ret =
			    _gnutls_send_supplemental(session,
						      AGAIN(STATE4));
			STATE = STATE4;
			IMED_RET("send supplemental data", ret, 0);
		}
		/* SEND CERTIFICATE + KEYEXCHANGE + CERTIFICATE_REQUEST */
		/* fall through */
	case STATE5:
		/* NOTE: these should not be send if we are resuming */

		if (session->internals.resumed == RESUME_FALSE)
			ret =
			    _gnutls_send_server_certificate(session,
							    AGAIN(STATE5));
		STATE = STATE5;
		IMED_RET("send server certificate", ret, 0);
		/* fall through */
	case STATE6:
#ifdef ENABLE_OCSP
		if (session->internals.resumed == RESUME_FALSE)
			ret =
			    _gnutls_send_server_certificate_status(session,
								   AGAIN
								   (STATE6));
		STATE = STATE6;
		IMED_RET("send server certificate status", ret, 0);
#endif
		/* fall through */
	case STATE7:
		/* send server key exchange (A) */
		if (session->internals.resumed == RESUME_FALSE)
			ret =
			    _gnutls_send_server_kx_message(session,
							   AGAIN(STATE7));
		STATE = STATE7;
		IMED_RET("send server kx", ret, 0);
		/* fall through */
	case STATE8:
		/* Send certificate request - if requested to */
		if (session->internals.resumed == RESUME_FALSE)
			ret =
			    _gnutls_send_server_crt_request(session,
							    AGAIN(STATE8));
		STATE = STATE8;
		IMED_RET("send server cert request", ret, 0);
		/* fall through */
	case STATE9:
		/* send the server hello done */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_send_empty_handshake(session,
							 GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
							 AGAIN(STATE9));
		STATE = STATE9;
		IMED_RET("send server hello done", ret, 1);
		/* fall through */
	case STATE10:
		if (session->security_parameters.do_recv_supplemental) {
			ret = _gnutls_recv_supplemental(session);
			STATE = STATE10;
			IMED_RET("recv client supplemental", ret, 1);
		}
		/* RECV CERTIFICATE + KEYEXCHANGE + CERTIFICATE_VERIFY */
		/* fall through */
	case STATE11:
		/* receive the client certificate message */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_client_certificate(session);
		STATE = STATE11;
		IMED_RET("recv client certificate", ret, 1);
		/* fall through */
	case STATE12:
		ret = run_verify_callback(session, GNUTLS_SERVER);
		STATE = STATE12;
		if (ret < 0)
			return gnutls_assert_val(ret);
		/* fall through */
	case STATE13:
		/* receive the client key exchange message */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_client_kx_message(session);
		STATE = STATE13;
		IMED_RET("recv client kx", ret, 1);
		/* fall through */
	case STATE14:
		/* receive the client certificate verify message */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_recv_client_certificate_verify_message
			    (session);
		STATE = STATE14;
		IMED_RET("recv client certificate verify", ret, 1);
		/* fall through */
	case STATE15:
		STATE = STATE15;
		if (session->internals.resumed == RESUME_FALSE) {	/* if we are not resuming */
			ret = recv_handshake_final(session, TRUE);
			IMED_RET("recv handshake final", ret, 1);
		} else {
			ret = send_handshake_final(session, TRUE);
			IMED_RET("send handshake final 2", ret, 1);
		}
		/* fall through */
	case STATE16:
#ifdef ENABLE_SESSION_TICKETS
		ret =
		    _gnutls_send_new_session_ticket(session,
						    AGAIN(STATE16));
		STATE = STATE16;
		IMED_RET("send handshake new session ticket", ret, 0);
#endif
		/* fall through */
	case STATE17:
		STATE = STATE17;
		if (session->internals.resumed == RESUME_FALSE) {	/* if we are not resuming */
			ret = send_handshake_final(session, FALSE);
			IMED_RET("send handshake final", ret, 1);

			if (session->security_parameters.entity ==
			    GNUTLS_SERVER
			    && session->internals.ticket_sent == 0) {
				/* if no ticket, save session data */
				_gnutls_server_register_current_session
				    (session);
			}
		} else {
			ret = recv_handshake_final(session, FALSE);
			IMED_RET("recv handshake final 2", ret, 1);
		}

		STATE = STATE0;
		/* fall through */
	default:
		break;
	}

	return _gnutls_check_id_for_change(session);
}

int _gnutls_generate_session_id(uint8_t * session_id, uint8_t * len)
{
	int ret;

	*len = GNUTLS_MAX_SESSION_ID_SIZE;

	ret =
	    gnutls_rnd(GNUTLS_RND_NONCE, session_id,
			GNUTLS_MAX_SESSION_ID_SIZE);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

int
_gnutls_recv_hello_request(gnutls_session_t session, void *data,
			   uint32_t data_size)
{
	uint8_t type;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET;
	}
	if (data_size < 1) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}
	type = ((uint8_t *) data)[0];
	if (type == GNUTLS_HANDSHAKE_HELLO_REQUEST) {
		if (IS_DTLS(session))
			session->internals.dtls.hsk_read_seq++;
		return GNUTLS_E_REHANDSHAKE;
	} else {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET;
	}
}

/**
 * gnutls_handshake_set_max_packet_length:
 * @session: is a #gnutls_session_t type.
 * @max: is the maximum number.
 *
 * This function will set the maximum size of all handshake messages.
 * Handshakes over this size are rejected with
 * %GNUTLS_E_HANDSHAKE_TOO_LARGE error code.  The default value is
 * 128kb which is typically large enough.  Set this to 0 if you do not
 * want to set an upper limit.
 *
 * The reason for restricting the handshake message sizes are to
 * limit Denial of Service attacks.
 *
 * Note that the maximum handshake size was increased to 128kb
 * from 48kb in GnuTLS 3.5.5.
 **/
void
gnutls_handshake_set_max_packet_length(gnutls_session_t session,
				       size_t max)
{
	session->internals.max_handshake_data_buffer_size = max;
}

/**
 * gnutls_handshake_get_last_in:
 * @session: is a #gnutls_session_t type.
 *
 * This function is only useful to check where the last performed
 * handshake failed.  If the previous handshake succeed or was not
 * performed at all then no meaningful value will be returned.
 *
 * Check %gnutls_handshake_description_t in gnutls.h for the
 * available handshake descriptions.
 *
 * Returns: the last handshake message type received, a
 * %gnutls_handshake_description_t.
 **/
gnutls_handshake_description_t
gnutls_handshake_get_last_in(gnutls_session_t session)
{
	return session->internals.last_handshake_in;
}

/**
 * gnutls_handshake_get_last_out:
 * @session: is a #gnutls_session_t type.
 *
 * This function is only useful to check where the last performed
 * handshake failed.  If the previous handshake succeed or was not
 * performed at all then no meaningful value will be returned.
 *
 * Check %gnutls_handshake_description_t in gnutls.h for the
 * available handshake descriptions.
 *
 * Returns: the last handshake message type sent, a
 * %gnutls_handshake_description_t.
 **/
gnutls_handshake_description_t
gnutls_handshake_get_last_out(gnutls_session_t session)
{
	return session->internals.last_handshake_out;
}

/* This returns the session hash as in draft-ietf-tls-session-hash-02.
 *
 * FIXME: It duplicates some of the actions in _gnutls_handshake_sign_crt_vrfy*.
 * See whether they can be merged.
 */
int _gnutls_handshake_get_session_hash(gnutls_session_t session, gnutls_datum_t *shash)
{
	const version_entry_st *ver = get_version(session);
	int ret;
	const mac_entry_st *me;
	uint8_t concat[2*MAX_HASH_SIZE];
	digest_hd_st td_md5;
	digest_hd_st td_sha;

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (session->internals.handshake_hash_buffer_client_kx_len == 0 ||
	    (session->internals.handshake_hash_buffer.length <
	    session->internals.handshake_hash_buffer_client_kx_len)) {
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	if (_gnutls_version_has_selectable_prf(ver)) { /* TLS 1.2+ */
		gnutls_mac_algorithm_t prf;

		prf = _gnutls_cipher_suite_get_prf(session->security_parameters.cipher_suite);
		if (prf == GNUTLS_MAC_UNKNOWN)
			return gnutls_assert_val(GNUTLS_E_UNKNOWN_PK_ALGORITHM);

		me = mac_to_entry(prf);

		ret =
		    _gnutls_hash_fast((gnutls_digest_algorithm_t)me->id,
				      session->internals.handshake_hash_buffer.
				      data,
				      session->internals.handshake_hash_buffer_client_kx_len,
				      concat);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return _gnutls_set_datum(shash, concat, me->output_size);
	} else {
		ret = _gnutls_hash_init(&td_sha, hash_to_entry(GNUTLS_DIG_SHA1));
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		_gnutls_hash(&td_sha,
			     session->internals.handshake_hash_buffer.data,
			     session->internals.handshake_hash_buffer_client_kx_len);

		_gnutls_hash_deinit(&td_sha, &concat[16]);

		ret =
		    _gnutls_hash_init(&td_md5,
				      hash_to_entry(GNUTLS_DIG_MD5));
		if (ret < 0)
			return gnutls_assert_val(ret);

		_gnutls_hash(&td_md5,
			     session->internals.handshake_hash_buffer.data,
			     session->internals.handshake_hash_buffer_client_kx_len);

		_gnutls_hash_deinit(&td_md5, concat);

		return _gnutls_set_datum(shash, concat, 36);
	}
}
