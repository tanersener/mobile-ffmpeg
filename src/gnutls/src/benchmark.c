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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "benchmark.h"

#define BSECS 5

volatile int benchmark_must_finish = 0;

#if defined(_WIN32)
#include <windows.h>
static DWORD WINAPI alarm_handler(LPVOID lpParameter)
{
	HANDLE wtimer = *((HANDLE *) lpParameter);
	WaitForSingleObject(wtimer, INFINITE);
	benchmark_must_finish = 1;
	return 0;
}
#else
static void alarm_handler(int signo)
{
	benchmark_must_finish = 1;
}
#endif

static void
value2human(unsigned long bytes, double time, double *data, double *speed,
	    char *metric)
{
	if (bytes > 1000 && bytes < 1000 * 1000) {
		*data = ((double) bytes) / 1000;
		*speed = *data / time;
		strcpy(metric, "KB");
		return;
	} else if (bytes >= 1000 * 1000 && bytes < 1000 * 1000 * 1000) {
		*data = ((double) bytes) / (1000 * 1000);
		*speed = *data / time;
		strcpy(metric, "MB");
		return;
	} else if (bytes >= 1000 * 1000 * 1000) {
		*data = ((double) bytes) / (1000 * 1000 * 1000);
		*speed = *data / time;
		strcpy(metric, "GB");
		return;
	} else {
		*data = (double) bytes;
		*speed = *data / time;
		strcpy(metric, "bytes");
		return;
	}
}

void start_benchmark(struct benchmark_st *st)
{
	memset(st, 0, sizeof(*st));
#ifndef _WIN32
	st->old_handler = signal(SIGALRM, alarm_handler);
#endif
	gettime(&st->start);
	benchmark_must_finish = 0;

#if defined(_WIN32)
	st->wtimer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (st->wtimer == NULL) {
		fprintf(stderr, "error: CreateWaitableTimer %u\n",
			GetLastError());
		exit(1);
	}
	st->wthread =
	    CreateThread(NULL, 0, alarm_handler, &st->wtimer, 0, NULL);
	if (st->wthread == NULL) {
		fprintf(stderr, "error: CreateThread %u\n",
			GetLastError());
		exit(1);
	}
	st->alarm_timeout.QuadPart = (BSECS) * 10000000;
	if (SetWaitableTimer
	    (st->wtimer, &st->alarm_timeout, 0, NULL, NULL, FALSE) == 0) {
		fprintf(stderr, "error: SetWaitableTimer %u\n",
			GetLastError());
		exit(1);
	}
#else
	alarm(BSECS);
#endif

}

/* returns the elapsed time */
double stop_benchmark(struct benchmark_st *st, const char *metric,
		      int quiet)
{
	double secs;
	unsigned long lsecs;
	struct timespec stop;
	double dspeed, ddata;
	char imetric[16];

#if defined(_WIN32)
	if (st->wtimer != NULL)
		CloseHandle(st->wtimer);
	if (st->wthread != NULL)
		CloseHandle(st->wthread);
#else
	signal(SIGALRM, st->old_handler);
#endif

	gettime(&stop);

	lsecs = (stop.tv_sec * 1000 + stop.tv_nsec / (1000 * 1000) -
		 (st->start.tv_sec * 1000 +
		  st->start.tv_nsec / (1000 * 1000)));
	secs = lsecs;
	secs /= 1000;

	if (metric == NULL) {	/* assume bytes/sec */
		value2human(st->size, secs, &ddata, &dspeed, imetric);
		if (quiet == 0)
			printf("  Processed %.2f %s in %.2f secs: ", ddata,
			       imetric, secs);
		printf("%.2f %s/sec\n", dspeed, imetric);
	} else {
		ddata = (double) st->size;
		dspeed = ddata / secs;
		if (quiet == 0)
			printf("  Processed %.2f %s in %.2f secs: ", ddata,
			       metric, secs);
		printf("%.2f %s/sec\n", dspeed, metric);
	}

	return secs;
}
