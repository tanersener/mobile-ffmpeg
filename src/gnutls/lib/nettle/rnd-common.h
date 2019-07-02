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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_NETTLE_RND_COMMON_H
#define GNUTLS_LIB_NETTLE_RND_COMMON_H

#include "gnutls_int.h"
#ifdef HAVE_GETPID
# include <unistd.h>		/* getpid */
#endif
#ifdef HAVE_GETRUSAGE
# include <sys/resource.h>
#endif

#include <fips.h>

int _rnd_system_entropy_init(void);
int _rnd_system_entropy_check(void);
void _rnd_system_entropy_deinit(void);

typedef int (*get_entropy_func)(void* rnd, size_t size);

extern get_entropy_func _rnd_get_system_entropy;


#endif /* GNUTLS_LIB_NETTLE_RND_COMMON_H */
