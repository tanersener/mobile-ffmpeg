/*
 * Copyright (C) 2002-2016 Free Software Foundation, Inc.
 * Copyright (C) 2014-2016 Nikos Mavrogiannopoulos
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

/* Functions to manipulate the session (gnutls_int.h), and some other stuff
 * are included here. The file's name is traditionally gnutls_state even if the
 * state has been renamed to session.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <auth.h>
#include <num.h>
#include <datum.h>
#include <db.h>
#include <record.h>
#include <handshake.h>
#include <dh.h>
#include <buffers.h>
#include <mbuffers.h>
#include <state.h>
#include <constate.h>
#include <auth/cert.h>
#include <auth/anon.h>
#include <auth/psk.h>
#include <algorithms.h>
#include <hello_ext.h>
#include <system.h>
#include <random.h>
#include <fips.h>
#include <intprops.h>
#include <gnutls/dtls.h>
#include "dtls.h"
#include "tls13/session_ticket.h"
#include "ext/cert_types.h"
#include "locks.h"
#include "kx.h"

/* to be used by supplemental data support to disable TLS1.3
 * when supplemental data have been globally registered */
unsigned _gnutls_disable_tls13 = 0;

/* These should really be static, but src/tests.c calls them.  Make
   them public functions?  */
void
_gnutls_rsa_pms_set_version(gnutls_session_t session,
			    unsigned char major, unsigned char minor);

/**
 * gnutls_cipher_get:
 * @session: is a #gnutls_session_t type.
 *
 * Get the currently used cipher.
 *
 * Returns: the currently used cipher, a #gnutls_cipher_algorithm_t
 *   type.
 **/
gnutls_cipher_algorithm_t gnutls_cipher_get(gnutls_session_t session)
{
	record_parameters_st *record_params;
	int ret;

	ret =
	    _gnutls_epoch_get(session, EPOCH_READ_CURRENT, &record_params);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_CIPHER_NULL);

	return record_params->cipher->id;
}

/**
 * gnutls_certificate_type_get:
 * @session: is a #gnutls_session_t type.
 *
 * This function returns the type of the certificate that is negotiated
 * for this side to send to the peer. The certificate type is by default
 * X.509, unless an alternative certificate type is enabled by
 * gnutls_init() and negotiated during the session.
 *
 * Resumed sessions will return the certificate type that was negotiated
 * and used in the original session.
 *
 * As of version 3.6.4 it is recommended to use
 * gnutls_certificate_type_get2() which is more fine-grained.
 *
 * Returns: the currently used #gnutls_certificate_type_t certificate
 *   type as negotiated for 'our' side of the connection.
 **/
gnutls_certificate_type_t
gnutls_certificate_type_get(gnutls_session_t session)
{
	return gnutls_certificate_type_get2(session, GNUTLS_CTYPE_OURS);
}

/**
 * gnutls_certificate_type_get2:
 * @session: is a #gnutls_session_t type.
 * @target: is a #gnutls_ctype_target_t type.
 *
 * This function returns the type of the certificate that a side
 * is negotiated to use.  The certificate type is by default X.509,
 * unless an alternative certificate type is enabled by gnutls_init() and
 * negotiated during the session.
 *
 * The @target parameter specifies whether to request the negotiated
 * certificate type for the client (%GNUTLS_CTYPE_CLIENT),
 * or for the server (%GNUTLS_CTYPE_SERVER). Additionally, in P2P mode
 * connection set up where you don't know in advance who will be client
 * and who will be server you can use the flag (%GNUTLS_CTYPE_OURS) and
 * (%GNUTLS_CTYPE_PEERS) to retrieve the corresponding certificate types.
 *
 * Resumed sessions will return the certificate type that was negotiated
 * and used in the original session. That is, this function can be used
 * to reliably determine the type of the certificate returned by
 * gnutls_certificate_get_peers().
 *
 * Returns: the currently used #gnutls_certificate_type_t certificate
 *   type for the client or the server.
 *
 * Since: 3.6.4
 **/
gnutls_certificate_type_t
gnutls_certificate_type_get2(gnutls_session_t session,
			     gnutls_ctype_target_t target)
{
	/* We want to inline this function so therefore
	 * we've defined it in gnutls_int.h */
	return get_certificate_type(session, target);
}

/**
 * gnutls_kx_get:
 * @session: is a #gnutls_session_t type.
 *
 * Get the currently used key exchange algorithm.
 *
 * This function will return %GNUTLS_KX_ECDHE_RSA, or %GNUTLS_KX_DHE_RSA
 * under TLS 1.3, to indicate an elliptic curve DH key exchange or
 * a finite field one. The precise group used is available
 * by calling gnutls_group_get() instead.
 *
 * Returns: the key exchange algorithm used in the last handshake, a
 *   #gnutls_kx_algorithm_t value.
 **/
gnutls_kx_algorithm_t gnutls_kx_get(gnutls_session_t session)
{
	if (session->security_parameters.cs == 0)
		return 0;

	if (session->security_parameters.cs->kx_algorithm == 0) { /* TLS 1.3 */
		const version_entry_st *ver = get_version(session);
		const gnutls_group_entry_st *group = get_group(session);

		if (ver->tls13_sem) {
			if (session->internals.hsk_flags & HSK_PSK_SELECTED) {
				if (group) {
					if (group->pk == GNUTLS_PK_DH)
						return GNUTLS_KX_DHE_PSK;
					else
						return GNUTLS_KX_ECDHE_PSK;
				} else {
					return GNUTLS_KX_PSK;
				}
			} else if (group) {
				if (group->pk == GNUTLS_PK_DH)
					return GNUTLS_KX_DHE_RSA;
				else
					return GNUTLS_KX_ECDHE_RSA;
			}
		}
	}

	return session->security_parameters.cs->kx_algorithm;
}

/**
 * gnutls_mac_get:
 * @session: is a #gnutls_session_t type.
 *
 * Get the currently used MAC algorithm.
 *
 * Returns: the currently used mac algorithm, a
 *   #gnutls_mac_algorithm_t value.
 **/
gnutls_mac_algorithm_t gnutls_mac_get(gnutls_session_t session)
{
	record_parameters_st *record_params;
	int ret;

	ret =
	    _gnutls_epoch_get(session, EPOCH_READ_CURRENT, &record_params);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_MAC_NULL);

	return record_params->mac->id;
}

/**
 * gnutls_compression_get:
 * @session: is a #gnutls_session_t type.
 *
 * Get the currently used compression algorithm.
 *
 * Returns: the currently used compression method, a
 *   #gnutls_compression_method_t value.
 **/
gnutls_compression_method_t
gnutls_compression_get(gnutls_session_t session)
{
	return GNUTLS_COMP_NULL;
}

/**
 * gnutls_prf_hash_get:
 * @session: is a #gnutls_session_t type.
 *
 * Get the currently used hash algorithm. In TLS 1.3, the hash
 * algorithm is used for both the key derivation function and
 * handshake message authentication code. In TLS 1.2, it matches the
 * hash algorithm used for PRF.
 *
 * Returns: the currently used hash algorithm, a
 *    #gnutls_digest_algorithm_t value.
 *
 * Since: 3.6.13
 **/
gnutls_digest_algorithm_t
gnutls_prf_hash_get(const gnutls_session_t session)
{
	if (session->security_parameters.prf == NULL)
		return gnutls_assert_val(GNUTLS_DIG_UNKNOWN);

	if (session->security_parameters.prf->id >= GNUTLS_MAC_AEAD)
		return gnutls_assert_val(GNUTLS_DIG_UNKNOWN);

	return (gnutls_digest_algorithm_t)session->security_parameters.prf->id;
}

void reset_binders(gnutls_session_t session)
{
	_gnutls_free_temp_key_datum(&session->key.binders[0].psk);
	_gnutls_free_temp_key_datum(&session->key.binders[1].psk);
	memset(session->key.binders, 0, sizeof(session->key.binders));
}

/* Check whether certificate credentials of type @cert_type are set
 * for the current session.
 */
static bool _gnutls_has_cert_credentials(gnutls_session_t session,
						gnutls_certificate_type_t cert_type)
{
	unsigned i;
	unsigned cert_found = 0;
	gnutls_certificate_credentials_t cred;

	/* First, check for certificate credentials. If we have no certificate
	 * credentials set then we don't support certificates at all.
	 */
	cred = (gnutls_certificate_credentials_t)
			_gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);

	if (cred == NULL)
		return false;

	/* There are credentials initialized. Now check whether we can find
	 * pre-set certificates of the required type, but only if we don't
	 * use the callback functions.
	 */
	if (cred->get_cert_callback3 == NULL) {
		for (i = 0; i < cred->ncerts; i++) {
			if (cred->certs[i].cert_list[0].type == cert_type) {
				cert_found = 1;
				break;
			}
		}

		if (cert_found == 0) {
			/* No matching certificate found. */
			return false;
		}
	}

	return true; // OK
}

/* Check if the given certificate type is supported.
 * This means that it is enabled by the priority functions,
 * and in some cases a matching certificate exists. A check for
 * the latter can be toggled via the parameter @check_credentials.
 */
int
_gnutls_session_cert_type_supported(gnutls_session_t session,
				    gnutls_certificate_type_t cert_type,
				    bool check_credentials,
				    gnutls_ctype_target_t target)
{
	unsigned i;
	priority_st* ctype_priorities;

	// Check whether this cert type is enabled by the application
	if (!is_cert_type_enabled(session, cert_type))
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE);

	// Perform a credentials check if requested
	if (check_credentials)	{
		if (!_gnutls_has_cert_credentials(session, cert_type))
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE);
	}

	/* So far so good. We have the required credentials (if needed).
	 * Now check whether we are allowed to use them according to our
	 * priorities.
	 */
	// Which certificate type should we query?
	switch (target) {
		case GNUTLS_CTYPE_CLIENT:
			ctype_priorities =
					&(session->internals.priorities->client_ctype);
			break;
		case GNUTLS_CTYPE_SERVER:
			ctype_priorities =
					&(session->internals.priorities->server_ctype);
			break;
		default:
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	// No explicit priorities set, and default ctype is asked
	if (ctype_priorities->num_priorities == 0
	    && cert_type == DEFAULT_CERT_TYPE)
		return 0;

	/* Now lets find out whether our cert type is in our priority
	 * list, i.e. set of allowed cert types.
	 */
	for (i = 0; i < ctype_priorities->num_priorities; i++) {
		if (ctype_priorities->priorities[i] == cert_type)
			return 0;
	}

	return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
}

static void deinit_keys(gnutls_session_t session)
{
	const version_entry_st *vers = get_version(session);

	if (vers == NULL)
		return;

	gnutls_pk_params_release(&session->key.kshare.ecdhx_params);
	gnutls_pk_params_release(&session->key.kshare.ecdh_params);
	gnutls_pk_params_release(&session->key.kshare.dh_params);

	if (!vers->tls13_sem && session->key.binders[0].prf == NULL) {
		gnutls_pk_params_release(&session->key.proto.tls12.ecdh.params);
		gnutls_pk_params_release(&session->key.proto.tls12.dh.params);
		zrelease_temp_mpi_key(&session->key.proto.tls12.ecdh.x);
		zrelease_temp_mpi_key(&session->key.proto.tls12.ecdh.y);
		_gnutls_free_temp_key_datum(&session->key.proto.tls12.ecdh.raw);

		zrelease_temp_mpi_key(&session->key.proto.tls12.dh.client_Y);

		/* SRP */
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.srp_p);
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.srp_g);
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.srp_key);

		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.u);
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.a);
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.x);
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.A);
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.B);
		zrelease_temp_mpi_key(&session->key.proto.tls12.srp.b);
	} else {
		gnutls_memset(session->key.proto.tls13.temp_secret, 0,
			      sizeof(session->key.proto.tls13.temp_secret));
	}

	reset_binders(session);
	_gnutls_free_temp_key_datum(&session->key.key);
}

/* An internal version of _gnutls_handshake_internal_state_clear(),
 * it will not attempt to deallocate, only initialize */
static void handshake_internal_state_clear1(gnutls_session_t session)
{
	/* by default no selected certificate */
	session->internals.adv_version_major = 0;
	session->internals.adv_version_minor = 0;
	session->internals.direction = 0;

	/* use out of band data for the last
	 * handshake messages received.
	 */
	session->internals.last_handshake_in = -1;
	session->internals.last_handshake_out = -1;

	session->internals.resumable = RESUME_TRUE;

	session->internals.handshake_suspicious_loops = 0;
	session->internals.dtls.hsk_read_seq = 0;
	session->internals.dtls.hsk_write_seq = 0;

	session->internals.cand_ec_group = 0;
	session->internals.cand_dh_group = 0;

	session->internals.hrr_cs[0] = CS_INVALID_MAJOR;
	session->internals.hrr_cs[1] = CS_INVALID_MINOR;
}

/* This function will clear all the variables in internals
 * structure within the session, which depend on the current handshake.
 * This is used to allow further handshakes.
 */
void _gnutls_handshake_internal_state_clear(gnutls_session_t session)
{
	handshake_internal_state_clear1(session);

	_gnutls_handshake_hash_buffers_clear(session);
	deinit_keys(session);

	_gnutls_epoch_gc(session);

	session->internals.handshake_abs_timeout.tv_sec = 0;
	session->internals.handshake_abs_timeout.tv_nsec = 0;
	session->internals.handshake_in_progress = 0;

	session->internals.tfo.connect_addrlen = 0;
	session->internals.tfo.connect_only = 0;
	session->internals.early_data_received = 0;
}

/**
 * gnutls_init:
 * @session: is a pointer to a #gnutls_session_t type.
 * @flags: indicate if this session is to be used for server or client.
 *
 * This function initializes the provided session. Every
 * session must be initialized before use, and must be deinitialized
 * after used by calling gnutls_deinit().
 *
 * @flags can be any combination of flags from %gnutls_init_flags_t.
 *
 * Note that since version 3.1.2 this function enables some common
 * TLS extensions such as session tickets and OCSP certificate status
 * request in client side by default. To prevent that use the %GNUTLS_NO_EXTENSIONS
 * flag.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int gnutls_init(gnutls_session_t * session, unsigned int flags)
{
	int ret;

	FAIL_IF_LIB_ERROR;

	*session = gnutls_calloc(1, sizeof(struct gnutls_session_int));
	if (*session == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	ret = gnutls_mutex_init(&(*session)->internals.post_negotiation_lock);
	if (ret < 0) {
		gnutls_assert();
		gnutls_free(*session);
		return ret;
	}

	ret = gnutls_mutex_init(&(*session)->internals.epoch_lock);
	if (ret < 0) {
		gnutls_assert();
		gnutls_mutex_deinit(&(*session)->internals.post_negotiation_lock);
		gnutls_free(*session);
		return ret;
	}

	ret = _gnutls_epoch_setup_next(*session, 1, NULL);
	if (ret < 0) {
		gnutls_mutex_deinit(&(*session)->internals.post_negotiation_lock);
		gnutls_mutex_deinit(&(*session)->internals.epoch_lock);
		gnutls_free(*session);
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}
	_gnutls_epoch_bump(*session);

	(*session)->security_parameters.entity =
	    (flags & GNUTLS_SERVER ? GNUTLS_SERVER : GNUTLS_CLIENT);

	/* the default certificate type for TLS */
	(*session)->security_parameters.client_ctype = DEFAULT_CERT_TYPE;
	(*session)->security_parameters.server_ctype = DEFAULT_CERT_TYPE;

	/* Initialize buffers */
	_gnutls_buffer_init(&(*session)->internals.handshake_hash_buffer);
	_gnutls_buffer_init(&(*session)->internals.post_handshake_hash_buffer);
	_gnutls_buffer_init(&(*session)->internals.hb_remote_data);
	_gnutls_buffer_init(&(*session)->internals.hb_local_data);
	_gnutls_buffer_init(&(*session)->internals.record_presend_buffer);
	_gnutls_buffer_init(&(*session)->internals.record_key_update_buffer);
	_gnutls_buffer_init(&(*session)->internals.reauth_buffer);

	_mbuffer_head_init(&(*session)->internals.record_buffer);
	_mbuffer_head_init(&(*session)->internals.record_send_buffer);
	_mbuffer_head_init(&(*session)->internals.record_recv_buffer);
	_mbuffer_head_init(&(*session)->internals.early_data_recv_buffer);
	_gnutls_buffer_init(&(*session)->internals.early_data_presend_buffer);

	_mbuffer_head_init(&(*session)->internals.handshake_send_buffer);
	_gnutls_handshake_recv_buffer_init(*session);

	(*session)->internals.expire_time = DEFAULT_EXPIRE_TIME;

	/* Ticket key rotation - set the default X to 3 times the ticket expire time */
	(*session)->key.totp.last_result = 0;

	gnutls_handshake_set_max_packet_length((*session),
					       MAX_HANDSHAKE_PACKET_SIZE);

	/* set the socket pointers to -1;
	 */
	(*session)->internals.transport_recv_ptr =
	    (gnutls_transport_ptr_t) - 1;
	(*session)->internals.transport_send_ptr =
	    (gnutls_transport_ptr_t) - 1;

	/* set the default maximum record size for TLS
	 */
	(*session)->security_parameters.max_record_recv_size =
	    DEFAULT_MAX_RECORD_SIZE;
	(*session)->security_parameters.max_record_send_size =
	    DEFAULT_MAX_RECORD_SIZE;
	(*session)->security_parameters.max_user_record_recv_size =
	    DEFAULT_MAX_RECORD_SIZE;
	(*session)->security_parameters.max_user_record_send_size =
	    DEFAULT_MAX_RECORD_SIZE;

	/* set the default early data size for TLS
	 */
	if ((*session)->security_parameters.entity == GNUTLS_SERVER) {
		(*session)->security_parameters.max_early_data_size =
			DEFAULT_MAX_EARLY_DATA_SIZE;
	} else {
		(*session)->security_parameters.max_early_data_size =
			UINT32_MAX;
	}

	/* everything else not initialized here is initialized
	 * as NULL or 0. This is why calloc is used.
	 */

	handshake_internal_state_clear1(*session);

#ifdef HAVE_WRITEV
#ifdef MSG_NOSIGNAL
	if (flags & GNUTLS_NO_SIGNAL)
		gnutls_transport_set_vec_push_function(*session, system_writev_nosignal);
	else
#endif
		gnutls_transport_set_vec_push_function(*session, system_writev);
#else
	gnutls_transport_set_push_function(*session, system_write);
#endif
	(*session)->internals.pull_timeout_func = gnutls_system_recv_timeout;
	(*session)->internals.pull_func = system_read;
	(*session)->internals.errno_func = system_errno;

	(*session)->internals.saved_username_size = -1;

	/* heartbeat timeouts */
	(*session)->internals.hb_retrans_timeout_ms = 1000;
	(*session)->internals.hb_total_timeout_ms = 60000;

	if (flags & GNUTLS_DATAGRAM) {
		(*session)->internals.dtls.mtu = DTLS_DEFAULT_MTU;
		(*session)->internals.transport = GNUTLS_DGRAM;

		gnutls_dtls_set_timeouts(*session, DTLS_RETRANS_TIMEOUT, 60000);
	} else {
		(*session)->internals.transport = GNUTLS_STREAM;
	}

	/* Enable useful extensions */
	if ((flags & GNUTLS_CLIENT) && !(flags & GNUTLS_NO_EXTENSIONS)) {
#ifdef ENABLE_OCSP
		gnutls_ocsp_status_request_enable_client(*session, NULL, 0,
							 NULL);
#endif
	}

	/* session tickets in server side are enabled by setting a key */
	if (flags & GNUTLS_SERVER)
		flags |= GNUTLS_NO_TICKETS;

	(*session)->internals.flags = flags;

	if (_gnutls_disable_tls13 != 0)
		(*session)->internals.flags |= INT_FLAG_NO_TLS13;

	/* Install the default keylog function */
	gnutls_session_set_keylog_function(*session, _gnutls_nss_keylog_func);

	return 0;
}

/* returns RESUME_FALSE or RESUME_TRUE.
 */
int _gnutls_session_is_resumable(gnutls_session_t session)
{
	return session->internals.resumable;
}


/**
 * gnutls_deinit:
 * @session: is a #gnutls_session_t type.
 *
 * This function clears all buffers associated with the @session.
 * This function will also remove session data from the session
 * database if the session was terminated abnormally.
 **/
void gnutls_deinit(gnutls_session_t session)
{
	unsigned int i;

	if (session == NULL)
		return;

	/* remove auth info firstly */
	_gnutls_free_auth_info(session);

	_gnutls_handshake_internal_state_clear(session);
	_gnutls_handshake_io_buffer_clear(session);
	_gnutls_hello_ext_priv_deinit(session);

	for (i = 0; i < MAX_EPOCH_INDEX; i++)
		if (session->record_parameters[i] != NULL) {
			_gnutls_epoch_free(session,
					   session->record_parameters[i]);
			session->record_parameters[i] = NULL;
		}

	_gnutls_buffer_clear(&session->internals.handshake_hash_buffer);
	_gnutls_buffer_clear(&session->internals.post_handshake_hash_buffer);
	_gnutls_buffer_clear(&session->internals.hb_remote_data);
	_gnutls_buffer_clear(&session->internals.hb_local_data);
	_gnutls_buffer_clear(&session->internals.record_presend_buffer);
	_gnutls_buffer_clear(&session->internals.record_key_update_buffer);
	_gnutls_buffer_clear(&session->internals.reauth_buffer);

	_mbuffer_head_clear(&session->internals.record_buffer);
	_mbuffer_head_clear(&session->internals.record_recv_buffer);
	_mbuffer_head_clear(&session->internals.record_send_buffer);

	_mbuffer_head_clear(&session->internals.early_data_recv_buffer);
	_gnutls_buffer_clear(&session->internals.early_data_presend_buffer);

	_gnutls_free_datum(&session->internals.resumption_data);
	_gnutls_free_datum(&session->internals.dtls.dcookie);

	gnutls_free(session->internals.rexts);
	gnutls_free(session->internals.post_handshake_cr_context.data);

	gnutls_free(session->internals.rsup);

	gnutls_credentials_clear(session);
	_gnutls_selected_certs_deinit(session);

	/* destroy any session ticket we may have received */
	_gnutls13_session_ticket_unset(session);

	/* we rely on priorities' internal reference counting */
	gnutls_priority_deinit(session->internals.priorities);

	/* overwrite any temp TLS1.3 keys */
	gnutls_memset(&session->key.proto, 0, sizeof(session->key.proto));

	gnutls_mutex_deinit(&session->internals.post_negotiation_lock);
	gnutls_mutex_deinit(&session->internals.epoch_lock);

	gnutls_free(session);
}

int _gnutls_dh_set_peer_public(gnutls_session_t session, bigint_t public)
{
	dh_info_st *dh;
	int ret;

	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		{
			anon_auth_info_t info;
			info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_PSK:
		{
			psk_auth_info_t info;
			info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_CERTIFICATE:
		{
			cert_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	default:
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	if (dh->public_key.data)
		_gnutls_free_datum(&dh->public_key);

	ret = _gnutls_mpi_dprint_lz(public, &dh->public_key);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

int _gnutls_dh_set_secret_bits(gnutls_session_t session, unsigned bits)
{
	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		{
			anon_auth_info_t info;
			info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			info->dh.secret_bits = bits;
			break;
		}
	case GNUTLS_CRD_PSK:
		{
			psk_auth_info_t info;
			info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			info->dh.secret_bits = bits;
			break;
		}
	case GNUTLS_CRD_CERTIFICATE:
		{
			cert_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			info->dh.secret_bits = bits;
			break;
	default:
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		}
	}

	return 0;
}

/* Sets the prime and the generator in the auth info structure.
 */
int
_gnutls_dh_save_group(gnutls_session_t session, bigint_t gen,
		     bigint_t prime)
{
	dh_info_st *dh;
	int ret;

	switch (gnutls_auth_get_type(session)) {
	case GNUTLS_CRD_ANON:
		{
			anon_auth_info_t info;
			info = _gnutls_get_auth_info(session, GNUTLS_CRD_ANON);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_PSK:
		{
			psk_auth_info_t info;
			info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	case GNUTLS_CRD_CERTIFICATE:
		{
			cert_auth_info_t info;

			info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
			if (info == NULL)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			dh = &info->dh;
			break;
		}
	default:
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	if (dh->prime.data)
		_gnutls_free_datum(&dh->prime);

	if (dh->generator.data)
		_gnutls_free_datum(&dh->generator);

	/* prime
	 */
	ret = _gnutls_mpi_dprint_lz(prime, &dh->prime);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* generator
	 */
	ret = _gnutls_mpi_dprint_lz(gen, &dh->generator);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(&dh->prime);
		return ret;
	}

	return 0;
}

/**
 * gnutls_certificate_send_x509_rdn_sequence:
 * @session: a #gnutls_session_t type.
 * @status: is 0 or 1
 *
 * If status is non zero, this function will order gnutls not to send
 * the rdnSequence in the certificate request message. That is the
 * server will not advertise its trusted CAs to the peer. If status
 * is zero then the default behaviour will take effect, which is to
 * advertise the server's trusted CAs.
 *
 * This function has no effect in clients, and in authentication
 * methods other than certificate with X.509 certificates.
 **/
void
gnutls_certificate_send_x509_rdn_sequence(gnutls_session_t session,
					  int status)
{
	session->internals.ignore_rdn_sequence = status;
}

/*-
 * _gnutls_record_set_default_version - Used to set the default version for the first record packet
 * @session: is a #gnutls_session_t type.
 * @major: is a tls major version
 * @minor: is a tls minor version
 *
 * This function sets the default version that we will use in the first
 * record packet (client hello). This function is only useful to people
 * that know TLS internals and want to debug other implementations.
 -*/
void
_gnutls_record_set_default_version(gnutls_session_t session,
				   unsigned char major,
				   unsigned char minor)
{
	session->internals.default_record_version[0] = major;
	session->internals.default_record_version[1] = minor;
}

/*-
 * _gnutls_hello_set_default_version - Used to set the default version for the first record packet
 * @session: is a #gnutls_session_t type.
 * @major: is a tls major version
 * @minor: is a tls minor version
 *
 * This function sets the default version that we will use in the first
 * record packet (client hello). This function is only useful to people
 * that know TLS internals and want to debug other implementations.
 -*/
void
_gnutls_hello_set_default_version(gnutls_session_t session,
				   unsigned char major,
				   unsigned char minor)
{
	session->internals.default_hello_version[0] = major;
	session->internals.default_hello_version[1] = minor;
}

/**
 * gnutls_handshake_set_private_extensions:
 * @session: is a #gnutls_session_t type.
 * @allow: is an integer (0 or 1)
 *
 * This function will enable or disable the use of private cipher
 * suites (the ones that start with 0xFF).  By default or if @allow
 * is 0 then these cipher suites will not be advertised nor used.
 *
 * Currently GnuTLS does not include such cipher-suites or
 * compression algorithms.
 *
 * Enabling the private ciphersuites when talking to other than
 * gnutls servers and clients may cause interoperability problems.
 **/
void
gnutls_handshake_set_private_extensions(gnutls_session_t session,
					int allow)
{
	/* we have no private extensions */
	return;
}


/**
 * gnutls_session_is_resumed:
 * @session: is a #gnutls_session_t type.
 *
 * Checks whether session is resumed or not. This is functional
 * for both server and client side.
 *
 * Returns: non zero if this session is resumed, or a zero if this is
 *   a new session.
 **/
int gnutls_session_is_resumed(gnutls_session_t session)
{
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		const version_entry_st *ver = get_version(session);
		if (ver && ver->tls13_sem &&
		    session->internals.resumed != RESUME_FALSE)
			return 1;

		if (session->security_parameters.session_id_size > 0 &&
		    session->security_parameters.session_id_size ==
		    session->internals.resumed_security_parameters.
		    session_id_size
		    && memcmp(session->security_parameters.session_id,
			      session->
			      internals.resumed_security_parameters.
			      session_id,
			      session->security_parameters.
			      session_id_size) == 0)
			return 1;
	} else {
		if (session->internals.resumed != RESUME_FALSE)
			return 1;
	}

	return 0;
}

/**
 * gnutls_session_resumption_requested:
 * @session: is a #gnutls_session_t type.
 *
 * Check whether the client has asked for session resumption.
 * This function is valid only on server side.
 *
 * Returns: non zero if session resumption was asked, or a zero if not.
 **/
int gnutls_session_resumption_requested(gnutls_session_t session)
{
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		return 0;
	} else {
		return session->internals.resumption_requested;
	}
}

/*-
 * _gnutls_session_is_psk - Used to check whether this session uses PSK kx
 * @session: is a #gnutls_session_t type.
 *
 * This function will return non zero if this session uses a PSK key
 * exchange algorithm.
 -*/
int _gnutls_session_is_psk(gnutls_session_t session)
{
	gnutls_kx_algorithm_t kx;

	kx = session->security_parameters.cs->kx_algorithm;
	if (kx == GNUTLS_KX_PSK || kx == GNUTLS_KX_DHE_PSK
	    || kx == GNUTLS_KX_RSA_PSK)
		return 1;

	return 0;
}

/*-
 * _gnutls_session_is_ecc - Used to check whether this session uses ECC kx
 * @session: is a #gnutls_session_t type.
 *
 * This function will return non zero if this session uses an elliptic
 * curves key exchange exchange algorithm.
 -*/
int _gnutls_session_is_ecc(gnutls_session_t session)
{
	gnutls_kx_algorithm_t kx;

	/* We get the key exchange algorithm through the ciphersuite because
	 * the negotiated key exchange might not have been set yet.
	 */
	kx = session->security_parameters.cs->kx_algorithm;

	return _gnutls_kx_is_ecc(kx);
}

/**
 * gnutls_session_get_ptr:
 * @session: is a #gnutls_session_t type.
 *
 * Get user pointer for session.  Useful in callbacks.  This is the
 *   pointer set with gnutls_session_set_ptr().
 *
 * Returns: the user given pointer from the session structure, or
 *   %NULL if it was never set.
 **/
void *gnutls_session_get_ptr(gnutls_session_t session)
{
	return session->internals.user_ptr;
}

/**
 * gnutls_session_set_ptr:
 * @session: is a #gnutls_session_t type.
 * @ptr: is the user pointer
 *
 * This function will set (associate) the user given pointer @ptr to
 * the session structure.  This pointer can be accessed with
 * gnutls_session_get_ptr().
 **/
void gnutls_session_set_ptr(gnutls_session_t session, void *ptr)
{
	session->internals.user_ptr = ptr;
}

/**
 * gnutls_session_set_verify_function:
 * @session: is a #gnutls_session_t type.
 * @func: is the callback function
 *
 * This function sets a callback to be called when peer's certificate
 * has been received in order to verify it on receipt rather than
 * doing after the handshake is completed. This overrides any callback
 * set using gnutls_certificate_set_verify_function().
 *
 * The callback's function prototype is:
 * int (*callback)(gnutls_session_t);
 *
 * If the callback function is provided then gnutls will call it, in the
 * handshake, just after the certificate message has been received.
 * To verify or obtain the certificate the gnutls_certificate_verify_peers2(),
 * gnutls_certificate_type_get(), gnutls_certificate_get_peers() functions
 * can be used.
 *
 * The callback function should return 0 for the handshake to continue
 * or non-zero to terminate.
 *
 * Since: 3.4.6
 **/
void
 gnutls_session_set_verify_function
    (gnutls_session_t session,
     gnutls_certificate_verify_function * func)
{
	session->internals.verify_callback = func;
}

/**
 * gnutls_record_get_direction:
 * @session: is a #gnutls_session_t type.
 *
 * This function is useful to determine whether a GnuTLS function was interrupted
 * while sending or receiving, so that select() or poll() may be called appropriately.
 *
 * It provides information about the internals of the record
 * protocol and is only useful if a prior gnutls function call,
 * e.g.  gnutls_handshake(), was interrupted and returned
 * %GNUTLS_E_INTERRUPTED or %GNUTLS_E_AGAIN. After such an interrupt
 * applications may call select() or poll() before restoring the
 * interrupted GnuTLS function.
 *
 * This function's output is unreliable if you are using the same
 * @session in different threads for sending and receiving.
 *
 * Returns: 0 if interrupted while trying to read data, or 1 while trying to write data.
 **/
int gnutls_record_get_direction(gnutls_session_t session)
{
	return session->internals.direction;
}

/*-
 * _gnutls_rsa_pms_set_version - Sets a version to be used at the RSA PMS
 * @session: is a #gnutls_session_t type.
 * @major: is the major version to use
 * @minor: is the minor version to use
 *
 * This function will set the given version number to be used at the
 * RSA PMS secret. This is only useful to clients, which want to
 * test server's capabilities.
 -*/
void
_gnutls_rsa_pms_set_version(gnutls_session_t session,
			    unsigned char major, unsigned char minor)
{
	session->internals.rsa_pms_version[0] = major;
	session->internals.rsa_pms_version[1] = minor;
}

void _gnutls_session_client_cert_type_set(gnutls_session_t session,
			      gnutls_certificate_type_t ct)
{
	_gnutls_handshake_log
	    ("HSK[%p]: Selected client certificate type %s (%d)\n", session,
	     gnutls_certificate_type_get_name(ct), ct);
	session->security_parameters.client_ctype = ct;
}

void _gnutls_session_server_cert_type_set(gnutls_session_t session,
			      gnutls_certificate_type_t ct)
{
	_gnutls_handshake_log
	    ("HSK[%p]: Selected server certificate type %s (%d)\n", session,
	     gnutls_certificate_type_get_name(ct), ct);
	session->security_parameters.server_ctype = ct;
}

/**
 * gnutls_handshake_set_post_client_hello_function:
 * @session: is a #gnutls_session_t type.
 * @func: is the function to be called
 *
 * This function will set a callback to be called after the client
 * hello has been received (callback valid in server side only). This
 * allows the server to adjust settings based on received extensions.
 *
 * Those settings could be ciphersuites, requesting certificate, or
 * anything else except for version negotiation (this is done before
 * the hello message is parsed).
 *
 * This callback must return 0 on success or a gnutls error code to
 * terminate the handshake.
 *
 * Since GnuTLS 3.3.5 the callback is
 * allowed to return %GNUTLS_E_AGAIN or %GNUTLS_E_INTERRUPTED to
 * put the handshake on hold. In that case gnutls_handshake()
 * will return %GNUTLS_E_INTERRUPTED and can be resumed when needed.
 *
 * Warning: You should not use this function to terminate the
 * handshake based on client input unless you know what you are
 * doing. Before the handshake is finished there is no way to know if
 * there is a man-in-the-middle attack being performed.
 **/
void
gnutls_handshake_set_post_client_hello_function(gnutls_session_t session,
						gnutls_handshake_simple_hook_func func)
{
	session->internals.user_hello_func = func;
}


/**
 * gnutls_session_enable_compatibility_mode:
 * @session: is a #gnutls_session_t type.
 *
 * This function can be used to disable certain (security) features in
 * TLS in order to maintain maximum compatibility with buggy
 * clients. Because several trade-offs with security are enabled,
 * if required they will be reported through the audit subsystem.
 *
 * Normally only servers that require maximum compatibility with
 * everything out there, need to call this function.
 *
 * Note that this function must be called after any call to gnutls_priority
 * functions.
 *
 * Since: 2.1.4
 **/
void gnutls_session_enable_compatibility_mode(gnutls_session_t session)
{
	ENABLE_COMPAT(&session->internals);
}

/**
 * gnutls_session_channel_binding:
 * @session: is a #gnutls_session_t type.
 * @cbtype: an #gnutls_channel_binding_t enumeration type
 * @cb: output buffer array with data
 *
 * Extract given channel binding data of the @cbtype (e.g.,
 * %GNUTLS_CB_TLS_UNIQUE) type.
 *
 * Returns: %GNUTLS_E_SUCCESS on success,
 * %GNUTLS_E_UNIMPLEMENTED_FEATURE if the @cbtype is unsupported,
 * %GNUTLS_E_CHANNEL_BINDING_NOT_AVAILABLE if the data is not
 * currently available, or an error code.
 *
 * Since: 2.12.0
 **/
int
gnutls_session_channel_binding(gnutls_session_t session,
			       gnutls_channel_binding_t cbtype,
			       gnutls_datum_t * cb)
{
	if (cbtype != GNUTLS_CB_TLS_UNIQUE)
		return GNUTLS_E_UNIMPLEMENTED_FEATURE;

	if (!session->internals.initial_negotiation_completed)
		return GNUTLS_E_CHANNEL_BINDING_NOT_AVAILABLE;

	cb->size = session->internals.cb_tls_unique_len;
	cb->data = gnutls_malloc(cb->size);
	if (cb->data == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	memcpy(cb->data, session->internals.cb_tls_unique, cb->size);

	return 0;
}

/**
 * gnutls_ecc_curve_get:
 * @session: is a #gnutls_session_t type.
 *
 * Returns the currently used elliptic curve for key exchange. Only valid
 * when using an elliptic curve ciphersuite.
 *
 * Returns: the currently used curve, a #gnutls_ecc_curve_t
 *   type.
 *
 * Since: 3.0
 **/
gnutls_ecc_curve_t gnutls_ecc_curve_get(gnutls_session_t session)
{
	const gnutls_group_entry_st *e;

	e = get_group(session);
	if (e == NULL || e->curve == 0)
		return 0;
	return e->curve;
}

/**
 * gnutls_group_get:
 * @session: is a #gnutls_session_t type.
 *
 * Returns the currently used group for key exchange. Only valid
 * when using an elliptic curve or DH ciphersuite.
 *
 * Returns: the currently used group, a #gnutls_group_t
 *   type.
 *
 * Since: 3.6.0
 **/
gnutls_group_t gnutls_group_get(gnutls_session_t session)
{
	const gnutls_group_entry_st *e;

	e = get_group(session);
	if (e == NULL)
		return 0;
	return e->id;
}

/**
 * gnutls_protocol_get_version:
 * @session: is a #gnutls_session_t type.
 *
 * Get TLS version, a #gnutls_protocol_t value.
 *
 * Returns: The version of the currently used protocol.
 **/
gnutls_protocol_t gnutls_protocol_get_version(gnutls_session_t session)
{
	return get_num_version(session);
}

/**
 * gnutls_session_get_random:
 * @session: is a #gnutls_session_t type.
 * @client: the client part of the random
 * @server: the server part of the random
 *
 * This function returns pointers to the client and server
 * random fields used in the TLS handshake. The pointers are
 * not to be modified or deallocated.
 *
 * If a client random value has not yet been established, the output
 * will be garbage.
 *
 * Since: 3.0
 **/
void
gnutls_session_get_random(gnutls_session_t session,
			  gnutls_datum_t * client, gnutls_datum_t * server)
{
	if (client) {
		client->data = session->security_parameters.client_random;
		client->size =
		    sizeof(session->security_parameters.client_random);
	}

	if (server) {
		server->data = session->security_parameters.server_random;
		server->size =
		    sizeof(session->security_parameters.server_random);
	}
}

/**
 * gnutls_session_get_master_secret:
 * @session: is a #gnutls_session_t type.
 * @secret: the session's master secret
 *
 * This function returns pointers to the master secret
 * used in the TLS session. The pointers are not to be modified or deallocated.
 *
 * This function is only applicable under TLS 1.2 or earlier versions.
 *
 * Since: 3.5.0
 **/
void
gnutls_session_get_master_secret(gnutls_session_t session, gnutls_datum_t *secret)
{
	secret->data = session->security_parameters.master_secret;
	secret->size = sizeof(session->security_parameters.master_secret);
}

unsigned int timespec_sub_ms(struct timespec *a, struct timespec *b)
{
	time_t dsecs;

	dsecs = a->tv_sec - b->tv_sec;
	if (!INT_MULTIPLY_OVERFLOW(dsecs, 1000)) {
		return (dsecs*1000 + (a->tv_nsec - b->tv_nsec) / (1000 * 1000));
	} else {
		return UINT_MAX;
	}
}

/**
 * gnutls_handshake_set_random:
 * @session: is a #gnutls_session_t type.
 * @random: a random value of 32-bytes
 *
 * This function will explicitly set the server or client hello 
 * random value in the subsequent TLS handshake. The random value 
 * should be a 32-byte value.
 *
 * Note that this function should not normally be used as gnutls
 * will select automatically a random value for the handshake.
 *
 * This function should not be used when resuming a session.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 *
 * Since 3.1.9
 **/
int
gnutls_handshake_set_random(gnutls_session_t session,
			    const gnutls_datum_t * random)
{
	if (random->size != GNUTLS_RANDOM_SIZE)
		return GNUTLS_E_INVALID_REQUEST;

	session->internals.sc_random_set = 1;
	if (session->security_parameters.entity == GNUTLS_CLIENT)
		memcpy(session->internals.resumed_security_parameters.
		       client_random, random->data, random->size);
	else
		memcpy(session->internals.resumed_security_parameters.
		       server_random, random->data, random->size);

	return 0;
}

/**
 * gnutls_handshake_set_hook_function:
 * @session: is a #gnutls_session_t type
 * @htype: the %gnutls_handshake_description_t of the message to hook at
 * @when: %GNUTLS_HOOK_* depending on when the hook function should be called
 * @func: is the function to be called
 *
 * This function will set a callback to be called after or before the specified
 * handshake message has been received or generated. This is a
 * generalization of gnutls_handshake_set_post_client_hello_function().
 *
 * To call the hook function prior to the message being generated or processed
 * use %GNUTLS_HOOK_PRE as @when parameter, %GNUTLS_HOOK_POST to call
 * after, and %GNUTLS_HOOK_BOTH for both cases.
 *
 * This callback must return 0 on success or a gnutls error code to
 * terminate the handshake.
 *
 * To hook at all handshake messages use an @htype of %GNUTLS_HANDSHAKE_ANY.
 *
 * Warning: You should not use this function to terminate the
 * handshake based on client input unless you know what you are
 * doing. Before the handshake is finished there is no way to know if
 * there is a man-in-the-middle attack being performed.
 **/
void
gnutls_handshake_set_hook_function(gnutls_session_t session,
				   unsigned int htype,
				   int when,
				   gnutls_handshake_hook_func func)
{
	session->internals.h_hook = func;
	session->internals.h_type = htype;
	session->internals.h_post = when;
}

/**
 * gnutls_record_get_state:
 * @session: is a #gnutls_session_t type
 * @read: if non-zero the read parameters are returned, otherwise the write
 * @mac_key: the key used for MAC (if a MAC is used)
 * @IV: the initialization vector or nonce used
 * @cipher_key: the cipher key
 * @seq_number: A 64-bit sequence number
 *
 * This function will return the parameters of the current record state.
 * These are only useful to be provided to an external off-loading device
 * or subsystem. The returned values should be considered constant
 * and valid for the lifetime of the session.
 *
 * In that case, to sync the state back you must call gnutls_record_set_state().
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 *
 * Since 3.4.0
 **/
int
gnutls_record_get_state(gnutls_session_t session,
			unsigned read,
			gnutls_datum_t *mac_key,
			gnutls_datum_t *IV,
			gnutls_datum_t *cipher_key,
			unsigned char seq_number[8])
{
	record_parameters_st *record_params;
	record_state_st *record_state;
	unsigned int epoch;
	int ret;

	if (read)
		epoch = EPOCH_READ_CURRENT;
	else
		epoch = EPOCH_WRITE_CURRENT;

	ret = _gnutls_epoch_get(session, epoch, &record_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (!record_params->initialized)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (read)
		record_state = &record_params->read;
	else
		record_state = &record_params->write;

	if (mac_key) {
		mac_key->data = record_state->mac_key;
		mac_key->size = record_state->mac_key_size;
	}

	if (IV) {
		IV->data = record_state->iv;
		IV->size = record_state->iv_size;
	}

	if (cipher_key) {
		cipher_key->data = record_state->key;
		cipher_key->size = record_state->key_size;
	}

	if (seq_number)
		_gnutls_write_uint64(record_state->sequence_number, seq_number);
	return 0;
}

/**
 * gnutls_record_set_state:
 * @session: is a #gnutls_session_t type
 * @read: if non-zero the read parameters are returned, otherwise the write
 * @seq_number: A 64-bit sequence number
 *
 * This function will set the sequence number in the current record state.
 * This function is useful if sending and receiving are offloaded from
 * gnutls. That is, if gnutls_record_get_state() was used.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 *
 * Since 3.4.0
 **/
int
gnutls_record_set_state(gnutls_session_t session,
			unsigned read,
			const unsigned char seq_number[8])
{
	record_parameters_st *record_params;
	record_state_st *record_state;
	int epoch, ret;

	if (read)
		epoch = EPOCH_READ_CURRENT;
	else
		epoch = EPOCH_WRITE_CURRENT;

	ret = _gnutls_epoch_get(session, epoch, &record_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (!record_params->initialized)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (read)
		record_state = &record_params->read;
	else
		record_state = &record_params->write;

	record_state->sequence_number = _gnutls_read_uint64(seq_number);

	if (IS_DTLS(session)) {
		_dtls_reset_window(record_params);
	}

	return 0;
}

/**
 * gnutls_session_get_flags:
 * @session: is a #gnutls_session_t type.
 *
 * This function will return a series (ORed) of flags, applicable
 * for the current session.
 *
 * This replaces individual informational functions such as
 * gnutls_safe_renegotiation_status(), gnutls_session_ext_master_secret_status(),
 * etc.
 *
 * Returns: An ORed sequence of flags (see %gnutls_session_flags_t)
 *
 * Since: 3.5.0
 **/
unsigned gnutls_session_get_flags(gnutls_session_t session)
{
	unsigned flags = 0;

	if (gnutls_safe_renegotiation_status(session))
		flags |= GNUTLS_SFLAGS_SAFE_RENEGOTIATION;
	if (gnutls_session_ext_master_secret_status(session))
		flags |= GNUTLS_SFLAGS_EXT_MASTER_SECRET;
	if (gnutls_session_etm_status(session))
		flags |= GNUTLS_SFLAGS_ETM;
	if (gnutls_heartbeat_allowed(session, GNUTLS_HB_LOCAL_ALLOWED_TO_SEND))
		flags |= GNUTLS_SFLAGS_HB_LOCAL_SEND;
	if (gnutls_heartbeat_allowed(session, GNUTLS_HB_PEER_ALLOWED_TO_SEND))
		flags |= GNUTLS_SFLAGS_HB_PEER_SEND;
	if (session->internals.hsk_flags & HSK_FALSE_START_USED)
		flags |= GNUTLS_SFLAGS_FALSE_START;
	if ((session->internals.hsk_flags & HSK_EARLY_START_USED) &&
	    (session->internals.flags & GNUTLS_ENABLE_EARLY_START))
		flags |= GNUTLS_SFLAGS_EARLY_START;
	if (session->internals.hsk_flags & HSK_USED_FFDHE)
		flags |= GNUTLS_SFLAGS_RFC7919;
	if (session->internals.hsk_flags & HSK_TICKET_RECEIVED)
		flags |= GNUTLS_SFLAGS_SESSION_TICKET;
	if (session->security_parameters.post_handshake_auth)
		flags |= GNUTLS_SFLAGS_POST_HANDSHAKE_AUTH;
	if (session->internals.hsk_flags & HSK_EARLY_DATA_ACCEPTED)
		flags |= GNUTLS_SFLAGS_EARLY_DATA;
	if (session->internals.hsk_flags & HSK_OCSP_REQUESTED)
		flags |= GNUTLS_SFLAGS_CLI_REQUESTED_OCSP;
	if (session->internals.hsk_flags & HSK_CLIENT_OCSP_REQUESTED)
		flags |= GNUTLS_SFLAGS_SERV_REQUESTED_OCSP;

	return flags;
}
