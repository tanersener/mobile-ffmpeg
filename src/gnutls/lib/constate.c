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
#include <hello_ext.h>
#include <buffers.h>
#include "dtls.h"
#include "secrets.h"
#include "handshake.h"
#include "crypto-api.h"
#include "locks.h"

static const char keyexp[] = "key expansion";
static const int keyexp_length = sizeof(keyexp) - 1;

static int
_tls13_init_record_state(gnutls_cipher_algorithm_t algo, record_state_st *state);

/* This function is to be called after handshake, when master_secret,
 *  client_random and server_random have been initialized. 
 * This function creates the keys and stores them into pending session.
 * (session->cipher_specs)
 */
static int
_gnutls_set_keys(gnutls_session_t session, record_parameters_st * params,
		 unsigned hash_size, unsigned IV_size, unsigned key_size)
{
	uint8_t rnd[2 * GNUTLS_RANDOM_SIZE];
	int pos, ret;
	int block_size;
	char buf[4 * MAX_HASH_SIZE + 4 * MAX_CIPHER_KEY_SIZE +
		 4 * MAX_CIPHER_BLOCK_SIZE];
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
		assert(hash_size<=sizeof(client_write->mac_key));
		client_write->mac_key_size = hash_size;
		memcpy(client_write->mac_key, &key_block[pos], hash_size);

		pos += hash_size;

		server_write->mac_key_size = hash_size;
		memcpy(server_write->mac_key, &key_block[pos], hash_size);

		pos += hash_size;

		_gnutls_hard_log("INT: CLIENT MAC KEY [%d]: %s\n",
				 key_size,
				 _gnutls_bin2hex(client_write->mac_key,
						 hash_size,
						 buf, sizeof(buf), NULL));

		_gnutls_hard_log("INT: SERVER MAC KEY [%d]: %s\n",
				 key_size,
				 _gnutls_bin2hex(server_write->mac_key,
						 hash_size,
						 buf, sizeof(buf), NULL));
	}

	if (key_size > 0) {
		assert(key_size <=sizeof(client_write->key));
		client_write->key_size = key_size;
		memcpy(client_write->key, &key_block[pos], key_size);

		pos += key_size;

		server_write->key_size = key_size;
		memcpy(server_write->key, &key_block[pos], key_size);

		pos += key_size;

		_gnutls_hard_log("INT: CLIENT WRITE KEY [%d]: %s\n",
				 key_size,
				 _gnutls_bin2hex(client_write->key,
						 key_size,
						 buf, sizeof(buf), NULL));

		_gnutls_hard_log("INT: SERVER WRITE KEY [%d]: %s\n",
				 key_size,
				 _gnutls_bin2hex(server_write->key,
						 key_size,
						 buf, sizeof(buf), NULL));

	}

	/* IV generation in export and non export ciphers.
	 */
	if (IV_size > 0) {
		assert(IV_size <= sizeof(client_write->iv));

		client_write->iv_size = IV_size;
		memcpy(client_write->iv, &key_block[pos], IV_size);

		pos += IV_size;

		server_write->iv_size = IV_size;
		memcpy(server_write->iv, &key_block[pos], IV_size);

		_gnutls_hard_log("INT: CLIENT WRITE IV [%d]: %s\n",
				 client_write->iv_size,
				 _gnutls_bin2hex(client_write->iv,
						 client_write->iv_size,
						 buf, sizeof(buf), NULL));

		_gnutls_hard_log("INT: SERVER WRITE IV [%d]: %s\n",
				 server_write->iv_size,
				 _gnutls_bin2hex(server_write->iv,
						 server_write->iv_size,
						 buf, sizeof(buf), NULL));
	}

	return 0;
}

static int
_tls13_update_keys(gnutls_session_t session, hs_stage_t stage,
		   record_parameters_st *params,
		   unsigned iv_size, unsigned key_size)
{
	uint8_t key_block[MAX_CIPHER_KEY_SIZE];
	uint8_t iv_block[MAX_CIPHER_IV_SIZE];
	char buf[65];
	record_state_st *upd_state;
	record_parameters_st *prev = NULL;
	int ret;

	/* generate new keys for direction needed and copy old from previous epoch */

	if (stage == STAGE_UPD_OURS) {
		upd_state = &params->write;

		ret = _gnutls_epoch_get(session, EPOCH_READ_CURRENT, &prev);
		if (ret < 0)
			return gnutls_assert_val(ret);
		assert(prev != NULL);

		params->read.sequence_number = prev->read.sequence_number;

		params->read.key_size = prev->read.key_size;
		memcpy(params->read.key, prev->read.key, prev->read.key_size);

		_gnutls_hard_log("INT: READ KEY [%d]: %s\n",
				 params->read.key_size,
				 _gnutls_bin2hex(params->read.key, params->read.key_size,
						 buf, sizeof(buf), NULL));

		params->read.iv_size = prev->read.iv_size;
		memcpy(params->read.iv, prev->read.iv, prev->read.key_size);

		_gnutls_hard_log("INT: READ IV [%d]: %s\n",
				 params->read.iv_size,
				 _gnutls_bin2hex(params->read.iv, params->read.iv_size,
						 buf, sizeof(buf), NULL));
	} else {
		upd_state = &params->read;

		ret = _gnutls_epoch_get(session, EPOCH_WRITE_CURRENT, &prev);
		if (ret < 0)
			return gnutls_assert_val(ret);
		assert(prev != NULL);

		params->write.sequence_number = prev->write.sequence_number;

		params->write.key_size = prev->write.key_size;
		memcpy(params->write.key, prev->write.key, prev->write.key_size);

		_gnutls_hard_log("INT: WRITE KEY [%d]: %s\n",
				 params->write.key_size,
				 _gnutls_bin2hex(params->write.key, params->write.key_size,
						 buf, sizeof(buf), NULL));

		params->write.iv_size = prev->write.iv_size;
		memcpy(params->write.iv, prev->write.iv, prev->write.iv_size);

		_gnutls_hard_log("INT: WRITE IV [%d]: %s\n",
				 params->write.iv_size,
				 _gnutls_bin2hex(params->write.iv, params->write.iv_size,
						 buf, sizeof(buf), NULL));
	}


	if ((session->security_parameters.entity == GNUTLS_CLIENT && stage == STAGE_UPD_OURS) ||
	    (session->security_parameters.entity == GNUTLS_SERVER && stage == STAGE_UPD_PEERS)) {

		/* client keys */
		ret = _tls13_expand_secret(session, APPLICATION_TRAFFIC_UPDATE,
					   sizeof(APPLICATION_TRAFFIC_UPDATE)-1,
					   NULL, 0,
					   session->key.proto.tls13.ap_ckey,
					   session->security_parameters.prf->output_size,
					   session->key.proto.tls13.ap_ckey);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _tls13_expand_secret(session, "key", 3, NULL, 0, session->key.proto.tls13.ap_ckey, key_size, key_block);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _tls13_expand_secret(session, "iv", 2, NULL, 0, session->key.proto.tls13.ap_ckey, iv_size, iv_block);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else {
		ret = _tls13_expand_secret(session, APPLICATION_TRAFFIC_UPDATE,
					   sizeof(APPLICATION_TRAFFIC_UPDATE)-1,
					   NULL, 0,
					   session->key.proto.tls13.ap_skey,
					   session->security_parameters.prf->output_size,
					   session->key.proto.tls13.ap_skey);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _tls13_expand_secret(session, "key", 3, NULL, 0, session->key.proto.tls13.ap_skey, key_size, key_block);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _tls13_expand_secret(session, "iv", 2, NULL, 0, session->key.proto.tls13.ap_skey, iv_size, iv_block);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	upd_state->mac_key_size = 0;

	assert(key_size <= sizeof(upd_state->key));
	memcpy(upd_state->key, key_block, key_size);
	upd_state->key_size = key_size;

	_gnutls_hard_log("INT: NEW %s KEY [%d]: %s\n",
			 (upd_state == &params->read)?"READ":"WRITE",
			 key_size,
			 _gnutls_bin2hex(key_block, key_size,
					 buf, sizeof(buf), NULL));

	if (iv_size > 0) {
		assert(iv_size <= sizeof(upd_state->iv));
		memcpy(upd_state->iv, iv_block, iv_size);
		upd_state->iv_size = iv_size;

		_gnutls_hard_log("INT: NEW %s IV [%d]: %s\n",
				 (upd_state == &params->read)?"READ":"WRITE",
				 iv_size,
				 _gnutls_bin2hex(iv_block, iv_size,
						 buf, sizeof(buf), NULL));
	}

	return 0;
}

static int
_tls13_set_early_keys(gnutls_session_t session,
		      record_parameters_st * params,
		      unsigned iv_size, unsigned key_size)
{
	uint8_t key_block[MAX_CIPHER_KEY_SIZE];
	uint8_t iv_block[MAX_CIPHER_IV_SIZE];
	char buf[65];
	record_state_st *early_state;
	int ret;

	if (session->security_parameters.entity == GNUTLS_CLIENT &&
	    !(session->internals.hsk_flags & HSK_TLS13_TICKET_SENT)) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _tls13_expand_secret(session, "key", 3, NULL, 0, session->key.proto.tls13.e_ckey, key_size, key_block);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _tls13_expand_secret(session, "iv", 2, NULL, 0, session->key.proto.tls13.e_ckey, iv_size, iv_block);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		early_state = &params->write;
	} else {
		early_state = &params->read;
	}

	early_state->mac_key_size = 0;

	assert(key_size <= sizeof(early_state->key));
	memcpy(early_state->key, key_block, key_size);
	early_state->key_size = key_size;

	_gnutls_hard_log("INT: EARLY KEY [%d]: %s\n",
			 key_size,
			 _gnutls_bin2hex(key_block, key_size,
					 buf, sizeof(buf), NULL));

	if (iv_size > 0) {
		assert(iv_size <= sizeof(early_state->iv));
		memcpy(early_state->iv, iv_block, iv_size);
		early_state->iv_size = iv_size;

		_gnutls_hard_log("INT: EARLY IV [%d]: %s\n",
				 iv_size,
				 _gnutls_bin2hex(iv_block, iv_size,
						 buf, sizeof(buf), NULL));
	}

	return 0;
}

static int
_tls13_set_keys(gnutls_session_t session, hs_stage_t stage,
		record_parameters_st * params,
		unsigned iv_size, unsigned key_size)
{
	uint8_t ckey_block[MAX_CIPHER_KEY_SIZE];
	uint8_t civ_block[MAX_CIPHER_IV_SIZE];
	uint8_t skey_block[MAX_CIPHER_KEY_SIZE];
	uint8_t siv_block[MAX_CIPHER_IV_SIZE];
	char buf[65];
	record_state_st *client_write, *server_write;
	const char *label;
	unsigned label_size, hsk_len;
	const char *keylog_label;
	void *ckey, *skey;
	int ret;

	if (stage == STAGE_UPD_OURS || stage == STAGE_UPD_PEERS)
		return _tls13_update_keys(session, stage,
					  params, iv_size, key_size);

	else if (stage == STAGE_EARLY)
		return _tls13_set_early_keys(session,
					     params, iv_size, key_size);

	else if (stage == STAGE_HS) {
		label = HANDSHAKE_CLIENT_TRAFFIC_LABEL;
		label_size = sizeof(HANDSHAKE_CLIENT_TRAFFIC_LABEL)-1;
		hsk_len = session->internals.handshake_hash_buffer.length;
		keylog_label = "CLIENT_HANDSHAKE_TRAFFIC_SECRET";
		ckey = session->key.proto.tls13.hs_ckey;
	} else {
		label = APPLICATION_CLIENT_TRAFFIC_LABEL;
		label_size = sizeof(APPLICATION_CLIENT_TRAFFIC_LABEL)-1;
		hsk_len = session->internals.handshake_hash_buffer_server_finished_len;
		keylog_label = "CLIENT_TRAFFIC_SECRET_0";
		ckey = session->key.proto.tls13.ap_ckey;
	}

	ret = _tls13_derive_secret(session, label, label_size,
				   session->internals.handshake_hash_buffer.data,
				   hsk_len,
				   session->key.proto.tls13.temp_secret,
				   ckey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_call_keylog_func(session, keylog_label,
				       ckey,
				       session->security_parameters.prf->output_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* client keys */
	ret = _tls13_expand_secret(session, "key", 3, NULL, 0, ckey, key_size, ckey_block);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _tls13_expand_secret(session, "iv", 2, NULL, 0, ckey, iv_size, civ_block);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* server keys */
	if (stage == STAGE_HS) {
		label = HANDSHAKE_SERVER_TRAFFIC_LABEL;
		label_size = sizeof(HANDSHAKE_SERVER_TRAFFIC_LABEL)-1;
		keylog_label = "SERVER_HANDSHAKE_TRAFFIC_SECRET";
		skey = session->key.proto.tls13.hs_skey;
	} else {
		label = APPLICATION_SERVER_TRAFFIC_LABEL;
		label_size = sizeof(APPLICATION_SERVER_TRAFFIC_LABEL)-1;
		keylog_label = "SERVER_TRAFFIC_SECRET_0";
		skey = session->key.proto.tls13.ap_skey;
	}

	ret = _tls13_derive_secret(session, label, label_size,
				   session->internals.handshake_hash_buffer.data,
				   hsk_len,
				   session->key.proto.tls13.temp_secret,
				   skey);

	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_call_keylog_func(session, keylog_label,
				       skey,
				       session->security_parameters.prf->output_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _tls13_expand_secret(session, "key", 3, NULL, 0, skey, key_size, skey_block);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _tls13_expand_secret(session, "iv", 2, NULL, 0, skey, iv_size, siv_block);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		client_write = &params->write;
		server_write = &params->read;
	} else {
		client_write = &params->read;
		server_write = &params->write;
	}

	client_write->mac_key_size = 0;
	server_write->mac_key_size = 0;

	assert(key_size <= sizeof(client_write->key));
	memcpy(client_write->key, ckey_block, key_size);
	client_write->key_size = key_size;

	_gnutls_hard_log("INT: CLIENT WRITE KEY [%d]: %s\n",
			 key_size,
			 _gnutls_bin2hex(ckey_block, key_size,
					 buf, sizeof(buf), NULL));

	memcpy(server_write->key, skey_block, key_size);
	server_write->key_size = key_size;

	_gnutls_hard_log("INT: SERVER WRITE KEY [%d]: %s\n",
			 key_size,
			 _gnutls_bin2hex(skey_block, key_size,
					 buf, sizeof(buf), NULL));

	if (iv_size > 0) {
		assert(iv_size <= sizeof(client_write->iv));
		memcpy(client_write->iv, civ_block, iv_size);
		client_write->iv_size = iv_size;

		_gnutls_hard_log("INT: CLIENT WRITE IV [%d]: %s\n",
				 iv_size,
				 _gnutls_bin2hex(civ_block, iv_size,
						 buf, sizeof(buf), NULL));

		memcpy(server_write->iv, siv_block, iv_size);
		server_write->iv_size = iv_size;

		_gnutls_hard_log("INT: SERVER WRITE IV [%d]: %s\n",
				 iv_size,
				 _gnutls_bin2hex(siv_block, iv_size,
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
	gnutls_datum_t *iv = NULL, _iv;
	gnutls_datum_t key;
	gnutls_datum_t mac;

	_iv.data = state->iv;
	_iv.size = state->iv_size;

	key.data = state->key;
	key.size = state->key_size;

	mac.data = state->mac_key;
	mac.size = state->mac_key_size;

	if (_gnutls_cipher_type(params->cipher) == CIPHER_BLOCK) {
		if (!_gnutls_version_has_explicit_iv(ver))
			iv = &_iv;
	} else if (_gnutls_cipher_type(params->cipher) == CIPHER_STREAM) {
		/* To handle GOST ciphersuites */
		if (_gnutls_cipher_get_implicit_iv_size(params->cipher))
			iv = &_iv;
	}

	ret = _gnutls_auth_cipher_init(&state->ctx.tls12,
				       params->cipher, &key, iv,
				       params->mac, &mac,
				       params->etm,
#ifdef ENABLE_SSL3
				       (ver->id == GNUTLS_SSL3) ? 1 : 0,
#endif
				       1 - read /*1==encrypt */ );
	if (ret < 0 && params->cipher->id != GNUTLS_CIPHER_NULL)
		return gnutls_assert_val(ret);

	return 0;
}

int
_gnutls_set_cipher_suite2(gnutls_session_t session,
			  const gnutls_cipher_suite_entry_st *cs)
{
	const cipher_entry_st *cipher_algo;
	const mac_entry_st *mac_algo;
	record_parameters_st *params;
	int ret;
	const version_entry_st *ver = get_version(session);

	ret = _gnutls_epoch_get(session, EPOCH_NEXT, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	cipher_algo = cipher_to_entry(cs->block_algorithm);
	mac_algo = mac_to_entry(cs->mac_algorithm);

	if (ver->tls13_sem && (session->internals.hsk_flags & HSK_HRR_SENT)) {
		if (params->initialized && (params->cipher != cipher_algo ||
		    params->mac != mac_algo || cs != session->security_parameters.cs))
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

		return 0;
	} else {
		if (params->initialized
		    || params->cipher != NULL || params->mac != NULL)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	if (_gnutls_cipher_is_ok(cipher_algo) == 0
	    || _gnutls_mac_is_ok(mac_algo) == 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	if (_gnutls_version_has_selectable_prf(get_version(session))) {
		if (cs->prf == GNUTLS_MAC_UNKNOWN ||
		    _gnutls_mac_is_ok(mac_to_entry(cs->prf)) == 0)
			return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);
		session->security_parameters.prf = mac_to_entry(cs->prf);
	} else {
		session->security_parameters.prf = mac_to_entry(GNUTLS_MAC_MD5_SHA1);
	}

	session->security_parameters.cs = cs;
	params->cipher = cipher_algo;
	params->mac = mac_algo;

	return 0;
}

/* Sets the next epoch to be a clone of the current one.
 * The keys are not cloned, only the cipher and MAC.
 */
int _gnutls_epoch_dup(gnutls_session_t session, unsigned int epoch_rel)
{
	record_parameters_st *prev;
	record_parameters_st *next;
	int ret;

	ret = _gnutls_epoch_get(session, epoch_rel, &prev);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_epoch_get(session, EPOCH_NEXT, &next);
	if (ret < 0) {
		ret = _gnutls_epoch_setup_next(session, 0, &next);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	if (next->initialized
	    || next->cipher != NULL || next->mac != NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	next->cipher = prev->cipher;
	next->mac = prev->mac;

	return 0;
}

int _gnutls_epoch_set_keys(gnutls_session_t session, uint16_t epoch, hs_stage_t stage)
{
	int hash_size;
	int IV_size;
	int key_size;
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

	if (_gnutls_cipher_is_ok(params->cipher) == 0 ||
	    _gnutls_mac_is_ok(params->mac) == 0)
		return gnutls_assert_val(GNUTLS_E_UNWANTED_ALGORITHM);

	if (_gnutls_version_has_explicit_iv(ver) &&
	    (_gnutls_cipher_type(params->cipher) != CIPHER_BLOCK)) {
		IV_size = _gnutls_cipher_get_implicit_iv_size(params->cipher);
	} else {
		IV_size = _gnutls_cipher_get_iv_size(params->cipher);
	}

	key_size = _gnutls_cipher_get_key_size(params->cipher);
	hash_size = _gnutls_mac_get_key_size(params->mac);
	params->etm = session->security_parameters.etm;

	if (ver->tls13_sem) {
		ret = _tls13_set_keys
		    (session, stage, params, IV_size, key_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (stage != STAGE_EARLY ||
		    session->security_parameters.entity == GNUTLS_SERVER) {
			ret = _tls13_init_record_state(params->cipher->id, &params->read);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}

		if (stage != STAGE_EARLY ||
		    session->security_parameters.entity == GNUTLS_CLIENT) {
			ret = _tls13_init_record_state(params->cipher->id, &params->write);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
	} else {
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
	}

	/* The TLS1.3 limit of 256 additional bytes is also enforced under CBC
	 * ciphers to ensure we interoperate with gnutls 2.12.x which could add padding
	 * data exceeding the maximum. */
	if (ver->tls13_sem || _gnutls_cipher_type(params->cipher) == CIPHER_BLOCK) {
		session->internals.max_recv_size = 256;
	} else {
		session->internals.max_recv_size = 0;
	}

	if (!ver->tls13_sem) {
		session->internals.max_recv_size += _gnutls_record_overhead(ver, params->cipher, params->mac, 1);
		if (session->internals.allow_large_records != 0)
			session->internals.max_recv_size += EXTRA_COMP_SIZE;
	}

	session->internals.max_recv_size += session->security_parameters.max_record_recv_size + RECORD_HEADER_SIZE(session);

	_dtls_reset_window(params);

	_gnutls_record_log("REC[%p]: Epoch #%u ready\n", session,
			   params->epoch);

	params->initialized = 1;
	return 0;
}

/* This copies the session values which apply to subsequent/resumed
 * sessions. Under TLS 1.3, these values are items which are not
 * negotiated on the subsequent session. */
#define CPY_COMMON(tls13_sem) \
	if (!tls13_sem) { \
		dst->cs = src->cs; \
		memcpy(dst->master_secret, src->master_secret, GNUTLS_MASTER_SIZE); \
		memcpy(dst->client_random, src->client_random, GNUTLS_RANDOM_SIZE); \
		memcpy(dst->server_random, src->server_random, GNUTLS_RANDOM_SIZE); \
		dst->ext_master_secret = src->ext_master_secret; \
		dst->etm = src->etm; \
		dst->prf = src->prf; \
		dst->grp = src->grp; \
		dst->pversion = src->pversion; \
	} \
	memcpy(dst->session_id, src->session_id, GNUTLS_MAX_SESSION_ID_SIZE); \
	dst->session_id_size = src->session_id_size; \
	dst->timestamp = src->timestamp; \
	dst->client_ctype = src->client_ctype; \
	dst->server_ctype = src->server_ctype; \
	dst->client_auth_type = src->client_auth_type; \
	dst->server_auth_type = src->server_auth_type

void _gnutls_set_resumed_parameters(gnutls_session_t session)
{
	security_parameters_st *src =
	    &session->internals.resumed_security_parameters;
	security_parameters_st *dst = &session->security_parameters;
	const version_entry_st *ver = get_version(session);

	CPY_COMMON(ver->tls13_sem);

	if (!ver->tls13_sem &&
	    !(session->internals.hsk_flags & HSK_RECORD_SIZE_LIMIT_NEGOTIATED)) {
		dst->max_record_recv_size = src->max_record_recv_size;
		dst->max_record_send_size = src->max_record_send_size;
	}
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
	if (session->internals.resumed != RESUME_FALSE &&
	    session->security_parameters.entity == GNUTLS_CLIENT)
		_gnutls_set_resumed_parameters(session);

	ret = _gnutls_epoch_set_keys(session, epoch_next, 0);
	if (ret < 0)
		return ret;

	_gnutls_handshake_log("HSK[%p]: Cipher Suite: %s\n",
			      session,
			      session->security_parameters.cs->name);

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

	/* reset max_record_send_size if it was negotiated in the
	 * previous handshake using the record_size_limit extension */
	if (!(session->internals.hsk_flags & HSK_RECORD_SIZE_LIMIT_NEGOTIATED) &&
	    session->security_parameters.entity == GNUTLS_SERVER)
		session->security_parameters.max_record_send_size =
			session->security_parameters.max_user_record_send_size;

/* Update internals from CipherSuite selected.
 * If we are resuming just copy the connection session
 */
	if (session->internals.resumed != RESUME_FALSE &&
	    session->security_parameters.entity == GNUTLS_SERVER)
		_gnutls_set_resumed_parameters(session);

	ret = _gnutls_epoch_set_keys(session, epoch_next, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_handshake_log("HSK[%p]: Cipher Suite: %s\n", session,
			      session->security_parameters.cs->name);

	_gnutls_handshake_log
	    ("HSK[%p]: Initializing internal [write] cipher sessions\n",
	     session);

	session->security_parameters.epoch_write = epoch_next;

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

	gnutls_mutex_lock(&session->internals.epoch_lock);

	ret = epoch_resolve(session, epoch_rel, &epoch);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	params = epoch_get_slot(session, epoch);
	if (params == NULL || *params == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	if (params_out)
		*params_out = *params;

	ret = 0;

cleanup:
	gnutls_mutex_unlock(&session->internals.epoch_lock);
	return ret;
}

/* Ensures that the next epoch is setup. When an epoch will null ciphers
 * is to be setup, call with @null_epoch set to true. In that case
 * the epoch is fully initialized after call.
 */
int
_gnutls_epoch_setup_next(gnutls_session_t session, unsigned null_epoch, record_parameters_st **newp)
{
	record_parameters_st **slot;

	slot = epoch_get_slot(session, session->security_parameters.epoch_next);

	/* If slot out of range or not empty. */
	if (slot == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (*slot != NULL) { /* already initialized */
		if (unlikely(null_epoch && !(*slot)->initialized))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (unlikely((*slot)->epoch != session->security_parameters.epoch_next))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		goto finish;
	}

	_gnutls_record_log("REC[%p]: Allocating epoch #%u\n", session,
			   session->security_parameters.epoch_next);

	*slot = gnutls_calloc(1, sizeof(record_parameters_st));
	if (*slot == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	(*slot)->epoch = session->security_parameters.epoch_next;

	if (null_epoch) {
		(*slot)->cipher = cipher_to_entry(GNUTLS_CIPHER_NULL);
		(*slot)->mac = mac_to_entry(GNUTLS_MAC_NULL);
		(*slot)->initialized = 1;
	} else {
		(*slot)->cipher = NULL;
		(*slot)->mac = NULL;
	}

	if (IS_DTLS(session)) {
		uint64_t seq = (*slot)->write.sequence_number;
		seq &= UINT64_C(0xffffffffffff);
		seq |= ((uint64_t)session->security_parameters.epoch_next) << 48;
		(*slot)->write.sequence_number = seq;
	}

 finish:
	if (newp != NULL)
		*newp = *slot;

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

	gnutls_mutex_lock(&session->internals.epoch_lock);

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
	if (min_index != 0) {
		for (i = 0, j = min_index; j < MAX_EPOCH_INDEX; i++, j++) {
			session->record_parameters[i] =
			    session->record_parameters[j];
			session->record_parameters[j] = NULL;
		}
	}

	/* Set the new epoch_min */
	if (session->record_parameters[0] != NULL)
		session->security_parameters.epoch_min =
		    session->record_parameters[0]->epoch;

	gnutls_mutex_unlock(&session->internals.epoch_lock);

	_gnutls_record_log("REC[%p]: End of epoch cleanup\n", session);
}

static inline void free_record_state(record_state_st * state)
{
	zeroize_temp_key(state->mac_key, state->mac_key_size);
	zeroize_temp_key(state->iv, state->iv_size);
	zeroize_temp_key(state->key, state->key_size);

	if (state->is_aead)
		_gnutls_aead_cipher_deinit(&state->ctx.aead);
	else
		_gnutls_auth_cipher_deinit(&state->ctx.tls12);
}

void
_gnutls_epoch_free(gnutls_session_t session, record_parameters_st * params)
{
	_gnutls_record_log("REC[%p]: Epoch #%u freed\n", session,
			   params->epoch);

	free_record_state(&params->read);
	free_record_state(&params->write);

	gnutls_free(params);
}

int _tls13_connection_state_init(gnutls_session_t session, hs_stage_t stage)
{
	const uint16_t epoch_next =
	    session->security_parameters.epoch_next;
	int ret;

	ret = _gnutls_epoch_set_keys(session, epoch_next, stage);
	if (ret < 0)
		return ret;

	_gnutls_handshake_log("HSK[%p]: TLS 1.3 re-key with cipher suite: %s\n",
			      session,
			      session->security_parameters.cs->name);

	session->security_parameters.epoch_read = epoch_next;
	session->security_parameters.epoch_write = epoch_next;

	return 0;
}

int _tls13_read_connection_state_init(gnutls_session_t session, hs_stage_t stage)
{
	const uint16_t epoch_next =
	    session->security_parameters.epoch_next;
	int ret;

	ret = _gnutls_epoch_set_keys(session, epoch_next, stage);
	if (ret < 0)
		return ret;

	_gnutls_handshake_log("HSK[%p]: TLS 1.3 set read key with cipher suite: %s\n",
			      session,
			      session->security_parameters.cs->name);

	session->security_parameters.epoch_read = epoch_next;

	return 0;
}

int _tls13_write_connection_state_init(gnutls_session_t session, hs_stage_t stage)
{
	const uint16_t epoch_next =
	    session->security_parameters.epoch_next;
	int ret;

	ret = _gnutls_epoch_set_keys(session, epoch_next, stage);
	if (ret < 0)
		return ret;

	_gnutls_handshake_log("HSK[%p]: TLS 1.3 set write key with cipher suite: %s\n",
			      session,
			      session->security_parameters.cs->name);

	session->security_parameters.epoch_write = epoch_next;

	return 0;
}

static int
_tls13_init_record_state(gnutls_cipher_algorithm_t algo, record_state_st *state)
{
	int ret;
	gnutls_datum_t key;

	key.data = state->key;
	key.size = state->key_size;

	ret = _gnutls_aead_cipher_init(&state->ctx.aead,
				       algo, &key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	state->aead_tag_size = gnutls_cipher_get_tag_size(algo);
	state->is_aead = 1;

	return 0;
}
