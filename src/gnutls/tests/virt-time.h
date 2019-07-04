/*
 * Copyright (C) 2018 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifndef GNUTLS_TESTS_VIRT_TIME_H
#define GNUTLS_TESTS_VIRT_TIME_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <gnutls/gnutls.h>

/* copied from ../lib/system.h so not to include that header from
 * every test program */
typedef void (*gnutls_gettime_func) (struct timespec *);
extern void _gnutls_global_set_gettime_function(gnutls_gettime_func gettime_func);

/* virtualize time in a test. This freezes the time in the test, except for
 * the advances due to calls to virt_sleep_sec(). This makes the test
 * independent of the test system load, and avoids any long delays. */
static time_t _now;
static struct timespec _now_ts;

#define virt_sec_sleep(s) { \
		_now += s; \
		_now_ts.tv_sec += s; \
	}

#define virt_time_init() { \
		_now = time(0); \
		gnutls_global_set_time_function(mytime); \
		_now_ts.tv_sec = _now; \
		_now_ts.tv_nsec = 0; \
		_gnutls_global_set_gettime_function(mygettime); \
	}


static time_t mytime(time_t * t)
{
	if (t)
		*t = _now;

	return _now;
}

static void mygettime(struct timespec * t)
{
	if (t)
		*t = _now_ts;
}

#endif /* GNUTLS_TESTS_VIRT_TIME_H */
