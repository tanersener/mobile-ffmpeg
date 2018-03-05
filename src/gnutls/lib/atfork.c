/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2014 Red Hat
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

#include <config.h>
#include "gnutls_int.h"
#include "errors.h"

#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <atfork.h>

unsigned int _gnutls_forkid = 0;

#ifndef _WIN32

# ifdef HAVE_ATFORK
static void fork_handler(void)
{
	_gnutls_forkid++;
}
# endif

# if defined(HAVE___REGISTER_ATFORK)
extern int __register_atfork(void (*)(void), void(*)(void), void (*)(void), void *);
extern void *__dso_handle;

int _gnutls_register_fork_handler(void)
{
	if (__register_atfork(NULL, NULL, fork_handler, __dso_handle) != 0)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	return 0;
}

# else

unsigned int _gnutls_get_forkid(void)
{
	return getpid();
}

int _gnutls_detect_fork(unsigned int forkid)
{
	if ((unsigned int)getpid() == forkid)
		return 0;
	return 1;
}

/* we have to detect fork manually */
int _gnutls_register_fork_handler(void)
{
	_gnutls_forkid = getpid();
	return 0;
}

# endif

#endif /* !_WIN32 */
