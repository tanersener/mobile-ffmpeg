/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

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

#include "utils.h"

#define MAX_BUF 1024
static void terminate(void);

/* This program tests gnutls_server_name_set() and gnutls_server_name_get().
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static unsigned char server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICVjCCAcGgAwIBAgIERiYdMTALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTIxWhcNMDgwNDE3MTMyOTIxWjA3MRsw\n"
    "GQYDVQQKExJHbnVUTFMgdGVzdCBzZXJ2ZXIxGDAWBgNVBAMTD3Rlc3QuZ251dGxz\n"
    "Lm9yZzCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA17pcr6MM8C6pJ1aqU46o63+B\n"
    "dUxrmL5K6rce+EvDasTaDQC46kwTHzYWk95y78akXrJutsoKiFV1kJbtple8DDt2\n"
    "DZcevensf9Op7PuFZKBroEjOd35znDET/z3IrqVgbtm2jFqab7a+n2q9p/CgMyf1\n"
    "tx2S5Zacc1LWn9bIjrECAwEAAaOBkzCBkDAMBgNVHRMBAf8EAjAAMBoGA1UdEQQT\n"
    "MBGCD3Rlc3QuZ251dGxzLm9yZzATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8B\n"
    "Af8EBQMDB6AAMB0GA1UdDgQWBBTrx0Vu5fglyoyNgw106YbU3VW0dTAfBgNVHSME\n"
    "GDAWgBTpPBz7rZJu5gakViyi4cBTJ8jylTALBgkqhkiG9w0BAQUDgYEAaFEPTt+7\n"
    "bzvBuOf7+QmeQcn29kT6Bsyh1RHJXf8KTk5QRfwp6ogbp94JQWcNQ/S7YDFHglD1\n"
    "AwUNBRXwd3riUsMnsxgeSDxYBfJYbDLeohNBsqaPDJb7XailWbMQKfAbFQ8cnOxg\n"
    "rOKLUQRWJ0K3HyXRMhbqjdLIaQiCvQLuizo=\n" "-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXAIBAAKBgQDXulyvowzwLqknVqpTjqjrf4F1TGuYvkrqtx74S8NqxNoNALjq\n"
    "TBMfNhaT3nLvxqResm62ygqIVXWQlu2mV7wMO3YNlx696ex/06ns+4VkoGugSM53\n"
    "fnOcMRP/PciupWBu2baMWppvtr6far2n8KAzJ/W3HZLllpxzUtaf1siOsQIDAQAB\n"
    "AoGAYAFyKkAYC/PYF8e7+X+tsVCHXppp8AoP8TEZuUqOZz/AArVlle/ROrypg5kl\n"
    "8YunrvUdzH9R/KZ7saNZlAPLjZyFG9beL/am6Ai7q7Ma5HMqjGU8kTEGwD7K+lbG\n"
    "iomokKMOl+kkbY/2sI5Czmbm+/PqLXOjtVc5RAsdbgvtmvkCQQDdV5QuU8jap8Hs\n"
    "Eodv/tLJ2z4+SKCV2k/7FXSKWe0vlrq0cl2qZfoTUYRnKRBcWxc9o92DxK44wgPi\n"
    "oMQS+O7fAkEA+YG+K9e60sj1K4NYbMPAbYILbZxORDecvP8lcphvwkOVUqbmxOGh\n"
    "XRmTZUuhBrJhJKKf6u7gf3KWlPl6ShKEbwJASC118cF6nurTjuLf7YKARDjNTEws\n"
    "qZEeQbdWYINAmCMj0RH2P0mvybrsXSOD5UoDAyO7aWuqkHGcCLv6FGG+qwJAOVqq\n"
    "tXdUucl6GjOKKw5geIvRRrQMhb/m5scb+5iw8A4LEEHPgGiBaF5NtJZLALgWfo5n\n"
    "hmC8+G8F0F78znQtPwJBANexu+Tg5KfOnzSILJMo3oXiXhf5PqXIDmbN0BKyCKAQ\n"
    "LfkcEcUbVfmDaHpvzwY9VEaoMOKVLitETXdNSxVpvWM=\n"
    "-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

/* internal function */
int _gnutls_server_name_set_raw(gnutls_session_t session,
				gnutls_server_name_type_t type,
				const void *name, size_t name_length);

static void client(const char *test_name, int fd, unsigned raw, const char *name, unsigned name_len)
{
	int ret;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL", NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);
	if (raw)
		_gnutls_server_name_set_raw(session, GNUTLS_NAME_DNS, name, name_len);
	else
		gnutls_server_name_set(session, GNUTLS_NAME_DNS, name, name_len);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		test_fail("Handshake failed\n");
		gnutls_perror(ret);
		exit(1);
	} else {
		if (debug)
			test_success("Handshake was completed\n");
	}

	if (debug)
		test_success("TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

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

static void server(const char *test_name, int fd, const char *name, unsigned name_len)
{
	int ret;
	char buffer[MAX_BUF + 1];
	unsigned int type;
	size_t buffer_size;
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
	gnutls_priority_set_direct(session, "NORMAL", NULL);

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
		test_fail("Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		test_success("Handshake was completed\n");

	if (debug)
		test_success("TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	buffer_size = sizeof(buffer);
	ret = gnutls_server_name_get(session, buffer, &buffer_size, &type, 0);

	if ((name == NULL || name[0] == 0) && (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE || ret == GNUTLS_E_IDNA_ERROR)) {
		/* expected */
		if (debug)
			test_success("empty name matches\n");
	} else if (ret < 0) {
		test_fail("server_name: %s/%d\n", gnutls_strerror(ret), ret);
	} else {
		if (name == NULL || name[0] == 0) {
			test_fail("did not received expected name\n");
			exit(1);
		}
		if (buffer_size != strlen(buffer)) {
			test_fail("received name '%s/%d/%d', with embedded null\n", buffer, (int)buffer_size, (int)strlen(buffer));
			exit(1);
		}
		if (name_len != buffer_size || memcmp(name, buffer, name_len) != 0) {
			test_fail("received name '%s/%d', expected '%s/%d'\n", buffer, (int)buffer_size, name, (int)name_len);
			exit(1);
		}
		if (debug)
			test_success("name matches (%s/%s)\n", buffer, name);
	}


	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		test_success("finished\n");
}

/* name: the name sent by client
 * server_exp: the name which should be expected by the server to see
 */
static void start(const char *test_name, unsigned raw, const char *name, unsigned len, const char *server_exp, unsigned server_exp_len)
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
		test_fail("fork");
		exit(1);
	}

	if (child) {
		/* parent */
		close(fd[1]);
		server(test_name, fd[0], server_exp, server_exp_len);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(test_name, fd[1], raw, name, len);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	check_wait_status(status);
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	start("NULL", 0, NULL, 0, NULL, 0);
	start("empty", 0, "", 0, "", 0);
	start("test.example.com", 0, "test.example.com", strlen("test.example.com"), "test.example.com", strlen("test.example.com"));
	start("longtest.example.com", 0, "longtest.example.com.", strlen("longtest.example.com"), "longtest.example.com.", strlen("longtest.example.com"));
#if defined(HAVE_LIBIDN) || defined(HAVE_LIBIDN2)
	/* test invalid UTF8 */
	start("invalid-utf8", 1, "invalid\xff.example.com.", sizeof("invalid\xff.example.com")-1, NULL, 0);
#endif
	/* test embedded NULL */
	start("embedded-NULL", 1, "invalid\x00.example.com.", sizeof("invalid\x00.example.com")-1, NULL, 0);
}

#endif				/* _WIN32 */
