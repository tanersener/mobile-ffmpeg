/*
 * Copyright (C) 2016 Red Hat, Inc.
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
#include <string.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>

#include "cert-common.h"
#include "utils.h"

/* Test for gnutls_certificate_set_key()
 *
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

static time_t mytime(time_t * t)
{
	time_t then = 1473674242;
	if (t)
		*t = then;

	return then;
}

static void auto_parse(void)
{
	gnutls_certificate_credentials_t x509_cred, clicred;
	gnutls_pcert_st pcert_list[16];
	gnutls_privkey_t key;
	gnutls_pcert_st second_pcert[2];
	gnutls_privkey_t second_key;
	unsigned pcert_list_size;
	int ret;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_time_function(mytime);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);
	assert(gnutls_privkey_init(&key)>=0);

	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));

	pcert_list_size = sizeof(pcert_list)/sizeof(pcert_list[0]);
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
		&server_ca3_localhost_cert_chain, GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("error in gnutls_pcert_list_import_x509_raw: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_privkey_import_x509_raw(key, &server_ca3_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0) {
		fail("error in key import: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_certificate_set_key(x509_cred, NULL, 0, pcert_list,
				pcert_list_size, key);
	if (ret < 0) {
		fail("error in gnutls_certificate_set_key: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	/* set the key with UTF8 names */
	assert(gnutls_privkey_init(&second_key)>=0);

	pcert_list_size = 2;
	ret = gnutls_pcert_list_import_x509_raw(second_pcert, &pcert_list_size,
		&server_ca3_localhost_utf8_cert, GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("error in gnutls_pcert_list_import_x509_raw: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_privkey_import_x509_raw(second_key, &server_ca3_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0) {
		fail("error in key import: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_certificate_set_key(x509_cred, NULL, 0, second_pcert,
				1, second_key);
	if (ret < 0) {
		fail("error in gnutls_certificate_set_key: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	test_cli_serv(x509_cred, clicred, "NORMAL", "localhost", NULL, NULL, NULL); /* the DNS name of the first cert */
#ifdef HAVE_LIBIDN /* IDNA2003-only */
	test_cli_serv(x509_cred, clicred, "NORMAL", "www.νίκος.com", NULL, NULL, NULL); /* the DNS name of second cert */
	test_cli_serv(x509_cred, clicred, "NORMAL", "raw:www.νίκος.com", NULL, NULL, NULL); /* the DNS name of second cert */
	test_cli_serv(x509_cred, clicred, "NORMAL", "www.xn--kxawhku.com", NULL, NULL, NULL); /* the previous name in IDNA format */
#endif
	test_cli_serv(x509_cred, clicred, "NORMAL", "简体中文.εξτρα.com", NULL, NULL, NULL); /* the second DNS name of cert */
	test_cli_serv(x509_cred, clicred, "NORMAL", "raw:简体中文.εξτρα.com", NULL, NULL, NULL); /* the second DNS name of cert */
	test_cli_serv(x509_cred, clicred, "NORMAL", "xn--fiqu1az03c18t.xn--mxah1amo.com", NULL, NULL, NULL); /* its IDNA equivalent */

	gnutls_certificate_free_credentials(x509_cred);
	gnutls_certificate_free_credentials(clicred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}

void doit(void)
{
#if !defined(HAVE_LIBIDN) && !defined(HAVE_LIBIDN2)
	exit(77);
#endif
	auto_parse();
}
