/*
 * Copyright (C) 2014 Nikos Mavrogiannopoulos
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
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#include "utils.h"

/* See <https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=760476>. */

void doit(void)
{
#ifndef _WIN32
	int res;
	unsigned i;
	int serial = 0;
	char buf[128];

	res = read(3, buf, 16);
	if (res == 16)
		serial = 1;

	/* close all descriptors */
	for (i=3;i<1024;i++)
		close(i);

	res = gnutls_global_init();
	if (res != 0)
		fail("global_init\n");

	if (serial != 0) {
		res = read(3, buf, 16);
		if (res != 16) {
			fail("could not open fd, or OS doesn't assign fds in a serial way (%d)\n", res);
		}
	}

	res = gnutls_global_init();
	if (res != 0)
		fail("global_init2\n");

	gnutls_rnd_refresh();

	res = gnutls_rnd(GNUTLS_RND_RANDOM, buf, sizeof(buf));
	if (res != 0)
		fail("gnutls_rnd\n");

	gnutls_global_deinit();

	if (debug)
		success("init-close success\n");
#else
	return;
#endif
}
