/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
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

#ifndef GNUTLS_LOCKS_H
#define GNUTLS_LOCKS_H

#include <gnutls/gnutls.h>
#include "gnutls_int.h"
#include <system.h>

#ifdef HAVE_STDATOMIC_H
# include <stdatomic.h>
#endif

extern mutex_init_func gnutls_mutex_init;
extern mutex_deinit_func gnutls_mutex_deinit;
extern mutex_lock_func gnutls_mutex_lock;
extern mutex_unlock_func gnutls_mutex_unlock;

#if defined(HAVE_WIN32_LOCKS)
# include <windows.h>

/* Idea based based on comment 2 at:
 * http://stackoverflow.com/questions/3555859/is-it-possible-to-do-static-initialization-of-mutexes-in-windows
 */
# define GNUTLS_STATIC_MUTEX(mutex) \
	static CRITICAL_SECTION *mutex = NULL

# define GNUTLS_STATIC_MUTEX_LOCK(mutex) \
	if (mutex == NULL) { \
		CRITICAL_SECTION *mutex##tmp = malloc(sizeof(CRITICAL_SECTION)); \
		InitializeCriticalSection(mutex##tmp); \
		if (InterlockedCompareExchangePointer((PVOID*)&mutex, (PVOID)mutex##tmp, NULL) != NULL) { \
			DeleteCriticalSection(mutex##tmp); \
			free(mutex##tmp); \
		} \
	} \
	EnterCriticalSection(mutex)

# define GNUTLS_STATIC_MUTEX_UNLOCK(mutex) \
	LeaveCriticalSection(mutex)

#elif defined(HAVE_PTHREAD_LOCKS)
# include <pthread.h>
# define GNUTLS_STATIC_MUTEX(mutex) \
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER

# define GNUTLS_STATIC_MUTEX_LOCK(mutex) \
	pthread_mutex_lock(&mutex)

# define GNUTLS_STATIC_MUTEX_UNLOCK(mutex) \
	pthread_mutex_unlock(&mutex)

#else
# define GNUTLS_STATIC_MUTEX(mutex)
# define GNUTLS_STATIC_MUTEX_LOCK(mutex)
# define GNUTLS_STATIC_MUTEX_UNLOCK(mutex)
#endif

#endif
