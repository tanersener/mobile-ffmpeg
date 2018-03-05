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

/* This test checks the behavior of gnutls_certificate_set_x509_key_file2()
 * when the GNUTLS_CERTIFICATE_API_V2 is not set */

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

static time_t mytime(time_t * t)
{
	time_t then = 1470002400;
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

static unsigned set_cert(gnutls_certificate_credentials_t xcred, const gnutls_datum_t *key, const gnutls_datum_t *cert)
{
	const char *certfile;
	FILE *fp;
	int ret;

	certfile = get_tmpname(NULL);

	fp = fopen(certfile, "w");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(cert->data, 1, cert->size, fp)>0);
	assert(fwrite(key->data, 1, key->size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_x509_key_file2(xcred, certfile, certfile,
						    GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("set_x509_key_file failed: %s\n", gnutls_strerror(ret));

	/* return index */
	return ret;
}

static void verify_written_cert(gnutls_certificate_credentials_t xcred, unsigned idx, const gnutls_datum_t *cert, unsigned ncerts)
{
	int ret;
	gnutls_datum_t tcert = {NULL, 0};

	/* verify whether the stored certificate match the ones we have */
	ret = gnutls_certificate_get_crt_raw(xcred, idx, 0, &tcert);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	compare(&tcert, cert->data);

	if (ncerts > 1) {
		ret = gnutls_certificate_get_crt_raw(xcred, idx, 1, &tcert);
		if (ret < 0) {
			fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
			exit(1);
		}

		/* skip headers of first cert */
		compare(&tcert, cert->data+2);
	}
}

void doit(void)
{
	int ret;
	gnutls_certificate_credentials_t xcred, clicred;
	const char *keyfile = "./certs/ecc256.pem";
	const char *certfile = "does-not-exist.pem";
	unsigned idx, i;

	global_init();
	assert(gnutls_certificate_allocate_credentials(&xcred) >= 0);
	gnutls_global_set_time_function(mytime);
	track_temp_files();

	/* this will fail */
	ret = gnutls_certificate_set_x509_key_file2(xcred, certfile, keyfile,
						   GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret != GNUTLS_E_FILE_ERROR)
		fail("set_x509_key_file failed: %s\n", gnutls_strerror(ret));

	gnutls_certificate_free_credentials(xcred);

	assert(gnutls_certificate_allocate_credentials(&xcred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &subca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));

	success("Testing store of certificates\n");

	idx = set_cert(xcred, &server_ca3_key, &server_ca3_localhost6_cert_chain);
	verify_written_cert(xcred, idx, &server_ca3_localhost6_cert_chain, 2);
	assert(idx == 0);

	success("Tested store of %d\n", idx);

	idx = set_cert(xcred, &server_ca3_key, &server_ca3_localhost_cert);
	assert(idx == 0);

	success("Tested store of %d\n", idx);

	test_cli_serv(xcred, clicred, "NORMAL", "localhost", NULL, NULL, NULL); /* the DNS name of the first cert */

	idx = set_cert(xcred, &server_key, &server_cert);
	assert(idx == 0);

	success("Tested store of %d\n", idx);

	for (i=0;i<16;i++) {
		idx = set_cert(xcred, &server_ecc_key, &server_ecc_cert);
		assert(idx == 0);
		success("Tested store of %d\n", idx);
	}

	gnutls_certificate_free_credentials(xcred);
	gnutls_certificate_free_credentials(clicred);
	gnutls_global_deinit();
	delete_temp_files();
}
