/*
 * Copyright (C) 2014 Nikos Mavrogiannopoulos
 * Copyright (C) 2017 Red Hat, Inc.
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)

int main()
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>
#include "cert-common.h"
#include "utils.h"

static void terminate(void);

/* This program tests the ability of the record layer,
 * to detect modified record packets, under various
 * ciphersuites.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static int modify = 0;

static ssize_t
client_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	unsigned int fd = (long)tr;

	if (modify == 0)
		return send(fd, data, len, 0);
	else {
		uint8_t *p = (void*)data;
		if (len < 30) {
			fail("test error in packet sending\n");
			exit(1);
		}
		p[len-30]++;
		return send(fd, data, len, 0);
	}
}

#define MAX_BUF 24*1024

static void client(int fd, const char *prio, int ign)
{
	int ret;
	char buffer[MAX_BUF + 1];
	const char* err;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();
	memset(buffer, 2, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	gnutls_transport_set_push_function(session, client_push);
	/* Use default priorities */
	ret = gnutls_priority_set_direct(session, prio, &err);
	if (ret < 0) {
		fail("error setting priority: %s\n", err);
		exit(1);
	}

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client (%s): Handshake has failed (%s)\n\n", prio,
		     gnutls_strerror(ret));
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	modify = 1;
	do {
		ret = gnutls_record_send(session, buffer, 2048);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	modify = 0;

	if (ret < 0) {
		fail("client[%s]: Error sending packet: %s\n", prio, gnutls_strerror(ret));
		terminate();
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


/* These are global */
pid_t child;

static void terminate(void)
{
	kill(child, SIGTERM);
	exit(1);
}

static void server(int fd, const char *prio, int ign)
{
	int ret;
	const char* err;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	ret = gnutls_priority_set_direct(session, prio, &err);
	if (ret < 0) {
		fail("error setting priority: %s\n", err);
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server (%s): Handshake has failed (%s)\n\n", prio,
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* Here we do both a receive and a send test because if valgrind
	 * detects an error on the peer, the main process will never know.
	 */

	/* make sure we are not blocked forever */
	gnutls_record_set_timeout(session, 10000);

	/* Test receiving */
	do {
		ret = gnutls_record_recv(session, buffer, MAX_BUF);
	} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	
	if (ret != GNUTLS_E_DECRYPTION_FAILED) {
		fail("server: received modified packet with error code %d\n", ret);
		exit(1);
	}

	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *name, const char *prio, int ign)
{
	int fd[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		exit(1);
	}

	if (child) {
		/* parent */
		success("testing %s\n", name);
		close(fd[1]);
		server(fd[0], prio, ign);
	} else {
		close(fd[0]);
		client(fd[1], prio, ign);
		exit(0);
	}
}

#define AES_CBC "NONE:+VERS-TLS1.0:-CIPHER-ALL:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_CBC_SHA256 "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-CBC:+AES-256-CBC:+SHA256:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_GCM "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-GCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_CCM "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-CCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define AES_CCM_8 "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-CCM-8:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"

#define ARCFOUR_SHA1 "NONE:+VERS-TLS1.0:-CIPHER-ALL:+ARCFOUR-128:+SHA1:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"
#define ARCFOUR_MD5 "NONE:+VERS-TLS1.0:-CIPHER-ALL:+ARCFOUR-128:+MD5:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL:+RSA"

#define NULL_SHA1 "NONE:+VERS-TLS1.0:-CIPHER-ALL:+NULL:+SHA1:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+RSA:+CURVE-ALL"

#define NO_ETM ":%NO_ETM"

#define TLS13_AES_GCM "NONE:+VERS-TLS1.3:-CIPHER-ALL:+RSA:+AES-128-GCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+GROUP-ALL"
#define TLS13_AES_CCM "NONE:+VERS-TLS1.3:-CIPHER-ALL:+RSA:+AES-128-CCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+GROUP-ALL"
#define TLS13_CHACHA_POLY1305 "NONE:+VERS-TLS1.3:-CIPHER-ALL:+RSA:+CHACHA20-POLY1305:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+GROUP-ALL"

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	check_wait_status(status);
	return;
}

void doit(void)
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, ch_handler);

	start("aes-cbc", AES_CBC, 1);
	start("aes-cbc-sha256", AES_CBC_SHA256, 1);
	start("aes-gcm", AES_GCM, 0);
	start("aes-ccm", AES_CCM, 0);
	start("aes-ccm-8", AES_CCM_8, 0);

	if (!gnutls_fips140_mode_enabled()) {
		start("null-sha1", NULL_SHA1, 0);

		start("arcfour-sha1", ARCFOUR_SHA1, 0);
		start("arcfour-md5", ARCFOUR_MD5, 0);
	}

	start("aes-cbc-no-etm", AES_CBC NO_ETM, 1);
	start("aes-cbc-sha256-no-etm", AES_CBC_SHA256 NO_ETM, 1);
	start("aes-gcm-no-etm", AES_GCM NO_ETM, 0);

	if (!gnutls_fips140_mode_enabled()) {
		start("null-sha1-no-etm", NULL_SHA1 NO_ETM, 0);

		start("arcfour-sha1-no-etm", ARCFOUR_SHA1 NO_ETM, 0);
		start("arcfour-md5-no-etm", ARCFOUR_MD5 NO_ETM, 0);
		start("tls13-chacha20-poly1305", TLS13_CHACHA_POLY1305, 0);
	}

	start("tls13-aes-gcm", TLS13_AES_GCM, 0);
	start("tls13-aes-ccm", TLS13_AES_CCM, 0);
}

#endif				/* _WIN32 */
