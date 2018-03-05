/*
 * Copyright (C) 2016 Red Hat, Inc.
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

/* Tests whether we can use gnutls_rnd() under mutliple threads.
 * We do a basic checking of random data match when gnutls_rnd()
 * is called in parallel.
 */

typedef struct thread_data_st {
	unsigned level;
	pthread_t id;
	char buf[32];
} thread_data_st;

static void *start_thread(void *arg)
{
	thread_data_st *data = arg;
	int ret;

	ret = gnutls_rnd(data->level, data->buf, sizeof(data->buf));
	if (ret < 0) {
		fail("gnutls_rnd: wrong size returned (%d)\n", ret);
	}
	if (debug)
		hexprint(data->buf, sizeof(data->buf));

	pthread_exit(0);
}

#define MAX_THREADS 48

static
void do_thread_stuff(unsigned level)
{
	int ret;
	thread_data_st *data;
	unsigned i, j;

	data = calloc(1, sizeof(thread_data_st)*MAX_THREADS);
	if (data == NULL)
		abort();

	for (i=0;i<MAX_THREADS;i++) {
		data[i].level = level;
		ret = pthread_create(&data[i].id, NULL, start_thread, &data[i]);
		if (ret != 0) {
			abort();
		}
	}

	for (i=0;i<MAX_THREADS;i++) {
		pthread_join(data[i].id, NULL);
		for (j=0;j<MAX_THREADS;j++) {
			if (i!=j) {
				if (memcmp(data[i].buf, data[j].buf, sizeof(data[i].buf)) == 0) {
					fail("identical data found in thread %d and %d\n", i,j);
				}
			}
		}
	}
	free(data);

}

void doit(void)
{
	unsigned i;
	signal(SIGPIPE, SIG_IGN);
	gnutls_global_deinit();
	for (i = GNUTLS_RND_NONCE; i <= GNUTLS_RND_KEY; i++) {
		gnutls_global_init();
		do_thread_stuff(i);
		gnutls_global_deinit();
	}
}
#endif				/* _WIN32 */
