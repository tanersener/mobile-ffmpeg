/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Daiki Ueno
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
#include "db.h"
#include "system.h"
#include "tls13/anti_replay.h"

/* The default time window in milliseconds; RFC8446 suggests the order
 * of ten seconds is sufficient for the clients on the Internet. */
#define DEFAULT_WINDOW_MS 10000

struct gnutls_anti_replay_st {
	uint32_t window;
	struct timespec start_time;
	gnutls_db_add_func db_add_func;
	void *db_ptr;
};

/**
 * gnutls_anti_replay_init:
 * @anti_replay: is a pointer to #gnutls_anti_replay_t type
 *
 * This function will allocate and initialize the @anti_replay context
 * to be usable for detect replay attacks. The context can then be
 * attached to a @gnutls_session_t with
 * gnutls_anti_replay_enable().
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 3.6.5
 **/
int
gnutls_anti_replay_init(gnutls_anti_replay_t *anti_replay)
{
	*anti_replay = gnutls_calloc(1, sizeof(struct gnutls_anti_replay_st));
	if (!*anti_replay)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	(*anti_replay)->window = DEFAULT_WINDOW_MS;

	gnutls_gettime(&(*anti_replay)->start_time);

	return 0;
}

/**
 * gnutls_anti_replay_set_window:
 * @anti_replay: is a #gnutls_anti_replay_t type.
 * @window: is the time window recording ClientHello, in milliseconds
 *
 * Sets the time window used for ClientHello recording.  In order to
 * protect against replay attacks, the server records ClientHello
 * messages within this time period from the last update, and
 * considers it a replay when a ClientHello outside of the period; if
 * a ClientHello arrives within this period, the server checks the
 * database and detects duplicates.
 *
 * For the details of the algorithm, see RFC 8446, section 8.2.
 *
 * Since: 3.6.5
 */
void
gnutls_anti_replay_set_window(gnutls_anti_replay_t anti_replay,
			      unsigned int window)
{
	anti_replay->window = window;
}

/**
 * gnutls_anti_replay_deinit:
 * @anti_replay: is a #gnutls_anti_replay type
 *
 * This function will deinitialize all resources occupied by the given
 * anti-replay context.
 *
 * Since: 3.6.5
 **/
void
gnutls_anti_replay_deinit(gnutls_anti_replay_t anti_replay)
{
	gnutls_free(anti_replay);
}

/**
 * gnutls_anti_replay_enable:
 * @session: is a #gnutls_session_t type.
 * @anti_replay: is a #gnutls_anti_replay_t type.
 *
 * Request that the server should use anti-replay mechanism.
 *
 * Since: 3.6.5
 **/
void
gnutls_anti_replay_enable(gnutls_session_t session,
			  gnutls_anti_replay_t anti_replay)
{
	if (unlikely(session->security_parameters.entity != GNUTLS_SERVER)) {
		gnutls_assert();
		return;
	}

	session->internals.anti_replay = anti_replay;
}

int
_gnutls_anti_replay_check(gnutls_anti_replay_t anti_replay,
			  uint32_t client_ticket_age,
			  struct timespec *ticket_creation_time,
			  gnutls_datum_t *id)
{
	struct timespec now;
	time_t window;
	uint32_t server_ticket_age, diff;
	gnutls_datum_t key = { NULL, 0 };
	gnutls_datum_t entry = { NULL, 0 };
	unsigned char key_buffer[MAX_HASH_SIZE + 12];
	unsigned char entry_buffer[12]; /* magic + timestamp + expire_time */
	unsigned char *p;
	int ret;

	if (unlikely(id->size > MAX_HASH_SIZE))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	gnutls_gettime(&now);
	server_ticket_age = timespec_sub_ms(&now, ticket_creation_time);

	/* It shouldn't be possible that the server's view of ticket
	 * age is smaller than the client's view.
	 */
	if (unlikely(server_ticket_age < client_ticket_age))
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	/* If ticket is created before recording has started, discard
	 * reject early data.
	 */
	if (_gnutls_timespec_cmp(ticket_creation_time,
				 &anti_replay->start_time) < 0) {
		_gnutls_handshake_log("anti_replay: ticket is created before recording has started\n");
		return gnutls_assert_val(GNUTLS_E_EARLY_DATA_REJECTED);
	}

	/* If certain amount of time (window) has elapsed, rollover
	 * the recording.
	 */
	diff = timespec_sub_ms(&now, &anti_replay->start_time);
	if (diff > anti_replay->window)
		gnutls_gettime(&anti_replay->start_time);

	/* If expected_arrival_time is out of window, reject early
	 * data.
	 */
	if (server_ticket_age - client_ticket_age > anti_replay->window) {
		_gnutls_handshake_log("anti_replay: server ticket age: %u, client ticket age: %u\n",
				      server_ticket_age,
				      client_ticket_age);
		return gnutls_assert_val(GNUTLS_E_EARLY_DATA_REJECTED);
	}

	/* Check if the ClientHello is stored in the database.
	 */
	if (!anti_replay->db_add_func)
		return gnutls_assert_val(GNUTLS_E_EARLY_DATA_REJECTED);

	/* Create a key for database lookup, prefixing window start
	 * time to ID.  Note that this shouldn't clash with session ID
	 * used in TLS 1.2, because such IDs are 32 octets, while here
	 * the key becomes 44+ octets.
	 */
	p = key_buffer;
	_gnutls_write_uint32((uint64_t) anti_replay->start_time.tv_sec >> 32, p);
	p += 4;
	_gnutls_write_uint32(anti_replay->start_time.tv_sec & 0xFFFFFFFF, p);
	p += 4;
	_gnutls_write_uint32(anti_replay->start_time.tv_nsec, p);
	p += 4;
	memcpy(p, id->data, id->size);
	p += id->size;
	key.data = key_buffer;
	key.size = p - key_buffer;

	/* Create an entry to be stored on database if the lookup
	 * failed.  This is formatted so that
	 * gnutls_db_check_entry_expire_time() work.
	 */
	p = entry_buffer;
	_gnutls_write_uint32(PACKED_SESSION_MAGIC, p);
	p += 4;
	_gnutls_write_uint32(now.tv_sec, p);
	p += 4;
	window = anti_replay->window / 1000;
	_gnutls_write_uint32(window, p);
	p += 4;
	entry.data = entry_buffer;
	entry.size = p - entry_buffer;

	ret = anti_replay->db_add_func(anti_replay->db_ptr,
				       (uint64_t)now.tv_sec+(uint64_t)window, &key, &entry);
	if (ret < 0) {
		_gnutls_handshake_log("anti_replay: duplicate ClientHello found\n");
		return gnutls_assert_val(GNUTLS_E_EARLY_DATA_REJECTED);
	}

	return 0;
}

/**
 * gnutls_anti_replay_set_ptr:
 * @anti_replay: is a #gnutls_anti_replay_t type.
 * @ptr: is the pointer
 *
 * Sets the pointer that will be provided to db add function
 * as the first argument.
 **/
void gnutls_anti_replay_set_ptr(gnutls_anti_replay_t anti_replay, void *ptr)
{
	anti_replay->db_ptr = ptr;
}

/**
 * gnutls_anti_replay_set_add_function:
 * @anti_replay: is a #gnutls_anti_replay_t type.
 * @add_func: is the function.
 *
 * Sets the function that will be used to store an entry if it is not
 * already present in the resumed sessions database.  This function returns 0
 * if the entry is successfully stored, and a negative error code
 * otherwise.  In particular, if the entry is found in the database,
 * it returns %GNUTLS_E_DB_ENTRY_EXISTS.
 *
 * The arguments to the @add_func are:
 *  - %ptr: the pointer set with gnutls_anti_replay_set_ptr()
 *  - %exp_time: the expiration time of the entry
 *  - %key: a pointer to the key
 *  - %data: a pointer to data to store
 *
 * The data set by this function can be examined using
 * gnutls_db_check_entry_expire_time() and gnutls_db_check_entry_time().
 *
 * Since: 3.6.5
 **/
void
gnutls_anti_replay_set_add_function(gnutls_anti_replay_t anti_replay,
				    gnutls_db_add_func add_func)
{
	anti_replay->db_add_func = add_func;
}
