#ifndef THREADS_H_
#define THREADS_H_
/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

/**
 * \ingroup Threading
 * \file
 * Abstractions for operating system specific stuff.
 */

#include "global.h" // IWYU pragma: keep

#include <pthread.h>

#if defined(__GNUC__) && !defined(__MINGW32__) 
#include <unistd.h> // IWYU pragma: export
#include <time.h> // IWYU pragma: export

#define KVZ_CLOCK_T struct timespec

#ifdef __MACH__
// Workaround Mac OS not having clock_gettime.
// This needs to work with pthread_cond_timedwait.
#  include <sys/time.h>
#  define KVZ_GET_TIME(clock_t) { \
     struct timeval tv; \
     gettimeofday(&tv, NULL); \
     (clock_t)->tv_sec = tv.tv_sec; \
     (clock_t)->tv_nsec = tv.tv_usec * 1000; \
   }
#else
#  define KVZ_GET_TIME(clock_t) { clock_gettime(CLOCK_MONOTONIC, (clock_t)); }
#endif

#define KVZ_CLOCK_T_AS_DOUBLE(ts) ((double)((ts).tv_sec) + (double)((ts).tv_nsec) / 1e9)
#define KVZ_CLOCK_T_DIFF(start, stop) ((double)((stop).tv_sec - (start).tv_sec) + (double)((stop).tv_nsec - (start).tv_nsec) / 1e9)

#define KVZ_ATOMIC_INC(ptr)                     __sync_add_and_fetch((volatile int32_t*)ptr, 1)
#define KVZ_ATOMIC_DEC(ptr)                     __sync_add_and_fetch((volatile int32_t*)ptr, -1)

#else //__GNUC__
//TODO: we assume !GCC => Windows... this may be bad
#include <windows.h> // IWYU pragma: export

#define KVZ_CLOCK_T struct _FILETIME
#define KVZ_GET_TIME(clock_t) { GetSystemTimeAsFileTime(clock_t); }
// _FILETIME has 32bit low and high part of 64bit 100ns resolution timestamp (since 12:00 AM January 1, 1601)
#define KVZ_CLOCK_T_AS_DOUBLE(ts) ((double)(((uint64_t)(ts).dwHighDateTime)<<32 | (uint64_t)(ts).dwLowDateTime) / 1e7)
#define KVZ_CLOCK_T_DIFF(start, stop) ((double)((((uint64_t)(stop).dwHighDateTime)<<32 | (uint64_t)(stop).dwLowDateTime) - \
                                  (((uint64_t)(start).dwHighDateTime)<<32 | (uint64_t)(start).dwLowDateTime)) / 1e7)

#define KVZ_ATOMIC_INC(ptr)                     InterlockedIncrement((volatile LONG*)ptr)
#define KVZ_ATOMIC_DEC(ptr)                     InterlockedDecrement((volatile LONG*)ptr)

#endif //__GNUC__

#ifdef __APPLE__
// POSIX semaphores are deprecated on Mac so we use Grand Central Dispatch
// semaphores instead.
#include <dispatch/dispatch.h>
typedef dispatch_semaphore_t kvz_sem_t;

static INLINE void kvz_sem_init(kvz_sem_t *sem, int value)
{
    assert(value >= 0);
    *sem = dispatch_semaphore_create(value);
}

static INLINE void kvz_sem_wait(kvz_sem_t *sem)
{
    dispatch_semaphore_wait(*sem, DISPATCH_TIME_FOREVER);
}

static INLINE void kvz_sem_post(kvz_sem_t *sem)
{
    dispatch_semaphore_signal(*sem);
}


static INLINE void kvz_sem_destroy(kvz_sem_t *sem)
{
    // Do nothing for GCD semaphores.
}

#else
// Use POSIX semaphores.
#include <semaphore.h>

typedef sem_t kvz_sem_t;

static INLINE void kvz_sem_init(kvz_sem_t *sem, int value)
{
    assert(value >= 0);
    // Pthreads-w32 does not support process-shared semaphores, so pshared
    // must always be zero.
    int pshared = 0;
    sem_init(sem, pshared, value);
}

static INLINE void kvz_sem_wait(kvz_sem_t *sem)
{
    sem_wait(sem);
}

static INLINE void kvz_sem_post(kvz_sem_t *sem)
{
    sem_post(sem);
}

static INLINE void kvz_sem_destroy(kvz_sem_t *sem)
{
    sem_destroy(sem);
}

#endif

#endif //THREADS_H_
