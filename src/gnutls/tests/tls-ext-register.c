/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * Author: Thierry Quemerais
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

/* A very basic TLS client, with extension
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define TLSEXT_TYPE_SAMPLE						0xF1

static int TLSEXT_TYPE_client_sent			= 0;
static int TLSEXT_TYPE_client_received		= 0;
static int TLSEXT_TYPE_server_sent			= 0;
static int TLSEXT_TYPE_server_received		= 0;

static const unsigned char ext_data[] =
{
	0xFE,
	0xED
};

static int ext_recv_client_params(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	if (buflen != sizeof(ext_data))
		fail("ext_recv_client_params: Invalid input buffer length\n");

	if (memcmp(buf, ext_data, sizeof(ext_data)) != 0)
		fail("ext_recv_client_params: Invalid input buffer data\n");

	TLSEXT_TYPE_client_received = 1;

	gnutls_ext_set_data(session, TLSEXT_TYPE_SAMPLE, session);

	return 0; //Success
}

static int ext_send_client_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	TLSEXT_TYPE_client_sent = 1;
	gnutls_buffer_append_data(extdata, ext_data, sizeof(ext_data));
	return sizeof(ext_data);
}

static int ext_recv_server_params(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	if (buflen != sizeof(ext_data))
		fail("ext_recv_server_params: Invalid input buffer length\n");

	if (memcmp(buf, ext_data, sizeof(ext_data)) != 0)
		fail("ext_recv_server_params: Invalid input buffer data\n");

	TLSEXT_TYPE_server_received = 1;

	return 0; //Success
}

static int ext_send_server_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	TLSEXT_TYPE_server_sent = 1;
	gnutls_buffer_append_data(extdata, ext_data, sizeof(ext_data));
	return sizeof(ext_data);
}

static void client(int sd, const char *prio)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;
	void *p;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	/* extensions are registered globally */
	ret = gnutls_ext_register("ext_client", TLSEXT_TYPE_SAMPLE, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL);
	assert(ret >= 0);

	gnutls_certificate_allocate_credentials(&clientx509cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (TLSEXT_TYPE_client_sent != 1 || TLSEXT_TYPE_client_received != 1)
		fail("client: extension not properly sent/received\n");

	ret = gnutls_ext_get_data(session, TLSEXT_TYPE_SAMPLE, &p);
	if (ret < 0) {
		fail("gnutls_ext_get_data: %s\n", gnutls_strerror(ret));
	}

	if (p != session) {
		fail("client: gnutls_ext_get_data failed\n");
	}

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

end:
	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);
}

/* This is a sample TLS 1.0 server, for extension
 */

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


static void server(int sd, const char *prio)
{
	gnutls_certificate_credentials_t serverx509cred;
	int ret;
	gnutls_session_t session;

	/* this must be called once in the program
	 */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER);

	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	assert(gnutls_ext_register("ext_server", TLSEXT_TYPE_SAMPLE, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL)>=0);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_handshake(session);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (TLSEXT_TYPE_server_sent != 1 || TLSEXT_TYPE_server_received != 1)
		fail("server: extension not properly sent/received\n");

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	if (debug)
		success("server: finished\n");
}

static
void start(const char *prio)
{
	pid_t child1, child2;
	int sockets[2];
	int err;

	success("trying %s\n", prio);

	signal(SIGPIPE, SIG_IGN);
	TLSEXT_TYPE_client_sent			= 0;
	TLSEXT_TYPE_client_received		= 0;
	TLSEXT_TYPE_server_sent			= 0;
	TLSEXT_TYPE_server_received		= 0;

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		perror("socketpair");
		fail("socketpair failed\n");
		return;
	}

	child1 = fork();
	if (child1 < 0) {
		perror("fork");
		fail("fork");
	}

	if (child1) {
		int status;
		/* parent */
		close(sockets[1]);

		child2 = fork();
		if (child2 < 0) {
			perror("fork");
			fail("fork");
		}

		if (child2) {
			waitpid(child1, &status, 0);
			check_wait_status(status);

			waitpid(child2, &status, 0);
			check_wait_status(status);
		} else {
			server(sockets[0], prio);
			exit(0);
		}
	} else {
		close(sockets[0]);
		client(sockets[1], prio);
		exit(0);
	}
}

void doit(void)
{
	int ret;
	unsigned i;

	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");

	/* check whether we can crash the library by adding many extensions */
	for (i=0;i<64;i++) {
		ret = gnutls_ext_register("ext_serverxx", TLSEXT_TYPE_SAMPLE+i+1, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL);
		if (ret < 0) {
			success("failed registering extension no %d (expected)\n", i+1);
			break;
		}
	}
}

#endif				/* _WIN32 */
