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
#define P11_KIT_FUTURE_UNSTABLE_API
#include <p11-kit/p11-kit.h>
#include "cert-common.h"

/* lists the registered PKCS#11 modules by p11-kit.
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static const char *opt_pin;

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
	     unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, opt_pin);
		return 0;
	}
	return -1;
}

int main(int argc, char **argv)
{
	int ret;
	unsigned i;
	int opt;
	char *url, *mod;
	unsigned flags;
	unsigned obj_flags = 0;
	int attrs = GNUTLS_PKCS11_OBJ_ATTR_ALL;
	gnutls_pkcs11_obj_t *crt_list;
	unsigned int crt_list_size = 0;
	const char *envvar;

	ret = gnutls_global_init();
	if (ret != 0) {
		fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);

	while((opt = getopt(argc, argv, "o:t:")) != -1) {
		switch(opt) {
			case 'o':
				mod = strdup(optarg);
				p11_kit_override_system_files(NULL, NULL, mod, mod, NULL);
				break;
			case 't':
				/* specify the object type to list */
				if (strcmp(optarg, "all") == 0)
					attrs = GNUTLS_PKCS11_OBJ_ATTR_ALL;
				else if (strcmp(optarg, "privkey") == 0)
					attrs = GNUTLS_PKCS11_OBJ_ATTR_PRIVKEY;
				else {
					fprintf(stderr, "Unknown object type %s\n", optarg);
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Unknown option %c\n", (char)opt);
				exit(1);
		}
	}

	if (optind == argc) {
		fprintf(stderr, "specify URL\n");
		exit(1);
	}
	url = argv[optind];

	envvar = getenv("GNUTLS_PIN");
	if (envvar && *envvar != '\0') {
		opt_pin = envvar;
		obj_flags |= GNUTLS_PKCS11_OBJ_FLAG_LOGIN;
		gnutls_pkcs11_set_pin_function(pin_func, NULL);
	}

	ret = gnutls_pkcs11_token_get_flags(url, &flags);
	if (ret < 0) {
		flags = 0;
	}

	ret =
	    gnutls_pkcs11_obj_list_import_url2(&crt_list, &crt_list_size,
					       url, attrs, obj_flags);
	if (ret != 0) {
		fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	for (i = 0; i < crt_list_size; i++) {
		char *output;

		ret =
		    gnutls_pkcs11_obj_export_url(crt_list[i], 0,
						 &output);
		if (ret != 0) {
			fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
			exit(1);
		}

		fprintf(stdout, "%s\n", output);
		gnutls_free(output);
		gnutls_pkcs11_obj_deinit(crt_list[i]);
	}
	gnutls_free(crt_list);

	gnutls_global_deinit();
}
