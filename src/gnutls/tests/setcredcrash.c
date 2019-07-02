/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include <gnutls/gnutls.h>

int main(int argc, char *argv[])
{
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_session_t client;

	global_init();

	gnutls_anon_allocate_client_credentials(&c_anoncred);
	gnutls_init(&client, GNUTLS_CLIENT);
	gnutls_priority_set_direct(client, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL);

	/* Test setting the same credential type twice.  Earlier GnuTLS had
	   a bug that crashed when this happened. */
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred);
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred);

	gnutls_deinit(client);
	gnutls_anon_free_client_credentials(c_anoncred);

	gnutls_global_deinit();

	return 0;
}
