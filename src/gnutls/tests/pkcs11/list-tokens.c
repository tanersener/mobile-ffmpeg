/*
 * Copyright (C) 2016-2017 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <getopt.h>
#include <assert.h>
#define P11_KIT_FUTURE_UNSTABLE_API
#include <p11-kit/p11-kit.h>
#include "cert-common.h"

/* lists the registered PKCS#11 modules by p11-kit.
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

int
_gnutls_pkcs11_token_get_url(unsigned int seq,
                            gnutls_pkcs11_url_type_t detailed, char **url,
                            unsigned flags);

int main(int argc, char **argv)
{
	int ret;
	unsigned i;
	int opt;
	char *url, *mod;
	gnutls_x509_trust_list_t tl;
	gnutls_x509_crt_t crt;
	gnutls_pkcs11_privkey_t key;
	unsigned flag = 1;
	unsigned int status;

	ret = gnutls_global_init();
	if (ret != 0) {
		fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	//gnutls_global_set_log_level(4711);

	while((opt = getopt(argc, argv, "s:o:mvatdp")) != -1) {
		switch(opt) {
			case 'o':
				mod = strdup(optarg);
				p11_kit_override_system_files(NULL, NULL, mod, mod, NULL);
				break;
			case 'm':
				/* initialize manually - i.e., do no module loading */
				ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
				if (ret != 0) {
					fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
					exit(1);
				}
				break;
			case 's':
				/* load module */
				ret = gnutls_pkcs11_add_provider(optarg, NULL);
				if (ret != 0) {
					fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
					exit(1);
				}
				break;
			case 'd':
				/* when call _gnutls_pkcs11_token_get_url() do proper initialization
				 * if none done */
				flag = 0;
				break;
			case 'p':
				/* do private key operations */
				assert(gnutls_pkcs11_privkey_init(&key) >= 0);
				gnutls_pkcs11_privkey_import_url(key, "pkcs11:", 0);
				gnutls_pkcs11_privkey_deinit(key);
				break;
			case 'a':
				/* initialize auto - i.e., do module loading */
				ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_AUTO, NULL);
				if (ret != 0) {
					fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
					exit(1);
				}
				break;
			case 't':
				/* do trusted module loading */
				ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_AUTO_TRUSTED, NULL);
				if (ret != 0) {
					fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
					exit(1);
				}
				break;
			case 'v':
				/* do verification which should trigger trusted module loading */
				assert(gnutls_x509_crt_init(&crt) >= 0);
				assert(gnutls_x509_crt_import(crt, &ca3_cert, GNUTLS_X509_FMT_PEM) >= 0);

				assert(gnutls_x509_trust_list_init(&tl, 0) >= 0);
				assert(gnutls_x509_trust_list_add_system_trust(tl, 0, 0) >= 0);
				gnutls_x509_trust_list_verify_crt2(tl, &crt, 1, NULL, 0, 0, &status, NULL);
				gnutls_x509_trust_list_deinit(tl, 1);
				gnutls_x509_crt_deinit(crt);
				break;
			default:
				fprintf(stderr, "Unknown option %c\n", (char)opt);
				exit(1);
		}
	}


	for (i=0;;i++) {
		ret = _gnutls_pkcs11_token_get_url(i, 0, &url, flag);
		if (ret < 0)
			break;
		printf("%s\n", url);
		free(url);
	}

	/* try whether these URIs are operational */
	for (i=0;;i++) {
		unsigned tflags;

		ret = _gnutls_pkcs11_token_get_url(i, 0, &url, flag);
		if (ret < 0)
			break;
		ret = gnutls_pkcs11_token_get_flags(url, &tflags);
		if (ret < 0) {
			fprintf(stderr, "cannot get token %s flags: %s\n", url, gnutls_strerror(ret));
			exit(1);
		}
		free(url);
	}

	gnutls_global_deinit();
}
