/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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

#include "utils.h"

static void terminate(void);

/* This program tests whether MD5 is rejected by a client as a
 * signature algorithm for the ServerKeyExchange.
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


static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+ECDHE-RSA", NULL);
	return 0;
}
	

/* A very basic TLS client, with anonymous authentication.
 */

#define MAX_BUF 1024

static void client(int fd)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-MD5", NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);
	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE,
					   GNUTLS_HOOK_PRE,
					   handshake_callback);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret == GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM) {
		/* success */
		goto end;
	}

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
		terminate();
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (gnutls_sign_algorithm_get(session) == GNUTLS_SIGN_RSA_MD5) {
		fail("client: MD5 was negotiated\n");
		terminate();
	}
	success("client: %s was negotiated\n", gnutls_sign_get_name(gnutls_sign_algorithm_get(session)));

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	do {
		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	} while (ret > 0);

	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
		terminate();
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

      end:

	close(fd);

	gnutls_deinit(session);

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

static void server(int fd)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
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

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-MD5", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		/* failure is expected here */
		goto end;
	}

	if (debug) {
		success("server: Handshake was completed\n");
		success("server: %s was negotiated\n", gnutls_sign_get_name(gnutls_sign_algorithm_get(session)));
	}

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

 end:
	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
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
	int fd[2];
	int ret;

	signal(SIGCHLD, ch_handler);

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
		close(fd[1]);
		client(fd[0]);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		server(fd[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
