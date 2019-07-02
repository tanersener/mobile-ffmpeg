/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include <config.h>
#include <system.h>
#include "gnutls_int.h"
#include "errors.h"

#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
# include <windows.h>
# include <wincrypt.h>

#else /* !_WIN32 */

# ifdef HAVE_PTHREAD_LOCKS
#  include <pthread.h>
# endif

#endif

/* System specific lock function wrappers.
 */

/* Thread stuff */

#ifdef HAVE_WIN32_LOCKS
static int gnutls_system_mutex_init(void **priv)
{
	CRITICAL_SECTION *lock = malloc(sizeof(CRITICAL_SECTION));

	if (lock == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	InitializeCriticalSection(lock);

	*priv = lock;

	return 0;
}

static int gnutls_system_mutex_deinit(void **priv)
{
	DeleteCriticalSection((CRITICAL_SECTION *) * priv);
	free(*priv);

	return 0;
}

static int gnutls_system_mutex_lock(void **priv)
{
	EnterCriticalSection((CRITICAL_SECTION *) * priv);
	return 0;
}

static int gnutls_system_mutex_unlock(void **priv)
{
	LeaveCriticalSection((CRITICAL_SECTION *) * priv);
	return 0;
}

#endif				/* WIN32_LOCKS */

#ifdef HAVE_PTHREAD_LOCKS

static int gnutls_system_mutex_init(void **priv)
{
	pthread_mutex_t *lock = malloc(sizeof(pthread_mutex_t));
	int ret;

	if (lock == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	ret = pthread_mutex_init(lock, NULL);
	if (ret) {
		free(lock);
		gnutls_assert();
		return GNUTLS_E_LOCKING_ERROR;
	}

	*priv = lock;

	return 0;
}

static int gnutls_system_mutex_deinit(void **priv)
{
	pthread_mutex_destroy((pthread_mutex_t *) * priv);
	free(*priv);
	return 0;
}

static int gnutls_system_mutex_lock(void **priv)
{
	if (pthread_mutex_lock((pthread_mutex_t *) * priv)) {
		gnutls_assert();
		return GNUTLS_E_LOCKING_ERROR;
	}

	return 0;
}

static int gnutls_system_mutex_unlock(void **priv)
{
	if (pthread_mutex_unlock((pthread_mutex_t *) * priv)) {
		gnutls_assert();
		return GNUTLS_E_LOCKING_ERROR;
	}

	return 0;
}

#endif				/* PTHREAD_LOCKS */

#ifdef HAVE_NO_LOCKS

static int gnutls_system_mutex_init(void **priv)
{
	return 0;
}

static int gnutls_system_mutex_deinit(void **priv)
{
	return 0;
}

static int gnutls_system_mutex_lock(void **priv)
{
	return 0;
}

static int gnutls_system_mutex_unlock(void **priv)
{
	return 0;
}

#endif				/* NO_LOCKS */

mutex_init_func gnutls_mutex_init = gnutls_system_mutex_init;
mutex_deinit_func gnutls_mutex_deinit = gnutls_system_mutex_deinit;
mutex_lock_func gnutls_mutex_lock = gnutls_system_mutex_lock;
mutex_unlock_func gnutls_mutex_unlock = gnutls_system_mutex_unlock;

