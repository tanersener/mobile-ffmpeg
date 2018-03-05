/*
 * Copyright (C) 2014-2016 Nikos Mavrogiannopoulos
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "cert-common.h"
#include "utils.h"

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

static void write_der(const char *file, const char *header, const char *ipem)
{
	gnutls_datum_t pem = {(void*)ipem, strlen((char*)ipem)};
	gnutls_datum_t der;
	FILE *fp;
	int ret;

	ret = gnutls_pem_base64_decode2(header, &pem, &der);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
	}

	fp = fopen(file, "wb");
	if (fp == NULL)
		fail("error in fopen\n");

	assert(fwrite(der.data, 1, der.size, fp)>0);
	fclose(fp);
	gnutls_free(der.data);
}

void doit(void)
{
	int ret;
	gnutls_certificate_credentials_t xcred, clicred;
	char keyfile[TMPNAME_SIZE];
	char certfile[TMPNAME_SIZE];
	gnutls_datum_t tcert;

	global_init();

	assert(gnutls_certificate_allocate_credentials(&xcred) >= 0);

	if (TMP_MAX < 2)
		exit(77);

	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca2_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));

	assert(get_tmpname(certfile)!=NULL);
	assert(get_tmpname(keyfile)!=NULL);

	write_der(certfile, "CERTIFICATE", (char*)server2_cert_pem);
	write_der(keyfile, "RSA PRIVATE KEY", (char*)server2_key_pem);

	ret = gnutls_certificate_set_x509_key_file2(xcred, certfile, keyfile,
						    GNUTLS_X509_FMT_DER, NULL, 0);
	if (ret < 0)
		fail("set_x509_key_file failed: %s\n", gnutls_strerror(ret));

	/* verify whether the stored certificate match the ones we have */
	ret = gnutls_certificate_get_crt_raw(xcred, 0, 0, &tcert);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	compare(&tcert, server2_cert_pem);

	remove(certfile);
	remove(keyfile);

	test_cli_serv(xcred, clicred, "NORMAL", "localhost", NULL, NULL, NULL); /* the DNS name of the first cert */

	gnutls_certificate_free_credentials(xcred);
	gnutls_certificate_free_credentials(clicred);
	gnutls_global_deinit();
}

