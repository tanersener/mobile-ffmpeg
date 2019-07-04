/*
 * Copyright (C) 2018 Free Software Foundation, Inc.
 *
 * Author: Ander Juaristi
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
int main(int argc, char **argv)
{
	exit(77);
}
#else

#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <gnutls/gnutls.h>
#include <assert.h>
#include "utils.h"
#include "cert-common.h"
#include "virt-time.h"

/*
 * This will set the following values:
 *
 *  - Ticket key expiration: 1 second.
 *  - Session ticket key rotation period: 3 seconds.
 */
#define TICKET_EXPIRATION 1 /* seconds */

unsigned num_stek_rotations;

static void stek_rotation_callback(const gnutls_datum_t *prev_key,
		const gnutls_datum_t *new_key,
		uint64_t t)
{
	num_stek_rotations++;
	success("STEK was rotated!\n");
}

typedef void (* gnutls_stek_rotation_callback_t) (const gnutls_datum_t *prev_key,
		const gnutls_datum_t *new_key,
		uint64_t t);
void _gnutls_set_session_ticket_key_rotation_callback(gnutls_session_t session,
		gnutls_stek_rotation_callback_t cb);

static int handshake(gnutls_session_t session, gnutls_datum_t *session_data,
		int resumption_should_succeed)
{
	int ret;

	do {
			ret = gnutls_handshake(session);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		gnutls_perror(ret);
		fail("client: Handshake failed\n");
	} else {
		success("client: Handshake was completed\n");
	}

	if (gnutls_session_is_resumed(session)) {
		if (!resumption_should_succeed)
			fail("client: Session was resumed (but should not)\n");
		else
			success("client: Success: Session was resumed\n");
	} else {
		if (resumption_should_succeed)
			fail("client: Session was not resumed (but should)\n");
		else
			success("client: Success: Session was NOT resumed\n");
	}

	ret = gnutls_session_get_data2(session, session_data);
	if (ret < 0) {
		gnutls_perror(ret);
		fail("client: Could not get session data\n");
	}

	do {
		ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	return 0;
}

static int resume_and_close(gnutls_session_t session, gnutls_datum_t *session_data,
		int resumption_should_succeed)
{
	int ret;

	ret = gnutls_session_set_data(session, session_data->data, session_data->size);
	if (ret < 0) {
		gnutls_perror(ret);
		fail("client: Could not get session data\n");
	}

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && !gnutls_error_is_fatal(ret));

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
	} else {
		success("client: Handshake was completed\n");
	}

	if (gnutls_session_is_resumed(session)) {
		if (!resumption_should_succeed)
			fail("client: Session was resumed (but should not)\n");
		else
			success("client: Success: Session was resumed\n");
	} else {
		if (resumption_should_succeed)
			fail("client: Session was not resumed (but should)\n");
		else
			success("client: Success: Session was NOT resumed\n");
	}

	do {
		ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	return 0;
}

static void client(int fd, int *resumption_should_succeed, unsigned num_sessions, const char *prio)
{
	gnutls_session_t session;
	gnutls_datum_t session_data;
	gnutls_certificate_credentials_t clientx509cred = NULL;

	gnutls_certificate_allocate_credentials(&clientx509cred);

	/* Initialize TLS layer */
	gnutls_init(&session, GNUTLS_CLIENT);
	gnutls_priority_set_direct(session, prio, NULL);

	/* put the anonymous credentials to the current session */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, fd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	if (handshake(session, &session_data, resumption_should_succeed[0]) < 0)
		return;

	if (clientx509cred)
		gnutls_certificate_free_credentials(clientx509cred);
	gnutls_deinit(session);

	for (unsigned i = 1; i < num_sessions; i++) {
		assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);

		/* Initialize TLS layer */
		assert(gnutls_init(&session, GNUTLS_CLIENT)>=0);
		assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

		/* put the anonymous credentials to the current session */
		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
					clientx509cred);

		gnutls_transport_set_int(session, fd);

		if (resume_and_close(session, &session_data, resumption_should_succeed[i]) < 0)
			return;

		if (clientx509cred)
			gnutls_certificate_free_credentials(clientx509cred);
		gnutls_deinit(session);
	}
}

static void server(int fd, int *resumption_should_succeed, unsigned num_sessions, const char *prio)
{
	int retval;
	gnutls_session_t session;
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_datum_t session_ticket_key = { NULL, 0 };

	virt_time_init();

	if (gnutls_session_ticket_key_generate(&session_ticket_key) < 0)
		fail("server: Could not generate session ticket key\n");

	for (unsigned i = 0; i < num_sessions; i++) {
		if ((retval = gnutls_init(&session, GNUTLS_SERVER)) < 0) {
			gnutls_perror(retval);
			fail("gnutls_init() failed\n");
		}

		assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
		assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					&server_cert, &server_key,
					GNUTLS_X509_FMT_PEM)>=0);

		assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, serverx509cred);

		retval = gnutls_session_ticket_enable_server(session, &session_ticket_key);
		if (retval != GNUTLS_E_SUCCESS) {
			gnutls_perror(retval);
			fail("server: Could not enable session tickets\n");
		}


		gnutls_db_set_cache_expiration(session, TICKET_EXPIRATION);

		_gnutls_set_session_ticket_key_rotation_callback(session, stek_rotation_callback);

		gnutls_transport_set_int(session, fd);
		gnutls_handshake_set_timeout(session, 20 * 1000);

		do {
			retval = gnutls_handshake(session);
		} while(retval == GNUTLS_E_AGAIN || retval == GNUTLS_E_INTERRUPTED);

		if (retval < 0) {
			fail("server: Handshake failed: %s\n", gnutls_strerror(retval));
		} else {
			success("server: Handshake was completed\n");
		}

		if (gnutls_session_is_resumed(session)) {
			if (!resumption_should_succeed[i])
				fail("server: Session was resumed (but should not)\n");
			else
				success("server: Success: Session was resumed\n");
		} else {
			if (resumption_should_succeed[i])
				fail("server: Session was not resumed (but should)\n");
			else
				success("server: Success: Session was NOT resumed\n");
		}

		gnutls_bye(session, GNUTLS_SHUT_RDWR);
		gnutls_deinit(session);
		gnutls_certificate_free_credentials(serverx509cred);
		serverx509cred = NULL;

		if (i != 0)
			virt_sec_sleep(TICKET_EXPIRATION);
	}

	if (num_stek_rotations != 4)
		fail("STEK should be rotated exactly 4 times!\n");

	if (serverx509cred)
		gnutls_certificate_free_credentials(serverx509cred);
	gnutls_free(session_ticket_key.data);
}

static void run(const char *name, const char *prio, int resumption_should_succeed[], int rounds)
{
	pid_t child;
	int retval, sockets[2], status = 0;

	success("\ntesting %s\n\n", name);

	retval = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (retval == -1) {
		perror("socketpair");
		fail("socketpair failed");
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork failed");
	}

	if (child) {
		/* We are the parent */
		server(sockets[0], resumption_should_succeed, rounds, prio);
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		/* We are the child */
		client(sockets[1], resumption_should_succeed, rounds, prio);
		exit(0);
	}
}

void doit(void)
{
	int resumption_should_succeed[] = { 0, 1, 1, 0 };

	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	num_stek_rotations = 0;
	run("tls1.2 resumption", "NORMAL:-VERS-ALL:+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0",
			resumption_should_succeed, 4);

	num_stek_rotations = 0;
	run("tls1.3 resumption", "NORMAL:-VERS-ALL:+VERS-TLS1.3",
			resumption_should_succeed, 4);
}

#endif
