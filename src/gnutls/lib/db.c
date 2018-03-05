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

/* This file contains functions that manipulate a database backend for
 * resumed sessions.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <db.h>
#include <session_pack.h>
#include <datum.h>
#include "ext/server_name.h"

/**
 * gnutls_db_set_retrieve_function:
 * @session: is a #gnutls_session_t type.
 * @retr_func: is the function.
 *
 * Sets the function that will be used to retrieve data from the
 * resumed sessions database.  This function must return a
 * gnutls_datum_t containing the data on success, or a gnutls_datum_t
 * containing null and 0 on failure.
 *
 * The datum's data must be allocated using the function
 * gnutls_malloc().
 *
 * The first argument to @retr_func will be null unless
 * gnutls_db_set_ptr() has been called.
 **/
void
gnutls_db_set_retrieve_function(gnutls_session_t session,
				gnutls_db_retr_func retr_func)
{
	session->internals.db_retrieve_func = retr_func;
}

/**
 * gnutls_db_set_remove_function:
 * @session: is a #gnutls_session_t type.
 * @rem_func: is the function.
 *
 * Sets the function that will be used to remove data from the
 * resumed sessions database. This function must return 0 on success.
 *
 * The first argument to @rem_func will be null unless
 * gnutls_db_set_ptr() has been called.
 **/
void
gnutls_db_set_remove_function(gnutls_session_t session,
			      gnutls_db_remove_func rem_func)
{
	session->internals.db_remove_func = rem_func;
}

/**
 * gnutls_db_set_store_function:
 * @session: is a #gnutls_session_t type.
 * @store_func: is the function
 *
 * Sets the function that will be used to store data in the resumed
 * sessions database. This function must return 0 on success.
 *
 * The first argument to @store_func will be null unless
 * gnutls_db_set_ptr() has been called.
 **/
void
gnutls_db_set_store_function(gnutls_session_t session,
			     gnutls_db_store_func store_func)
{
	session->internals.db_store_func = store_func;
}

/**
 * gnutls_db_set_ptr:
 * @session: is a #gnutls_session_t type.
 * @ptr: is the pointer
 *
 * Sets the pointer that will be provided to db store, retrieve and
 * delete functions, as the first argument.
 **/
void gnutls_db_set_ptr(gnutls_session_t session, void *ptr)
{
	session->internals.db_ptr = ptr;
}

/**
 * gnutls_db_get_ptr:
 * @session: is a #gnutls_session_t type.
 *
 * Get db function pointer.
 *
 * Returns: the pointer that will be sent to db store, retrieve and
 *   delete functions, as the first argument.
 **/
void *gnutls_db_get_ptr(gnutls_session_t session)
{
	return session->internals.db_ptr;
}

/**
 * gnutls_db_set_cache_expiration:
 * @session: is a #gnutls_session_t type.
 * @seconds: is the number of seconds.
 *
 * Set the expiration time for resumed sessions. The default is 3600
 * (one hour) at the time of this writing.
 **/
void gnutls_db_set_cache_expiration(gnutls_session_t session, int seconds)
{
	session->internals.expire_time = seconds;
}

/**
 * gnutls_db_get_default_cache_expiration:
 *
 * Returns the expiration time (in seconds) of stored sessions for resumption. 
 **/
unsigned gnutls_db_get_default_cache_expiration(void)
{
	return DEFAULT_EXPIRE_TIME;
}

/**
 * gnutls_db_check_entry:
 * @session: is a #gnutls_session_t type.
 * @session_entry: is the session data (not key)
 *
 * This function has no effect. 
 *
 * Returns: Returns %GNUTLS_E_EXPIRED, if the database entry has
 *   expired or 0 otherwise.
 **/
int
gnutls_db_check_entry(gnutls_session_t session,
		      gnutls_datum_t session_entry)
{
	return 0;
}

/**
 * gnutls_db_check_entry_time:
 * @entry: is a pointer to a #gnutls_datum_t type.
 * @t: is the time of the session handshake
 *
 * This function returns the time that this entry was active.
 * It can be used for database entry expiration.
 *
 * Returns: The time this entry was created, or zero on error.
 **/
time_t gnutls_db_check_entry_time(gnutls_datum_t * entry)
{
	uint32_t t;
	uint32_t magic;

	if (entry->size < 8)
		return gnutls_assert_val(0);

	magic = _gnutls_read_uint32(entry->data);

	if (magic != PACKED_SESSION_MAGIC)
		return gnutls_assert_val(0);

	t = _gnutls_read_uint32(&entry->data[4]);

	return t;
}

/* Checks if both db_store and db_retrieve functions have
 * been set up.
 */
static int db_func_is_ok(gnutls_session_t session)
{
	if (session->internals.db_store_func != NULL &&
	    session->internals.db_retrieve_func != NULL)
		return 0;
	else
		return GNUTLS_E_DB_ERROR;
}

/* Stores session data to the db backend.
 */
static int
store_session(gnutls_session_t session,
	      gnutls_datum_t session_id, gnutls_datum_t session_data)
{
	int ret = 0;

	if (db_func_is_ok(session) != 0) {
		return GNUTLS_E_DB_ERROR;
	}

	if (session_data.data == NULL || session_data.size == 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_SESSION;
	}

	/* if we can't read why bother writing? */
	ret = session->internals.db_store_func(session->internals.db_ptr,
					       session_id, session_data);

	return (ret == 0 ? ret : GNUTLS_E_DB_ERROR);
}

int _gnutls_server_register_current_session(gnutls_session_t session)
{
	gnutls_datum_t key;
	gnutls_datum_t content;
	int ret = 0;

	key.data = session->security_parameters.session_id;
	key.size = session->security_parameters.session_id_size;

	if (session->internals.resumable == RESUME_FALSE) {
		gnutls_assert();
		return GNUTLS_E_INVALID_SESSION;
	}

	if (session->security_parameters.session_id_size == 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_SESSION;
	}

	ret = _gnutls_session_pack(session, &content);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = store_session(session, key, content);
	_gnutls_free_datum(&content);

	return ret;
}

int _gnutls_check_resumed_params(gnutls_session_t session)
{
	if (session->internals.resumed_security_parameters.ext_master_secret != 
	    session->security_parameters.ext_master_secret)
	    return gnutls_assert_val(GNUTLS_E_INVALID_SESSION);

	if (!_gnutls_server_name_matches_resumed(session))
	    return gnutls_assert_val(GNUTLS_E_INVALID_SESSION);

	return 0;
}

int
_gnutls_server_restore_session(gnutls_session_t session,
			       uint8_t * session_id, int session_id_size)
{
	gnutls_datum_t data;
	gnutls_datum_t key;
	int ret;

	if (session_id == NULL || session_id_size == 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (session->internals.premaster_set != 0) {	/* hack for CISCO's DTLS-0.9 */
		if (session_id_size ==
		    session->internals.resumed_security_parameters.
		    session_id_size
		    && memcmp(session_id,
			      session->internals.
			      resumed_security_parameters.session_id,
			      session_id_size) == 0)
			return 0;
	}

	key.data = session_id;
	key.size = session_id_size;

	if (db_func_is_ok(session) != 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_SESSION;
	}

	data =
	    session->internals.db_retrieve_func(session->internals.db_ptr,
						key);

	if (data.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_SESSION;
	}

	/* expiration check is performed inside */
	ret = gnutls_session_set_data(session, data.data, data.size);
	gnutls_free(data.data);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_check_resumed_params(session);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/**
 * gnutls_db_remove_session:
 * @session: is a #gnutls_session_t type.
 *
 * This function will remove the current session data from the
 * session database.  This will prevent future handshakes reusing
 * these session data.  This function should be called if a session
 * was terminated abnormally, and before gnutls_deinit() is called.
 *
 * Normally gnutls_deinit() will remove abnormally terminated
 * sessions.
 **/
void gnutls_db_remove_session(gnutls_session_t session)
{
	gnutls_datum_t session_id;
	int ret = 0;

	session_id.data = session->security_parameters.session_id;
	session_id.size = session->security_parameters.session_id_size;

	if (session->internals.db_remove_func == NULL) {
		gnutls_assert();
		return /* GNUTLS_E_DB_ERROR */ ;
	}

	if (session_id.data == NULL || session_id.size == 0) {
		gnutls_assert();
		return /* GNUTLS_E_INVALID_SESSION */ ;
	}

	/* if we can't read why bother writing? */
	ret = session->internals.db_remove_func(session->internals.db_ptr,
						session_id);
	if (ret != 0)
		gnutls_assert();
}
