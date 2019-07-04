/*
 * Copyright (C) 2017-2019 Red Hat, Inc.
 *
 * Author: Daiki Ueno
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
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
#include <assert.h>

#include "cert-common.h"
#include "tls13/ext-parse.h"
#include "pkcs11/softhsm.h"
#include "utils.h"

/* This program tests whether the Post Handshake Auth extension is
 * present in the client hello, and whether it is missing from server
 * hello. In addition it contains basic functionality test for
 * post handshake authentication.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

#define MAX_BUF 1024

#define P11LIB "libpkcs11mock2.so"

#define PIN "1234"

#define CONFIG_NAME "softhsm-post-handshake-with-cert-pkcs11"
#define CONFIG CONFIG_NAME".config"

static
int pin_func(void *userdata, int attempt, const char *url, const char *label,
	     unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

static void client(int fd, int err)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	gnutls_x509_crt_t crt;
	gnutls_x509_privkey_t key;
	gnutls_datum_t tmp;
	const char *lib;
	char buffer[MAX_BUF + 1];

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	/* point to SoftHSM token that libpkcs11mock2.so internally uses */
	setenv(SOFTHSM_ENV, CONFIG, 1);

	gnutls_pkcs11_set_pin_function(pin_func, NULL);

	lib = getenv("P11MOCKLIB2");
	if (lib == NULL)
		lib = P11LIB;

	ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_add_provider(lib, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_import(crt, &cli_ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_import: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (debug) {
		gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_ONELINE, &tmp);

		printf("\tCertificate: %.*s\n", tmp.size, tmp.data);
		gnutls_free(tmp.data);
	}

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_import(key, &cli_ca3_key, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_import: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	/* initialize softhsm token */
	ret = gnutls_pkcs11_token_init(SOFTHSM_URL, PIN, "test");
	if (ret < 0) {
		fail("gnutls_pkcs11_token_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_pkcs11_token_set_pin(SOFTHSM_URL, NULL, PIN,
					GNUTLS_PIN_USER);
	if (ret < 0) {
		fail("gnutls_pkcs11_token_set_pin: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_copy_x509_crt(SOFTHSM_URL, crt, "cert",
					  GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE |
					  GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_crt: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_pkcs11_copy_x509_privkey(SOFTHSM_URL, key, "cert",
					    GNUTLS_KEY_DIGITAL_SIGNATURE |
					    GNUTLS_KEY_KEY_ENCIPHERMENT,
					    GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE
					    |
					    GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE
					    | GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_privkey: %s\n",
		     gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_privkey_deinit(key);

	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);

	/* Initialize TLS session
	 */
	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_POST_HANDSHAKE_AUTH|GNUTLS_AUTO_REAUTH)>=0);

	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3:-SIGN-RSA-SHA256", NULL);
	if (ret < 0)
		fail("cannot set TLS 1.3 priorities\n");


	assert(gnutls_certificate_set_x509_key_file(x509_cred,
						    SOFTHSM_URL
						    ";object=cert;object-type=cert",
						    SOFTHSM_URL
						    ";object=cert;object-type=private;pin-value="
						    PIN,
						    GNUTLS_X509_FMT_DER)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret != 0)
		fail("handshake failed: %s\n", gnutls_strerror(ret));

	if (debug)
		success("client handshake completed\n");

	gnutls_record_set_timeout(session, 20 * 1000);

	if (debug)
		success("waiting for auth\n");

	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (err) {
		if (ret != err)
			fail("client: expected error %s, got: %s\n", gnutls_strerror(err),
			     gnutls_strerror(ret));
	} else if (ret < 0)
		fail("client: gnutls_record_recv did not succeed as expected: %s\n", gnutls_strerror(ret));

	do {
		ret = gnutls_bye(session, GNUTLS_SHUT_WR);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

static unsigned client_hello_ok = 0;
static unsigned server_hello_ok = 0;

#define TLS_EXT_POST_HANDSHAKE 49

static void parse_ext(void *priv, gnutls_datum_t *msg)
{
	if (msg->size != 0) {
		fail("error in extension length: %d\n", (int)msg->size);
	}
}

static int hellos_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	if (htype == GNUTLS_HANDSHAKE_SERVER_HELLO && post == GNUTLS_HOOK_POST) {
		if (find_server_extension(msg, TLS_EXT_POST_HANDSHAKE, NULL, NULL)) {
			fail("Post handshake extension seen in server hello!\n");
		}
		server_hello_ok = 1;

		return GNUTLS_E_INTERRUPTED;
	}

	if (htype != GNUTLS_HANDSHAKE_CLIENT_HELLO || post != GNUTLS_HOOK_PRE)
		return 0;

	if (find_client_extension(msg, TLS_EXT_POST_HANDSHAKE, NULL, parse_ext))
		client_hello_ok = 1;
	else
		fail("Post handshake extension NOT seen in client hello!\n");

	return 0;
}

static void server(int fd, int err, int type)
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
		gnutls_global_set_log_level(6);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER|GNUTLS_POST_HANDSHAKE_AUTH);

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH,
					   hellos_callback);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret != 0)
		fail("handshake failed: %s\n", gnutls_strerror(ret));

	if (!(gnutls_session_get_flags(session) & GNUTLS_SFLAGS_POST_HANDSHAKE_AUTH)) {
		fail("server: session flags did not contain GNUTLS_SFLAGS_POST_HANDSHAKE_AUTH\n");
	}


	if (client_hello_ok == 0) {
		fail("server: did not verify the client hello\n");
	}

	if (server_hello_ok == 0) {
		fail("server: did not verify the server hello contents\n");
	}

	if (debug)
		success("server handshake completed\n");

	gnutls_certificate_server_set_request(session, type);

	/* ask peer for re-authentication */
	do {
		ret = gnutls_reauth(session, 0);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (err) {
		if (ret != err)
			fail("server: expected error %s, got: %s\n", gnutls_strerror(err),
			     gnutls_strerror(ret));
	} else if (ret != 0)
		fail("server: gnutls_reauth did not succeed as expected: %s\n", gnutls_strerror(ret));

	do {
		ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: client/server hello were verified\n");
}

static
void start(const char *name, int err, int cli_err, int type)
{
	int fd[2];
	int ret;
	pid_t child;
	int status = 0;

	success("testing %s\n", name);

	client_hello_ok = 0;
	server_hello_ok = 0;

	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

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
		server(fd[0], err, type);
		kill(child, SIGTERM);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], cli_err);
		exit(0);
	}

}

void doit(void)
{
	const char *bin;
	char buf[128];

	/* check if softhsm module is loadable */
	(void) softhsm_lib();

	/* initialize SoftHSM token that libpkcs11mock2.so internally uses */
	bin = softhsm_bin();

	set_softhsm_conf(CONFIG);
	snprintf(buf, sizeof(buf),
		 "%s --init-token --slot 0 --label test --so-pin " PIN " --pin "
		 PIN, bin);
	system(buf);

	start("reauth-require", GNUTLS_E_CERTIFICATE_REQUIRED, GNUTLS_E_SUCCESS, GNUTLS_CERT_REQUIRE);
	start("reauth-request", 0, GNUTLS_E_SUCCESS, GNUTLS_CERT_REQUEST);
}
#endif				/* _WIN32 */
