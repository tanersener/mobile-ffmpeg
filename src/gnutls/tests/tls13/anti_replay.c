/*
 * Copyright (C) 2015-2018 Red Hat, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdint.h>

#include "utils.h"
#include "virt-time.h"
#include "../../lib/tls13/anti_replay.h"
#include "../../lib/system.h"

#define MAX_CLIENT_HELLO_RECORDED 10

struct storage_st {
	gnutls_datum_t entries[MAX_CLIENT_HELLO_RECORDED];
	size_t num_entries;
};

static int
storage_add(void *ptr, time_t expires, const gnutls_datum_t *key, const gnutls_datum_t *value)
{
	struct storage_st *storage = ptr;
	gnutls_datum_t *datum;
	size_t i;

	for (i = 0; i < storage->num_entries; i++) {
		if (key->size == storage->entries[i].size &&
		    memcmp(storage->entries[i].data, key->data, key->size) == 0) {
			return GNUTLS_E_DB_ENTRY_EXISTS;
		}
	}

	/* If the maximum number of ClientHello exceeded, reject early
	 * data until next time.
	 */
	if (storage->num_entries == MAX_CLIENT_HELLO_RECORDED)
		return GNUTLS_E_DB_ERROR;

	datum = &storage->entries[storage->num_entries];
	datum->data = gnutls_malloc(key->size);
	if (!datum->data)
		return GNUTLS_E_MEMORY_ERROR;
	memcpy(datum->data, key->data, key->size);
	datum->size = key->size;

	storage->num_entries++;

	return 0;
}

static void
storage_clear(struct storage_st *storage)
{
	size_t i;

	for (i = 0; i < storage->num_entries; i++)
		gnutls_free(storage->entries[i].data);
	storage->num_entries = 0;
}

void doit(void)
{
	gnutls_anti_replay_t anti_replay;
	gnutls_datum_t key = { (unsigned char *) "\xFF\xFF\xFF\xFF", 4 };
	struct timespec creation_time;
	struct storage_st storage;
	int ret;

	virt_time_init();
	memset(&storage, 0, sizeof(storage));

	/* server_ticket_age < client_ticket_age */
	ret = gnutls_anti_replay_init(&anti_replay);
	assert(ret == 0);
	gnutls_anti_replay_set_window(anti_replay, 10000);
	gnutls_anti_replay_set_add_function(anti_replay, storage_add);
	gnutls_anti_replay_set_ptr(anti_replay, &storage);
	mygettime(&creation_time);
	ret = _gnutls_anti_replay_check(anti_replay, 10000, &creation_time, &key);
	if (ret != GNUTLS_E_ILLEGAL_PARAMETER)
		fail("error is not returned, while server_ticket_age < client_ticket_age\n");
	gnutls_anti_replay_deinit(anti_replay);
	storage_clear(&storage);

	/* server_ticket_age - client_ticket_age > window */
	ret = gnutls_anti_replay_init(&anti_replay);
	assert(ret == 0);
	gnutls_anti_replay_set_add_function(anti_replay, storage_add);
	gnutls_anti_replay_set_ptr(anti_replay, &storage);
	gnutls_anti_replay_set_window(anti_replay, 10000);
	mygettime(&creation_time);
	virt_sec_sleep(30);
	ret = _gnutls_anti_replay_check(anti_replay, 10000, &creation_time, &key);
	if (ret != GNUTLS_E_EARLY_DATA_REJECTED)
		fail("early data is NOT rejected, while freshness check fails\n");
	gnutls_anti_replay_deinit(anti_replay);
	storage_clear(&storage);

	/* server_ticket_age - client_ticket_age < window */
	ret = gnutls_anti_replay_init(&anti_replay);
	assert(ret == 0);
	gnutls_anti_replay_set_add_function(anti_replay, storage_add);
	gnutls_anti_replay_set_ptr(anti_replay, &storage);
	gnutls_anti_replay_set_window(anti_replay, 10000);
	mygettime(&creation_time);
	virt_sec_sleep(15);
	ret = _gnutls_anti_replay_check(anti_replay, 10000, &creation_time, &key);
	if (ret != 0)
		fail("early data is rejected, while freshness check succeeds\n");
	ret = _gnutls_anti_replay_check(anti_replay, 10000, &creation_time, &key);
	if (ret != GNUTLS_E_EARLY_DATA_REJECTED)
		fail("early data is NOT rejected for a duplicate key\n");
	gnutls_anti_replay_deinit(anti_replay);
	storage_clear(&storage);
}
