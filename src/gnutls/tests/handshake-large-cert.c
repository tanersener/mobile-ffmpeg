/*
 * Copyright (C) 2016-2018 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)

/* socketpair isn't supported on Win32. */
int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#include <signal.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <assert.h>

#include "utils.h"

/* This test checks whether a large certificate packet can be sent by
 * server and received by client. */
const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static void client(int sd, const char *prio)
{
	int ret;
	gnutls_session_t session;
	char buf[1];
	gnutls_certificate_credentials_t clientx509cred;

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);

	assert(gnutls_init(&session, GNUTLS_CLIENT)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 60 * 1000);

	do {
		ret = gnutls_handshake(session);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		fail("client: Handshake failed\n");
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	do {
		ret = gnutls_record_recv(session, buf, 1);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	assert(ret == 0);

	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static unsigned char server_key_pem[] =
	"-----BEGIN RSA PRIVATE KEY-----\n"
	"MIIEowIBAAKCAQEAwJo7662RNezA254WRsXlbuzkPV5eNX7pX5Pj66T8/RKfz7rL\n"
	"GpKs4fNxhhIMSUDjy2KbEQXPUH9MWROgU8V//CfcnagLYCHtniqRE+eXv0fCLwWU\n"
	"SrA8n0qCBj94/NMA4kSmHf6dE5YdjDSpukyza5IshAuxZ32MDevE3JqMjvnZ5vY7\n"
	"drJSfal0V5gof3/7J41ZVxl+WJph50e2pY1E27/hY8q5yQ3DXnE5kTQjX664ozQ8\n"
	"UPtqGfkr4YjYe4e6PUWAjU27mQng0O0K+/w8gg6xBxN/AH3U7dg5/cY5IKDsN+Iq\n"
	"4UrcCgXWjhosv9IlIXqzK7IBMMphPmRMCLMH2QIDAQABAoIBAGpOdxZdZdH6zHQr\n"
	"rKYBouJ39H5+8MbcNtmfWmT9WvogZn8U3ffbz3qjkRxsJ8XjABiJY4egyk3nBXAB\n"
	"KjQyxbKbGeUXFLhJ4cq0OgFfid11MRQdIz2aSsutJ1llfVUm7cz2ES5rE6305Hg3\n"
	"tRr0LPAJ7XIwtgmmPUCNysnsr/pVrmPLfAnl/CfbLF2v/SfpbSpkgUTrZCNUMC44\n"
	"929K4c7cFEM4SP6pUad6MipPzY/SmxZ9yhX3MsROcLp+XLCOOJhhkqoB6LWiess2\n"
	"d7odweFRZ0Q0gBD/9EMMy3J5iUwfasf8b5n7z8AgPg9CeB+p/As2/RhRPXnwlS0A\n"
	"2KrxWQECgYEA0wM+5fJeL91s19vozCqi3mKVXTv68aL9iQJQNJc4UQm+yu7JvMn9\n"
	"koPri74QUpYkmyttaJsGNc90Oj54rSsR/cmEFJKgHOEAYSLeVetyO2XNoQvKdyB9\n"
	"UVof6joMLxQ368YCahfz4ogHTQqpzN0BD2TTnKXwCXQDikN/EBb4fHkCgYEA6aov\n"
	"8XVIVlxUY4VB/9PQ03OwxTLi+zTJMFJvNJozkat6MLJjAv2zxMt2kmlb0xx3wftD\n"
	"VJKHIQCeZmU8qWEZS0G58OPg+TPvQPqdnZmRz3bGfW6F++IDAqV4DEhQ+zXQL8Js\n"
	"j9+ocre+s0zXq1HkHgemBGOHy5/jN9cXnH3XTmECgYATRFiZ5mdzN2SY0RuQiNQW\n"
	"OiopOTDQn3FG8U8hfi1GOP2Syfrhog/lMOZw/AnBgLQW9wAmbQFEKI0URGAAb85U\n"
	"vfGxbzHvcRv3wpdKgRUNF16PNeRmvDC1HOWNHX+/TLlObeYKieVa6dDA2Bho/ET8\n"
	"gthPlVc1hcJM/Zy8e1x1AQKBgQCuLDiugGDaVtpkkIlAu8/WPk9Ovv6oh5FMHrZb\n"
	"/HFiLPLY56+cJCZjE9Kfkj9rHrY59yQaH1rwg7iO1PmhvAoRqb2DTSl+OHMn+WeR\n"
	"eU5R2dRc3QysU60wxMy2QxVyG4vCfedUW0ABuutAVZARWOp0Y/khHluzscu57O/h\n"
	"q3/ZIQKBgEXHmOjftWrkWV+/zfZT64k2Z1g7s3dpXW/SFK9jPrt6oqI1GNkYz6Ds\n"
	"O1dUiPsNXDCLytUtvYrvrT3rJaPjJDRU2HrN/cYdxXgf6HSEr3Cdcpqyp/5rOOxD\n"
	"ALEix6R4MZlsQV8FfgWjvTAET7NtY303JrCdFPqIigwl/PFGPLiB\n"
	"-----END RSA PRIVATE KEY-----\n";

static void server(int sd, const char *prio)
{
	gnutls_certificate_credentials_t serverx509cred;
	const gnutls_datum_t key = { server_key_pem,
		sizeof(server_key_pem)-1
	};
	int ret;
	gnutls_session_t session;
	gnutls_datum_t cert;
	const char *src = getenv("srcdir");
	char cert_path[256];

	if (src == NULL)
		src = ".";

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	snprintf(cert_path, sizeof(cert_path), "%s/data/large-cert.pem", src);

	assert(gnutls_load_file(cert_path, &cert)>=0);

	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &cert, &key,
					    GNUTLS_X509_FMT_PEM)>=0);

	gnutls_free(cert.data);

	assert(gnutls_init(&session, GNUTLS_SERVER)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 60 * 1000);

	do {
		ret = gnutls_handshake(session);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static
void start(const char *name, const char *prio)
{
	pid_t child;
	int sockets[2];
	int err;

	success("testing %s\n", name);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		perror("socketpair");
		fail("socketpair failed\n");
		return;
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status = 0;
		/* parent */
		close(sockets[0]);
		client(sockets[1], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[1]);
		server(sockets[0], prio);
		exit(0);
	}
}

void doit(void)
{
	start("tls1.2", "NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("tls1.3", "NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("default", "NORMAL");
}

#endif				/* _WIN32 */
