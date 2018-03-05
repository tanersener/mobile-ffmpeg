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
#define MIN(x,y) (((x)<(y))?(x):(y))

/* Test for gnutls_certificate_set_x509_key()
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

static void compare(const gnutls_datum_t *der, const void *ipem)
{
	gnutls_datum_t pem = {(void*)ipem, strlen((char*)ipem)};
	gnutls_datum_t new_der;
	int ret;

	ret = gnutls_pem_base64_decode2("CERTIFICATE", &pem, &new_der);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
	}

	if (der->size != new_der.size || memcmp(der->data, new_der.data, der->size) != 0) {
		fail("error in %d: %s\n", __LINE__, "cert don't match");
		exit(1);
	}
	gnutls_free(new_der.data);
	return;
}

static int import_key(gnutls_certificate_credentials_t xcred, const gnutls_datum_t *skey, const gnutls_datum_t *cert)
{
	gnutls_x509_privkey_t key;
	gnutls_x509_crt_t *crt_list;
	unsigned crt_list_size, idx, i;
	gnutls_datum_t tcert;
	int ret;

	assert(gnutls_x509_privkey_init(&key)>=0);

	ret = gnutls_x509_crt_list_import2(&crt_list, &crt_list_size, cert, GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		fail("error in gnutls_x509_crt_list_import2: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_x509_privkey_import(key, skey, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in key import: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_certificate_set_x509_key(xcred, crt_list,
				crt_list_size, key);
	if (ret < 0) {
		success("error in gnutls_certificate_set_x509_key: %s\n", gnutls_strerror(ret));
		idx = ret;
		goto cleanup;
	}

	/* return index */
	idx = ret;

	/* verify whether the stored certificate match the ones we have */
	for (i=0;i<MIN(2, crt_list_size);i++) {
		ret = gnutls_certificate_get_crt_raw(xcred, idx, i, &tcert);
		if (ret < 0) {
			fail("error in %d: cert: %d: %s\n", __LINE__, i, gnutls_strerror(ret));
			exit(1);
		}

		compare(&tcert, cert->data+i);
	}

 cleanup:
	gnutls_x509_privkey_deinit(key);
	for (i=0;i<crt_list_size;i++) {
		gnutls_x509_crt_deinit(crt_list[i]);
	}
	gnutls_free(crt_list);

	return idx;
}

void doit(void)
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_certificate_credentials_t clicred;
	int ret;
	unsigned idx;

#if !defined(HAVE_LIBIDN) && !defined(HAVE_LIBIDN2)
	exit(77);
#endif

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_time_function(mytime);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);

	gnutls_certificate_set_flags(x509_cred, GNUTLS_CERTIFICATE_API_V2);

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));

	idx = import_key(x509_cred, &server_ca3_key, &server_ca3_localhost_cert_chain);
	assert(idx == 0);

	idx = import_key(x509_cred, &server_ca3_key, &server_ca3_localhost_utf8_cert);
	assert(idx == 1);

	test_cli_serv(x509_cred, clicred, "NORMAL", "localhost", NULL, NULL, NULL);
#if defined(HAVE_LIBIDN) /* IDNA2003 */
	test_cli_serv(x509_cred, clicred, "NORMAL", "www.νίκος.com", NULL, NULL, NULL); /* the DNS name of second cert */
	test_cli_serv(x509_cred, clicred, "NORMAL", "raw:www.νίκος.com", NULL, NULL, NULL); /* the DNS name of second cert */
#endif
	test_cli_serv(x509_cred, clicred, "NORMAL", "www.xn--kxawhku.com", NULL, NULL, NULL); /* the previous name in IDNA format */
	test_cli_serv(x509_cred, clicred, "NORMAL", "简体中文.εξτρα.com", NULL, NULL, NULL); /* the second DNS name of cert */
	test_cli_serv(x509_cred, clicred, "NORMAL", "raw:简体中文.εξτρα.com", NULL, NULL, NULL); /* the second DNS name of cert */
	test_cli_serv(x509_cred, clicred, "NORMAL", "xn--fiqu1az03c18t.xn--mxah1amo.com", NULL, NULL, NULL); /* its IDNA equivalent */

	gnutls_certificate_free_credentials(x509_cred);
	gnutls_certificate_free_credentials(clicred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}

