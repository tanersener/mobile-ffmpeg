/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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

#if defined(_WIN32)

int main()
{
	exit(77);
}

#else

# include <sys/types.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <sys/wait.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <gnutls/gnutls.h>
# include <gnutls/dtls.h>
# include <signal.h>

# ifndef MSG_NOSIGNAL

int main(void)
{
	exit(77);
}

# else

# include "utils.h"

static
void sigpipe(int sig)
{
	_exit(2);
}

#define BUF_SIZE 64

static void client(int fd)
{
	int ret;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_session_t session;
	char buf[BUF_SIZE];
	char buf2[BUF_SIZE];
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Use default priorities */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-TLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED));

	ret = gnutls_record_recv(session, buf, sizeof(buf));
	if (ret < 0 || ret != sizeof(buf)) {
		kill(getppid(), SIGPIPE);
		fail("client: recv failed");
	}
	if (debug)
		success("client: received %d bytes\n", ret);

	memset(buf2, 0, sizeof(buf));
	if (memcmp(buf, buf2, sizeof(buf)) != 0) {
		kill(getppid(), SIGPIPE);
		fail("client: recv data failed");
	}

	close(fd);
	gnutls_deinit(session);
	gnutls_anon_free_client_credentials(anoncred);
	gnutls_global_deinit();

	if (ret < 0) {
		fail("client: Handshake failed with unexpected reason: %s\n", gnutls_strerror(ret));
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}
}


/* These are global */
pid_t child;

static void server(int fd)
{
	gnutls_anon_server_credentials_t anoncred;
	gnutls_session_t session;
	int ret;
	char buf[BUF_SIZE];
	unsigned i;
	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER|GNUTLS_NO_SIGNAL);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-TLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED));

	if (ret < 0) {
		fail("error in handshake: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	memset(buf, 0, sizeof(buf));
	for (i=0;i<5;i++) {
		sleep(3);
		ret = gnutls_record_send(session, buf, sizeof(buf));
		if (ret < 0)
			break;
	}

	sleep(3);

	gnutls_deinit(session);
	gnutls_anon_free_server_credentials(anoncred);
	gnutls_global_deinit();

}

static void start(void)
{
	int fd[2];
	int ret;

	/* we need dgram in this test */
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
		close(fd[0]);
		server(fd[1]);
		close(fd[1]);
		kill(child, SIGTERM);
	} else {
		close(fd[1]);
		client(fd[0]);
		close(fd[0]);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	int status = 0;
	wait(&status);
	check_wait_status(status);
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, sigpipe);

	start();
}

# endif /* MSG_NOSIGNAL */
#endif				/* _WIN32 */
