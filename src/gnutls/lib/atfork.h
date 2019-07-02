/*
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_ATFORK_H
#define GNUTLS_LIB_ATFORK_H

#include <config.h>
#include "gnutls_int.h"

extern unsigned int _gnutls_forkid;

#if defined(HAVE___REGISTER_ATFORK)
# define HAVE_ATFORK
#endif

#ifndef _WIN32

/* API */
int _gnutls_register_fork_handler(void); /* global init */

# if defined(HAVE_ATFORK)
inline static int _gnutls_detect_fork(unsigned int forkid)
{
	if (forkid == _gnutls_forkid)
		return 0;
	return 1;
}

inline static unsigned int _gnutls_get_forkid(void)
{
	return _gnutls_forkid;
}
# else
int _gnutls_detect_fork(unsigned int forkid);
unsigned int _gnutls_get_forkid(void);
# endif

#else

# define _gnutls_detect_fork(x) 0
# define _gnutls_get_forkid() 0

#endif

#endif /* GNUTLS_LIB_ATFORK_H */
