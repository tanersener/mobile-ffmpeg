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

#if defined(_WIN32) || !defined(ENABLE_SRP)

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

#include "utils.h"

static void terminate(void);

/* This program tests the SRP and SRP-RSA ciphersuites.
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


static void client(int fd, const char *prio)
{
	int ret;
	gnutls_session_t session;
	gnutls_srp_client_credentials_t srp_cred;
	gnutls_certificate_credentials_t x509_cred;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_srp_allocate_client_credentials(&srp_cred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	gnutls_srp_set_client_credentials(srp_cred, "test", "test");

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, prio, NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_SRP, srp_cred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_srp_free_client_credentials(srp_cred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


/* These are global */
gnutls_srp_server_credentials_t s_srp_cred;
gnutls_certificate_credentials_t s_x509_cred;
pid_t child;

static gnutls_session_t initialize_tls_session(const char *prio)
{
	gnutls_session_t session;

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, prio, NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_SRP, s_srp_cred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				s_x509_cred);

	return session;
}

static void terminate(void)
{
	int status;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void server(int fd, const char *prio)
{
	int ret;
	gnutls_session_t session;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_srp_allocate_server_credentials(&s_srp_cred);
	gnutls_srp_set_server_credentials_file(s_srp_cred, "tpasswd",
						"tpasswd.conf");

	gnutls_certificate_allocate_credentials(&s_x509_cred);
	gnutls_certificate_set_x509_key_mem(s_x509_cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	session = initialize_tls_session(prio);

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

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_srp_free_server_credentials(s_srp_cred);
	gnutls_certificate_free_credentials(s_x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *prio)
{
	int fd[2];
	int ret;

	ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
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
		server(fd[0], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		client(fd[1], prio);
		exit(0);
	}
}

const char *tpasswd_file =
    "test:CsrY0PxYlYCAa8UuWUrcjpqBvG6ImlAdGwEUh3tN2DSDBbMWTvnUl7A8Hw7l0zFHwyLH5rh0llrmu/v.Df2FjDEGy0s0rYR5ARE2XlXPl66xhevHj5vitD0Qvq/J0x1v0zMWJSgq/Ah2MoOrw9aBEsQUgf9MddiHQKjE3Vetoq3:3h3cfS0WrBgPUsldDASSK0:1\n"
    "test2:1J14yVX4iBa97cySs2/SduwnSbHxiz7WieE761psJQDxkc5flpumEwXbAgK5PrSZ0aZ6q7zyrAN1apJR1QQPAdyScJ6Jw4zjDP7AnezUVGbUNMJXhsI0NPwSc0c/415XfrnM1139yjWCr1qkcYMoN4bALppMMLB8glJkxy7t.3cmH9MkRRAjXXdUgAvHw2ZFLmB/8TlZDhnDS78xCSgLQs.oubZEEIgOWl7BT2.aW76fW3yKWdVrrHQDYPtR4hKx:11rUG9wSMLHe2Cu2p7dmFY:2\n"
    "test3:LVJZDDuElMHuRt5/fcx64AhJ4erhFvbIhv/XCtD0tJI3OC6yEBzthZ1FSqblri9qtsvboPApbFHwP9WEluGtCOuzOON4LS8sSeQDBO.PaqjTnsmXKPYMKa.SuLXFuRTtdiFRwX2ZRy3GIWoCvxJtPDWCEYGBWfnjjGEYmQWvo534JVtVDyMaFItYlMTOtBSgsg488oJ5hIAU6jVyIQZGPVv8OHsPCpEt2UlTixzI9nAgQ0WL5ShKaAq0dksF/AY7UMKm0oHbtZeqAx6YcBzLbBhNvcEqYzH95ONpr.cUh91iRhVzdVscsFweSCtWsQrVT4zmSRwdsljeFQPqFbdeK:iWkELSVg3JxmyEq.XbjAW:3\n";

const char *tpasswd_conf_file =
    "1:Ewl2hcjiutMd3Fu2lgFnUXWSc67TVyy2vwYCKoS9MLsrdJVT9RgWTCuEqWJrfB6uE3LsE9GkOlaZabS7M29sj5TnzUqOLJMjiwEzArfiLr9WbMRANlF68N5AVLcPWvNx6Zjl3m5Scp0BzJBz9TkgfhzKJZ.WtP3Mv/67I/0wmRZ:2\n"
    "2:dUyyhxav9tgnyIg65wHxkzkb7VIPh4o0lkwfOKiPp4rVJrzLRYVBtb76gKlaO7ef5LYGEw3G.4E0jbMxcYBetDy2YdpiP/3GWJInoBbvYHIRO9uBuxgsFKTKWu7RnR7yTau/IrFTdQ4LY/q.AvoCzMxV0PKvD9Odso/LFIItn8PbTov3VMn/ZEH2SqhtpBUkWtmcIkEflhX/YY/fkBKfBbe27/zUaKUUZEUYZ2H2nlCL60.JIPeZJSzsu/xHDVcx:2\n"
    "3:2iQzj1CagQc/5ctbuJYLWlhtAsPHc7xWVyCPAKFRLWKADpASkqe9djWPFWTNTdeJtL8nAhImCn3Sr/IAdQ1FrGw0WvQUstPx3FO9KNcXOwisOQ1VlL.gheAHYfbYyBaxXL.NcJx9TUwgWDT0hRzFzqSrdGGTN3FgSTA1v4QnHtEygNj3eZ.u0MThqWUaDiP87nqha7XnT66bkTCkQ8.7T8L4KZjIImrNrUftedTTBi.WCi.zlrBxDuOM0da0JbUkQlXqvp0yvJAPpC11nxmmZOAbQOywZGmu9nhZNuwTlxjfIro0FOdthaDTuZRL9VL7MRPUDo/DQEyW.d4H.UIlzp:2\n";

void doit(void)
{
	FILE *fd;

	fd = fopen("tpasswd.conf", "w");
	if (fd == NULL)
		exit(1);

	fwrite(tpasswd_conf_file, 1, strlen(tpasswd_conf_file), fd);
	fclose(fd);

	fd = fopen("tpasswd", "w");
	if (fd == NULL)
		exit(1);

	fwrite(tpasswd_file, 1, strlen(tpasswd_file), fd);
	fclose(fd);

	start("NORMAL:-KX-ALL:+SRP");
	start("NORMAL:-KX-ALL:+SRP-RSA");

	remove("tpasswd");
	remove("tpasswd.conf");
}

#endif				/* _WIN32 */
