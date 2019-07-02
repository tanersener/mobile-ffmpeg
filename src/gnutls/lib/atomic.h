/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifndef GNUTLS_LIB_ATOMIC_H
#define GNUTLS_LIB_ATOMIC_H

#ifdef HAVE_STDATOMIC_H
# include <stdatomic.h>

# define gnutls_atomic_uint_t atomic_uint
# define DEF_ATOMIC_INT(x) atomic_uint x
# define gnutls_atomic_increment(x) (*x)++
# define gnutls_atomic_decrement(x) (*x)--
# define gnutls_atomic_init(x) (*x)=(0)
# define gnutls_atomic_deinit(x)
# define gnutls_atomic_val(x) (*x)
#else
# include "locks.h"

struct gnutls_atomic_uint_st {
	void *lock;
	unsigned int value;
};
typedef struct gnutls_atomic_uint_st *gnutls_atomic_uint_t;

# define DEF_ATOMIC_INT(x) struct gnutls_atomic_uint_st x

inline static unsigned gnutls_atomic_val(gnutls_atomic_uint_t x)
{
	unsigned int t;
	gnutls_mutex_lock(&x->lock);
	t = x->value;
	gnutls_mutex_unlock(&x->lock);
	return t;
}

inline static void gnutls_atomic_increment(gnutls_atomic_uint_t x)
{
	gnutls_mutex_lock(&x->lock);
	x->value++;
	gnutls_mutex_unlock(&x->lock);
}

inline static void gnutls_atomic_decrement(gnutls_atomic_uint_t x)
{
	gnutls_mutex_lock(&x->lock);
	x->value--;
	gnutls_mutex_unlock(&x->lock);
}

inline static void gnutls_atomic_init(gnutls_atomic_uint_t x)
{
	gnutls_mutex_init(&x->lock);
	x->value = 0;
}
inline static void gnutls_atomic_deinit(gnutls_atomic_uint_t x)
{
	gnutls_mutex_deinit(&x->lock);
}
#endif

#endif /* GNUTLS_LIB_ATOMIC_H */
