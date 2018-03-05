/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 * Copyright (C) 2000, 2001, 2008 Niels Möller
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

/* Here are the common parts of the random generator layer. 
 * Some of this code was based on the LSH 
 * random generator (the trivia and device source functions for POSIX)
 * and modified to fit gnutls' needs. Relicenced with permission. 
 * Original author Niels Möller.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <locks.h>
#include <num.h>
#include <nettle/yarrow.h>
#include <errno.h>
#include <rnd-common.h>
#include <hash-pjw-bare.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* gnulib wants to claim strerror even if it cannot provide it. WTF */
#undef strerror

#ifdef HAVE_GETRUSAGE
# ifdef RUSAGE_THREAD
#  define ARG_RUSAGE RUSAGE_THREAD
# else
#  define ARG_RUSAGE RUSAGE_SELF
# endif
#endif

get_entropy_func _rnd_get_system_entropy = NULL;

void _rnd_get_event(struct event_st *e)
{
	static unsigned count = 0;

	memset(e, 0, sizeof(*e));
	gettime(&e->now);

#ifdef HAVE_GETRUSAGE
	if (getrusage(ARG_RUSAGE, &e->rusage) < 0) {
		_gnutls_debug_log("getrusage failed: %s\n",
			  strerror(errno));
	}
#endif

#ifdef HAVE_GETPID
	e->pid = getpid();
#endif
	e->count = count++;
	e->err = errno;

	return;
}
