/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2010
 * Free Software Foundation, Inc.
 *
 * Copyright (C) 2011
 * Bardenheuer GmbH, Munich and Bundesdruckerei GmbH, Berlin
 *
 * Copyright (C) 2013
 * Frank Morgner <morgner@informatik.hu-berlin.de>
 *
 * Author: Frank Morgner
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

#ifndef GNUTLS_LIB_AUTH_RSA_COMMON_H
#define GNUTLS_LIB_AUTH_RSA_COMMON_H

#include <abstract_int.h>

int
_gnutls_get_public_rsa_params(gnutls_session_t session,
			      gnutls_pk_params_st * params);

#endif /* GNUTLS_LIB_AUTH_RSA_COMMON_H */
