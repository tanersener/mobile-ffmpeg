/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GNUTLS_SRC_BENCHMARK_H
#define GNUTLS_SRC_BENCHMARK_H

#include <sys/time.h>
#include <time.h>
#include <signal.h>
#if defined(_WIN32)
#include <windows.h>
#endif

/* for uint64_t */
# include <stdint.h>

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_PROCESS_CPUTIME_ID)
#undef gettime
#define gettime(x) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, x)
#else
inline static void gettime(struct timespec *ts)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000;
}
#endif

typedef void (*sighandler_t) (int);

void benchmark_cipher(int debug_level);
void benchmark_tls(int debug_level, int ciphers);

struct benchmark_st {
	struct timespec start;
	uint64_t size;
	sighandler_t old_handler;
#if defined(_WIN32)
	HANDLE wtimer;
	HANDLE wthread;
	LARGE_INTEGER alarm_timeout;
#endif
};

extern volatile int benchmark_must_finish;

void start_benchmark(struct benchmark_st *st);
double stop_benchmark(struct benchmark_st *st, const char *metric,
		      int quiet);

inline static unsigned int
timespec_sub_ms(struct timespec *a, struct timespec *b)
{
	return (a->tv_sec - b->tv_sec) * 1000 + (a->tv_nsec - b->tv_nsec) / (1000 * 1000);
}

#endif /* GNUTLS_SRC_BENCHMARK_H */
