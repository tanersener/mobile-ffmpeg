/*
 * Copyright (C) 2008-2014 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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
#include <string.h>
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>

#include "../utils.h"
#include "../test-chains.h"
#include "softhsm.h"

#define CONFIG "softhsm-issuer.config"

/* GnuTLS internally calls time() to find out the current time when
   verifying certificates.  To avoid a time bomb, we hard code the
   current time.  This should work fine on systems where the library
   call to time is resolved at run-time.  */
static time_t mytime(time_t * t)
{
	time_t then = 1256803113;

	if (t)
		*t = then;

	return then;
}

#define PIN "1234"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
		unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

void doit(void)
{
	char buf[128];
	int exit_val = 0;
	int ret;
	unsigned j;
	const char *lib, *bin;
	gnutls_x509_crt_t issuer = NULL;
	gnutls_x509_trust_list_t tl;
	gnutls_x509_crt_t certs[MAX_CHAIN];
	gnutls_x509_crt_t ca;
	gnutls_datum_t tmp;
	int idx = -1;

	/* The overloading of time() seems to work in linux (ELF?)
	 * systems only. Disable it on windows.
	 */
#ifdef _WIN32
	exit(77);
#endif
	for (j=0;;j++) {
		if (chains[j].name == NULL)
			break;
		if (strcmp(chains[j].name, "verisign.com v1 ok") == 0) {
			idx = j;
			break;
		}
	}

	if (idx == -1) {
		fail("could not find proper chain\n");
		exit(1);
	}

	bin = softhsm_bin();

	lib = softhsm_lib();

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs11_set_pin_function(pin_func, NULL);
	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	/* write softhsm.config */

	set_softhsm_conf(CONFIG);
	snprintf(buf, sizeof(buf), "%s --init-token --slot 0 --label test --so-pin "PIN" --pin "PIN, bin);
	system(buf);

	ret = gnutls_pkcs11_add_provider(lib, "trusted");
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	for (j = 0; chains[idx].chain[j]; j++) {
		if (debug > 2)
			printf("\tAdding certificate %d...",
			       (int) j);

		ret = gnutls_x509_crt_init(&certs[j]);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crt_init[%d,%d]: %s\n",
				(int) 3, (int) j,
				gnutls_strerror(ret));
			exit(1);
		}

		tmp.data = (unsigned char *) chains[idx].chain[j];
		tmp.size = strlen(chains[idx].chain[j]);

		ret =
		    gnutls_x509_crt_import(certs[j], &tmp,
					   GNUTLS_X509_FMT_PEM);
		if (debug > 2)
			printf("done\n");
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crt_import[%s,%d]: %s\n",
				chains[idx].name, (int) j,
				gnutls_strerror(ret));
			exit(1);
		}

		gnutls_x509_crt_print(certs[j],
				      GNUTLS_CRT_PRINT_ONELINE,
				      &tmp);
		if (debug)
			printf("\tCertificate %d: %.*s\n", (int) j,
			       tmp.size, tmp.data);
		gnutls_free(tmp.data);
	}

	if (debug > 2)
		printf("\tAdding CA certificate...");

	ret = gnutls_x509_crt_init(&ca);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	tmp.data = (unsigned char *) *chains[idx].ca;
	tmp.size = strlen(*chains[idx].ca);

	ret =
	    gnutls_x509_crt_import(ca, &tmp, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_import: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	if (debug > 2)
		printf("done\n");

	gnutls_x509_crt_print(ca, GNUTLS_CRT_PRINT_ONELINE, &tmp);
	if (debug)
		printf("\tCA Certificate: %.*s\n", tmp.size,
		       tmp.data);
	gnutls_free(tmp.data);

	if (debug)
		printf("\tVerifying...");

	/* initialize softhsm token */
	ret = gnutls_pkcs11_token_init(SOFTHSM_URL, PIN, "test");
	if (ret < 0) {
		fail("gnutls_pkcs11_token_init\n");
		exit(1);
	}

	/* write CA certificate to softhsm */
	ret = gnutls_pkcs11_copy_x509_crt(SOFTHSM_URL, ca, "test-ca", GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED|GNUTLS_PKCS11_OBJ_FLAG_LOGIN_SO);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_crt: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_trust_list_init(&tl, 0);

	ret = gnutls_x509_trust_list_add_trust_file(tl, SOFTHSM_URL, NULL, 0, 0, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_trust_file\n");
		exit(1);
	}

	/* extract the issuer of the certificate */
	issuer = NULL;
	ret = gnutls_x509_trust_list_get_issuer(tl, certs[2], &issuer, GNUTLS_TL_GET_COPY);
	if (ret < 0) {
		fail("error in gnutls_x509_trust_list_get_issuer\n");
		exit(1);
	}
	if (issuer == NULL) {
		fail("error in gnutls_x509_trust_list_get_issuer return value\n");
		exit(1);
	}
	gnutls_x509_crt_deinit(issuer);

	/* extract the issuer of the certificate using the non-thread-safe approach */
	issuer = NULL;
	ret = gnutls_x509_trust_list_get_issuer(tl, certs[2], &issuer, 0);
	if (ret < 0) {
		fail("error in gnutls_x509_trust_list_get_issuer\n");
		exit(1);
	}
	if (issuer == NULL) {
		fail("error in gnutls_x509_trust_list_get_issuer return value\n");
		exit(1);
	}

	/* extract (again) the issuer of the certificate - check for any leaks */
	ret = gnutls_x509_trust_list_get_issuer(tl, certs[2], &issuer, 0);
	if (ret < 0) {
		fail("error in gnutls_x509_trust_list_get_issuer\n");
		exit(1);
	}

	/* Check gnutls_x509_trust_list_get_raw_issuer_by_dn */
	ret = gnutls_x509_crt_get_raw_issuer_dn(certs[2], &tmp);
	if (ret < 0) {
		fail("error in gnutls_x509_crt_get_raw_issuer_dn: %s\n", gnutls_strerror(ret));
		exit(1);
	}
	
	ret = gnutls_x509_trust_list_get_issuer_by_dn(tl, &tmp, &issuer, 0);
	gnutls_free(tmp.data);
	if (ret < 0) {
		fail("error in gnutls_x509_trust_list_get_issuer\n");
		exit(1);
	}
	if (issuer == NULL) {
		fail("error in gnutls_x509_trust_list_get_issuer_by_dn return value\n");
		exit(1);
	}
	gnutls_x509_crt_deinit(issuer);

	if (debug)
		printf("\tCleanup...");

	gnutls_x509_trust_list_deinit(tl, 0);
	gnutls_x509_crt_deinit(ca);
	for (j = 0; chains[idx].chain[j]; j++)
		gnutls_x509_crt_deinit(certs[j]);
	if (debug)
		printf("done\n\n\n");

	gnutls_global_deinit();

	if (debug)
		printf("Exit status...%d\n", exit_val);
	remove(CONFIG);

	exit(exit_val);
}
