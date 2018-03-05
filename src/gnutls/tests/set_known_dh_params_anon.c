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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"
#include "cert-common.h"

/* Test for gnutls_certificate_set_known_dh_params()
 *
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

void doit(void)
{
	gnutls_anon_client_credentials_t clicred;
	gnutls_anon_server_credentials_t servcred;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_anon_allocate_client_credentials(&clicred) >= 0);
	assert(gnutls_anon_allocate_server_credentials(&servcred) >= 0);

	assert(gnutls_anon_set_server_known_dh_params(servcred, GNUTLS_SEC_PARAM_LEGACY) >= 0);
	assert(test_cli_serv_anon(servcred, clicred, "NORMAL:-KX-ALL:+ANON-DH") >= 0);

	assert(gnutls_anon_set_server_known_dh_params(servcred, GNUTLS_SEC_PARAM_NORMAL) >= 0);
	assert(test_cli_serv_anon(servcred, clicred, "NORMAL:-KX-ALL:+ANON-DH") >= 0);

	assert(gnutls_anon_set_server_known_dh_params(servcred, GNUTLS_SEC_PARAM_HIGH) >= 0);
	assert(test_cli_serv_anon(servcred, clicred, "NORMAL:-KX-ALL:+ANON-DH") >= 0);

	assert(gnutls_anon_set_server_known_dh_params(servcred, GNUTLS_SEC_PARAM_ULTRA) >= 0);
	assert(test_cli_serv_anon(servcred, clicred, "NORMAL:-KX-ALL:+ANON-DH") >= 0);

	gnutls_anon_free_server_credentials(servcred);
	gnutls_anon_free_client_credentials(clicred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
