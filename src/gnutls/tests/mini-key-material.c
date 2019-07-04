/*
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
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

#if defined(_WIN32) || !defined(ENABLE_ALPN)

int main(int argc, char **argv)
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

#include "utils.h"

static void terminate(void);

/* This program tests whether the gnutls_record_get_state() works as
 * expected.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

/* These are global */
static pid_t child;

/* A very basic DTLS client, with anonymous authentication, that negotiates SRTP
 */

static void dump(const char *name, uint8_t *data, unsigned data_size)
{
	unsigned i;

	fprintf(stderr, "%s", name);
	for (i=0;i<data_size;i++)
		fprintf(stderr, "%.2x", (unsigned)data[i]);
	fprintf(stderr, "\n");
}

static void terminate(void)
{
	int status = 0;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void client(int fd)
{
	gnutls_session_t session;
	int ret;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_datum_t mac_key, iv, cipher_key;
	gnutls_datum_t read_mac_key, read_iv, read_cipher_key;
	unsigned char rseq_number[8];
	unsigned char wseq_number[8];
	unsigned char key_material[512], *p;
	unsigned i;
	unsigned block_size, hash_size, key_size, iv_size;
	const char *err;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	ret = gnutls_priority_set_direct(session,
				   "NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+ANON-DH:+ANON-ECDH:+CURVE-ALL",
				   &err);
	if (ret < 0) {
		fail("client: priority set failed (%s): %s\n",
		     gnutls_strerror(ret), err);
		exit(1);
	}

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", strerror(ret));
		terminate();
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	ret = gnutls_cipher_get(session);
	if (ret != GNUTLS_CIPHER_AES_128_CBC) {
		fprintf(stderr, "negotiated unexpected cipher: %s\n", gnutls_cipher_get_name(ret));
		terminate();
	}

	ret = gnutls_mac_get(session);
	if (ret != GNUTLS_MAC_SHA1) {
		fprintf(stderr, "negotiated unexpected mac: %s\n", gnutls_mac_get_name(ret));
		terminate();
	}

	iv_size = 16;
	hash_size = 20;
	key_size = 16;
	block_size = 2*hash_size + 2*key_size + 2 *iv_size;

	ret = gnutls_prf(session, 13, "key expansion", 1, 0, NULL, block_size,
			 (void*)key_material);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		gnutls_perror(ret);
		terminate();
	}
	p = key_material;

	/* check whether the key material matches our calculations */
	ret = gnutls_record_get_state(session, 0, &mac_key, &iv, &cipher_key, wseq_number);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		gnutls_perror(ret);
		terminate();
	}

	if (memcmp(wseq_number, "\x00\x00\x00\x00\x00\x00\x00\x01", 8) != 0) {
		dump("wseq:", wseq_number, 8);
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}

	ret = gnutls_record_get_state(session, 1, &read_mac_key, &read_iv, &read_cipher_key, rseq_number);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		gnutls_perror(ret);
		terminate();
	}

	if (memcmp(rseq_number, "\x00\x00\x00\x00\x00\x00\x00\x01", 8) != 0) {
		dump("rseq:", rseq_number, 8);
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}

	if (hash_size != mac_key.size || memcmp(p, mac_key.data, hash_size) != 0) {
		dump("MAC:", mac_key.data, mac_key.size);
		dump("Block:", key_material, block_size);
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}
	p+= hash_size;

	if (hash_size != read_mac_key.size || memcmp(p, read_mac_key.data, hash_size) != 0) {
		dump("MAC:", read_mac_key.data, read_mac_key.size);
		dump("Block:", key_material, block_size);
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}
	p+= hash_size;

	if (key_size != cipher_key.size || memcmp(p, cipher_key.data, key_size) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}
	p+= key_size;

	if (key_size != read_cipher_key.size || memcmp(p, read_cipher_key.data, key_size) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}
	p+= key_size;

	if (iv_size != iv.size || memcmp(p, iv.data, iv_size) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}
	p+=iv_size;

	if (iv_size != read_iv.size || memcmp(p, read_iv.data, iv_size) != 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}

	/* check sequence numbers */
	for (i=0;i<5;i++) {
		ret = gnutls_record_send(session, "hello", 5);
		if (ret < 0) {
			fail("gnutls_record_send: %s\n", gnutls_strerror(ret));
		}
	}

	ret = gnutls_record_get_state(session, 0, NULL, NULL, NULL, wseq_number);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		gnutls_perror(ret);
		terminate();
	}

	if (memcmp(wseq_number, "\x00\x00\x00\x00\x00\x00\x00\x06", 8) != 0) {
		dump("wseq:", wseq_number, 8);
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}

	ret = gnutls_record_get_state(session, 1, NULL, NULL, NULL, rseq_number);
	if (ret < 0) {
		fprintf(stderr, "error in %d\n", __LINE__);
		gnutls_perror(ret);
		terminate();
	}

	if (memcmp(rseq_number, "\x00\x00\x00\x00\x00\x00\x00\x01", 8) != 0) {
		dump("wseq:", wseq_number, 8);
		fprintf(stderr, "error in %d\n", __LINE__);
		terminate();
	}
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);

	gnutls_global_deinit();
}

static void server(int fd)
{
	int ret;
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_dh_params_t dh_params;
	char buf[128];
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_server_credentials(&anoncred);
	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_anon_set_server_dh_params(anoncred, dh_params);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	ret = gnutls_priority_set_direct(session,
				   "NORMAL:+ANON-DH:+ANON-ECDH:-VERS-ALL:+VERS-TLS1.0", NULL);
	if (ret < 0) {
		fail("server: priority set failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	do {
		ret = gnutls_record_recv(session, buf, sizeof(buf));
	} while(ret > 0);

	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
	}

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);
	gnutls_dh_params_deinit(dh_params);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(void)
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
		int status;
		/* parent */

		server(fd[0]);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1]);
		exit(0);
	}
}

void doit(void)
{
	start();
}

#endif				/* _WIN32 */
