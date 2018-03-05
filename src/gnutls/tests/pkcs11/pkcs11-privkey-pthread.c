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
#include <gnutls/abstract.h>
#include <gnutls/crypto.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/wait.h>
# include <pthread.h>
#endif
#include "utils.h"

#if !defined(HAVE___REGISTER_ATFORK)

void doit(void)
{
	exit(77);
}

#else

#ifdef _WIN32
# define P11LIB "libpkcs11mock1.dll"
#else
# include <dlfcn.h>
# define P11LIB "libpkcs11mock1.so"
#endif

/* Tests whether we can use gnutls_privkey_sign() under mutliple threads
 * with the same key when PKCS#11 is in use.
 */

#include "../cert-common.h"

#define PIN "1234"

static const gnutls_datum_t testdata = {(void*)"test test", 9};

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
		unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

typedef struct thread_data_st {
	gnutls_privkey_t pkey;
	pthread_t id;
} thread_data_st;

static void *start_thread(void *arg)
{
	thread_data_st *data = arg;
	int ret;
	gnutls_datum_t sig;

	ret = gnutls_privkey_sign_data(data->pkey, GNUTLS_DIG_SHA256, 0, &testdata, &sig);
	if (ret < 0)
		pthread_exit((void*)-2);

	gnutls_free(sig.data);

	pthread_exit(0);
}

#define MAX_THREADS 48

static
void do_thread_stuff(gnutls_privkey_t pkey)
{
	int ret;
	thread_data_st *data;
	unsigned i;
	void *retval;

	data = calloc(1, sizeof(thread_data_st)*MAX_THREADS);
	if (data == NULL)
		abort();

	for (i=0;i<MAX_THREADS;i++) {
		data[i].pkey = pkey;
		ret = pthread_create(&data[i].id, NULL, start_thread, &data[i]);
		if (ret != 0) {
			fail("Error creating thread\n");
		}
	}

	for (i=0;i<MAX_THREADS;i++) {
		ret = pthread_join(data[i].id, &retval);
		if (ret != 0 || retval != NULL) {
			fail("Error in %d: %p\n", (int)data[i].id, retval);
		}
	}
	free(data);

}

void doit(void)
{
	int ret, status;
	const char *lib;
	gnutls_privkey_t pkey;
	pid_t pid;

	signal(SIGPIPE, SIG_IGN);

	gnutls_pkcs11_set_pin_function(pin_func, NULL);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	lib = getenv("P11MOCKLIB1");
	if (lib == NULL)
		lib = P11LIB;

	ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_add_provider(lib, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(gnutls_privkey_init(&pkey) == 0);

	ret = gnutls_privkey_import_url(pkey, "pkcs11:object=test", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fprintf(stderr, "error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	/* fork to force PKCS#11 reinitialization */
	pid = fork();
	if (pid == -1) {
		exit(1);
	} else if (pid) {
                waitpid(pid, &status, 0);
                check_wait_status(status);
                goto cleanup;
	}

	do_thread_stuff(pkey);

 cleanup:
	gnutls_privkey_deinit(pkey);
}

#endif
