/*
 * Copyright (C) 2016 Tim Kosse
 *
 * Author: Tim Kosse
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
#include <time.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>

#include "utils.h"

/* This program tests that handshakes fail if the server does not include the
 * requested certificate status with the server certificate having
 * TLS feature 5 (status request).
 *
 * See RFC 7633 section 4.2.3.1 paragraph 1
 *
 * Remark: Doesn't the MUST in section 4.3.3 para. 1 overrule the SHOULD of 4.2.3.1 para. 1?
 */

static time_t mytime(time_t * t)
{
	time_t then = 1464610242;
	if (t)
		*t = then;

	return then;
}

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
		"MIICBzCCAXCgAwIBAgIMVpjt8TL5Io/frpvkMA0GCSqGSIb3DQEBCwUAMCIxIDAe\n"
		"BgNVBAMTF0dudVRMUyB0ZXN0IGNlcnRpZmljYXRlMB4XDTE2MDExNTEzMDI0MVoX\n"
		"DTMyMDYxOTEzMDI0MVowIjEgMB4GA1UEAxMXR251VExTIHRlc3QgY2VydGlmaWNh\n"
		"dGUwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBANe6XK+jDPAuqSdWqlOOqOt/\n"
		"gXVMa5i+Suq3HvhLw2rE2g0AuOpMEx82FpPecu/GpF6ybrbKCohVdZCW7aZXvAw7\n"
		"dg2XHr3p7H/Tqez7hWSga6BIznd+c5wxE/89yK6lYG7Ztoxamm+2vp9qvafwoDMn\n"
		"9bcdkuWWnHNS1p/WyI6xAgMBAAGjQjBAMBEGCCsGAQUFBwEYBAUwAwIBBTAMBgNV\n"
		"HRMBAf8EAjAAMB0GA1UdDgQWBBRTSzvcXshETAIgvzlIb0z+zSVSEDANBgkqhkiG\n"
		"9w0BAQsFAAOBgQB+VcJuLPL2PMog0HZ8RRbqVvLU5d209ROg3s1oXUBFW8+AV+71\n"
		"CsHg9Xx7vqKVwyKGI9ghds1B44lNPxGH2Sk1v2czjKbzwujo9+kLnDS6i0jyrDdn\n"
		"um4ivpkwmlUFSQVXvENLwe9gTlIgN4+0I9WLcMTCDtHWkcxMRwCm2BMsXw==\n"
		"-----END CERTIFICATE-----\n";


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

static int received = 0;

static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	fail("received status request\n");
	received = 1;
	return 0;
}

#define MAX_BUF 1024

static void client(int fd, const char *prio)
{
	int ret;
	unsigned int status;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	gnutls_global_set_time_function(mytime);
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
	gnutls_priority_set_direct(session, prio, NULL);

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CERTIFICATE_STATUS,
					   GNUTLS_HOOK_POST,
					   handshake_callback);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

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
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	if (received == 1) {
		fail("client: received certificate status when we shouldn't.\n");
	}

	ret = gnutls_certificate_verify_peers2(session, &status);
	if (ret != GNUTLS_E_SUCCESS) {
		fail("client: Peer certificate validation failed: %s\n", gnutls_strerror(ret));
	}
	else {
		if (status & GNUTLS_CERT_MISSING_OCSP_STATUS) {
			success("client: Validation failed with GNUTLS_CERT_MISSING_OCSP_STATUS\n");
		}
		else {
			fail("client: Validation status does not include GNUTLS_CERT_MISSING_OCSP_STATUS. Status is %d\n", status);
		}
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

      end:

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


static void server(int fd, const char *prio)
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
	gnutls_priority_set_direct(session, prio, NULL);

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
	return;
}

static void start(const char *name, const char *prio)
{
	pid_t child;
	int fd[2];
	int ret, status = 0;

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	received = 0;
	success("running: %s\n", name);

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
		client(fd[0], prio);
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		close(fd[0]);
		server(fd[1], prio);
		exit(0);
	}

	return;
}

void doit(void)
{
	start("tls1.2", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2");
	start("tls1.3", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3");
	start("default", "NORMAL");
}

#endif				/* _WIN32 */
