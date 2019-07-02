/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
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
 * along with GnuTLS.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

#include "utils.h"
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

/* This tests the operation of the provided random generator
 * to try() function. It will check whether it can perform more than
 * 16k iterations, and provide a substantial amount of data.
 */

static void try(int rnd)
{
	unsigned char buf1[64];
	unsigned char *tmp;
	int ret;
	unsigned i;

	global_init();

	for (i = 0; i <= 65539; i++) {
		ret = gnutls_rnd(rnd, buf1, sizeof(buf1));
		if (ret < 0) {
			fail("Error iterating RNG-%d more than %u times\n", rnd, i);
			exit(1);
		}
	}

#define TMP_SIZE (65*1024)
	tmp = malloc(TMP_SIZE);
	if (tmp == NULL) {
		fail("memory error\n");
		exit(1);
	}

	for (i = 0; i <= 65539; i++) {
		ret = gnutls_rnd(rnd, tmp, TMP_SIZE);
		if (ret < 0) {
			fail("Error iterating RNG-%d more than %u times for %d data\n", rnd, i, TMP_SIZE);
			exit(1);
		}
	}
	free(tmp);

	gnutls_global_deinit();
}
