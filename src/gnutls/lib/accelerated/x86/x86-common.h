/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#if defined(ASM_X86)

void gnutls_cpuid(unsigned int func, unsigned int *ax, unsigned int *bx,
		  unsigned int *cx, unsigned int *dx);

# ifdef ASM_X86_32
unsigned int gnutls_have_cpuid(void);
# else
#  define gnutls_have_cpuid() 1
# endif				/* ASM_X86_32 */

#endif

#define CHECK_AES_KEYSIZE(s) \
	if (s != 16 && s != 24 && s != 32) \
		return GNUTLS_E_INVALID_REQUEST

#define NN_HASH(name, update_func, digest_func, NAME) {	\
 #name,						\
 sizeof(struct name##_ctx),			\
 NAME##_DIGEST_SIZE,				\
 NAME##_DATA_SIZE,				\
 (nettle_hash_init_func *) name##_init,		\
 (nettle_hash_update_func *) update_func,	\
 (nettle_hash_digest_func *) digest_func	\
} 
