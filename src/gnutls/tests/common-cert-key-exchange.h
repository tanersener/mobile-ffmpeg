/*
 * Copyright (C) 2016 Nikos Mavrogiannopoulos
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnutls/gnutls.h>

#define USE_CERT 1
#define ASK_CERT 2

void try(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
		gnutls_sign_algorithm_t server_sign_algo,
		gnutls_sign_algorithm_t client_sign_algo,
		unsigned client_cert);

void dtls_try(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
		gnutls_sign_algorithm_t server_sign_algo,
		gnutls_sign_algorithm_t client_sign_algo,
		unsigned client_cert);
