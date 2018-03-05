/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
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

#include "gnutls_int.h"
#ifdef HAVE_GETPID
# include <unistd.h>		/* getpid */
#endif
#ifdef HAVE_GETRUSAGE
# include <sys/resource.h>
#endif

#include <fips.h>

struct event_st {
		struct timespec now; /* current time */
#ifdef HAVE_GETRUSAGE
		struct rusage rusage;
#endif
#ifdef HAVE_GETPID
		pid_t pid;	/* the process PID */
#endif
		unsigned count; /* a running counter */
		unsigned err; /* the last errno */
};

void _rnd_get_event(struct event_st *e);

int _rnd_system_entropy_init(void);
int _rnd_system_entropy_check(void);
void _rnd_system_entropy_deinit(void);

typedef int (*get_entropy_func)(void* rnd, size_t size);

extern get_entropy_func _rnd_get_system_entropy;

