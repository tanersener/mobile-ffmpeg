/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
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

#if defined(_WIN32)

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
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <sys/wait.h>
#include <signal.h>

#include "utils.h"

static void wrap_db_init(void);
static void wrap_db_deinit(void);
static int wrap_db_store(void *dbf, gnutls_datum_t key,
			 gnutls_datum_t data);
static gnutls_datum_t wrap_db_fetch(void *dbf, gnutls_datum_t key);
static int wrap_db_delete(void *dbf, gnutls_datum_t key);

#define TLS_SESSION_CACHE 50

struct params_res {
	const char *desc;
	int enable_db;
	int enable_session_ticket_server;
	int enable_session_ticket_client;
	int expect_resume;
};

pid_t child;

struct params_res resume_tests[] = {
	{"try to resume from db", 50, 0, 0, 1},
	{"try to resume from session ticket", 0, 1, 1, 1},
	{"try to resume from session ticket (server only)", 0, 1, 0, 0},
	{"try to resume from session ticket (client only)", 0, 0, 1, 0},
	{NULL, -1}
};

/* A very basic TLS client, with anonymous authentication.
 */

#define SESSIONS 2
#define MAX_BUF 5*1024
#define MSG "Hello TLS"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", child ? "server" : "client", level,
		str);
}

static void client(int sds[], struct params_res *params)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_anon_client_credentials_t anoncred;
	/* Need to enable anonymous KX specifically. */

	/* variables used in session resuming
	 */
	int t;
	gnutls_datum_t session_data;

	if (debug) {
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(3);
	}
	global_init();

	gnutls_anon_allocate_client_credentials(&anoncred);

	for (t = 0; t < SESSIONS; t++) {
		int sd = sds[t];

		/* Initialize TLS session
		 */
		gnutls_init(&session,
			    GNUTLS_CLIENT | GNUTLS_DATAGRAM);

		/* Use default priorities */
		if (params->enable_session_ticket_client)
			gnutls_priority_set_direct(session,
					   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-DH",
					   NULL);
		else
			gnutls_priority_set_direct(session,
					   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-DH:%NO_TICKETS",
					   NULL);

		/* put the anonymous credentials to the current session
		 */
		gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

		if (t > 0) {
			/* if this is not the first time we connect */
			gnutls_session_set_data(session, session_data.data,
						session_data.size);
			gnutls_free(session_data.data);
		}

		gnutls_transport_set_int(session, sd);

		/* Perform the TLS handshake
		 */
		gnutls_dtls_set_timeouts(session, 3*1000, 240 * 1000);
		do {
			ret = gnutls_handshake(session);
		} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

		if (ret < 0) {
			gnutls_perror(ret);
			fail("client: Handshake failed\n");
			goto end;
		} else {
			if (debug)
				success
				    ("client: Handshake was completed\n");
		}

		if (t == 0) {	/* the first time we connect */
			/* get the session data size */
			ret =
			    gnutls_session_get_data2(session,
						     &session_data);
			if (ret < 0)
				fail("Getting resume data failed\n");
		} else {	/* the second time we connect */

			/* check if we actually resumed the previous session */
			if (gnutls_session_is_resumed(session) != 0) {
				if (params->expect_resume) {
					if (debug)
						success
						    ("- Previous session was resumed\n");
				} else
					fail("- Previous session was resumed\n");
			} else {
				if (params->expect_resume) {
					fail("*** Previous session was NOT resumed\n");
				} else {
					if (debug)
						success
						    ("*** Previous session was NOT resumed (expected)\n");
				}
			}
		}

		gnutls_record_send(session, MSG, strlen(MSG));

		ret = gnutls_record_recv(session, buffer, MAX_BUF);
		if (ret == 0) {
			if (debug)
				success
				    ("client: Peer has closed the TLS connection\n");
			goto end;
		} else if (ret < 0) {
			fail("client: Error: %s\n", gnutls_strerror(ret));
			goto end;
		}

		if (debug) {
			printf("- Received %d bytes: ", ret);
			for (ii = 0; ii < ret; ii++) {
				fputc(buffer[ii], stdout);
			}
			fputs("\n", stdout);
		}

		gnutls_bye(session, GNUTLS_SHUT_RDWR);

		close(sd);

		gnutls_deinit(session);
	}

      end:
	gnutls_anon_free_client_credentials(anoncred);
}

/* This is a sample TLS 1.0 echo server, for anonymous authentication only.
 */

#define DH_BITS 1024

/* These are global */

static gnutls_dh_params_t dh_params;

static int generate_dh_params(void)
{
	const gnutls_datum_t p3 = { (void *) pkcs3, strlen(pkcs3) };
	/* Generate Diffie-Hellman parameters - for use with DHE
	 * kx algorithms. These should be discarded and regenerated
	 * once a day, once a week or once a month. Depending on the
	 * security requirements.
	 */
	gnutls_dh_params_init(&dh_params);
	return gnutls_dh_params_import_pkcs3(dh_params, &p3,
					     GNUTLS_X509_FMT_PEM);
}

static char buffer[MAX_BUF + 1];

static void global_stop(void)
{
	if (debug)
		success("global stop\n");

	gnutls_dh_params_deinit(dh_params);

	gnutls_global_deinit();
}

static void server(int sds[], struct params_res *params)
{
	gnutls_anon_server_credentials_t anoncred;
	static gnutls_datum_t session_ticket_key = { NULL, 0 };
	int ret;
	size_t t;
	gnutls_session_t session;

	/* this must be called once in the program, it is mostly for the server.
	 */
	if (debug) {
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(3);
	}

	global_init();
	gnutls_anon_allocate_server_credentials(&anoncred);

	if (debug)
		success("Launched, generating DH parameters...\n");

	gnutls_anon_set_server_dh_params(anoncred, dh_params);

	if (params->enable_db) {
		wrap_db_init();
	}

	if (params->enable_session_ticket_server)
		gnutls_session_ticket_key_generate(&session_ticket_key);

	for (t = 0; t < SESSIONS; t++) {
		int sd = sds[t];

		gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);

		gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-DH",
				   NULL);

		gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
		gnutls_dh_set_prime_bits(session, DH_BITS);

		if (params->enable_db) {
			gnutls_db_set_retrieve_function(session, wrap_db_fetch);
			gnutls_db_set_remove_function(session, wrap_db_delete);
			gnutls_db_set_store_function(session, wrap_db_store);
			gnutls_db_set_ptr(session, NULL);
		}

		if (params->enable_session_ticket_server)
			gnutls_session_ticket_enable_server(session,
						    &session_ticket_key);

		gnutls_transport_set_int(session, sd);
		gnutls_dtls_set_timeouts(session, 3*1000, 240 * 1000);
		
		do {
			ret = gnutls_handshake(session);
		} while (ret < 0 && (ret == GNUTLS_E_INTERRUPTED||ret == GNUTLS_E_AGAIN));
		if (ret < 0) {
			close(sd);
			gnutls_deinit(session);
			kill(child, SIGTERM);
			fail("server: Handshake has failed (%s)\n\n",
			     gnutls_strerror(ret));
			return;
		}
		if (debug)
			success("server: Handshake was completed\n");

		/* see the Getting peer's information example */
		/* print_info(session); */

		for (;;) {
			memset(buffer, 0, MAX_BUF + 1);
			ret = gnutls_record_recv(session, buffer, MAX_BUF);

			if (ret == 0) {
				if (debug)
					success
					    ("server: Peer has closed the GnuTLS connection\n");
				break;
			} else if (ret < 0) {
				kill(child, SIGTERM);
				fail("server: Received corrupted data(%d). Closing...\n", ret);
				break;
			} else if (ret > 0) {
				/* echo data back to the client
				 */
				gnutls_record_send(session, buffer,
						   strlen(buffer));
			}
		}
		/* do not wait for the peer to close the connection.
		 */
		gnutls_bye(session, GNUTLS_SHUT_WR);

		close(sd);

		gnutls_deinit(session);
	}

	if (params->enable_db) {
		wrap_db_deinit();
	}

	gnutls_free(session_ticket_key.data);
	session_ticket_key.data = NULL;
	gnutls_anon_free_server_credentials(anoncred);

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	int i, err;

	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	generate_dh_params();

	for (i = 0; resume_tests[i].desc; i++) {
		int client_sds[SESSIONS], server_sds[SESSIONS];
		int j;

		printf("%s\n", resume_tests[i].desc);

		for (j = 0; j < SESSIONS; j++) {
			int sockets[2];

			err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
			if (err == -1) {
				perror("socketpair");
				fail("socketpair failed\n");
				return;
			}

			server_sds[j] = sockets[0];
			client_sds[j] = sockets[1];
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
			for (j = 0; j < SESSIONS; j++)
				close(client_sds[j]);
			server(server_sds, &resume_tests[i]);
			wait(&status);
			check_wait_status(status);
		} else {
			for (j = 0; j < SESSIONS; j++)
				close(server_sds[j]);
			client(client_sds, &resume_tests[i]);
			gnutls_global_deinit();
			exit(0);
		}
	}
	global_stop();
}

/* Functions and other stuff needed for session resuming.
 * This is done using a very simple list which holds session ids
 * and session data.
 */

#define MAX_SESSION_ID_SIZE 32
#define MAX_SESSION_DATA_SIZE 1024

typedef struct {
	unsigned char session_id[MAX_SESSION_ID_SIZE];
	unsigned int session_id_size;

	char session_data[MAX_SESSION_DATA_SIZE];
	int session_data_size;
} CACHE;

static CACHE *cache_db;
static int cache_db_ptr = 0;

static void wrap_db_init(void)
{

	/* allocate cache_db */
	cache_db = calloc(1, TLS_SESSION_CACHE * sizeof(CACHE));
}

static void wrap_db_deinit(void)
{
	free(cache_db);
	cache_db = NULL;
	return;
}

static int
wrap_db_store(void *dbf, gnutls_datum_t key, gnutls_datum_t data)
{
	if (debug)
		success("resume db storing... (%d-%d)\n", key.size,
			data.size);

	if (debug) {
		unsigned int i;
		printf("key:\n");
		for (i = 0; i < key.size; i++) {
			printf("%02x ", key.data[i] & 0xFF);
			if ((i + 1) % 16 == 0)
				printf("\n");
		}
		printf("\n");
		printf("data:\n");
		for (i = 0; i < data.size; i++) {
			printf("%02x ", data.data[i] & 0xFF);
			if ((i + 1) % 16 == 0)
				printf("\n");
		}
		printf("\n");
	}

	if (cache_db == NULL)
		return -1;

	if (key.size > MAX_SESSION_ID_SIZE)
		return -1;

	if (data.size > MAX_SESSION_DATA_SIZE)
		return -1;

	memcpy(cache_db[cache_db_ptr].session_id, key.data, key.size);
	cache_db[cache_db_ptr].session_id_size = key.size;

	memcpy(cache_db[cache_db_ptr].session_data, data.data, data.size);
	cache_db[cache_db_ptr].session_data_size = data.size;

	cache_db_ptr++;
	cache_db_ptr %= TLS_SESSION_CACHE;

	return 0;
}

static gnutls_datum_t wrap_db_fetch(void *dbf, gnutls_datum_t key)
{
	gnutls_datum_t res = { NULL, 0 };
	unsigned i;

	if (debug)
		success("resume db fetch... (%d)\n", key.size);
	if (debug) {
		printf("key:\n");
		for (i = 0; i < key.size; i++) {
			printf("%02x ", key.data[i] & 0xFF);
			if ((i + 1) % 16 == 0)
				printf("\n");
		}
		printf("\n");
	}

	if (cache_db == NULL)
		return res;

	for (i = 0; i < TLS_SESSION_CACHE; i++) {
		if (key.size == cache_db[i].session_id_size &&
		    memcmp(key.data, cache_db[i].session_id,
			   key.size) == 0) {
			if (debug)
				success
				    ("resume db fetch... return info\n");

			res.size = cache_db[i].session_data_size;

			res.data = gnutls_malloc(res.size);
			if (res.data == NULL)
				return res;

			memcpy(res.data, cache_db[i].session_data,
				res.size);

			if (debug) {
				unsigned j;
				printf("data:\n");
				for (j = 0; j < res.size; j++) {
					printf("%02x ",
						res.data[j] & 0xFF);
					if ((j + 1) % 16 == 0)
						printf("\n");
				}
				printf("\n");
			}

			return res;
		}
	}

	if (debug)
		success("resume db fetch... NOT FOUND\n");
	return res;
}

static int wrap_db_delete(void *dbf, gnutls_datum_t key)
{
	int i;

	if (cache_db == NULL)
		return -1;

	for (i = 0; i < TLS_SESSION_CACHE; i++) {
		if (key.size == cache_db[i].session_id_size &&
		    memcmp(key.data, cache_db[i].session_id,
			   key.size) == 0) {

			cache_db[i].session_id_size = 0;
			cache_db[i].session_data_size = 0;

			return 0;
		}
	}

	return -1;

}

#endif				/* _WIN32 */
