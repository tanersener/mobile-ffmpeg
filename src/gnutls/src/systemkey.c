/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/openpgp.h>
#include <gnutls/pkcs12.h>
#include <gnutls/system-keys.h>
#include <gnutls/abstract.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Gnulib portability files. */
#include <read-file.h>

#include "certtool-common.h"
#include "systemkey-args.h"

static void cmd_parser(int argc, char **argv);
static void systemkey_delete(const char *url, FILE * outfile);
static void systemkey_list(FILE * outfile);

static gnutls_x509_crt_fmt_t incert_format, outcert_format;
static gnutls_x509_crt_fmt_t inkey_format, outkey_format;

static FILE *outfile;
static FILE *infile;
int batch = 0;
int ask_pass = 0;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}


int main(int argc, char **argv)
{
	cmd_parser(argc, argv);

	return 0;
}

static void cmd_parser(int argc, char **argv)
{
	unsigned int optct;
	/* Note that the default sec-param is legacy because several TPMs
	 * cannot handle larger keys.
	 */
	optct = optionProcess(&systemkey_toolOptions, argc, argv);
	argc += optct;
	argv += optct;

	gnutls_global_set_log_function(tls_log_func);

	if (HAVE_OPT(DEBUG)) {
		gnutls_global_set_log_level(OPT_VALUE_DEBUG);
		printf("Setting log level to %d\n", (int) OPT_VALUE_DEBUG);
	}

	if (HAVE_OPT(INDER)) {
		incert_format = GNUTLS_X509_FMT_DER;
		inkey_format = GNUTLS_X509_FMT_DER;
	} else {
		incert_format = GNUTLS_X509_FMT_PEM;
		inkey_format = GNUTLS_X509_FMT_PEM;
	}

	if (HAVE_OPT(OUTDER)) {
		outcert_format = GNUTLS_X509_FMT_DER;
		outkey_format = GNUTLS_X509_FMT_DER;
	} else {
		outcert_format = GNUTLS_X509_FMT_PEM;
		outkey_format = GNUTLS_X509_FMT_PEM;
	}

	if (HAVE_OPT(OUTFILE)) {
		outfile = safe_open_rw(OPT_ARG(OUTFILE), 0);
		if (outfile == NULL) {
			fprintf(stderr, "%s", OPT_ARG(OUTFILE));
			exit(1);
		}
	} else
		outfile = stdout;

	if (HAVE_OPT(INFILE)) {
		infile = fopen(OPT_ARG(INFILE), "rb");
		if (infile == NULL) {
			fprintf(stderr, "%s", OPT_ARG(INFILE));
			exit(1);
		}
	} else
		infile = stdin;

	if (HAVE_OPT(DELETE)) {
		systemkey_delete(OPT_ARG(DELETE), outfile);
	} else if (HAVE_OPT(LIST)) {
		systemkey_list(outfile);
	} else {
		USAGE(1);
	}

	fclose(outfile);

	gnutls_global_deinit();
}
static void systemkey_delete(const char *url, FILE * out)
{
	int ret;

	ret = gnutls_system_key_delete(url, url);
	if (ret < 0) {
		fprintf(stderr, "gnutls_systemkey_privkey_delete: %s",
			gnutls_strerror(ret));
		exit(1);
	}

	fprintf(out, "Key %s deleted\n", url);
}

static void systemkey_list(FILE * out)
{
	int ret;
	gnutls_system_key_iter_t iter = NULL;
	char *cert_url, *key_url, *label;

	do {
		ret = gnutls_system_key_iter_get_info(&iter, GNUTLS_CRT_X509, &cert_url, &key_url, &label, NULL, 0);
		if (ret >= 0) {
			fprintf(out, "Label:\t%s\nCert:\t%s\nKey:\t%s\n\n", label, cert_url, key_url);
		}
	} while(ret >= 0);

	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fprintf(stderr, "gnutls_system_key_iter_get_url: %s",
			gnutls_strerror(ret));
		exit(1);
	}
	gnutls_system_key_iter_deinit(iter);
	fputs("\n", out);
}
