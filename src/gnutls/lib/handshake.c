/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2018 Red Hat, Inc.
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

/* Functions that relate to the TLS handshake procedure.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "dh.h"
#include "debug.h"
#include "algorithms.h"
#include "cipher.h"
#include "buffers.h"
#include "mbuffers.h"
#include "kx.h"
#include "handshake.h"
#include "num.h"
#include "hash_int.h"
#include "db.h"
#include "hello_ext.h"
#include "supplemental.h"
#include "auth.h"
#include "sslv2_compat.h"
#include <auth/cert.h>
#include "constate.h"
#include <record.h>
#include <state.h>
#include <ext/pre_shared_key.h>
#include <ext/srp.h>
#include <ext/session_ticket.h>
#include <ext/status_request.h>
#include <ext/safe_renegotiation.h>
#include <auth/anon.h>		/* for gnutls_anon_server_credentials_t */
#include <auth/psk.h>		/* for gnutls_psk_server_credentials_t */
#include <random.h>
#include <dtls.h>
#include "secrets.h"
#include "tls13/session_ticket.h"
#include "locks.h"

#define TRUE 1
#define FALSE 0

static int check_if_null_comp_present(gnutls_session_t session,
					     uint8_t * data, int datalen);
static int handshake_client(gnutls_session_t session);
static int handshake_server(gnutls_session_t session);

static int
read_server_hello(gnutls_session_t session,
		  uint8_t * data, int datalen);

static int
recv_handshake_final(gnutls_session_t session, int init);
static int
send_handshake_final(gnutls_session_t session, int init);

/* Empties but does not free the buffer
 */
inline static void
handshake_hash_buffer_reset(gnutls_session_t session)
{
	_gnutls_buffers_log("BUF[HSK]: Emptied buffer\n");

	session->internals.handshake_hash_buffer_client_hello_len = 0;
	session->internals.handshake_hash_buffer_client_kx_len = 0;
	session->internals.handshake_hash_buffer_server_finished_len = 0;
	session->internals.handshake_hash_buffer_client_finished_len = 0;
	session->internals.handshake_hash_buffer_prev_len = 0;
	session->internals.handshake_hash_buffer.length = 0;
	session->internals.full_client_hello.length = 0;
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
	handshake_hash_buffer_reset(session);
	_gnutls_buffer_clear(&session->internals.handshake_hash_buffer);
	_gnutls_buffer_clear(&session->internals.full_client_hello);
}

/* Replace handshake message buffer, with the special synthetic message
 * needed by TLS1.3 when HRR is sent. */
int _gnutls13_handshake_hash_buffers_synth(gnutls_session_t session,
					   const mac_entry_st *prf,
					   unsigned client)
{
	int ret;
	uint8_t hdata[4+MAX_HASH_SIZE];
	size_t length;

	if (client)
		length = session->internals.handshake_hash_buffer_prev_len;
	else
		length = session->internals.handshake_hash_buffer.length;

	/* calculate hash */
	hdata[0] = 254;
	_gnutls_write_uint24(prf->output_size, &hdata[1]);

	ret = gnutls_hash_fast((gnutls_digest_algorithm_t)prf->id,
			       session->internals.handshake_hash_buffer.data,
			       length, hdata+4);
	if (ret < 0)
		return gnutls_assert_val(ret);

	handshake_hash_buffer_reset(session);

	ret =
	    _gnutls_buffer_append_data(&session->internals.
				       handshake_hash_buffer,
				       hdata, prf->output_size+4);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_buffers_log("BUF[HSK]: Replaced handshake buffer with synth message (%d bytes)\n",
			    prf->output_size+4);

	return 0;
}

/* this will copy the required values for resuming to
 * internals, and to security_parameters.
 * this will keep as less data to security_parameters.
 */
static int tls12_resume_copy_required_vals(gnutls_session_t session, unsigned ticket)
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
	ret = _gnutls_set_cipher_suite2(session,
				       session->internals.
				       resumed_security_parameters.
				       cs);
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

	session->security_parameters.client_ctype =
	    session->internals.resumed_security_parameters.client_ctype;
	session->security_parameters.server_ctype =
	    session->internals.resumed_security_parameters.server_ctype;

	if (!ticket) {
		memcpy(session->security_parameters.session_id,
		       session->internals.resumed_security_parameters.session_id,
		       sizeof(session->security_parameters.session_id));
		session->security_parameters.session_id_size =
		    session->internals.resumed_security_parameters.session_id_size;
	}

	return 0;
}

void _gnutls_set_client_random(gnutls_session_t session, uint8_t * rnd)
{
	memcpy(session->security_parameters.client_random, rnd,
	       GNUTLS_RANDOM_SIZE);
}

static
int _gnutls_gen_client_random(gnutls_session_t session)
{
	int ret;

	/* no random given, we generate. */
	if (session->internals.sc_random_set != 0) {
		memcpy(session->security_parameters.client_random,
		       session->internals.
		       resumed_security_parameters.client_random,
		       GNUTLS_RANDOM_SIZE);
	} else {
		ret = gnutls_rnd(GNUTLS_RND_NONCE,
			session->security_parameters.client_random,
			GNUTLS_RANDOM_SIZE);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return 0;
}

static
int _gnutls_set_server_random(gnutls_session_t session, const version_entry_st *vers, uint8_t * rnd)
{
	const version_entry_st *max;

	memcpy(session->security_parameters.server_random, rnd,
	       GNUTLS_RANDOM_SIZE);

	/* check whether the server random value is set according to
	 * to TLS 1.3. p4.1.3 requirements */
	if (!IS_DTLS(session) && vers->id <= GNUTLS_TLS1_2 && have_creds_for_tls13(session)) {

		max = _gnutls_version_max(session);
		if (max->id <= GNUTLS_TLS1_2)
			return 0;

		if (vers->id == GNUTLS_TLS1_2 &&
		    memcmp(&session->security_parameters.server_random[GNUTLS_RANDOM_SIZE-8],
		    "\x44\x4F\x57\x4E\x47\x52\x44\x01", 8) == 0) {

			_gnutls_audit_log(session,
				  "Detected downgrade to TLS 1.2 from TLS 1.3\n");
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
		} else if (vers->id <= GNUTLS_TLS1_1 &&
			   memcmp(&session->security_parameters.server_random[GNUTLS_RANDOM_SIZE-8],
			   "\x44\x4F\x57\x4E\x47\x52\x44\x00", 8) == 0) {

			_gnutls_audit_log(session,
				  "Detected downgrade to TLS 1.1 or earlier from TLS 1.3\n");
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
		}
	}

	return 0;
}

int _gnutls_gen_server_random(gnutls_session_t session, int version)
{
	int ret;
	const version_entry_st *max;

	if (session->internals.sc_random_set != 0) {
		memcpy(session->security_parameters.server_random,
		       session->internals.
		       resumed_security_parameters.server_random,
		       GNUTLS_RANDOM_SIZE);
		return 0;
	}

	max = _gnutls_version_max(session);

	if (!IS_DTLS(session) && max->id >= GNUTLS_TLS1_3 &&
	    version <= GNUTLS_TLS1_2) {
		if (version == GNUTLS_TLS1_2) {
			memcpy(&session->security_parameters.server_random[GNUTLS_RANDOM_SIZE-8],
				"\x44\x4F\x57\x4E\x47\x52\x44\x01", 8);
		} else {
			memcpy(&session->security_parameters.server_random[GNUTLS_RANDOM_SIZE-8],
				"\x44\x4F\x57\x4E\x47\x52\x44\x00", 8);
		}
		ret =
		    gnutls_rnd(GNUTLS_RND_NONCE, session->security_parameters.server_random, GNUTLS_RANDOM_SIZE-8);

	} else {
		ret =
		    gnutls_rnd(GNUTLS_RND_NONCE, session->security_parameters.server_random, GNUTLS_RANDOM_SIZE);
	}

	if (ret < 0) {
		gnutls_assert();
		return ret;
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
	uint8_t concat[MAX_HASH_SIZE];
	size_t hash_len;
	const char *mesg;
	int rc, len, algorithm;

	if (sending)
		len = session->internals.handshake_hash_buffer.length;
	else
		len = session->internals.handshake_hash_buffer_prev_len;

	algorithm = session->security_parameters.prf->id;
	rc = _gnutls_hash_fast(algorithm,
			       session->internals.
			       handshake_hash_buffer.data, len,
			       concat);
	if (rc < 0)
		return gnutls_assert_val(rc);

	hash_len = session->security_parameters.prf->output_size;

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
			  uint8_t major, uint8_t minor, unsigned allow_tls13)
{
	const version_entry_st *vers;
	const version_entry_st *aversion = nversion_to_entry(major, minor);

	/* if we do not support that version, unless that version is TLS 1.2;
	 * TLS 1.2 is handled separately because it is always advertized under TLS 1.3 or later */
	if (aversion == NULL ||
	    _gnutls_nversion_is_supported(session, major, minor) == 0) {

		if (aversion && aversion->id == GNUTLS_TLS1_2) {
			vers = _gnutls_version_max(session);
			if (unlikely(vers == NULL))
				return gnutls_assert_val(GNUTLS_E_NO_CIPHER_SUITES);

			if (vers->id >= GNUTLS_TLS1_2) {
				session->security_parameters.pversion = aversion;
				return 0;
			}
		}

		/* if we get an unknown/unsupported version, then fail if the version we
		 * got is too low to be supported */
		if (!_gnutls_version_is_too_high(session, major, minor))
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

		/* If he requested something we do not support
		 * then we send him the highest we support.
		 */
		vers = _gnutls_legacy_version_max(session);
		if (vers == NULL) {
			/* this check is not really needed.
			 */
			gnutls_assert();
			return GNUTLS_E_UNKNOWN_CIPHER_SUITE;
		}

		session->security_parameters.pversion = vers;

		return 0;
	} else {
		session->security_parameters.pversion = aversion;

		/* we do not allow TLS1.3 negotiation using this mechanism */
		if (aversion->tls13_sem && !allow_tls13) {
			vers = _gnutls_legacy_version_max(session);
			session->security_parameters.pversion = vers;
		}

		return 0;
	}
}

/* This function returns:
 *  - zero on success
 *  - GNUTLS_E_INT_RET_0 if GNUTLS_E_AGAIN || GNUTLS_E_INTERRUPTED were returned by the callback
 *  - a negative error code on other error
 */
int
_gnutls_user_hello_func(gnutls_session_t session,
			uint8_t major, uint8_t minor)
{
	int ret, sret = 0;
	const version_entry_st *vers, *old_vers;
	const version_entry_st *new_max;

	if (session->internals.user_hello_func != NULL) {
		ret = session->internals.user_hello_func(session);

		if (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED) {
			gnutls_assert();
			sret = GNUTLS_E_INT_RET_0;
		} else if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* This callback is often used to switch the priority string of the
		 * server, and that includes switching version which we have already
		 * negotiated; note that this doesn't apply when resuming as the version
		 * negotiation is already complete. */
		if (session->internals.resumed != RESUME_TRUE) {
			new_max = _gnutls_version_max(session);
			old_vers = get_version(session);

			if (!old_vers->tls13_sem || (new_max && !new_max->tls13_sem)) {
#if GNUTLS_TLS_VERSION_MAX != GNUTLS_TLS1_3
# error "Need to update the following logic"
#endif
				/* Here we need to renegotiate the version since the callee might
				 * have disabled some TLS versions. This logic does not cope for
				 * protocols later than TLS1.3 if they have the tls13_sem set */
				ret = _gnutls_negotiate_version(session, major, minor, 0);
				if (ret < 0)
					return gnutls_assert_val(ret);

				vers = get_version(session);
				if (old_vers != vers) {
					/* at this point we need to regenerate the random value to
					 * avoid the peer detecting this session as a rollback
					 * attempt. */
					ret = _gnutls_gen_server_random(session, vers->id);
					if (ret < 0)
						return gnutls_assert_val(ret);
				}
			}
		}
	}
	return sret;
}

/* Associates the right credential types for the session, and
 * performs sanity checks. */
static int set_auth_types(gnutls_session_t session)
{
	const version_entry_st *ver = get_version(session);
	gnutls_kx_algorithm_t kx;

	/* sanity check:
	 * we see TLS1.3 negotiated but no key share was sent */
	if (ver->tls13_sem) {
		if (unlikely(!(session->internals.hsk_flags & HSK_PSK_KE_MODE_PSK) &&
			     !(session->internals.hsk_flags & HSK_KEY_SHARE_RECEIVED))) {
			return gnutls_assert_val(GNUTLS_E_MISSING_EXTENSION);
		}

		/* Under TLS1.3 this returns a KX which matches the negotiated
		 * groups from the key shares; if we are resuming then the KX seen
		 * here doesn't match the original session. */
		if (session->internals.resumed == RESUME_FALSE)
			kx = gnutls_kx_get(session);
		else
			kx = GNUTLS_KX_UNKNOWN;
	} else {
		/* TLS1.2 or earlier, kx is associated with ciphersuite */
		kx = session->security_parameters.cs->kx_algorithm;
	}

	if (kx != GNUTLS_KX_UNKNOWN) {
		session->security_parameters.server_auth_type = _gnutls_map_kx_get_cred(kx, 1);
		session->security_parameters.client_auth_type = _gnutls_map_kx_get_cred(kx, 0);
	} else if (unlikely(session->internals.resumed == RESUME_FALSE)) {
		/* Here we can only arrive if something we received
		 * prevented the session from completing. */
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
	}

	return 0;
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
	uint16_t suite_size, comp_size;
	int ext_size;
	int neg_version, sret = 0;
	int len = datalen;
	uint8_t major, minor;
	uint8_t *suite_ptr, *comp_ptr, *session_id, *ext_ptr;
	const version_entry_st *vers;

	DECR_LEN(len, 2);
	_gnutls_handshake_log("HSK[%p]: Client's version: %d.%d\n",
			      session, data[pos], data[pos + 1]);

	major = data[pos];
	minor = data[pos+1];

	set_adv_version(session, major, minor);

	ret = _gnutls_negotiate_version(session, major, minor, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	vers = get_version(session);
	if (vers == NULL)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	neg_version = vers->id;

	pos += 2;

	/* Read client random value.
	 */
	DECR_LEN(len, GNUTLS_RANDOM_SIZE);
	_gnutls_set_client_random(session, &data[pos]);

	pos += GNUTLS_RANDOM_SIZE;

	ret = _gnutls_gen_server_random(session, neg_version);
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

	if (suite_size == 0 || (suite_size % 2) != 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	suite_ptr = &data[pos];
	DECR_LEN(len, suite_size);
	pos += suite_size;

	DECR_LEN(len, 1);
	comp_size = data[pos++]; /* the number of compression methods */

	if (comp_size == 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	comp_ptr = &data[pos];
	DECR_LEN(len, comp_size);
	pos += comp_size;

	ext_ptr = &data[pos];
	ext_size = len;

	/* Parse only the mandatory to read extensions for resumption
	 * and version negotiation. We don't want to parse any other
	 * extensions since  we don't want new extension values to override
	 * the resumed ones. */
	ret =
	    _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_CLIENT_HELLO,
					   GNUTLS_EXT_VERSION_NEG,
					   ext_ptr, ext_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_CLIENT_HELLO,
					   GNUTLS_EXT_MANDATORY,
					   ext_ptr, ext_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	vers = get_version(session);
	if (unlikely(vers == NULL))
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	if (!vers->tls13_sem) {
		ret =
		    _gnutls_server_restore_session(session, session_id,
						   session_id_len);

		if (session_id_len > 0)
			session->internals.resumption_requested = 1;

		if (ret == 0) {		/* resumed using default TLS resumption! */
			ret = _gnutls_server_select_suite(session, suite_ptr, suite_size, 1);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ret = tls12_resume_copy_required_vals(session, 0);
			if (ret < 0)
				return gnutls_assert_val(ret);

			session->internals.resumed = RESUME_TRUE;

			return _gnutls_user_hello_func(session, major, minor);
		} else {
			ret = _gnutls_generate_session_id(session->security_parameters.
							  session_id,
							  &session->security_parameters.
							  session_id_size);
			if (ret < 0)
				return gnutls_assert_val(ret);

			session->internals.resumed = RESUME_FALSE;
		}
	} else { /* TLS1.3 */
		/* we echo client's session ID - length was checked previously */
		assert(session_id_len <= GNUTLS_MAX_SESSION_ID_SIZE);
		if (session_id_len > 0)
			memcpy(session->security_parameters.session_id, session_id, session_id_len);
		session->security_parameters.session_id_size = session_id_len;
	}

	/* Parse the extensions (if any)
	 *
	 * Unconditionally try to parse extensions; safe renegotiation uses them in
	 * sslv3 and higher, even though sslv3 doesn't officially support them.
	 */
	ret = _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_CLIENT_HELLO,
					     GNUTLS_EXT_APPLICATION,
					     ext_ptr, ext_size);
	/* len is the rest of the parsed length */
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* we cache this error code */
	sret = _gnutls_user_hello_func(session, major, minor);
	if (sret < 0 && sret != GNUTLS_E_INT_RET_0) {
		gnutls_assert();
		return sret;
	}

	/* Session tickets are parsed in this point */
	ret =
	    _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_CLIENT_HELLO,
					   GNUTLS_EXT_TLS, ext_ptr, ext_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* resumed by session_ticket extension */
	if (!vers->tls13_sem && session->internals.resumed != RESUME_FALSE) {
		session->internals.resumed_security_parameters.
		    max_record_recv_size =
		    session->security_parameters.max_record_recv_size;
		session->internals.resumed_security_parameters.
		    max_record_send_size =
		    session->security_parameters.max_record_send_size;

		ret = _gnutls_server_select_suite(session, suite_ptr, suite_size, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = tls12_resume_copy_required_vals(session, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		/* to indicate to the client that the current session is resumed */
		memcpy(session->security_parameters.session_id, session_id, session_id_len);
		session->security_parameters.session_id_size = session_id_len;

		return 0;
	}

	/* select an appropriate cipher suite (as well as certificate)
	 */
	ret = _gnutls_server_select_suite(session, suite_ptr, suite_size, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_handshake_log("HSK[%p]: Selected version %s\n", session, session->security_parameters.pversion->name);

	/* select appropriate compression method */
	ret =
	    check_if_null_comp_present(session, comp_ptr,
					      comp_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* call extensions that are intended to be parsed after the ciphersuite/cert
	 * are known. */
	ret =
	    _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_CLIENT_HELLO,
				     _GNUTLS_EXT_TLS_POST_CS, ext_ptr, ext_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* Calculate TLS 1.3 Early Secret */
	if (session->security_parameters.pversion->tls13_sem &&
	    !(session->internals.hsk_flags & HSK_PSK_SELECTED)) {
		ret = _tls13_init_secret(session, NULL, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	ret = set_auth_types(session);
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
int _gnutls_send_finished(gnutls_session_t session, int again)
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
int _gnutls_recv_finished(gnutls_session_t session)
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

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
	/* When fuzzying allow to proceed without verifying the handshake
	 * consistency */
# warning This is unsafe for production builds

#else
	if (memcmp(vrfy, data, data_size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_ERROR_IN_FINISHED_PACKET;
		goto cleanup;
	}
#endif

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


      cleanup:
	_gnutls_buffer_clear(&buf);

	return ret;
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
	unsigned int i;
	ciphersuite_list_st peer_clist;
	const gnutls_cipher_suite_entry_st *selected;
	gnutls_kx_algorithm_t kx;
	int retval;
	const version_entry_st *vers = get_version(session);

	peer_clist.size = 0;

	for (i = 0; i < datalen; i += 2) {
		/* we support the TLS renegotiation SCSV, even if we are
		 * not under SSL 3.0, because openssl sends this SCSV
		 * on resumption unconditionally. */
		/* TLS_RENEGO_PROTECTION_REQUEST = { 0x00, 0xff } */
		if (session->internals.priorities->sr != SR_DISABLED &&
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
			const version_entry_st *max = _gnutls_version_max(session);

			_gnutls_handshake_log
			    ("HSK[%p]: Received fallback CS\n",
			     session);

			if (vers != max)
				return gnutls_assert_val(GNUTLS_E_INAPPROPRIATE_FALLBACK);
		} else if (!scsv_only) {
			if (peer_clist.size < MAX_CIPHERSUITE_SIZE) {
				peer_clist.entry[peer_clist.size] = ciphersuite_to_entry(&data[i]);
				if (peer_clist.entry[peer_clist.size] != NULL)
					peer_clist.size++;
			}
		}
	}

	if (scsv_only)
		return 0;

	ret = _gnutls_figure_common_ciphersuite(session, &peer_clist, &selected);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	_gnutls_handshake_log
		    ("HSK[%p]: Selected cipher suite: %s\n", session, selected->name);

	ret = _gnutls_set_cipher_suite2(session, selected);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (!vers->tls13_sem) {
		/* check if the credentials (username, public key etc.) are ok
		 */
		kx = selected->kx_algorithm;
		if (_gnutls_get_kx_cred(session, kx) == NULL) {
			gnutls_assert();
			return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
		}

		/* set the mod_auth_st to the appropriate struct
		 * according to the KX algorithm. This is needed since all the
		 * handshake functions are read from there;
		 */
		session->internals.auth_struct = _gnutls_kx_auth_struct(kx);
		if (session->internals.auth_struct == NULL) {
			_gnutls_handshake_log
			    ("HSK[%p]: Cannot find the appropriate handler for the KX algorithm\n",
			     session);
			gnutls_assert();
			return GNUTLS_E_INTERNAL_ERROR;
		}
	}

	return 0;

}


/* This checks whether the null compression method is present.
 */
static int
check_if_null_comp_present(gnutls_session_t session,
			  uint8_t * data, int datalen)
{
	int j;

	for (j = 0; j < datalen; j++) {
		if (data[j] == 0)
			return 0;
	}

	/* we were not able to find a the NULL compression
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

int _gnutls_call_hook_func(gnutls_session_t session,
			   gnutls_handshake_description_t type,
			   int post, unsigned incoming,
			   const uint8_t *data, unsigned data_size)
{
	gnutls_datum_t msg = {(void*)data, data_size};

	if (session->internals.h_hook != NULL) {
		if ((session->internals.h_type == type
		     || session->internals.h_type == GNUTLS_HANDSHAKE_ANY)
		    && (session->internals.h_post == post
			|| session->internals.h_post == GNUTLS_HOOK_BOTH)) {

			/* internal API for testing: when we are expected to
			 * wait for GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC, we
			 * do so, but not when doing for all messages. The
			 * reason is that change cipher specs are not handshake
			 * messages, and we don't support waiting for them
			 * consistently (only sending is tracked, not receiving).
			 */
			if (type == GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC &&
			    session->internals.h_type != GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC)
				return 0;

			return session->internals.h_hook(session, type,
							 post, incoming, &msg);
		}
	}
	return 0;
}

/* Note that the "New session ticket" handshake packet behaves differently under
 * TLS 1.2 or 1.3. In 1.2 it is included in the handshake process, while in 1.3
 * it is sent asynchronously */
#define IS_ASYNC(t, v) \
	(t == GNUTLS_HANDSHAKE_HELLO_REQUEST || t == GNUTLS_HANDSHAKE_KEY_UPDATE || \
	 (t == GNUTLS_HANDSHAKE_NEW_SESSION_TICKET && v->tls13_sem))

int
_gnutls_send_handshake(gnutls_session_t session, mbuffer_st * bufel,
		       gnutls_handshake_description_t type)
{
	return _gnutls_send_handshake2(session, bufel, type, 0);
}

/* This function sends a handshake message of type 'type' containing the
 * data specified here. If the previous _gnutls_send_handshake() returned
 * GNUTLS_E_AGAIN or GNUTLS_E_INTERRUPTED, then it must be called again 
 * (until it returns ok), with NULL parameters.
 */
int
_gnutls_send_handshake2(gnutls_session_t session, mbuffer_st * bufel,
		        gnutls_handshake_description_t type, unsigned queue_only)
{
	int ret;
	uint8_t *data;
	uint32_t datasize, i_datasize;
	int pos = 0;
	const version_entry_st *vers = get_version(session);

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

	data[pos++] = (uint8_t) REAL_HSK_TYPE(type);
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
	if (!IS_ASYNC(type, vers)) {
		if ((ret =
		     handshake_hash_add_sent(session, type, data,
						     datasize)) < 0) {
			gnutls_assert();
			_mbuffer_xfree(&bufel);
			return ret;
		}
		/* If we are sending a PSK, generate early secrets here.
		 * This cannot be done in pre_shared_key.c, because it
		 * relies on transcript hash of a Client Hello. */
		if (type == GNUTLS_HANDSHAKE_CLIENT_HELLO &&
		    session->key.binders[0].prf != NULL) {
			ret = _gnutls_generate_early_secrets_for_psk(session);
			if (ret < 0) {
				gnutls_assert();
				_mbuffer_xfree(&bufel);
				return ret;
			}
		}
	}

	ret = _gnutls_call_hook_func(session, type, GNUTLS_HOOK_PRE, 0,
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

	ret = _gnutls_call_hook_func(session, type, GNUTLS_HOOK_POST, 0,
				      _mbuffer_get_udata_ptr(bufel), _mbuffer_get_udata_size(bufel));
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (queue_only)
		return 0;

	/* Decide when to cache and when to send */
	if (vers && vers->tls13_sem) {

		if (session->internals.initial_negotiation_completed) {
			/* we are under TLS1.3 in a re-authentication phase.
			 * we don't attempt to cache any messages */
			goto force_send;
		}

		/* The messages which are followed by another are not sent by default
		 * but are cached instead */
		switch (type) {
		case GNUTLS_HANDSHAKE_SERVER_HELLO:	/* always followed by something */
		case GNUTLS_HANDSHAKE_ENCRYPTED_EXTENSIONS: /* followed by finished or cert */
		case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:  /* followed by certificate */
		case GNUTLS_HANDSHAKE_CERTIFICATE_PKT:	/* this one is followed by cert verify */
		case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY: /* followed by finished */
			ret = 0; /* cache */
			break;
		default:
			/* send this and any cached messages */
			goto force_send;
		}
	} else {
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
			/* send this and any cached messages */
			goto force_send;
		}
	}

	return ret;

 force_send:
	return _gnutls_handshake_io_write_flush(session);
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
	     IS_ASYNC(recv_type, vers))
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
	if (recv_type == GNUTLS_HANDSHAKE_CLIENT_HELLO)
		session->internals.handshake_hash_buffer_client_hello_len =
			session->internals.handshake_hash_buffer.length;
	if (recv_type == GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE)
		session->internals.handshake_hash_buffer_client_kx_len =
			session->internals.handshake_hash_buffer.length;
	if (recv_type == GNUTLS_HANDSHAKE_FINISHED && session->security_parameters.entity == GNUTLS_CLIENT)
		session->internals.handshake_hash_buffer_server_finished_len =
			session->internals.handshake_hash_buffer.length;
	if (recv_type == GNUTLS_HANDSHAKE_FINISHED && session->security_parameters.entity == GNUTLS_SERVER)
		session->internals.handshake_hash_buffer_client_finished_len =
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

	if (IS_ASYNC(type, vers))
		return 0;

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

	if (type == GNUTLS_HANDSHAKE_CLIENT_HELLO)
		session->internals.handshake_hash_buffer_client_hello_len =
			session->internals.handshake_hash_buffer.length;
	if (type == GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE)
		session->internals.handshake_hash_buffer_client_kx_len =
			session->internals.handshake_hash_buffer.length;
	if (type == GNUTLS_HANDSHAKE_FINISHED && session->security_parameters.entity == GNUTLS_SERVER)
		session->internals.handshake_hash_buffer_server_finished_len =
			session->internals.handshake_hash_buffer.length;
	if (type == GNUTLS_HANDSHAKE_FINISHED && session->security_parameters.entity == GNUTLS_CLIENT)
		session->internals.handshake_hash_buffer_client_finished_len =
			session->internals.handshake_hash_buffer.length;

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

	ret = _gnutls_call_hook_func(session, hsk.htype, GNUTLS_HOOK_PRE, 1, hsk.data.data, hsk.data.length);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = handshake_hash_add_recvd(session, hsk.rtype,
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
		if (!(IS_SERVER(session))) {
			ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
			goto cleanup;
		}

#ifdef ENABLE_SSL2
		if (hsk.htype == GNUTLS_HANDSHAKE_CLIENT_HELLO_V2)
			ret =
			    _gnutls_read_client_hello_v2(session,
							 hsk.data.data,
							 hsk.data.length);
		else
#endif
		{
			/* Reference the full ClientHello in case an extension needs it */
			ret = _gnutls_ext_set_full_client_hello(session, &hsk);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ret = read_client_hello(session, hsk.data.data,
						hsk.data.length);
		}

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		break;

	case GNUTLS_HANDSHAKE_SERVER_HELLO:
		if (IS_SERVER(session)) {
			ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
			goto cleanup;
		}

		ret = read_server_hello(session, hsk.data.data,
					hsk.data.length);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		break;
	case GNUTLS_HANDSHAKE_HELLO_VERIFY_REQUEST:
		if (IS_SERVER(session)) {
			ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
			goto cleanup;
		}

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
	case GNUTLS_HANDSHAKE_HELLO_RETRY_REQUEST: {
		/* hash buffer synth message is generated during hello retry parsing */
		gnutls_datum_t hrr = {hsk.data.data, hsk.data.length};

		if (IS_SERVER(session)) {
			ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
			goto cleanup;
		}

		ret =
		    _gnutls13_recv_hello_retry_request(session,
						       &hsk.data);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		} else {
			/* during hello retry parsing, we reset handshake hash buffer,
			 * re-add this message */
			ret = handshake_hash_add_recvd(session, hsk.htype,
						       hsk.header, hsk.header_size,
						       hrr.data,
						       hrr.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			/* Signal our caller we have received a retry request
			   and ClientHello needs to be sent again. */
			ret = 1;
		}

		break;
	}
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
	case GNUTLS_HANDSHAKE_ENCRYPTED_EXTENSIONS:
	case GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
	case GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
	case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:
	case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY:
	case GNUTLS_HANDSHAKE_SUPPLEMENTAL:
	case GNUTLS_HANDSHAKE_NEW_SESSION_TICKET:
	case GNUTLS_HANDSHAKE_END_OF_EARLY_DATA:
		ret = hsk.data.length;
		break;
	default:
		gnutls_assert();
		/* we shouldn't actually arrive here in any case .
		 * unexpected messages should be caught after _gnutls_handshake_io_recv_int()
		 */
		ret = GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET;
		goto cleanup;
	}

	ret2 = _gnutls_call_hook_func(session, hsk.htype, GNUTLS_HOOK_POST, 1, hsk.data.data, hsk.data.length);
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
	unsigned j;
	int ret;
	const gnutls_cipher_suite_entry_st *selected = NULL;
	const version_entry_st *vers = get_version(session);
	gnutls_kx_algorithm_t kx;

	for (j = 0; j < session->internals.priorities->cs.size; j++) {
		if (suite[0] == session->internals.priorities->cs.entry[j]->id[0] &&
		    suite[1] == session->internals.priorities->cs.entry[j]->id[1]) {
			selected = session->internals.priorities->cs.entry[j];
			break;
		}
	}

	if (!selected) {
		gnutls_assert();
		_gnutls_handshake_log
		    ("HSK[%p]: unsupported cipher suite %.2X.%.2X was negotiated\n",
		     session, (unsigned int) suite[0],
		     (unsigned int) suite[1]);
		return GNUTLS_E_UNKNOWN_CIPHER_SUITE;
	}

	ret = _gnutls_set_cipher_suite2(session, selected);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_handshake_log("HSK[%p]: Selected cipher suite: %s\n",
			      session,
			      selected->name);

	/* check if the credentials (username, public key etc.) are ok.
	 * Actually checks if they exist.
	 */
	if (!vers->tls13_sem) {
		kx = selected->kx_algorithm;

		if (!session->internals.premaster_set &&
		    _gnutls_get_kx_cred
		    (session, kx) == NULL) {
			gnutls_assert();
			return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
		}

		/* set the mod_auth_st to the appropriate struct
		 * according to the KX algorithm. This is needed since all the
		 * handshake functions are read from there;
		 */
		session->internals.auth_struct =
		    _gnutls_kx_auth_struct(kx);

		if (session->internals.auth_struct == NULL) {
			_gnutls_handshake_log
			    ("HSK[%p]: Cannot find the appropriate handler for the KX algorithm\n",
			     session);
			gnutls_assert();
			return GNUTLS_E_INTERNAL_ERROR;
		}
	} else {
		if (session->internals.hsk_flags & HSK_PSK_SELECTED) {
			if (session->key.binders[0].prf->id != selected->prf) {
				_gnutls_handshake_log
				    ("HSK[%p]: PRF of ciphersuite differs with the PSK identity (cs: %s, id: %s)\n",
				     session, selected->name, session->key.binders[0].prf->name);
				gnutls_assert();
				return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			}
		}
	}

	return 0;
}

/* This function returns 0 if we are resuming a session or -1 otherwise.
 * This also sets the variables in the session. Used only while reading a server
 * hello. Only applicable to TLS1.2 or earlier.
 */
static int
client_check_if_resuming(gnutls_session_t session,
			 uint8_t * session_id, int session_id_len)
{
	char buf[2 * GNUTLS_MAX_SESSION_ID_SIZE + 1];
	int ret;

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

		ret = _gnutls_set_cipher_suite2
		    (session,
		     session->internals.resumed_security_parameters.
		     cs);
		if (ret < 0) {
			gnutls_assert();
			goto no_resume;
		}

		session->internals.resumed = RESUME_TRUE;	/* we are resuming */

		return 0;
	} else {
no_resume:
		/* keep the new session id */
		session->internals.resumed = RESUME_FALSE;	/* we are not resuming */
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
	uint8_t *session_id;
	uint8_t *cs_pos, *comp_pos, *srandom_pos;
	uint8_t major, minor;
	int pos = 0;
	int ret;
	int len = datalen;
	unsigned ext_parse_flag = 0;
	const version_entry_st *vers, *saved_vers;

	if (datalen < GNUTLS_RANDOM_SIZE+2) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	_gnutls_handshake_log("HSK[%p]: Server's version: %d.%d\n",
			      session, data[pos], data[pos + 1]);

	DECR_LEN(len, 2);
	major = data[pos];
	minor = data[pos+1];

	saved_vers = get_version(session); /* will be non-null if HRR has been received */

	vers = nversion_to_entry(major, minor);
	if (unlikely(vers == NULL))
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	if (vers->tls13_sem) /* that shouldn't have been negotiated here */
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	if (_gnutls_set_current_version(session, vers->id) < 0)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	pos += 2;

	DECR_LEN(len, GNUTLS_RANDOM_SIZE);
	srandom_pos = &data[pos];
	pos += GNUTLS_RANDOM_SIZE;

	/* Read session ID
	 */
	DECR_LEN(len, 1);
	session_id_len = data[pos++];

	if (len < session_id_len || session_id_len > GNUTLS_MAX_SESSION_ID_SIZE) {
		gnutls_assert();
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}
	DECR_LEN(len, session_id_len);
	session_id = &data[pos];
	pos += session_id_len;

	DECR_LEN(len, 2);
	cs_pos = &data[pos];
	pos += 2;

	/* move to compression
	 */
	DECR_LEN(len, 1);
	comp_pos = &data[pos];
	pos++;

	/* parse extensions to figure version */
	ret =
	    _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO|
					   GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO,
					   GNUTLS_EXT_VERSION_NEG,
					   &data[pos], len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	vers = get_version(session);
	if (unlikely(vers == NULL))
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);
	if (vers->tls13_sem) {
		if (major != 0x03 || minor != 0x03)
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);
	}

	if (_gnutls_nversion_is_supported(session, vers->major, vers->minor) == 0)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	/* set server random - done after final version is selected */
	ret = _gnutls_set_server_random(session, vers, srandom_pos);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* reset keys and binders if we are not using TLS 1.3 */
	if (!vers->tls13_sem) {
		gnutls_memset(&session->key.proto.tls13, 0,
			      sizeof(session->key.proto.tls13));
		reset_binders(session);
	}

	/* check if we are resuming and set the appropriate
	 * values;
	 */
	if (!vers->tls13_sem &&
	    client_check_if_resuming(session, session_id, session_id_len) == 0) {
		ret =
		    _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
						   GNUTLS_EXT_MANDATORY,
						   &data[pos], len);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return 0;
	} else {
		session->security_parameters.session_id_size = session_id_len;
		if (session_id_len > 0)
			memcpy(session->security_parameters.session_id, session_id,
			       session_id_len);
	}

	/* Check if the given cipher suite is supported and copy
	 * it to the session.
	 */
	ret = set_client_ciphersuite(session, cs_pos);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (session->internals.hsk_flags & HSK_HRR_RECEIVED) {
		/* check if ciphersuite matches */
		if (memcmp(cs_pos, session->internals.hrr_cs, 2) != 0)
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

		/* check if HRR version matches this version */
		if (vers != saved_vers)
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	if (*comp_pos != 0)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	if (vers->tls13_sem)
		ext_parse_flag |= GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO;
	else
		ext_parse_flag |= GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO;

	/* Parse extensions in order.
	 */
	ret =
	    _gnutls_parse_hello_extensions(session,
					   ext_parse_flag,
					   GNUTLS_EXT_MANDATORY,
					   &data[pos], len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* check if EtM is required */
	if (!vers->tls13_sem && session->internals.priorities->force_etm && !session->security_parameters.etm) {
		const cipher_entry_st *cipher = cipher_to_entry(session->security_parameters.cs->block_algorithm);
		if (_gnutls_cipher_type(cipher) == CIPHER_BLOCK)
			return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);
	}


	ret =
	    _gnutls_parse_hello_extensions(session,
					   ext_parse_flag,
					   GNUTLS_EXT_APPLICATION,
					   &data[pos], len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_parse_hello_extensions(session,
					   ext_parse_flag,
					   GNUTLS_EXT_TLS,
					   &data[pos], len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_parse_hello_extensions(session,
					   ext_parse_flag,
					   _GNUTLS_EXT_TLS_POST_CS,
					   &data[pos], len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Calculate TLS 1.3 Early Secret */
	if (vers->tls13_sem &&
	    !(session->internals.hsk_flags & HSK_PSK_SELECTED)) {
		ret = _tls13_init_secret(session, NULL, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	ret = set_auth_types(session);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/* This function copies the appropriate compression methods, to a locally allocated buffer 
 * Needed in hello messages. Returns the new data length.
 */
static int
append_null_comp(gnutls_session_t session,
			  gnutls_buffer_st * cdata)
{
	uint8_t compression_methods[2] = {0x01, 0x00};
	size_t init_length = cdata->length;
	int ret;

	ret =
	    _gnutls_buffer_append_data(cdata, compression_methods, 2);
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
	const version_entry_st *hver, *min_ver, *max_ver;
	uint8_t tver[2];
	gnutls_buffer_st extdata;
	int rehandshake = 0;
	unsigned add_sr_scsv = 0;
	uint8_t session_id_len =
	    session->internals.resumed_security_parameters.session_id_size;


	if (again == 0) {
		/* note that rehandshake is different than resuming
		 */
		if (session->internals.initial_negotiation_completed)
			rehandshake = 1;

		ret = _gnutls_buffer_init_handshake_mbuffer(&extdata);
		if (ret < 0)
			return gnutls_assert_val(ret);

		/* if we are resuming a session then we set the
		 * version number to the previously established.
		 */
		if (session->internals.resumption_requested == 0 &&
		    session->internals.premaster_set == 0) {
			if (rehandshake)	/* already negotiated version thus version_max == negotiated version */
				hver = get_version(session);
			else	/* new handshake. just get the max */
				hver = _gnutls_legacy_version_max(session);
		} else {
			/* we are resuming a session */
			hver =
			    session->internals.resumed_security_parameters.
			    pversion;

			if (hver && hver->tls13_sem)
				hver = _gnutls_legacy_version_max(session);
		}

		if (hver == NULL) {
			gnutls_assert();
			if (session->internals.flags & INT_FLAG_NO_TLS13)
				ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS;
			else
				ret = GNUTLS_E_NO_PRIORITIES_WERE_SET;
			goto cleanup;
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

		min_ver = _gnutls_version_lowest(session);
		max_ver = _gnutls_version_max(session);
		if (min_ver == NULL || max_ver == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_NO_PRIORITIES_WERE_SET;
			goto cleanup;
		}

		/* if we are replying to an HRR the version is already negotiated */
		if (!(session->internals.hsk_flags & HSK_HRR_RECEIVED) || !get_version(session)) {
			/* Set the version we advertized as maximum
			 * (RSA uses it). */
			set_adv_version(session, hver->major, hver->minor);
			if (_gnutls_set_current_version(session, hver->id) < 0) {
				ret = gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);
				goto cleanup;
			}
		}

		if (session->internals.priorities->min_record_version != 0) {
			/* Advertize the lowest supported (SSL 3.0) record packet
			 * version in record packets during the handshake.
			 * That is to avoid confusing implementations
			 * that do not support TLS 1.2 and don't know
			 * how 3,3 version of record packets look like.
			 */
			set_default_version(session, min_ver);
		} else {
			set_default_version(session, hver);
		}

		/* In order to know when this session was initiated.
		 */
		session->security_parameters.timestamp = gnutls_time(NULL);

		/* Generate random data 
		 */
		if (!(session->internals.hsk_flags & HSK_HRR_RECEIVED) &&
		    !(IS_DTLS(session) && session->internals.dtls.hsk_hello_verify_requests == 0)) {
			ret = _gnutls_gen_client_random(session);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

		}

		ret = _gnutls_buffer_append_data(&extdata,
						 session->security_parameters.client_random,
						 GNUTLS_RANDOM_SIZE);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

#ifdef TLS13_APPENDIX_D4
		if (max_ver->tls13_sem &&
		    session->security_parameters.session_id_size == 0) {

			/* Under TLS1.3 we generate a random session ID to make
			 * the TLS1.3 session look like a resumed TLS1.2 session */
			ret = _gnutls_generate_session_id(session->security_parameters.
							  session_id,
							  &session->security_parameters.
							  session_id_size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}
#endif

		/* Copy the Session ID - if any
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
			ret = _gnutls_buffer_append_data_prefix(&extdata, 8,
								session->internals.dtls.dcookie.data,
								session->internals.dtls.dcookie.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			_gnutls_free_datum(&session->internals.dtls.dcookie);
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
		     session->internals.priorities->no_extensions != 0)) {
			add_sr_scsv = 1;
		}
#endif
		ret = _gnutls_get_client_ciphersuites(session, &extdata, min_ver, add_sr_scsv);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Copy the compression methods.
		 */
		ret = append_null_comp(session, &extdata);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Generate and copy TLS extensions.
		 */
		if (session->internals.priorities->no_extensions == 0) {
			if (_gnutls_version_has_extensions(hver)) {
				type = GNUTLS_EXT_ANY;
			} else {
				type = GNUTLS_EXT_MANDATORY;
			}

			ret =
			    _gnutls_gen_hello_extensions(session, &extdata,
							 GNUTLS_EXT_FLAG_CLIENT_HELLO,
							 type);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}

		bufel = _gnutls_buffer_to_mbuffer(&extdata);
	}

	ret = _gnutls_send_handshake(session, bufel,
				     GNUTLS_HANDSHAKE_CLIENT_HELLO);

	return ret;

 cleanup:
	_gnutls_buffer_clear(&extdata);
	return ret;
}

int _gnutls_send_server_hello(gnutls_session_t session, int again)
{
	mbuffer_st *bufel = NULL;
	gnutls_buffer_st buf;
	int ret;
	uint8_t session_id_len =
	    session->security_parameters.session_id_size;
	char tmpbuf[2 * GNUTLS_MAX_SESSION_ID_SIZE + 1];
	const version_entry_st *vers;
	uint8_t vbytes[2];
	unsigned extflag = 0;
	gnutls_ext_parse_type_t etype;

	_gnutls_buffer_init(&buf);

	if (again == 0) {
		vers = get_version(session);
		if (unlikely(vers == NULL || session->security_parameters.cs == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (vers->tls13_sem) {
			vbytes[0] = 0x03; /* TLS1.2 */
			vbytes[1] = 0x03;
			extflag |= GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO;
		} else {
			vbytes[0] = vers->major;
			vbytes[1] = vers->minor;
			extflag |= GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO;
		}

		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		ret = _gnutls_buffer_append_data(&buf, vbytes, 2);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		ret = _gnutls_buffer_append_data(&buf,
						 session->security_parameters.server_random,
						 GNUTLS_RANDOM_SIZE);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		ret = _gnutls_buffer_append_data_prefix(&buf, 8,
							session->security_parameters.session_id,
							session_id_len);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		_gnutls_handshake_log("HSK[%p]: SessionID: %s\n", session,
				      _gnutls_bin2hex(session->
						      security_parameters.session_id,
						      session_id_len, tmpbuf,
						      sizeof(tmpbuf), NULL));

		ret = _gnutls_buffer_append_data(&buf,
						 session->security_parameters.cs->id,
						 2);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		/* compression */
		ret = _gnutls_buffer_append_prefix(&buf, 8, 0);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		if (!vers->tls13_sem && session->internals.resumed != RESUME_FALSE)
			etype = GNUTLS_EXT_MANDATORY;
		else
			etype = GNUTLS_EXT_ANY;
		ret =
		    _gnutls_gen_hello_extensions(session, &buf, extflag, etype);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		if (vers->tls13_sem) {
			/* Under TLS1.3, the session ID is used for different purposes than
			 * the TLS1.0 session ID. Ensure that there is an internally set
			 * value which the server will see on the original and resumed sessions */
			ret = _gnutls_generate_session_id(session->security_parameters.
							  session_id,
							  &session->security_parameters.
							  session_id_size);
			if (ret < 0) {
				gnutls_assert();
				goto fail;
			}
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	ret =
	    _gnutls_send_handshake(session, bufel,
				   GNUTLS_HANDSHAKE_SERVER_HELLO);

fail:
	_gnutls_buffer_clear(&buf);
	return ret;
}

static int
recv_hello_verify_request(gnutls_session_t session,
				  uint8_t * data, int datalen)
{
	ssize_t len = datalen;
	size_t pos = 0;
	uint8_t cookie_len;
	unsigned int nb_verifs;
	int ret;

	if (!IS_DTLS(session)) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET;
	}

	nb_verifs = ++session->internals.dtls.hsk_hello_verify_requests;
	if (nb_verifs >= MAX_HANDSHAKE_HELLO_VERIFY_REQUESTS) {
		/* The server is either buggy, malicious or changing cookie
		   secrets _way_ too fast. */
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET;
	}

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

	gnutls_free(session->internals.dtls.dcookie.data);
	ret = _gnutls_set_datum(&session->internals.dtls.dcookie, &data[pos], cookie_len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (len != 0) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	/* reset handshake hash buffers */
	handshake_hash_buffer_reset(session);
	/* reset extensions used in previous hello */
	session->internals.used_exts = 0;

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
 * This function can only be called in server side, and
 * instructs a TLS 1.2 or earlier client to renegotiate
 * parameters (perform a handshake), by sending a 
 * hello request message.
 *
 * If this function succeeds, the calling application
 * should call gnutls_record_recv() until %GNUTLS_E_REHANDSHAKE
 * is returned to clear any pending data. If the %GNUTLS_E_REHANDSHAKE
 * error code is not seen, then the handshake request was
 * not followed by the peer (the TLS protocol does not require
 * the client to do, and such compliance should be handled
 * by the application protocol).
 *
 * Once the %GNUTLS_E_REHANDSHAKE error code is seen, the
 * calling application should proceed to calling
 * gnutls_handshake() to negotiate the new
 * parameters.
 *
 * If the client does not wish to renegotiate parameters he 
 * may reply with an alert message, and in that case the return code seen
 * by subsequent gnutls_record_recv() will be
 * %GNUTLS_E_WARNING_ALERT_RECEIVED with the specific alert being
 * %GNUTLS_A_NO_RENEGOTIATION.  A client may also choose to ignore
 * this request.
 *
 * Under TLS 1.3 this function is equivalent to gnutls_session_key_update()
 * with the %GNUTLS_KU_PEER flag. In that case subsequent calls to
 * gnutls_record_recv() will not return %GNUTLS_E_REHANDSHAKE, and
 * calls to gnutls_handshake() in server side are a no-op.
 *
 * This function always fails with %GNUTLS_E_INVALID_REQUEST when
 * called in client side.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 **/
int gnutls_rehandshake(gnutls_session_t session)
{
	int ret;
	const version_entry_st *vers = get_version(session);

	/* only server sends that handshake packet */
	if (session->security_parameters.entity == GNUTLS_CLIENT)
		return GNUTLS_E_INVALID_REQUEST;

	if (vers->tls13_sem) {
		return gnutls_session_key_update(session, GNUTLS_KU_PEER);
	}

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
	mbuffer_st *bufel = NULL;
	int ret = 0;

	_gnutls_debug_log("EXT[%p]: Sending supplemental data\n", session);

	if (!again) {
		gnutls_buffer_st buf;
		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_gen_supplemental(session, &buf);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_buffer_clear(&buf);
			return ret;
		}

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel,
				      GNUTLS_HANDSHAKE_SUPPLEMENTAL);
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
 * This function performs the handshake of the TLS/SSL protocol, and
 * initializes the TLS session parameters.
 *
 * The non-fatal errors expected by this function are:
 * %GNUTLS_E_INTERRUPTED, %GNUTLS_E_AGAIN, 
 * %GNUTLS_E_WARNING_ALERT_RECEIVED. When this function is called
 * for re-handshake under TLS 1.2 or earlier, the non-fatal error code
 * %GNUTLS_E_GOT_APPLICATION_DATA may also be returned.
 *
 * The former two interrupt the handshake procedure due to the transport
 * layer being interrupted, and the latter because of a "warning" alert that
 * was sent by the peer (it is always a good idea to check any
 * received alerts). On these non-fatal errors call this function again,
 * until it returns 0; cf.  gnutls_record_get_direction() and
 * gnutls_error_is_fatal(). In DTLS sessions the non-fatal error
 * %GNUTLS_E_LARGE_PACKET is also possible, and indicates that
 * the MTU should be adjusted.
 *
 * When this function is called by a server after a rehandshake request
 * under TLS 1.2 or earlier the %GNUTLS_E_GOT_APPLICATION_DATA error code indicates
 * that some data were pending prior to peer initiating the handshake.
 * Under TLS 1.3 this function when called after a successful handshake, is a no-op
 * and always succeeds in server side; in client side this function is
 * equivalent to gnutls_session_key_update() with %GNUTLS_KU_PEER flag.
 *
 * This function handles both full and abbreviated TLS handshakes (resumption).
 * For abbreviated handshakes, in client side, the gnutls_session_set_data()
 * should be called prior to this function to set parameters from a previous session.
 * In server side, resumption is handled by either setting a DB back-end, or setting
 * up keys for session tickets.
 *
 * Returns: %GNUTLS_E_SUCCESS on a successful handshake, otherwise a negative error code.
 **/
int gnutls_handshake(gnutls_session_t session)
{
	const version_entry_st *vers = get_version(session);
	int ret;

	if (unlikely(session->internals.initial_negotiation_completed)) {
		if (vers->tls13_sem) {
			if (session->security_parameters.entity == GNUTLS_CLIENT) {
				return gnutls_session_key_update(session, GNUTLS_KU_PEER);
			} else {
				/* This is a no-op for a server under TLS 1.3, as
				 * a server has already called gnutls_rehandshake()
				 * which performed a key update.
				 */
				return 0;
			}
		}
	}

	if (STATE == STATE0) {
		unsigned int tmo_ms;
		struct timespec *end;
		struct timespec *start;

		/* first call */
		if (session->internals.priorities == NULL ||
		    session->internals.priorities->cs.size == 0)
			return gnutls_assert_val(GNUTLS_E_NO_PRIORITIES_WERE_SET);

		ret =
		    _gnutls_epoch_setup_next(session, 0, NULL);
		if (ret < 0)
			return gnutls_assert_val(ret);

		session->internals.used_exts = 0;
		session->internals.hsk_flags = 0;
		session->internals.handshake_in_progress = 1;
		session->internals.vc_status = -1;
		gnutls_gettime(&session->internals.handshake_start_time);

		tmo_ms = session->internals.handshake_timeout_ms;
		end = &session->internals.handshake_abs_timeout;
		start = &session->internals.handshake_start_time;

		if (tmo_ms && end->tv_sec == 0 && end->tv_nsec == 0) {
			end->tv_sec =
				start->tv_sec + (start->tv_nsec + tmo_ms * 1000000LL) / 1000000000LL;
			end->tv_nsec =
				(start->tv_nsec + tmo_ms * 1000000LL) % 1000000000LL;
		}
	}

	if (session->internals.recv_state == RECV_STATE_FALSE_START) {
		session_invalidate(session);
		return gnutls_assert_val(GNUTLS_E_HANDSHAKE_DURING_FALSE_START);
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
	if (session->internals.recv_state != RECV_STATE_FALSE_START &&
	    session->internals.recv_state != RECV_STATE_EARLY_START) {

		_gnutls_handshake_hash_buffers_clear(session);

		if (IS_DTLS(session) == 0) {
			_gnutls_handshake_io_buffer_clear(session);
		} else {
			_dtls_async_timer_init(session);
		}

		_gnutls_handshake_internal_state_clear(session);

		_gnutls_buffer_clear(&session->internals.record_presend_buffer);

		_gnutls_epoch_bump(session);
	}

	/* Give an estimation of the round-trip under TLS1.3, used by gnutls_session_get_data2() */
	if (!IS_SERVER(session) && vers->tls13_sem) {
		struct timespec handshake_finish_time;
		gnutls_gettime(&handshake_finish_time);

		if (!(session->internals.hsk_flags & HSK_HRR_RECEIVED)) {
			session->internals.ertt = timespec_sub_ms(&handshake_finish_time, &session->internals.handshake_start_time)/2;
		} else {
			session->internals.ertt = timespec_sub_ms(&handshake_finish_time, &session->internals.handshake_start_time)/4;
		}
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

/* Runs the certificate verification callback.
 * side is the side that we verify the certificate
 * from (either GNUTLS_CLIENT or GNUTLS_SERVER).
 */
int _gnutls_run_verify_callback(gnutls_session_t session, unsigned int side)
{
	gnutls_certificate_credentials_t cred;
	int ret, type;

	if (session->internals.hsk_flags & HSK_PSK_SELECTED)
		return 0;

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
	const version_entry_st *ver;

 reset:
	if (STATE >= STATE99)
		return _gnutls13_handshake_client(session);

	switch (STATE) {
	case STATE0:
	case STATE1:
		ret = send_client_hello(session, AGAIN(STATE1));
		STATE = STATE1;
		IMED_RET("send hello", ret, 1);
		FALLTHROUGH;
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
		} else {
			ret =
			    _gnutls_recv_handshake(session,
						   GNUTLS_HANDSHAKE_HELLO_RETRY_REQUEST,
						   1, NULL);
			STATE = STATE2;
			IMED_RET("recv hello retry", ret, 1);

			if (ret == 1) {
				STATE = STATE0;
				return 1;
			}
		}
		FALLTHROUGH;
	case STATE3:
		/* receive the server hello */
		ret =
		    _gnutls_recv_handshake(session,
					   GNUTLS_HANDSHAKE_SERVER_HELLO,
					   0, NULL);
		STATE = STATE3;
		IMED_RET("recv hello", ret, 1);
		FALLTHROUGH;
	case STATE4:
		ver = get_version(session);
		if (ver->tls13_sem) { /* TLS 1.3 state machine */
			STATE = STATE99;
			goto reset;
		}

		ret = _gnutls_ext_sr_verify(session);
		STATE = STATE4;
		IMED_RET_FATAL("recv hello", ret, 0);
		FALLTHROUGH;
	case STATE5:
		if (session->security_parameters.do_recv_supplemental) {
			ret = _gnutls_recv_supplemental(session);
			STATE = STATE5;
			IMED_RET("recv supplemental", ret, 1);
		}
		FALLTHROUGH;
	case STATE6:
		/* RECV CERTIFICATE */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_server_certificate(session);
		STATE = STATE6;
		IMED_RET("recv server certificate", ret, 1);
		FALLTHROUGH;
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
		FALLTHROUGH;
	case STATE8:
		ret = _gnutls_run_verify_callback(session, GNUTLS_CLIENT);
		STATE = STATE8;
		if (ret < 0)
			return gnutls_assert_val(ret);

		FALLTHROUGH;
	case STATE9:
		/* receive the server key exchange */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_server_kx_message(session);
		STATE = STATE9;
		IMED_RET("recv server kx message", ret, 1);
		FALLTHROUGH;
	case STATE10:
		/* receive the server certificate request - if any 
		 */

		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_server_crt_request(session);
		STATE = STATE10;
		IMED_RET("recv server certificate request message", ret,
			 1);
		FALLTHROUGH;
	case STATE11:
		/* receive the server hello done */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_recv_handshake(session,
						   GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
						   0, NULL);
		STATE = STATE11;
		IMED_RET("recv server hello done", ret, 1);
		FALLTHROUGH;
	case STATE12:
		if (session->security_parameters.do_send_supplemental) {
			ret =
			    _gnutls_send_supplemental(session,
						      AGAIN(STATE12));
			STATE = STATE12;
			IMED_RET("send supplemental", ret, 0);
		}
		FALLTHROUGH;
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
		FALLTHROUGH;
	case STATE14:
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_send_client_kx_message(session,
							   AGAIN(STATE14));
		STATE = STATE14;
		IMED_RET("send client kx", ret, 0);
		FALLTHROUGH;
	case STATE15:
		/* send client certificate verify */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_send_client_certificate_verify(session,
								   AGAIN
								   (STATE15));
		STATE = STATE15;
		IMED_RET("send client certificate verify", ret, 1);
		FALLTHROUGH;
	case STATE16:
		STATE = STATE16;
		if (session->internals.resumed == RESUME_FALSE) {
			ret = send_handshake_final(session, TRUE);
			IMED_RET("send handshake final 2", ret, 1);
		} else {
			ret = _gnutls_recv_new_session_ticket(session);
			IMED_RET("recv handshake new session ticket", ret,
				 1);
		}
		FALLTHROUGH;
	case STATE17:
		STATE = STATE17;
		if (session->internals.resumed == RESUME_FALSE && (session->internals.flags & GNUTLS_ENABLE_FALSE_START) && can_send_false_start(session)) {
			session->internals.hsk_flags |= HSK_FALSE_START_USED;
			session->internals.recv_state = RECV_STATE_FALSE_START;
			/* complete this phase of the handshake. We
			 * should be called again by gnutls_record_recv()
			 */
			STATE = STATE18;
			gnutls_assert();

			return 0;
		}
		FALLTHROUGH;
	case STATE18:
		STATE = STATE18;

		if (session->internals.resumed == RESUME_FALSE) {
			ret = _gnutls_recv_new_session_ticket(session);
			IMED_RET("recv handshake new session ticket", ret,
				 1);
		} else {
			ret = recv_handshake_final(session, TRUE);
			IMED_RET("recv handshake final", ret, 1);
		}
		FALLTHROUGH;
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
		FALLTHROUGH;
	default:
		break;
	}

	/* explicitly reset any false start flags */
	gnutls_mutex_lock(&session->internals.post_negotiation_lock);
	session->internals.initial_negotiation_completed = 1;
	session->internals.recv_state = RECV_STATE_0;
	gnutls_mutex_unlock(&session->internals.post_negotiation_lock);

	return 0;
}



/* This function is to be called if the handshake was successfully 
 * completed. This sends a Change Cipher Spec packet to the peer.
 */
ssize_t _gnutls_send_change_cipher_spec(gnutls_session_t session, int again)
{
	uint8_t *data;
	mbuffer_st *bufel;
	int ret;
	const version_entry_st *vers;

	if (again == 0) {
		bufel = _gnutls_handshake_alloc(session, 3); /* max for DTLS0.9 */
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

		ret = _gnutls_call_hook_func(session, GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC, GNUTLS_HOOK_PRE, 0,
					     data, 1);
		if (ret < 0) {
			_mbuffer_xfree(&bufel);
			return gnutls_assert_val(ret);
		}

		ret =
		    _gnutls_handshake_io_cache_int(session,
						   GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC,
						   bufel);
		if (ret < 0) {
			_mbuffer_xfree(&bufel);
			return gnutls_assert_val(ret);
		}

		ret = _gnutls_call_hook_func(session, GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC, GNUTLS_HOOK_POST, 0,
					     data, 1);
		if (ret < 0) {
			return gnutls_assert_val(ret);
		}

		/* under TLS 1.3, CCS may be immediately followed by
		 * receiving ClientHello thus cannot be cached */
		if (vers->tls13_sem) {
			ret = _gnutls_handshake_io_write_flush(session);
			if (ret < 0)
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
		ret = _gnutls_send_change_cipher_spec(session, FAGAIN(STATE1));
		FINAL_STATE = STATE0;

		if (ret < 0) {
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

		FALLTHROUGH;
	case STATE2:
		/* send the finished message */
		ret = _gnutls_send_finished(session, FAGAIN(STATE2));
		FINAL_STATE = STATE2;
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		FINAL_STATE = STATE0;
		FALLTHROUGH;
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
		FALLTHROUGH;
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
			gnutls_assert();
			return ret;
		}
		FINAL_STATE = STATE0;
		FALLTHROUGH;
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
	const version_entry_st *ver;

 reset:

	if (STATE >= STATE90)
		return _gnutls13_handshake_server(session);

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

		if (ret == GNUTLS_E_NO_COMMON_KEY_SHARE) {
			STATE = STATE90;
			session->internals.hsk_flags |= HSK_HRR_SENT;
			goto reset;
		}

		IMED_RET("recv hello", ret, 1);
		FALLTHROUGH;
	case STATE2:

		ret = _gnutls_ext_sr_verify(session);
		STATE = STATE2;
		IMED_RET_FATAL("recv hello", ret, 0);
		FALLTHROUGH;
	case STATE3:
		ret = _gnutls_send_server_hello(session, AGAIN(STATE3));
		STATE = STATE3;
		IMED_RET("send hello", ret, 1);

		ver = get_version(session);
		if (ver->tls13_sem) { /* TLS 1.3 state machine */
			STATE = STATE99;
			goto reset;
		}

		FALLTHROUGH;
	case STATE4:
		if (session->security_parameters.do_send_supplemental) {
			ret =
			    _gnutls_send_supplemental(session,
						      AGAIN(STATE4));
			STATE = STATE4;
			IMED_RET("send supplemental data", ret, 0);
		}
		/* SEND CERTIFICATE + KEYEXCHANGE + CERTIFICATE_REQUEST */
		FALLTHROUGH;
	case STATE5:
		/* NOTE: these should not be send if we are resuming */

		if (session->internals.resumed == RESUME_FALSE)
			ret =
			    _gnutls_send_server_certificate(session,
							    AGAIN(STATE5));
		STATE = STATE5;
		IMED_RET("send server certificate", ret, 0);
		FALLTHROUGH;
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
		FALLTHROUGH;
	case STATE7:
		/* send server key exchange (A) */
		if (session->internals.resumed == RESUME_FALSE)
			ret =
			    _gnutls_send_server_kx_message(session,
							   AGAIN(STATE7));
		STATE = STATE7;
		IMED_RET("send server kx", ret, 0);
		FALLTHROUGH;
	case STATE8:
		/* Send certificate request - if requested to */
		if (session->internals.resumed == RESUME_FALSE)
			ret =
			    _gnutls_send_server_crt_request(session,
							    AGAIN(STATE8));
		STATE = STATE8;
		IMED_RET("send server cert request", ret, 0);
		FALLTHROUGH;
	case STATE9:
		/* send the server hello done */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_send_empty_handshake(session,
							 GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
							 AGAIN(STATE9));
		STATE = STATE9;
		IMED_RET("send server hello done", ret, 1);
		FALLTHROUGH;
	case STATE10:
		if (session->security_parameters.do_recv_supplemental) {
			ret = _gnutls_recv_supplemental(session);
			STATE = STATE10;
			IMED_RET("recv client supplemental", ret, 1);
		}
		/* RECV CERTIFICATE + KEYEXCHANGE + CERTIFICATE_VERIFY */
		FALLTHROUGH;
	case STATE11:
		/* receive the client certificate message */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_client_certificate(session);
		STATE = STATE11;
		IMED_RET("recv client certificate", ret, 1);
		FALLTHROUGH;
	case STATE12:
		ret = _gnutls_run_verify_callback(session, GNUTLS_SERVER);
		STATE = STATE12;
		if (ret < 0)
			return gnutls_assert_val(ret);
		FALLTHROUGH;
	case STATE13:
		/* receive the client key exchange message */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret = _gnutls_recv_client_kx_message(session);
		STATE = STATE13;
		IMED_RET("recv client kx", ret, 1);
		FALLTHROUGH;
	case STATE14:
		/* receive the client certificate verify message */
		if (session->internals.resumed == RESUME_FALSE)	/* if we are not resuming */
			ret =
			    _gnutls_recv_client_certificate_verify_message
			    (session);
		STATE = STATE14;
		IMED_RET("recv client certificate verify", ret, 1);
		FALLTHROUGH;
	case STATE15:
		STATE = STATE15;
		if (session->internals.resumed == RESUME_FALSE) {	/* if we are not resuming */
			ret = recv_handshake_final(session, TRUE);
			IMED_RET("recv handshake final", ret, 1);
		} else {
			ret = send_handshake_final(session, TRUE);
			IMED_RET("send handshake final 2", ret, 1);
		}
		FALLTHROUGH;
	case STATE16:
		ret =
		    _gnutls_send_new_session_ticket(session,
						    AGAIN(STATE16));
		STATE = STATE16;
		IMED_RET("send handshake new session ticket", ret, 0);
		FALLTHROUGH;
	case STATE17:
		STATE = STATE17;
		if (session->internals.resumed == RESUME_FALSE) {	/* if we are not resuming */
			ret = send_handshake_final(session, FALSE);
			IMED_RET("send handshake final", ret, 1);

			if (session->security_parameters.entity ==
			    GNUTLS_SERVER
			    && !(session->internals.hsk_flags & HSK_TLS12_TICKET_SENT)) {
				/* if no ticket, save session data */
				_gnutls_server_register_current_session
				    (session);
			}
		} else {
			ret = recv_handshake_final(session, FALSE);
			IMED_RET("recv handshake final 2", ret, 1);
		}

		STATE = STATE0;
		FALLTHROUGH;
	default:
		break;
	}

	/* no lock of post_negotiation_lock is required here as this is not run
	 * after handshake */
	session->internals.initial_negotiation_completed = 1;

	return _gnutls_check_id_for_change(session);
}

int _gnutls_generate_session_id(uint8_t * session_id, uint8_t *len)
{
	int ret;

	*len = GNUTLS_DEF_SESSION_ID_SIZE;

	ret =
	    gnutls_rnd(GNUTLS_RND_NONCE, session_id,
			GNUTLS_DEF_SESSION_ID_SIZE);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
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
 */
int _gnutls_handshake_get_session_hash(gnutls_session_t session, gnutls_datum_t *shash)
{
	const version_entry_st *ver = get_version(session);
	int ret;
	uint8_t concat[2*MAX_HASH_SIZE];

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (session->internals.handshake_hash_buffer_client_kx_len == 0 ||
	    (session->internals.handshake_hash_buffer.length <
	    session->internals.handshake_hash_buffer_client_kx_len)) {
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	ret =
	    _gnutls_hash_fast((gnutls_digest_algorithm_t)session->security_parameters.prf->id,
			      session->internals.handshake_hash_buffer.
			      data,
			      session->internals.handshake_hash_buffer_client_kx_len,
			      concat);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _gnutls_set_datum(shash, concat, session->security_parameters.prf->output_size);
}
