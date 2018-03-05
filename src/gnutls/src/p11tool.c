/*
 * Copyright (C) 2010-2014 Free Software Foundation, Inc.
 * Copyright (C) 2013-2014 Nikos Mavrogiannopoulos
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
#include <gnutls/pkcs11.h>
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

#include "p11tool-args.h"
#include "p11tool.h"
#include "certtool-common.h"

static void cmd_parser(int argc, char **argv);

static FILE *outfile;
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

static
unsigned opt_to_flags(common_info_st *cinfo, unsigned *key_usage)
{
	unsigned flags = 0;
	
	*key_usage = 0;

	if (HAVE_OPT(MARK_PRIVATE)) {
		if (ENABLED_OPT(MARK_PRIVATE)) {
			flags |= GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE;
		} else {
			flags |= GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_PRIVATE;
		}
	} else { /* if not given mark as private the private objects, and public the public ones */
		if (cinfo->privkey)
			flags |= GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE;
		else if (cinfo->pubkey || cinfo->cert)
			flags |= GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_PRIVATE;
		/* else set the defaults of the token */
	}

	if (HAVE_OPT(MARK_DISTRUSTED)) {
		flags |=
		    GNUTLS_PKCS11_OBJ_FLAG_MARK_DISTRUSTED;
	} else {
		if (ENABLED_OPT(MARK_TRUSTED))
			flags |=
			    GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED;
	}

	if (ENABLED_OPT(MARK_SIGN))
		*key_usage |= GNUTLS_KEY_DIGITAL_SIGNATURE;

	if (ENABLED_OPT(MARK_DECRYPT))
		*key_usage |= GNUTLS_KEY_DECIPHER_ONLY;

	if (ENABLED_OPT(MARK_CA))
		flags |=
		    GNUTLS_PKCS11_OBJ_FLAG_MARK_CA;

	if (ENABLED_OPT(MARK_WRAP))
		flags |= GNUTLS_PKCS11_OBJ_FLAG_MARK_KEY_WRAP;

	if (ENABLED_OPT(LOGIN))
		flags |= GNUTLS_PKCS11_OBJ_FLAG_LOGIN;

	if (ENABLED_OPT(SO_LOGIN))
		flags |= GNUTLS_PKCS11_OBJ_FLAG_LOGIN_SO;

	return flags;
}

static void cmd_parser(int argc, char **argv)
{
	int ret, debug = 0;
	common_info_st cinfo;
	unsigned int pkcs11_type = -1, key_type = GNUTLS_PK_UNKNOWN;
	const char *url = NULL;
	unsigned int detailed_url = 0, optct;
	unsigned int bits = 0;
	const char *label = NULL, *sec_param = NULL, *id = NULL;
	unsigned flags;
	unsigned key_usage;

	optct = optionProcess(&p11toolOptions, argc, argv);
	argc += optct;
	argv += optct;

	if (url == NULL && argc > 0)
		url = argv[0];
	else
		url = "pkcs11:";

	if (HAVE_OPT(DEBUG))
		debug = OPT_VALUE_DEBUG;

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(debug);
	if (debug > 1)
		printf("Setting log level to %d\n", debug);

	if ((ret = gnutls_global_init()) < 0) {
		fprintf(stderr, "global_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (HAVE_OPT(PROVIDER)) {
		ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
		if (ret < 0)
			fprintf(stderr, "pkcs11_init: %s\n",
				gnutls_strerror(ret));
		else {
			ret =
			    gnutls_pkcs11_add_provider(OPT_ARG(PROVIDER),
						       NULL);
			if (ret < 0) {
				fprintf(stderr, "pkcs11_add_provider: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	} else {
		ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_AUTO, NULL);
		if (ret < 0)
			fprintf(stderr, "pkcs11_init: %s\n",
				gnutls_strerror(ret));
	}

	if (HAVE_OPT(OUTFILE)) {
		outfile = safe_open_rw(OPT_ARG(OUTFILE), 0);
		if (outfile == NULL) {
			fprintf(stderr, "cannot open %s\n", OPT_ARG(OUTFILE));
			exit(1);
		}
	} else
		outfile = stdout;

	memset(&cinfo, 0, sizeof(cinfo));

	if (HAVE_OPT(SECRET_KEY))
		cinfo.secret_key = OPT_ARG(SECRET_KEY);

	if (HAVE_OPT(LOAD_PRIVKEY))
		cinfo.privkey = OPT_ARG(LOAD_PRIVKEY);

	if (HAVE_OPT(PKCS8))
		cinfo.pkcs8 = 1;

	if (HAVE_OPT(BATCH)) {
		batch = cinfo.batch = 1;
	}

	if (HAVE_OPT(ONLY_URLS)) {
		cinfo.only_urls = 1;
	}

	if (ENABLED_OPT(INDER) || ENABLED_OPT(INRAW))
		cinfo.incert_format = GNUTLS_X509_FMT_DER;
	else
		cinfo.incert_format = GNUTLS_X509_FMT_PEM;

	if (HAVE_OPT(OUTDER) || HAVE_OPT(OUTRAW))
		cinfo.outcert_format = GNUTLS_X509_FMT_DER;
	else
		cinfo.outcert_format = GNUTLS_X509_FMT_PEM;

	if (HAVE_OPT(SET_PIN))
		cinfo.pin = OPT_ARG(SET_PIN);

	if (HAVE_OPT(SET_SO_PIN))
		cinfo.so_pin = OPT_ARG(SET_SO_PIN);

	if (HAVE_OPT(LOAD_CERTIFICATE))
		cinfo.cert = OPT_ARG(LOAD_CERTIFICATE);

	if (HAVE_OPT(LOAD_PUBKEY))
		cinfo.pubkey = OPT_ARG(LOAD_PUBKEY);

	if (ENABLED_OPT(DETAILED_URL))
		detailed_url = 1;

	if (HAVE_OPT(LABEL)) {
		label = OPT_ARG(LABEL);
	}

	if (HAVE_OPT(ID)) {
		id = OPT_ARG(ID);
	}

	if (HAVE_OPT(BITS)) {
		bits = OPT_VALUE_BITS;
	}

	if (HAVE_OPT(CURVE)) {
		gnutls_ecc_curve_t curve = str_to_curve(OPT_ARG(CURVE));
		bits = GNUTLS_CURVE_TO_BITS(curve);
	}

	if (HAVE_OPT(SEC_PARAM)) {
		sec_param = OPT_ARG(SEC_PARAM);
	}

	flags = opt_to_flags(&cinfo, &key_usage);
	cinfo.key_usage = key_usage;

	/* handle actions 
	 */
	if (HAVE_OPT(LIST_TOKENS)) {
		pkcs11_token_list(outfile, detailed_url, &cinfo, 0);
	} else if (HAVE_OPT(LIST_TOKEN_URLS)) {
		pkcs11_token_list(outfile, detailed_url, &cinfo, 1);
	} else if (HAVE_OPT(LIST_MECHANISMS)) {
		pkcs11_mechanism_list(outfile, url, flags, &cinfo);
	} else if (HAVE_OPT(GENERATE_RANDOM)) {
		pkcs11_get_random(outfile, url, OPT_VALUE_GENERATE_RANDOM,
				  &cinfo);
	} else if (HAVE_OPT(INFO)) {
		pkcs11_type = PKCS11_TYPE_INFO;
		pkcs11_list(outfile, url, pkcs11_type,
			    flags, detailed_url, &cinfo);
	} else if (HAVE_OPT(LIST_ALL)) {
		pkcs11_type = PKCS11_TYPE_ALL;
		pkcs11_list(outfile, url, pkcs11_type,
			    flags, detailed_url, &cinfo);
	} else if (HAVE_OPT(LIST_ALL_CERTS)) {
		pkcs11_type = PKCS11_TYPE_CRT_ALL;
		pkcs11_list(outfile, url, pkcs11_type,
			    flags, detailed_url, &cinfo);
	} else if (HAVE_OPT(LIST_CERTS)) {
		pkcs11_type = PKCS11_TYPE_PK;
		pkcs11_list(outfile, url, pkcs11_type,
			    flags, detailed_url, &cinfo);
	} else if (HAVE_OPT(LIST_ALL_PRIVKEYS)) {
		pkcs11_type = PKCS11_TYPE_PRIVKEY;
		pkcs11_list(outfile, url, pkcs11_type,
			    flags, detailed_url, &cinfo);
	} else if (HAVE_OPT(LIST_ALL_TRUSTED)) {
		pkcs11_type = PKCS11_TYPE_TRUSTED;
		pkcs11_list(outfile, url, pkcs11_type,
			    flags, detailed_url, &cinfo);
	} else if (HAVE_OPT(EXPORT)) {
		pkcs11_export(outfile, url, flags, &cinfo);
	} else if (HAVE_OPT(EXPORT_STAPLED)) {
		pkcs11_export(outfile, url, flags|GNUTLS_PKCS11_OBJ_FLAG_OVERWRITE_TRUSTMOD_EXT, &cinfo);
	} else if (HAVE_OPT(EXPORT_CHAIN)) {
		pkcs11_export_chain(outfile, url, flags, &cinfo);
	} else if (HAVE_OPT(WRITE)) {
		pkcs11_write(outfile, url, label, id,
			     flags, &cinfo);
	} else if (HAVE_OPT(TEST_SIGN)) {
		pkcs11_test_sign(outfile, url, flags, &cinfo);
	} else if (HAVE_OPT(INITIALIZE)) {
		pkcs11_init(outfile, url, label, &cinfo);
	} else if (HAVE_OPT(INITIALIZE_PIN)) {
		pkcs11_set_pin(outfile, url, &cinfo, 0);
	} else if (HAVE_OPT(INITIALIZE_SO_PIN)) {
		pkcs11_set_pin(outfile, url, &cinfo, 1);
	} else if (HAVE_OPT(DELETE))
		pkcs11_delete(outfile, url, flags, &cinfo);
	else if (HAVE_OPT(GENERATE_ECC)) {
		key_type = GNUTLS_PK_EC;
		pkcs11_generate(outfile, url, key_type,
				get_bits(key_type, bits, sec_param, 0),
				label, id, detailed_url,
				flags, &cinfo);
	} else if (HAVE_OPT(GENERATE_RSA)) {
		key_type = GNUTLS_PK_RSA;
		pkcs11_generate(outfile, url, key_type,
				get_bits(key_type, bits, sec_param, 0),
				label, id, detailed_url,
				flags, &cinfo);
	} else if (HAVE_OPT(GENERATE_DSA)) {
		key_type = GNUTLS_PK_DSA;
		pkcs11_generate(outfile, url, key_type,
				get_bits(key_type, bits, sec_param, 0),
				label, id, detailed_url,
				flags, &cinfo);
	} else if (HAVE_OPT(EXPORT_PUBKEY)) {
		pkcs11_export_pubkey(outfile, url, detailed_url, flags, &cinfo);
	} else if (HAVE_OPT(SET_ID)) {
		pkcs11_set_id(outfile, url, detailed_url, flags, &cinfo, OPT_ARG(SET_ID));
	} else if (HAVE_OPT(SET_LABEL)) {
		pkcs11_set_label(outfile, url, detailed_url, flags, &cinfo, OPT_ARG(SET_LABEL));
	} else {
		USAGE(1);
	}

	fclose(outfile);

#ifdef ENABLE_PKCS11
	gnutls_pkcs11_deinit();
#endif
	gnutls_global_deinit();
}
