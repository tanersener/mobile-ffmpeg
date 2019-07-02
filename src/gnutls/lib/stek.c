/*
 * Copyright (C) 2018 Free Software Foundation, Inc.
 *
 * Author: Ander Juaristi
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
#include "stek.h"

#define NAME_POS (0)
#define KEY_POS (TICKET_KEY_NAME_SIZE)
#define MAC_SECRET_POS (TICKET_KEY_NAME_SIZE+TICKET_CIPHER_KEY_SIZE)

static int totp_sha3(gnutls_session_t session,
		uint64_t t,
		const gnutls_datum_t *secret,
		uint8_t out[TICKET_MASTER_KEY_SIZE])
{
	int retval;
	uint8_t t_be[8];
	digest_hd_st hd;
	/*
	 * We choose SHA3-512 because it outputs 64 bytes,
	 * just the same length as the ticket key.
	 */
	const gnutls_digest_algorithm_t algo = GNUTLS_DIG_SHA3_512;
#if TICKET_MASTER_KEY_SIZE != 64
#error "TICKET_MASTER_KEY_SIZE must be 64 bytes"
#endif

	if (unlikely(secret == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if ((retval = _gnutls_hash_init(&hd, hash_to_entry(algo))) < 0)
		return gnutls_assert_val(retval);

	_gnutls_write_uint64(t, t_be);

	if ((retval = _gnutls_hash(&hd, t_be, sizeof(t_be))) < 0)
		return gnutls_assert_val(retval);
	if ((retval = _gnutls_hash(&hd, secret->data, secret->size)) < 0)
		return gnutls_assert_val(retval);

	_gnutls_hash_deinit(&hd, out);
	return GNUTLS_E_SUCCESS;
}

static uint64_t T(gnutls_session_t session, time_t t)
{
	uint64_t numeral = t;
	unsigned int x = session->internals.expire_time * STEK_ROTATION_PERIOD_PRODUCT;

	if (numeral <= 0)
		return 0;

	return (numeral / x);
}

static int64_t totp_next(gnutls_session_t session)
{
	time_t t;
	uint64_t result;

	t = gnutls_time(NULL);
	if (unlikely(t == (time_t) -1))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	result = T(session, t);
	if (result == 0)
		return 0;

	if (result == session->key.totp.last_result)
		return 0;

	return result;
}

static int64_t totp_previous(gnutls_session_t session)
{
	uint64_t result;

	if (session->key.totp.last_result == 0)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	if (!session->key.totp.was_rotated)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	result = session->key.totp.last_result - 1;
	if (result == 0)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	return result;
}

static void call_rotation_callback(gnutls_session_t session,
		uint8_t key[TICKET_MASTER_KEY_SIZE], uint64_t t)
{
	gnutls_datum_t prev_key, new_key;

	if (session->key.totp.cb) {
		new_key.data = key;
		new_key.size = TICKET_MASTER_KEY_SIZE;
		prev_key.data = session->key.session_ticket_key;
		prev_key.size = TICKET_MASTER_KEY_SIZE;

		session->key.totp.cb(&prev_key, &new_key, t);
	}
}

static int rotate(gnutls_session_t session)
{
	int64_t t;
	gnutls_datum_t secret;
	uint8_t key[TICKET_MASTER_KEY_SIZE];

	/* Do we need to calculate new totp? */
	t = totp_next(session);
	if (t > 0) {
		secret.data = session->key.initial_stek;
		secret.size = TICKET_MASTER_KEY_SIZE;

		/* Generate next key */
		if (totp_sha3(session, t, &secret, key) < 0) {
			gnutls_assert();
			return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		}

		/* Replace old key with new one, and call callback if provided */
		call_rotation_callback(session, key, t);
		session->key.totp.last_result = t;
		memcpy(session->key.session_ticket_key, key, sizeof(key));

		session->key.totp.was_rotated = 1;
	} else if (t < 0) {
		return gnutls_assert_val(t);
	}

	return GNUTLS_E_SUCCESS;
}

static int rotate_back_and_peek(gnutls_session_t session,
				uint8_t key[TICKET_MASTER_KEY_SIZE])
{
	int64_t t;
	gnutls_datum_t secret;

	/* Get the previous TOTP */
	t = totp_previous(session);
	if (t < 0)
		return gnutls_assert_val(t);

	secret.data = session->key.initial_stek;
	secret.size = TICKET_MASTER_KEY_SIZE;

	if (totp_sha3(session, t, &secret, key) < 0)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	return 0;
}

/*
 * _gnutls_get_session_ticket_encryption_key:
 * @key_name: an empty datum that will receive the key name part of the STEK
 * @mac_key: an empty datum that will receive the MAC key part of the STEK
 * @enc_key: an empty datum that will receive the encryption key part of the STEK
 *
 * Get the currently active session ticket encryption key (STEK).
 *
 * The STEK is a 64-byte blob which is further divided in three parts,
 * and this function requires the caller to supply three separate datums for each one.
 * Though the caller might omit one or more of those if not interested in that part of the STEK.
 *
 * These are the three parts the STEK is divided in:
 *
 *  - Key name: 16 bytes
 *  - Encryption key: 32 bytes
 *  - MAC key: 16 bytes
 *
 * This function will transparently rotate the key, if the time has come for that,
 * before returning it to the caller.
 */
int _gnutls_get_session_ticket_encryption_key(gnutls_session_t session,
					      gnutls_datum_t *key_name,
					      gnutls_datum_t *mac_key,
					      gnutls_datum_t *enc_key)
{
	int retval;

	if (unlikely(session == NULL)) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if ((retval = rotate(session)) < 0)
		return gnutls_assert_val(retval);

	/* Copy key parts to user-supplied datums (if provided) */
	if (key_name) {
		key_name->data = &session->key.session_ticket_key[NAME_POS];
		key_name->size = TICKET_KEY_NAME_SIZE;
	}
	if (mac_key) {
		mac_key->data = &session->key.session_ticket_key[MAC_SECRET_POS];
		mac_key->size = TICKET_MAC_SECRET_SIZE;
	}
	if (enc_key) {
		enc_key->data = &session->key.session_ticket_key[KEY_POS];
		enc_key->size = TICKET_CIPHER_KEY_SIZE;
	}

	return retval;
}

/*
 * _gnutls_get_session_ticket_decryption_key:
 * @ticket_data: the bytes of a session ticket that must be decrypted
 * @key_name: an empty datum that will receive the key name part of the STEK
 * @mac_key: an empty datum that will receive the MAC key part of the STEK
 * @enc_key: an empty datum that will receive the encryption key part of the STEK
 *
 * Get the key (STEK) the given session ticket was encrypted with.
 *
 * As with its encryption counterpart (%_gnutls_get_session_ticket_encryption_key),
 * this function will also transparently rotate
 * the currently active STEK if time has come for that, and it also requires the different
 * parts of the STEK to be obtained in different datums.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or a negative error code, such as
 * %GNUTLS_E_REQUSTED_DATA_NOT_AVAILABLE if no key could be found for the supplied ticket.
 */
int _gnutls_get_session_ticket_decryption_key(gnutls_session_t session,
					      const gnutls_datum_t *ticket_data,
					      gnutls_datum_t *key_name,
					      gnutls_datum_t *mac_key,
					      gnutls_datum_t *enc_key)
{
	int retval;
	gnutls_datum_t key = {
		.data = session->key.session_ticket_key,
		.size = TICKET_MASTER_KEY_SIZE
	};

	if (unlikely(session == NULL || ticket_data == NULL || ticket_data->data == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (ticket_data->size < TICKET_KEY_NAME_SIZE)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if ((retval = rotate(session)) < 0)
		return gnutls_assert_val(retval);

	/*
	 * Is current key valid?
	 * We compare the first 16 bytes --> The key_name field.
	 */
	if (memcmp(ticket_data->data,
		   &key.data[NAME_POS],
		   TICKET_KEY_NAME_SIZE) == 0)
		goto key_found;

	key.size = TICKET_MASTER_KEY_SIZE;
	key.data = session->key.previous_ticket_key;

	/*
	 * Current key is not valid.
	 * Compute previous key and see if that matches.
	 */
	if ((retval = rotate_back_and_peek(session, key.data)) < 0)
		return gnutls_assert_val(retval);

	if (memcmp(ticket_data->data,
		   &key.data[NAME_POS],
		   TICKET_KEY_NAME_SIZE) == 0)
		goto key_found;

	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

key_found:
	if (key_name) {
		key_name->data = &key.data[NAME_POS];
		key_name->size = TICKET_KEY_NAME_SIZE;
	}
	if (mac_key) {
		mac_key->data = &key.data[MAC_SECRET_POS];
		mac_key->size = TICKET_MAC_SECRET_SIZE;
	}
	if (enc_key) {
		enc_key->data = &key.data[KEY_POS];
		enc_key->size = TICKET_CIPHER_KEY_SIZE;
	}

	return GNUTLS_E_SUCCESS;
}

/*
 * _gnutls_initialize_session_ticket_key_rotation:
 * @key: Initial session ticket key
 *
 * Initialize the session ticket key rotation.
 *
 * This function will not enable session ticket keys on the server side. That is done
 * with the gnutls_session_ticket_enable_server() function. This function just initializes
 * the internal state to support periodical rotation of the session ticket encryption key.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success, or %GNUTLS_E_INVALID_REQUEST on error.
 */
int _gnutls_initialize_session_ticket_key_rotation(gnutls_session_t session, const gnutls_datum_t *key)
{
	if (unlikely(session == NULL || key == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (session->key.totp.last_result == 0) {
		int64_t t;
		memcpy(session->key.initial_stek, key->data, key->size);
		t = totp_next(session);
		if (t < 0)
			return gnutls_assert_val(t);

		session->key.totp.last_result = t;
		session->key.totp.was_rotated = 0;

		return GNUTLS_E_SUCCESS;
	}

	return GNUTLS_E_INVALID_REQUEST;
}

/*
 * _gnutls_set_session_ticket_key_rotation_callback:
 * @cb: the callback function
 *
 * Set a callback function that will be invoked every time the session ticket key
 * is rotated.
 *
 * The function will take as arguments the previous key, the new key and the time
 * step value that caused the key to rotate.
 *
 */
void _gnutls_set_session_ticket_key_rotation_callback(gnutls_session_t session, gnutls_stek_rotation_callback_t cb)
{
	if (session)
		session->key.totp.cb = cb;
}
