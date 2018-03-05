/*
 * Copyright (C) 2016 Nikos Mavrogiannopoulos
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
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"
#include "cert-common.h"

/* Test for GNUTLS_KEYLOGFILE being functional.
 *
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#define LSTR "CLIENT_RANDOM "
static void search_for_str(const char *filename)
{
	char line[512];
	FILE *fp = fopen(filename, "r");
	char *p;

	while( (p = fgets(line, sizeof(line), fp)) != NULL) {
		success("%s", line);
		if (strncmp(line, LSTR, sizeof(LSTR)-1) == 0) {
			fclose(fp);
			return;
		}
	}
	fclose(fp);
	fail("file did not contain CLIENT_RANDOM\n");
}

static void run(const char *env, const char *filename)
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_certificate_credentials_t clicred;
	int ret;

	remove(filename);

#ifdef _WIN32
	{
		char buf[512];
		snprintf(buf, sizeof(buf), "%s=%s", env, filename);
		_putenv(buf);
	}
#else
	setenv(env, filename, 1);
#endif

	global_init();

	if (debug) {
		gnutls_global_set_log_level(6);
		gnutls_global_set_log_function(tls_log_func);
	}

	/* test gnutls_certificate_flags() */
	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);
	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);

	ret = gnutls_certificate_set_x509_key_mem(x509_cred, &server_ca3_localhost_cert_chain,
					    &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in error code\n");
		exit(1);
	}

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));


	test_cli_serv(x509_cred, clicred, "NORMAL", "localhost", NULL, NULL, NULL);

	if (access(filename, R_OK) != 0) {
		fail("keylog file was not created\n");
		exit(1);
	}

	search_for_str(filename);

	gnutls_certificate_free_credentials(x509_cred);
	gnutls_certificate_free_credentials(clicred);

	gnutls_global_deinit();
	remove(filename);

	if (debug)
		success("success");
}

void doit(void)
{
	char filename[TMPNAME_SIZE];

	assert(get_tmpname(filename)!=NULL);

	run("SSLKEYLOGFILE", filename);
}
