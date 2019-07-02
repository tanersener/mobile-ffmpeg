/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <signal.h>
#include <unistd.h>
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/wait.h>
# include <pthread.h>
#endif
#include "utils.h"

#ifdef _WIN32

void doit(void)
{
	exit(77);
}

#else

/* Tests whether we can use gnutls_fips140_set_mode() and
 * gnutls_fips140_mode_enabled() under mutliple threads.
 */

typedef struct thread_data_st {
	unsigned mode;
	unsigned set_mode;
	int line;
	pthread_t id;
} thread_data_st;

static void *test_set_per_thread(void *arg)
{
	thread_data_st *data = arg;
	unsigned mode;
	int ret;
	unsigned char digest[20];

	mode = gnutls_fips140_mode_enabled();
	if (mode != data->mode)
		fail("%d: gnutls_fips140_mode_enabled: wrong mode returned (%d, exp: %d)\n", data->line, mode, data->mode);

	if (data->set_mode)
		gnutls_fips140_set_mode(data->set_mode, GNUTLS_FIPS140_SET_MODE_THREAD);

	mode = gnutls_fips140_mode_enabled();
	if (mode != data->set_mode) {
		fail("%d: gnutls_fips140_mode_enabled: wrong mode returned after set (%d, exp: %d)\n", data->line, mode, data->set_mode);
	}

	/* reset mode */
	gnutls_fips140_set_mode(data->mode, GNUTLS_FIPS140_SET_MODE_THREAD);
	mode = gnutls_fips140_mode_enabled();
	if (mode != data->mode)
		fail("%d: gnutls_fips140_mode_enabled: wrong mode returned after set (%d, exp: %d)\n", data->line, mode, data->mode);

	ret = gnutls_hmac_fast(GNUTLS_MAC_MD5, "keykeykey", 9, "abcdefgh",
			       8, digest);
	if (mode == GNUTLS_FIPS140_STRICT && ret >= 0) {
		fail("gnutls_hmac_fast(MD5): succeeded in strict mode!\n");
	} else if (mode != GNUTLS_FIPS140_STRICT && ret < 0) {
		fail("gnutls_hmac_fast(MD5): failed in non-strict mode!\n");
	}

	/* put to a random state */
	gnutls_fips140_set_mode(data->set_mode, GNUTLS_FIPS140_SET_MODE_THREAD);

	pthread_exit(0);
}

#define MAX_THREADS 48

void doit(void)
{
	int ret;
	thread_data_st *data;
	unsigned i, j;
	unsigned mode;

	signal(SIGPIPE, SIG_IGN);

	mode = gnutls_fips140_mode_enabled();
	if (mode == 0) {
		success("We are not in FIPS140 mode\n");
		exit(77);
	}

	data = calloc(1, sizeof(thread_data_st)*MAX_THREADS);
	if (data == NULL)
		abort();

	success("starting threads\n");
	/* Test if changes per thread apply, and whether the global
	 * setting will remain the same */
	for (i=0;i<MAX_THREADS;i++) {
		data[i].line = __LINE__;
		data[i].mode = mode;

		j = i%3;
		if (j==0)
			data[i].set_mode = GNUTLS_FIPS140_LAX;
		else if (j==1)
			data[i].set_mode = GNUTLS_FIPS140_LOG;
		else
			data[i].set_mode = GNUTLS_FIPS140_STRICT;

		ret = pthread_create(&data[i].id, NULL, test_set_per_thread, &data[i]);
		if (ret != 0) {
			abort();
		}
	}

	success("waiting for threads to finish\n");
	for (i=0;i<MAX_THREADS;i++) {
		ret = pthread_join(data[i].id, NULL);
		if (ret != 0)
			abort();
	}

	success("checking main process mode\n");

	/* main thread should be in the same state */
	if (mode != gnutls_fips140_mode_enabled())
		fail("gnutls_fips140_mode_enabled: main thread changed mode (%d, exp: %d)\n", gnutls_fips140_mode_enabled(), mode);

	success("checking whether global changes are seen in threads\n");
	/* Test if changes globally are visible in threads */
	mode = GNUTLS_FIPS140_LOG;
	gnutls_fips140_set_mode(mode, 0);

	for (i=0;i<MAX_THREADS;i++) {
		data[i].line = __LINE__;
		data[i].mode = mode;
		data[i].set_mode = GNUTLS_FIPS140_LAX;
		ret = pthread_create(&data[i].id, NULL, test_set_per_thread, &data[i]);
		if (ret != 0)
			abort();
	}

	success("waiting for threads to finish\n");
	for (i=0;i<MAX_THREADS;i++) {
		ret = pthread_join(data[i].id, NULL);
		if (ret != 0)
			abort();
	}

	if (mode != gnutls_fips140_mode_enabled())
		fail("gnutls_fips140_mode_enabled: main thread changed mode (%d, exp: %d)\n", gnutls_fips140_mode_enabled(), mode);

	gnutls_fips140_set_mode(GNUTLS_FIPS140_SELFTESTS, 0);
	if (GNUTLS_FIPS140_SELFTESTS == gnutls_fips140_mode_enabled())
		fail("gnutls_fips140_mode_enabled: setting to GNUTLS_FIPS140_SELFTESTS succeeded!\n");

	free(data);

}

#endif				/* _WIN32 */
