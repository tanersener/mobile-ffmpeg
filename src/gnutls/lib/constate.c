/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

/* Functions that are supposed to run after the handshake procedure is
 * finished. These functions activate the established security parameters.
 */

#include "gnutls_int.h"
#include <constate.h>
#include "errors.h"
#include <kx.h>
#include <algorithms.h>
#include <num.h>
#include <datum.h>
#include <state.h>
#include <extensions.h>
#include <buffers.h>
#include "dtls.h"

static int
_gnutls_set_kx(gnutls_session_t session, gnutls_kx_algorithm_t algo);

static const char keyexp[] = "key expansion";
static const int keyexp_length = sizeof(keyexp) - 1;

/* This function is to be called after handshake, when master_secret,
 *  client_random and server_random have been initialized. 
 * This function creates the keys and stores them into pending session.
 * (session->cipher_specs)
 */
static int
_gnutls_set_keys(gnutls_session_t session, record_parameters_st * params,
		 int hash_size, int IV_size, int key_size)
{
	/* FIXME: This function is too long
	 */
	uint8_t rnd[2 * GNUTLS_RANDOM_SIZE];
	uint8_t rrnd[2 * GNUTLS_RANDOM_SIZE];
	int pos, ret;
	int block_size;
	char buf[65];
	/* avoid using malloc */
	uint8_t key_block[2 * MAX_HASH_SIZE + 2 * MAX_CIPHER_KEY_SIZE +
			  2 * MAX_CIPHER_BLOCK_SIZE];
	record_state_st *client_write, *server_write;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		client_write = &params->write;
		server_write = &params->read;
	} else {
		client_write = &params->read;
		server_write = &params->write;
	}

	block_size = 2 * hash_size + 2 * key_size;
	block_size += 2 * IV_size;

	memcpy(rnd, session->security_parameters.server_random,
	       GNUTLS_RANDOM_SIZE);
	memcpy(&rnd[GNUTLS_RANDOM_SIZE],
	       session->security_parameters.client_random,
	       GNUTLS_RANDOM_SIZE);

	memcpy(rrnd, session->security_parameters.client_random,
	       GNUTLS_RANDOM_SIZE);
	memcpy(&rrnd[GNUTLS_RANDOM_SIZE],
	       session->security_parameters.server_random,
	       GNUTLS_RANDOM_SIZE);

#ifdef ENABLE_SSL3
	if (get_num_version(session) == GNUTLS_SSL3) {	/* SSL 3 */
		ret =
		    _gnutls_ssl3_generate_random
		    (session->security_parameters.master_secret,
		     GNUTLS_MASTER_SIZE, rnd, 2 * GNUTLS_RANDOM_SIZE,
		     block_size, key_block);
	} else /* TLS 1.0+ */
#endif
		ret =
		    _gnutls_PRF(session,
				session->security_parameters.master_secret,
				GNUTLS_MASTER_SIZE, keyexp, keyexp_length,
				rnd, 2 * GNUTLS_RANDOM_SIZE, block_size,
				key_block);

	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_hard_log("INT: KEY BLOCK[%d]: %s\n", block_size,
			 _gnutls_bin2hex(key_block, block_size, buf,
					 sizeof(buf), NULL));

	pos = 0;
	if (hash_size > 0) {

		if (_gnutls_set_datum
		    (&client_write->mac_secret, &key_block[pos],
		     hash_size) < 0)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		pos += hash_size;

		if (_gnutls_set_datum
		    (&server_write->mac_secret, &key_block[pos],
		     hash_size) < 0)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		pos += hash_size;
	}

	if (key_size > 0) {
		uint8_t *client_write_key, *server_write_key;
		int client_write_key_size, server_write_key_size;

		client_write_key = &key_block[pos];
		client_write_key_size = key_size;

		pos += key_size;

		server_write_key = &key_block[pos];
		server_write_key_size = key_size;

		pos += key_size;

		if (_gnutls_set_datum
		    (&client_write->key, client_write_key,
		     client_write_key_size) < 0)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		_gnutls_hard_log("INT: CLIENT WRITE KEY [%d]: %s\n",
				 client_write_key_size,
				 _gnutls_bin2hex(client_write_key,
						 client_write_key_size,
						 buf, sizeof(buf), NULL));

		if (_gnutls_set_datum
		    (&server_write->key, server_write_key,
		     server_write_key_size) < 0)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		_gnutls_hard_log("INT: SERVER WRITE KEY [%d]: %s\n",
				 server_write_key_size,
				 _gnutls_bin2hex(server_write_key,
						 server_write_key_size,
						 buf, sizeof(buf), NULL));

	}

	/* IV generation in export and non export ciphers.
	 */
	if (IV_size > 0) {
		if (_gnutls_set_datum
		    (&client_write->IV, &key_block[pos], IV_size) < 0)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		pos += IV_size;

		if (_gnutls_set_datum
		    (&server_write->IV, &key_block[pos], IV_size) < 0)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);


		_gnutls_hard_log("INT: CLIENT WRITE IV [%d]: %s\n",
				 client_write->IV.size,
				 _gnutls_bin2hex(client_write->IV.data,
						 client_write->IV.size,
						 buf, sizeof(buf), NULL));

		_gnutls_hard_log("INT: SERVER WRITE IV [%d]: %s\n",
				 server_write->IV.size,
				 _gnutls_bin2hex(server_write->IV.data,
						 server_write->IV.size,
						 buf, sizeof(buf), NULL));
	}

	return 0;
}

static int
_gnutls_init_record_state(record_parameters_st * params,
			  const version_entry_st * ver, int read,
			  record_state_st * state)
{
	int ret;
	gnutls_datum_t *iv = NULL;

	if (!_gnutls_version_has_explicit_iv(ver)) {
		if (_gnutls_cipher_type(params->cipher) == CIPHER_BLOCK)
			iv = &state->IV;
	}

	ret = _gnutls_auth_cipher_init(&state->cipher_state,
				       params->cipher, &state->key, iv,
				       params->mac, &state->mac_secret,
				       params->etm,
#ifdef ENABLE_SSL3
				       (ver->id == GNUTLS_SSL3) ? 1 : 0,
#endif
				       1 - read /*1==encrypt */ );
	if (ret < 0 && params->cipher->id != GNUTLS_CIPHER_NULL)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_comp_init(&state->compression_state,
			      params->compression_algorithm,
			      read /*1==decompress */ );

	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

int
_gnutls_epoch_set_cipher_suite(gnutls_session_t session,
			       int epoch_rel, const uint8_t suite[2])
{
	const cipher_entry_st *cipher_algo;
	const mac_entry_st *mac_algo;
	record_parameters_st *params;
	const gnutls_cipher_suite_entry_st *cs;
	int ret;

	ret = _gnutls_epoch_get(session, epoch_rel, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (params->initialized
	    || params->cipher != NULL || params->mac != NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	cs = ciphersuite_to_entry(suite);
	if (cs == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	cipher_algo = cipher_to_entry(cs->block_algorithm);
	mac_algo = mac_to_entry(cs->mac_algorithm);

	if (_gnutls_cipher_is_ok(cipher_algo) == 0
	    || _gnutls_mac_is_ok(mac_algo) == 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	if (_gnutls_cipher_priority(session, cipher_algo->id) < 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	if (_gnutls_mac_priority(session, mac_algo->id) < 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	params->cipher = cipher_algo;
	params->mac = mac_algo;

	return 0;
}

int
_gnutls_epoch_set_compression(gnutls_session_t session,
			      int epoch_rel,
			      gnutls_compression_method_t comp_algo)
{
	record_parameters_st *params;
	int ret;

	ret = _gnutls_epoch_get(session, epoch_rel, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (params->initialized
	    || params->compression_algorithm != GNUTLS_COMP_UNKNOWN)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (_gnutls_compression_is_ok(comp_algo) != 0)
		return
		    gnutls_assert_val
		    (GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM);

	params->compression_algorithm = comp_algo;

	return 0;
}

void
_gnutls_epoch_set_null_algos(gnutls_session_t session,
			     record_parameters_st * params)
{
	/* This is only called on startup. We are extra paranoid about this
	   because it may cause unencrypted application data to go out on
	   the wire. */
	if (params->initialized || params->epoch != 0) {
		gnutls_assert();
		return;
	}

	params->cipher = cipher_to_entry(GNUTLS_CIPHER_NULL);
	params->mac = mac_to_entry(GNUTLS_MAC_NULL);
	params->compression_algorithm = GNUTLS_COMP_NULL;
	params->initialized = 1;
}

int _gnutls_epoch_set_keys(gnutls_session_t session, uint16_t epoch)
{
	int hash_size;
	int IV_size;
	int key_size;
	gnutls_compression_method_t comp_algo;
	record_parameters_st *params;
	int ret;
	const version_entry_st *ver = get_version(session);

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	ret = _gnutls_epoch_get(session, epoch, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (params->initialized)
		return 0;

	_gnutls_record_log
	    ("REC[%p]: Initializing epoch #%u\n", session, params->epoch);

	comp_algo = params->compression_algorithm;

	if (_gnutls_cipher_is_ok(params->cipher) == 0
	    || _gnutls_mac_is_ok(params->mac) == 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	if (_gnutls_cipher_priority(session, params->cipher->id) < 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	if (_gnutls_mac_priority(session, params->mac->id) < 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	if (_gnutls_compression_is_ok(comp_algo) != 0)
		return
		    gnutls_assert_val
		    (GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM);

	if (!_gnutls_version_has_explicit_iv(ver) &&
	    _gnutls_cipher_type(params->cipher) == CIPHER_BLOCK) {
		IV_size = _gnutls_cipher_get_iv_size(params->cipher);
	} else {
		IV_size = _gnutls_cipher_get_implicit_iv_size(params->cipher);
	}

	key_size = _gnutls_cipher_get_key_size(params->cipher);
	hash_size = _gnutls_mac_get_key_size(params->mac);
	params->etm = session->security_parameters.etm;

	ret = _gnutls_set_keys
	    (session, params, hash_size, IV_size, key_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_init_record_state(params, ver, 1, &params->read);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_init_record_state(params, ver, 0, &params->write);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_dtls_reset_window(params);

	_gnutls_record_log("REC[%p]: Epoch #%u ready\n", session,
			   params->epoch);

	params->initialized = 1;
	return 0;
}


#define CPY_COMMON dst->entity = src->entity; \
	dst->kx_algorithm = src->kx_algorithm; \
	memcpy( dst->cipher_suite, src->cipher_suite, 2); \
	memcpy( dst->master_secret, src->master_secret, GNUTLS_MASTER_SIZE); \
	memcpy( dst->client_random, src->client_random, GNUTLS_RANDOM_SIZE); \
	memcpy( dst->server_random, src->server_random, GNUTLS_RANDOM_SIZE); \
	memcpy( dst->session_id, src->session_id, GNUTLS_MAX_SESSION_ID_SIZE); \
	dst->session_id_size = src->session_id_size; \
	dst->cert_type = src->cert_type; \
	dst->compression_method = src->compression_method; \
	dst->timestamp = src->timestamp; \
	dst->ext_master_secret = src->ext_master_secret; \
	dst->etm = src->etm; \
	dst->max_record_recv_size = src->max_record_recv_size; \
	dst->max_record_send_size = src->max_record_send_size

static void _gnutls_set_resumed_parameters(gnutls_session_t session)
{
	security_parameters_st *src =
	    &session->internals.resumed_security_parameters;
	security_parameters_st *dst = &session->security_parameters;

	CPY_COMMON;
	dst->pversion = src->pversion;
}

/* Sets the current connection session to conform with the
 * Security parameters(pending session), and initializes encryption.
 * Actually it initializes and starts encryption ( so it needs
 * secrets and random numbers to have been negotiated)
 * This is to be called after sending the Change Cipher Spec packet.
 */
int _gnutls_connection_state_init(gnutls_session_t session)
{
	int ret;

/* Setup the master secret 
 */
	if ((ret = _gnutls_generate_master(session, 0)) < 0)
		return gnutls_assert_val(ret);

	return 0;
}

int _gnutls_epoch_get_compression(gnutls_session_t session, int epoch)
{
	record_parameters_st *params;
	int ret;

	ret = _gnutls_epoch_get(session, epoch, &params);
	if (ret < 0)
		return GNUTLS_COMP_UNKNOWN;

	return params->compression_algorithm;
}

/* Initializes the read connection session
 * (read encrypted data)
 */
int _gnutls_read_connection_state_init(gnutls_session_t session)
{
	const uint16_t epoch_next =
	    session->security_parameters.epoch_next;
	int ret;

	/* Update internals from CipherSuite selected.
	 * If we are resuming just copy the connection session
	 */
	if (session->internals.resumed == RESUME_FALSE) {
		ret = _gnutls_set_kx(session,
				     _gnutls_cipher_suite_get_kx_algo
				     (session->security_parameters.
				      cipher_suite));
		if (ret < 0)
			return ret;
	} else if (session->security_parameters.entity == GNUTLS_CLIENT)
		_gnutls_set_resumed_parameters(session);

	ret = _gnutls_epoch_set_keys(session, epoch_next);
	if (ret < 0)
		return ret;

	_gnutls_handshake_log("HSK[%p]: Cipher Suite: %s\n",
			      session,
			      _gnutls_cipher_suite_get_name
			      (session->security_parameters.cipher_suite));

	session->security_parameters.epoch_read = epoch_next;

	return 0;
}



/* Initializes the write connection session
 * (write encrypted data)
 */
int _gnutls_write_connection_state_init(gnutls_session_t session)
{
	const uint16_t epoch_next =
	    session->security_parameters.epoch_next;
	int ret;

/* Update internals from CipherSuite selected.
 * If we are resuming just copy the connection session
 */
	if (session->internals.resumed == RESUME_FALSE) {
		ret = _gnutls_set_kx(session,
				     _gnutls_cipher_suite_get_kx_algo
				     (session->security_parameters.
				      cipher_suite));
		if (ret < 0)
			return ret;
	} else if (session->security_parameters.entity == GNUTLS_SERVER)
		_gnutls_set_resumed_parameters(session);

	ret = _gnutls_epoch_set_keys(session, epoch_next);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_handshake_log("HSK[%p]: Cipher Suite: %s\n", session,
			      _gnutls_cipher_suite_get_name
			      (session->security_parameters.cipher_suite));

	_gnutls_handshake_log
	    ("HSK[%p]: Initializing internal [write] cipher sessions\n",
	     session);

	session->security_parameters.epoch_write = epoch_next;

	return 0;
}

/* Sets the specified kx algorithm into pending session
 */
static int
_gnutls_set_kx(gnutls_session_t session, gnutls_kx_algorithm_t algo)
{

	if (_gnutls_kx_is_ok(algo) == 0) {
		session->security_parameters.kx_algorithm = algo;
	} else
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (_gnutls_kx_priority(session, algo) < 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	return 0;
}

static inline int
epoch_resolve(gnutls_session_t session,
	      unsigned int epoch_rel, uint16_t * epoch_out)
{
	switch (epoch_rel) {
	case EPOCH_READ_CURRENT:
		*epoch_out = session->security_parameters.epoch_read;
		return 0;

	case EPOCH_WRITE_CURRENT:
		*epoch_out = session->security_parameters.epoch_write;
		return 0;

	case EPOCH_NEXT:
		*epoch_out = session->security_parameters.epoch_next;
		return 0;

	default:
		if (epoch_rel > 0xffffu)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		*epoch_out = epoch_rel;
		return 0;
	}
}

static inline record_parameters_st **epoch_get_slot(gnutls_session_t
						    session,
						    uint16_t epoch)
{
	uint16_t epoch_index =
	    epoch - session->security_parameters.epoch_min;

	if (epoch_index >= MAX_EPOCH_INDEX) {
		_gnutls_handshake_log
		    ("Epoch %d out of range (idx: %d, max: %d)\n",
		     (int) epoch, (int) epoch_index, MAX_EPOCH_INDEX);
		gnutls_assert();
		return NULL;
	}
	/* The slot may still be empty (NULL) */
	return &session->record_parameters[epoch_index];
}

int
_gnutls_epoch_get(gnutls_session_t session, unsigned int epoch_rel,
		  record_parameters_st ** params_out)
{
	uint16_t epoch;
	record_parameters_st **params;
	int ret;

	ret = epoch_resolve(session, epoch_rel, &epoch);
	if (ret < 0)
		return gnutls_assert_val(ret);

	params = epoch_get_slot(session, epoch);
	if (params == NULL || *params == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	*params_out = *params;

	return 0;
}

int
_gnutls_epoch_alloc(gnutls_session_t session, uint16_t epoch,
		    record_parameters_st ** out)
{
	record_parameters_st **slot;

	_gnutls_record_log("REC[%p]: Allocating epoch #%u\n", session,
			   epoch);

	slot = epoch_get_slot(session, epoch);

	/* If slot out of range or not empty. */
	if (slot == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (*slot != NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	*slot = gnutls_calloc(1, sizeof(record_parameters_st));
	if (*slot == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	(*slot)->epoch = epoch;
	(*slot)->cipher = NULL;
	(*slot)->mac = NULL;
	(*slot)->compression_algorithm = GNUTLS_COMP_UNKNOWN;

	if (IS_DTLS(session))
		_gnutls_write_uint16(epoch,
				     UINT64DATA((*slot)->write.
						sequence_number));

	if (out != NULL)
		*out = *slot;

	return 0;
}

static inline int
epoch_is_active(gnutls_session_t session, record_parameters_st * params)
{
	const security_parameters_st *sp = &session->security_parameters;

	if (params->epoch == sp->epoch_read)
		return 1;

	if (params->epoch == sp->epoch_write)
		return 1;

	if (params->epoch == sp->epoch_next)
		return 1;

	return 0;
}

static inline int
epoch_alive(gnutls_session_t session, record_parameters_st * params)
{
	if (params->usage_cnt > 0)
		return 1;

	return epoch_is_active(session, params);
}

void _gnutls_epoch_gc(gnutls_session_t session)
{
	int i, j;
	unsigned int min_index = 0;

	_gnutls_record_log("REC[%p]: Start of epoch cleanup\n", session);

	/* Free all dead cipher state */
	for (i = 0; i < MAX_EPOCH_INDEX; i++) {
		if (session->record_parameters[i] != NULL) {
			if (!epoch_is_active
			    (session, session->record_parameters[i])
			    && session->record_parameters[i]->usage_cnt)
				_gnutls_record_log
				    ("REC[%p]: Note inactive epoch %d has %d users\n",
				     session,
				     session->record_parameters[i]->epoch,
				     session->record_parameters[i]->
				     usage_cnt);
			if (!epoch_alive
			    (session, session->record_parameters[i])) {
				_gnutls_epoch_free(session,
						   session->
						   record_parameters[i]);
				session->record_parameters[i] = NULL;
			}
		}
	}

	/* Look for contiguous NULLs at the start of the array */
	for (i = 0;
	     i < MAX_EPOCH_INDEX && session->record_parameters[i] == NULL;
	     i++);
	min_index = i;

	/* Pick up the slack in the epoch window. */
	for (i = 0, j = min_index; j < MAX_EPOCH_INDEX; i++, j++)
		session->record_parameters[i] =
		    session->record_parameters[j];

	/* Set the new epoch_min */
	if (session->record_parameters[0] != NULL)
		session->security_parameters.epoch_min =
		    session->record_parameters[0]->epoch;

	_gnutls_record_log("REC[%p]: End of epoch cleanup\n", session);
}

static inline void free_record_state(record_state_st * state, int d)
{
	_gnutls_free_datum(&state->mac_secret);
	_gnutls_free_datum(&state->IV);
	_gnutls_free_datum(&state->key);

	_gnutls_auth_cipher_deinit(&state->cipher_state);

	if (state->compression_state.handle != NULL)
		_gnutls_comp_deinit(&state->compression_state, d);
}

void
_gnutls_epoch_free(gnutls_session_t session, record_parameters_st * params)
{
	_gnutls_record_log("REC[%p]: Epoch #%u freed\n", session,
			   params->epoch);

	free_record_state(&params->read, 1);
	free_record_state(&params->write, 0);

	gnutls_free(params);
}
