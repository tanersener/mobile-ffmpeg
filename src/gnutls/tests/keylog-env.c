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

#include <stdbool.h>
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

/* Test for SSLKEYLOGFILE being functional.
 *
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

/* In TLS 1.2, we only expect CLIENT_RANDOM.  */
static const char *tls12_included_labels[] = { "CLIENT_RANDOM", NULL };
static const char *tls12_excluded_labels[] = { NULL };

/* In TLS 1.3, we expect secrets derived in handshake phases, but not
 * CLIENT_RANDOM.  */
static const char *tls13_included_labels[] = {
	"CLIENT_HANDSHAKE_TRAFFIC_SECRET",
	"SERVER_HANDSHAKE_TRAFFIC_SECRET",
	"CLIENT_TRAFFIC_SECRET_0",
	"SERVER_TRAFFIC_SECRET_0",
	"EXPORTER_SECRET",
	NULL
};
static const char *tls13_excluded_labels[] = { "CLIENT_RANDOM", NULL };

static void search_for_str(const char *filename, const char *label, bool excluded)
{
	char line[512];
	FILE *fp = fopen(filename, "r");
	char *p;

	while( (p = fgets(line, sizeof(line), fp)) != NULL) {
		success("%s", line);
		if (strncmp(line, label, strlen(label)) == 0 &&
		    line[strlen(label)] == ' ') {
			fclose(fp);
			if (excluded)
				fail("file should not contain %s\n", label);
			return;
		}
	}
	fclose(fp);
	if (!excluded)
		fail("file should contain %s\n", label);
}

static void run(const char *filename, const char *prio,
		const char **included, const char **excluded)
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_certificate_credentials_t clicred;
	const char **p;
	int ret;

#ifdef _WIN32
	{
		char buf[512];
		snprintf(buf, sizeof(buf), "SSLKEYLOGFILE=%s", filename);
		_putenv(buf);
	}
#else
	setenv("SSLKEYLOGFILE", filename, 1);
#endif

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


	test_cli_serv(x509_cred, clicred, prio, "localhost", NULL, NULL, NULL);

	if (access(filename, R_OK) != 0) {
		fail("keylog file was not created\n");
		exit(1);
	}

	for (p = included; *p; p++)
		search_for_str(filename, *p, false);
	for (p = excluded; *p; p++)
		search_for_str(filename, *p, true);

	gnutls_certificate_free_credentials(x509_cred);
	gnutls_certificate_free_credentials(clicred);

	if (debug)
		success("success");
}

void doit(void)
{
	char filename[TMPNAME_SIZE];

	assert(get_tmpname(filename)!=NULL);

	remove(filename);
	global_init();

	run(filename,
	    "NONE:+VERS-TLS1.2:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA",
	    tls12_included_labels, tls12_excluded_labels);

	/* This is needed because the SSLKEYLOGFILE envvar is checked
	 * only once and the file is never closed until the library is
	 * unloaded.  Truncate the file to zero length, so we can
	 * reuse the same file for multiple tests.  */
	truncate(filename, 0);

	run(filename,
	    "NONE:+VERS-TLS1.3:+AES-256-GCM:+AEAD:+SIGN-ALL:+GROUP-ALL",
	    tls13_included_labels, tls13_excluded_labels);

	gnutls_global_deinit();
	remove(filename);
}
