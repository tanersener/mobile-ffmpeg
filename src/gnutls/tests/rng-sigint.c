/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* This program verifies whether the low-level random functions can operate
 * properly, even if interrupted by signals */

#if defined(HAVE_SETITIMER) && (defined(HAVE_LINUX_GETRANDOM) || defined(__linux__))

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <stdint.h>
#include "utils.h"

#define _gnutls_debug_log printf
#define gnutls_assert()
#define gnutls_assert_val(val) val

int _rnd_system_entropy_init(void);
int _rnd_system_entropy_check(void);
void _rnd_system_entropy_deinit(void);

typedef int (*get_entropy_func)(void* rnd, size_t size);
get_entropy_func _rnd_get_system_entropy;

#define RND_NO_INCLUDES
#include "../lib/nettle/sysrng-linux.c"

static volatile int stop_loop = 0;

static void sig_handler(int signo)
{
	stop_loop++;
}

void doit(void)
{
	char buf[512];
	char empty[32];
	int ret;
	struct itimerval ival;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handler;
	sigemptyset (&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	memset(&ival, 0, sizeof(ival));
	ival.it_interval.tv_usec = 5000;
	ival.it_value.tv_usec = 5000;

	_rnd_system_entropy_init();

	ret = setitimer(ITIMER_REAL, &ival, NULL);
	if (ret < 0) {
		fail("error in setitimer: %s\n", strerror(errno));
	}

	memset(empty, 0, sizeof(empty));
	for (;stop_loop<1024;) {
		memset(buf, 0, sizeof(buf));
		ret = _rnd_get_system_entropy(buf, sizeof(buf));
		if (ret < 0) {
			fail("error obtaining entropy: %s\n", gnutls_strerror(ret));
		}

		if (memcmp(empty, buf+sizeof(buf)-sizeof(empty)-1, sizeof(empty)) == 0) {
			fail("_rnd_get_system_entropy: did not fill buffer\n");
		}
	}

	_rnd_system_entropy_deinit();
}
#else
void doit(void)
{
	exit(77);
}

#endif
