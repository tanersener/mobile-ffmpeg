/*
 * Copyright (C) 2014 Red Hat, Inc.
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

/* This program tests whether the import functions sort the 
 * chain.
 */

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static unsigned char server_cert_pem[] =
/* 0 */    "-----BEGIN CERTIFICATE-----\n"
    "MIIDIzCCAgugAwIBAgIMVHc8lDcqr/T62g5oMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
    "BgNVBAMTBENBLTIwIhgPMjAxNDExMjcxNTAwMzZaGA85OTk5MTIzMTIzNTk1OVow\n"
    "EzERMA8GA1UEAxMIc2VydmVyLTMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
    "AoIBAQCsfFrZmOxA7ijkppwPtakc8ecuBRr9Dr4pe/alC/OXVZsZlAlnR0xd53XF\n"
    "uUPwo9Ga2q7iY8+8yPRNs8gfl6IrHvUUtaukWdMlQq5nhRFaPgOzHOEZGGEUk3UF\n"
    "R/8lld6xQFoe7FvHwQ5cIkIl0cN/I4jiUb9fQhRwcBPjmQbCisYXUZDe8KtCnkjw\n"
    "ZZfOp7UclWPm+hv4G3cfeRUUis0Xf8sScjLAam7ojkGL9CeETXl1JGSqqmVN7svN\n"
    "yDsiQebCSrA4wCt+ENe9rE6Cme6dEv+U4lyx4oijn4sNvPwwgmu+/g6XjhE6IWBL\n"
    "kWXLJ1K4rixbqt3d3+H7IAFiX99bAgMBAAGjdzB1MAwGA1UdEwEB/wQCMAAwFAYD\n"
    "VR0RBA0wC4IJbG9jYWxob3N0MA8GA1UdDwEB/wQFAwMHoAAwHQYDVR0OBBYEFNt6\n"
    "DwawLeNaZ+5LMNBdeTVWZsmOMB8GA1UdIwQYMBaAFCjlkQq5yKVHzXPQLahHCcmS\n"
    "AJRpMA0GCSqGSIb3DQEBCwUAA4IBAQClbMnEQpHwwqcdrGKiNXQYyJDClVfQFTlh\n"
    "fTU2qUx8gfyP+1yR0lqsdremSzSjLPM6LmcJLAdu7GhL32Lc3068CCzDtd6vJDGf\n"
    "vO1eudcixbAf7NuELCZM08wLuJvKQFlNYFSVZSb04habhcwgowsiy0YC+dF9XQKa\n"
    "5YDGvOuMTqqKt5Wph+izCGQ+6WyRZQp2CIFWo0vBCYFaslaA/TBnsldIuACJFmg9\n"
    "kmspW97ROmNr1jfQNyBVWjd1EER80zZCngXq4+JnP1tppJNcYFhHeqSGQCqASehY\n"
    "CC7ITbKAK8IdwU4gVk7R92rOKyrFPimc1UwObNpxbL5jizZqemW7\n"
    "-----END CERTIFICATE-----\n"
/* 3 */    "-----BEGIN CERTIFICATE-----\n"
    "MIIC9TCCAd2gAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
    "MCIYDzIwMTQxMTI3MTUwMDM2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
    "BENBLTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC16A/jNGbd6oP3\n"
    "t/neq6hlWopjKEPnM9FMZgPSNVsKtQEb1dOx2EDCuP3rC2POogAjo0NuE/SZtM0N\n"
    "Nyf+X3QdjwcFdugMLTXGmGlEhCeWhSEjLwrd6eapdHzwpV0Ag22CvzoKEQenu92+\n"
    "TI1MN/1j3XOgnOP3t4q5TeSZn7XtAMCBqt9b+LJT5XJ/sF6b1sH803HqV3CZ6ga+\n"
    "kFY+uDcpImQEJNZi/B1xYObSHF+frg4SyeqjxiV9vmFHhRgLmD96iVukQTC/RPX3\n"
    "ntl0wGBjpmglUVdcAJdZL7L2um1T1n3u+jS3U5FW7+MOnnTGqRT2pcYtHHLg2GDf\n"
    "SSUpeuphAgMBAAGjWDBWMA8GA1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYB\n"
    "BQUHAwkwDwYDVR0PAQH/BAUDAwcGADAdBgNVHQ4EFgQU1aTwaphaQJeRY+IF9NNT\n"
    "mHHpIQ0wDQYJKoZIhvcNAQELBQADggEBAGUjVPu4aeel7UmgUvjBEpGbw6j9wKRL\n"
    "4vVgGllKWH0diISEjPcJv0dTDJs4ZbY7KAEc4DRCl1QwNFsuASP5BlMSrWo+eGiC\n"
    "oxsndY2EIpHAheLHXkVwbOwM5VRN2IhlcmVtHM370luvJjNa1MXy1p1/VEjGS794\n"
    "FgtMOm9yILCM8WqwRHOY/mAOu/9iY/Zfqfobm+IfqgBmQMOLAIMKJffh15meTDRi\n"
    "W3QXdf/khr1T3JEJ55t1WxcC1cWV4FnecUU4wlKs1mBghV+/8cgbYjoIdUAsYsdv\n"
    "SjySP3B65XXw9G3MmHOjNoRpF7Oeea8tN+zxw3xFx/a9Uq19BdOlrHE=\n"
    "-----END CERTIFICATE-----\n"
/* 1 */    "-----BEGIN CERTIFICATE-----\n"
    "MIIDFjCCAf6gAwIBAgIBAjANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0x\n"
    "MCIYDzIwMTQxMTI3MTUwMDM2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
    "BENBLTIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDOnFo6ntaysv14\n"
    "ewFwkm+mbE/8hEiZEcMNnWNSJb9tgpATLgu9LefStzRIvzns4OyLL77TEz8Gl8gJ\n"
    "syfba2aIKxLveO1jpqQSfkcVlufa/GHspPKMkHMjz1UB+fQEAazAjVKHoofemKxW\n"
    "0TtLeuL0LoE4g452Yy60vxRNwOs7WPZ5lktIQTYZTYcEjiiVlrRXXGgo9qCSfG4n\n"
    "B+TmlraGHHPlKINcsOJnZOOZ6qHx+ZpqeCvuD7apiPcVzfLhxdJFoznY4r/bdCZT\n"
    "ehChrKCYk5DmaPRBW0TLWoYrny745SG9U8XzTkKYaCDLhyMvn1oMrRVdbwO3/e6q\n"
    "DbEvbpUrAgMBAAGjeTB3MA8GA1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYB\n"
    "BQUHAwkwDwYDVR0PAQH/BAUDAwcEADAdBgNVHQ4EFgQUKOWRCrnIpUfNc9AtqEcJ\n"
    "yZIAlGkwHwYDVR0jBBgwFoAUarsjXomEecxGrV0LDBX8Dy+vh68wDQYJKoZIhvcN\n"
    "AQELBQADggEBAKgo2SIyLywamhcqLnhxCXgx0SHJgmEVD7CvPgTISZisg5yMS77G\n"
    "WqtHbyo7kOYIjbrzVRGOsijKmgCgqNTQXSMbWUfDOV93q82nV0bjQtnvZKMc0+OM\n"
    "/cB5PA7BFKvVrpYGefFQtrgkFhHSoUwDtpJAdYJPWgUMiqpvDuQdD/d6FQ18rb7w\n"
    "QuIIvUeHaawm8HLrJ5JZoy7BnryY4SEFqGSTeNWp4CyeTeQPAcCdZ3NlnSDV1RM2\n"
    "QelcD8S6GAp8l8LcF1zqiaoqWVYdeVnO6Doabx/IP7ZxctcdaEAdUQYjJ/dG3A2p\n"
    "wpf3tVoOBKFByhdBrz7uda09sq57+AmvQdk=\n"
    "-----END CERTIFICATE-----\n"
/* 2 */    "-----BEGIN CERTIFICATE-----\n"
    "MIIDFjCCAf6gAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
    "MCIYDzIwMTQxMTI3MTUwMDM2WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
    "BENBLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDft9fjnipXU+WO\n"
    "NsIhqVgTkwQPWklvuJbAphYa0oCm/+S0dvalVEq9RMqV+sUtqrZ55LsHxvtD1iu9\n"
    "03kf/FcqaAjSVZBt6n8JIfl4xyi//FYizamm2KEsBCEsUCH6iJGMGXfYAWgpMJ/6\n"
    "yHwikBDI0Ea5ckIW58eWHI6Hmd11DTSy6OGNnOFqyEe3S/m1zTtNNGiA0VcSyAjg\n"
    "98zaWGQHaQuqczqfoMz0dB5ly0mw2LfVxCPM8Z8xH1S9TNVqWnKu483Gp+2TkeKl\n"
    "bJ5dI1XMihaxFq6xf9OsULGtMd8biRNxl8f8zsfd0A9LoPJWKdp345OJ33ULwogI\n"
    "M6kUMw63AgMBAAGjeTB3MA8GA1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYB\n"
    "BQUHAwkwDwYDVR0PAQH/BAUDAwcEADAdBgNVHQ4EFgQUarsjXomEecxGrV0LDBX8\n"
    "Dy+vh68wHwYDVR0jBBgwFoAU1aTwaphaQJeRY+IF9NNTmHHpIQ0wDQYJKoZIhvcN\n"
    "AQELBQADggEBALGbNfhgr46cnDIbvPxXmNmMm840oVc9n5pW4be9emTWO67zkqll\n"
    "KBjLbEAZTVSsjqPh8357iR5nVAen23eVYD5eGkuDZZAP3kvfVNVNCTQAEm0XDAse\n"
    "kxbxL0ZWezMbC/U8R3tFSDZOCb/bM+wCKg1hX5My0+utKAmhbwlYQY9fKyhZCUdv\n"
    "GnO3f5JInJDH2FmG80RouZ8Av6CjOwfChz+SPTgrMsbTugYWX9SVQ8oRF+N7cudC\n"
    "7XlvScNQKlbzmMl2zLQOrL78djCLVdU70bZcpq1o7L/R59YNAB+4fGH8rTWZMYQB\n"
    "rSoCPlyNAYAqMPXPsUFV/ngeYNSbpTz3SGA=\n"
    "-----END CERTIFICATE-----\n"
    "\n";

const gnutls_datum_t server_cert = { 
	server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEowIBAAKCAQEArHxa2ZjsQO4o5KacD7WpHPHnLgUa/Q6+KXv2pQvzl1WbGZQJ\n"
    "Z0dMXed1xblD8KPRmtqu4mPPvMj0TbPIH5eiKx71FLWrpFnTJUKuZ4URWj4Dsxzh\n"
    "GRhhFJN1BUf/JZXesUBaHuxbx8EOXCJCJdHDfyOI4lG/X0IUcHAT45kGworGF1GQ\n"
    "3vCrQp5I8GWXzqe1HJVj5vob+Bt3H3kVFIrNF3/LEnIywGpu6I5Bi/QnhE15dSRk\n"
    "qqplTe7Lzcg7IkHmwkqwOMArfhDXvaxOgpnunRL/lOJcseKIo5+LDbz8MIJrvv4O\n"
    "l44ROiFgS5FlyydSuK4sW6rd3d/h+yABYl/fWwIDAQABAoIBAQCL0vc25C/I5wfB\n"
    "a4qhdYsVCsh0VvEs6TGgoXwtCYY7TMtBre79iR/QE902HtyDi9lT5ijVH0J88I6T\n"
    "GsWFTr/Iovzb//WXcrWmw+prwsRxWkpXfXbAiDHSo0K+uEGOr3JqUBd+b+5q/QZu\n"
    "C9uBmw0W2LCTft9bEk9NYp3M5/VB6DaQbk//b7E9KFc7nFgzeQaSYHu9NBSLGZ2e\n"
    "HqvzotiwlI6yfWTPm/esipXWaB4zqesx0TedoNK9SUAFdFBEHTyqm5RoGotjNLoM\n"
    "bN08Fj3qOJekjPGBrMu37UKoRGdaTyPlmCGZ0+HN2F4kuaUGE8HHnUU3VIA3lTMh\n"
    "LGt8jYpxAoGBAMsr8XlLsGFUgntHbCe5GhNKd9RJtRH1+zNw88ilfjttpxjggcL7\n"
    "KGbcCK1VOhuD0Ud1pTklYFOUckZY6y1b4nUkp5SG4w8OiIcIZeE9erKwprnHa9RF\n"
    "cewMtYhJ68evPrbM9UHEkTbdNBI4Cv561cY80pnsMTxy9al/aM23SLIJAoGBANlV\n"
    "0J/lUuA4Lsvrtu/IriwUguMIBw7hC5gBIU58K9Xpo6fr55VTt6OALDrY5zbCPf38\n"
    "pGMZgPsP3FG61BycA7jWB01Y++3COYKNKQtddWuY0SqCVS7Mdt6DwpYwUD7gRDY3\n"
    "aIHMUP45glYEVnHgpwNM09f+ldiK4TnCJuKYRM9DAoGAYM3NPlf78EQN76M2Oy8M\n"
    "54gh1DpSVf539CirXzzLCpHSfh3qdfapZ2kLkVr8VsPV4VCCqtnOLcSbNj2DwJb5\n"
    "LYuLdU9XvILWNlCgClP6tE1LA1WrYPa9sxTTId7mwrwTC5JYgT+hWRzIhK3DP0FT\n"
    "viKYzdImG4FC38HfM7VSo9ECgYBiP+wnTKlxmZR2NWIm9ibe4IrnDYr7S/tMxT4E\n"
    "WBgNBSkp0XiIxibfcCMOm12zII6b0mmSL0ZiuSHVhMs8/76jAYadjdud+U68WQo0\n"
    "DBT4BkaQnAjcNiyKnTALa13rfsD3bYb+HpqCwwbL0fwuUOvPjxy5qWqeUPJOhRnF\n"
    "GCcLNwKBgHtDlVG5lJqtNty4aL9oBgcP0VcY/73Dx+l25DhprdlTHsjg+ue0rpjA\n"
    "ieq7o2hENu6MA1AQ8o+BP6SlRuhYmvzh7vVbs3qFjnslaMCveHZDITN/0NJqF9xO\n"
    "IeKrLzOIboyQw/sMSrPIPYILgXP0YnueteOgPUSZEcrqPIJI08Sb\n"
    "-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};


/* A very basic TLS client, with anonymous authentication.
 */

#define MAX_BUF 1024

static void client(int fd)
{
	int ret;
	const char *p;
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
	ret =
	    gnutls_priority_set_direct(session,
					"NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA",
					&p);
	if (ret < 0) {
		fail("error in setting priority: %s\n", p);
		exit(1);
	}

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

	if (ret < 0) {
		if (debug) {
			fail("client: Handshake failed\n");
			gnutls_perror(ret);
		}
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


/* These are global */
pid_t child;

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

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA",
				   NULL);

	if (debug) {
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	ret = gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("server: gnutls_certificate_set_x509_key_mem: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);


	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));
	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

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
		/* parent */
		close(fd[0]);
		client(fd[1]);
		waitpid(-1, NULL, 0);
		//kill(child, SIGTERM);
	} else {
		close(fd[1]);
		server(fd[0]);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	int status = 0;

	waitpid(-1, &status, 0);
	check_wait_status(status);
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	start();
}

#endif				/* _WIN32 */
