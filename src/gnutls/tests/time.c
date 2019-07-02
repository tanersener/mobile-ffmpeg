/*
 * Copyright (C) 2019 Red Hat, Inc.
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
 *
 */

/* That's a unit test of _gnutls_utcTime2gtime() and _gnutls_x509_generalTime2gtime()
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <gnutls/gnutls.h>

#include "utils.h"

time_t _gnutls_utcTime2gtime(const char *ttime);
time_t _gnutls_x509_generalTime2gtime(const char *ttime);

struct time_tests_st {
	const char *time_str;
	time_t utime;
};

struct time_tests_st general_time_tests[] = {
	{
		.time_str = "20190520133237Z",
		.utime = 1558359157
	},
	{
		.time_str = "20170101000000Z",
		.utime = 1483228800
	},
	{
		.time_str = "19700101000000Z",
		.utime = 0
	},
};

struct time_tests_st utc_time_tests[] = {
	{
		.time_str = "190520133237",
		.utime = 1558359157
	},
	{
		.time_str = "170101000000Z",
		.utime = 1483228800
	},
};


void doit(void)
{
	time_t t;
	unsigned i;

	for (i=0;i<sizeof(general_time_tests)/sizeof(general_time_tests[0]);i++) {
		t = _gnutls_x509_generalTime2gtime(general_time_tests[i].time_str);
		if (t != general_time_tests[i].utime) {
			fprintf(stderr, "%s: Error in GeneralTime conversion\n", general_time_tests[i].time_str);
			fprintf(stderr, "got: %lu, expected: %lu\n", (unsigned long)t, general_time_tests[i].utime);
		}
	}

	for (i=0;i<sizeof(utc_time_tests)/sizeof(utc_time_tests[0]);i++) {
		t = _gnutls_utcTime2gtime(utc_time_tests[i].time_str);
		if (t != utc_time_tests[i].utime) {
			fprintf(stderr, "%s: Error in utcTime conversion\n", utc_time_tests[i].time_str);
			fprintf(stderr, "got: %lu, expected: %lu\n", (unsigned long)t, utc_time_tests[i].utime);
		}
	}
}

