/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_EXT_SRP_H
#define GNUTLS_LIB_EXT_SRP_H

#include <hello_ext.h>

#define IS_SRP_KX(kx) ((kx == GNUTLS_KX_SRP || (kx == GNUTLS_KX_SRP_RSA) || \
	  kx == GNUTLS_KX_SRP_DSS)?1:0)

#ifdef ENABLE_SRP

extern const hello_ext_entry_st ext_mod_srp;

typedef struct {
	char *username;
	char *password;
} srp_ext_st;

#endif

#endif /* GNUTLS_LIB_EXT_SRP_H */
