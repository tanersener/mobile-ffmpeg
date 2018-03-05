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
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>

#include "utils.h"

/* A very basic TLS client, with supplemental data
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define TLS_SUPPLEMENTALDATATYPE_SAMPLE						0xBABE

static int TLS_SUPPLEMENTALDATA_client_sent			= 0;
static int TLS_SUPPLEMENTALDATA_client_received		= 0;
static int TLS_SUPPLEMENTALDATA_server_sent			= 0;
static int TLS_SUPPLEMENTALDATA_server_received		= 0;

static const unsigned char supp_data[] =
{	
	0xFE,
	0xED
};

static
int supp_client_recv_func(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	TLS_SUPPLEMENTALDATA_client_received = 1;

	if (buflen != sizeof(supp_data))
		fail("supp_client_recv_func: Invalid input buffer len\n");

	if (memcmp(buf, supp_data, sizeof(supp_data)) != 0)
		fail("supp_client_recv_func: Invalid input buffer data\n");

	return GNUTLS_E_SUCCESS;
}

static
int supp_client_send_func(gnutls_session_t session, gnutls_buffer_t buf)
{
	TLS_SUPPLEMENTALDATA_client_sent = 1;
	gnutls_buffer_append_data(buf, supp_data, sizeof(supp_data));
	return GNUTLS_E_SUCCESS;
}

static
int supp_server_recv_func(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	TLS_SUPPLEMENTALDATA_server_received = 1;

	if (buflen != sizeof(supp_data))
		fail("supp_server_recv_func: Invalid input buffer len\n");

	if (memcmp(buf, supp_data, sizeof(supp_data)) != 0)
		fail("supp_server_recv_func: Invalid input buffer data\n");

	return GNUTLS_E_SUCCESS;
}

static
int supp_server_send_func(gnutls_session_t session, gnutls_buffer_t buf)
{
	TLS_SUPPLEMENTALDATA_server_sent = 1;
	gnutls_buffer_append_data(buf, supp_data, sizeof(supp_data));
	return GNUTLS_E_SUCCESS;
}

static void client(int sd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	gnutls_certificate_allocate_credentials(&clientx509cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "PERFORMANCE:+ANON-ECDH:+ANON-DH",
				   NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);

	gnutls_supplemental_recv(session, 1);
	gnutls_supplemental_send(session, 1);

	gnutls_supplemental_register("supplemental_client", TLS_SUPPLEMENTALDATATYPE_SAMPLE, supp_client_recv_func, supp_client_send_func);

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

	if (TLS_SUPPLEMENTALDATA_client_sent != 1 || TLS_SUPPLEMENTALDATA_client_received != 1)
		fail("client: extension not properly sent/received\n");

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

end:
	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
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

int err, ret;
char topbuf[512];
gnutls_session_t session;
int optval = 1;

static void server(int sd)
{
	gnutls_certificate_credentials_t serverx509cred;

	/* this must be called once in the program
	 */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "PERFORMANCE:+ANON-ECDH:+ANON-DH",
				   NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_supplemental_recv(session, 1);
	gnutls_supplemental_send(session, 1);

	gnutls_supplemental_register("supplemental_server", TLS_SUPPLEMENTALDATATYPE_SAMPLE, supp_server_recv_func, supp_server_send_func);

	gnutls_transport_set_int(session, sd);
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

	if (TLS_SUPPLEMENTALDATA_server_sent != 1 || TLS_SUPPLEMENTALDATA_server_received != 1)
		fail("server: extension not properly sent/received\n");

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	pid_t child;
	int sockets[2];

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
		int status;
		/* parent */
		server(sockets[0]);
		wait(&status);
	} else
		client(sockets[1]);
}

#endif				/* _WIN32 */
