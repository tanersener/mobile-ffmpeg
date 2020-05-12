/*
 * Copyright (C) 2018 Red Hat, Inc.
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

#ifndef GNUTLS_LIB_GTHREADS_H
#define GNUTLS_LIB_GTHREADS_H

#include <config.h>

/* Using a C99-only compiler installed in parallel with modern C11 environment
 * will see HAVE_THREADS_H, but won't be able to use _Thread_local. */
#if __STDC_VERSION__ >= 201112 && !defined(__STDC_NO_THREADS__) && defined(HAVE_THREADS_H)
# include <threads.h>
#elif defined(__GNUC__) || defined(__SUNPRO_C) || defined(__xlC__) /* clang is covered by __GNUC__ */
# define _Thread_local __thread
#elif defined(_MSC_VER)
# define _Thread_local __declspec(thread)
#else
# error Unsupported platform
#endif

#endif /* GNUTLS_LIB_GTHREADS_H */
