/*
 * Copyright (C) 2011-2014 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Red Hat, Inc.
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
 * <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gnutls/gnutls.h>
#include <gnutls/ocsp.h>
#include <gnutls/x509.h>
#include <gnutls/crypto.h>

#include <unistd.h> /* getpass */

/* Gnulib portability files. */
#include <read-file.h>
#include <socket.h>
#include <minmax.h>

#include <ocsptool-common.h>
#include <ocsptool-args.h>
#include "certtool-common.h"

FILE *outfile;
static unsigned int incert_format, outcert_format;
static const char *outfile_name = NULL; /* to delete on exit */
FILE *infile;
static unsigned int encoding;
unsigned int verbose = 0;
static unsigned int vflags = 0;

const char *get_pass(void)
{
	return getpass("Enter password: ");
}

const char *get_confirmed_pass(bool ign)
{
	return getpass("Enter password: ");
}

void app_exit(int val)
{
	if (val != 0) {
		if (outfile_name)
			(void)remove(outfile_name);
	}
	exit(val);
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

gnutls_session_t init_tls_session(const char *host)
{
	return NULL;
}

int do_handshake(socket_st * socket)
{
	return -1;
}

static void request_info(void)
{
	gnutls_ocsp_req_t req;
	int ret;
	gnutls_datum_t dat, rbuf;
	size_t size;

	ret = gnutls_ocsp_req_init(&req);
	if (ret < 0) {
		fprintf(stderr, "ocsp_req_init: %s\n", gnutls_strerror(ret));
		app_exit(1);
	}

	if (HAVE_OPT(LOAD_REQUEST))
		dat.data =
		    (void *) read_binary_file(OPT_ARG(LOAD_REQUEST),
					      &size);
	else
		dat.data = (void *) fread_file(infile, &size);
	if (dat.data == NULL) {
		fprintf(stderr, "error reading request\n");
		app_exit(1);
	}
	dat.size = size;


	ret = gnutls_ocsp_req_import(req, &dat);
	free(dat.data);
	if (ret < 0) {
		fprintf(stderr, "error importing request: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_ocsp_req_print(req, GNUTLS_OCSP_PRINT_FULL, &dat);
	if (ret != 0) {
		fprintf(stderr, "ocsp_req_print: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	if (HAVE_OPT(OUTFILE)) {
		ret = gnutls_ocsp_req_export(req, &rbuf);
		if (ret < 0) {
			fprintf(stderr, "error exporting request: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		if (outcert_format == GNUTLS_X509_FMT_PEM) {
			fprintf(stderr, "Cannot export requests into PEM form\n");
			app_exit(1);
		} else {
			fwrite(rbuf.data, 1, rbuf.size, outfile);
		}

		gnutls_free(rbuf.data);
	} else {
		printf("%.*s", dat.size, dat.data);
	}

	gnutls_free(dat.data);

	gnutls_ocsp_req_deinit(req);
}

static void _response_info(const gnutls_datum_t * data, unsigned force_print)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	gnutls_datum_t buf, rbuf;

	if (data->size == 0) {
		fprintf(stderr, "Received empty response\n");
		app_exit(1);
	}

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret < 0) {
		fprintf(stderr, "ocsp_resp_init: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_ocsp_resp_import2(resp, data, incert_format);
	if (ret == GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR) {
		int ret2 = gnutls_ocsp_resp_import(resp, data);
		if (ret2 >= 0)
			ret = ret2;
	}
	if (ret < 0) {
		fprintf(stderr, "error importing response: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	if (ENABLED_OPT(VERBOSE))
		ret =
		    gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_FULL,
					   &buf);
	else
		ret =
		    gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_COMPACT,
					   &buf);
	if (ret != 0) {
		fprintf(stderr, "ocsp_resp_print: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	if (HAVE_OPT(OUTFILE)) {
		ret = gnutls_ocsp_resp_export2(resp, &rbuf, outcert_format);
		if (ret < 0) {
			fprintf(stderr, "error exporting response: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		if (outcert_format == GNUTLS_X509_FMT_PEM)
			fprintf(outfile, "%.*s\n", buf.size, buf.data);

		fwrite(rbuf.data, 1, rbuf.size, outfile);

		if (outcert_format == GNUTLS_X509_FMT_PEM)
			fprintf(outfile, "\n");
		gnutls_free(rbuf.data);
	}

	if (force_print || !HAVE_OPT(OUTFILE)) {
		ret = gnutls_ocsp_resp_export2(resp, &rbuf, GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fprintf(stderr, "error exporting response: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		fprintf(stdout, "%.*s\n", buf.size, buf.data);
		fwrite(rbuf.data, 1, rbuf.size, stdout);
		gnutls_free(rbuf.data);
	}
	gnutls_free(buf.data);

	gnutls_ocsp_resp_deinit(resp);
}

static void response_info(void)
{
	gnutls_datum_t dat;
	size_t size;

	if (HAVE_OPT(LOAD_RESPONSE))
		dat.data =
		    (void *) read_binary_file(OPT_ARG(LOAD_RESPONSE),
					      &size);
	else
		dat.data = (void *) fread_file(infile, &size);
	if (dat.data == NULL) {
		fprintf(stderr, "error reading response\n");
		app_exit(1);
	}
	dat.size = size;

	_response_info(&dat, 0);

	gnutls_free(dat.data);
}

static void generate_request(gnutls_datum_t *nonce)
{
	gnutls_datum_t dat;
	gnutls_x509_crt_t cert, issuer;
	common_info_st info;

	memset(&info, 0, sizeof(info));
	info.verbose = verbose;
	if (!HAVE_OPT(LOAD_CERT)) {
		fprintf(stderr, "Missing option --load-cert\n");
		app_exit(1);
	}
	info.cert = OPT_ARG(LOAD_CERT);

	cert = load_cert(1, &info);

	memset(&info, 0, sizeof(info));
	info.verbose = verbose;
	if (!HAVE_OPT(LOAD_ISSUER)) {
		fprintf(stderr, "Missing option --load-issuer\n");
		app_exit(1);
	}
	info.cert = OPT_ARG(LOAD_ISSUER);

	issuer = load_cert(1, &info);

	_generate_request(cert, issuer, &dat, nonce);

	gnutls_x509_crt_deinit(cert);
	gnutls_x509_crt_deinit(issuer);
	fwrite(dat.data, 1, dat.size, outfile);

	gnutls_free(dat.data);
}


static int _verify_response(gnutls_datum_t * data, gnutls_datum_t * nonce,
	gnutls_x509_crt_t signer, unsigned print_resp)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	size_t size;
	gnutls_x509_crt_t *x509_ca_list = NULL;
	gnutls_x509_trust_list_t list;
	unsigned int x509_ncas = 0;
	unsigned verify;
	gnutls_datum_t dat;

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret < 0) {
		fprintf(stderr, "ocsp_resp_init: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	ret = gnutls_ocsp_resp_import(resp, data);
	if (ret < 0) {
		fprintf(stderr, "importing response: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	if (print_resp) {
		ret =
		    gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_COMPACT,
					   &dat);
		if (ret < 0) {
			fprintf(stderr, "ocsp_resp_print: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		printf("%s\n", dat.data);
		gnutls_free(dat.data);
	}

	if (nonce) {
		gnutls_datum_t rnonce;

		ret = gnutls_ocsp_resp_get_nonce(resp, NULL, &rnonce);
		if (ret < 0) {
			fprintf(stderr, "could not read response's nonce: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		if (rnonce.size != nonce->size || memcmp(nonce->data, rnonce.data,
			nonce->size) != 0) {
			fprintf(stderr, "nonce in the response doesn't match\n");
			app_exit(1);
		}

		gnutls_free(rnonce.data);
	}

	if (HAVE_OPT(LOAD_TRUST)) {
		dat.data =
		    (void *) read_binary_file(OPT_ARG(LOAD_TRUST), &size);
		if (dat.data == NULL) {
			fprintf(stderr, "error reading --load-trust: %s\n",
				OPT_ARG(LOAD_TRUST));
			app_exit(1);
		}
		dat.size = size;

		ret = gnutls_x509_trust_list_init(&list, 0);
		if (ret < 0) {
			fprintf(stderr, "gnutls_x509_trust_list_init: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		ret =
		    gnutls_x509_crt_list_import2(&x509_ca_list, &x509_ncas,
						 &dat, GNUTLS_X509_FMT_PEM,
						 0);
		if (ret < 0 || x509_ncas < 1) {
			fprintf(stderr, "error parsing CAs: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		if (HAVE_OPT(VERBOSE)) {
			unsigned int i;
			printf("Trust anchors:\n");
			for (i = 0; i < x509_ncas; i++) {
				gnutls_datum_t out;

				ret =
				    gnutls_x509_crt_print(x509_ca_list[i],
							  GNUTLS_CRT_PRINT_ONELINE,
							  &out);
				if (ret < 0) {
					fprintf(stderr,
						"gnutls_x509_crt_print: %s\n",
						gnutls_strerror(ret));
					app_exit(1);
				}

				printf("%d: %.*s\n", i, out.size,
				       out.data);
				gnutls_free(out.data);
			}
			printf("\n");
		}

		ret =
		    gnutls_x509_trust_list_add_cas(list, x509_ca_list,
						   x509_ncas, 0);
		if (ret < 0) {
			fprintf(stderr, "gnutls_x509_trust_add_cas: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		if (HAVE_OPT(VERBOSE))
			fprintf(stdout, "Loaded %d trust anchors\n",
				x509_ncas);

		ret = gnutls_ocsp_resp_verify(resp, list, &verify, vflags);
		if (ret < 0) {
			fprintf(stderr, "gnutls_ocsp_resp_verify: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}
	} else if (signer) {
		if (HAVE_OPT(VERBOSE)) {
			gnutls_datum_t out;

			ret =
			    gnutls_x509_crt_print(signer,
						  GNUTLS_CRT_PRINT_ONELINE,
						  &out);
			if (ret < 0) {
				fprintf(stderr,
					"gnutls_x509_crt_print: %s\n",
					gnutls_strerror(ret));
				app_exit(1);
			}

			printf("Signer: %.*s\n", out.size, out.data);
			gnutls_free(out.data);
			printf("\n");
		}

		ret =
		    gnutls_ocsp_resp_verify_direct(resp, signer, &verify,
						   vflags);
		if (ret < 0) {
			fprintf(stderr,
				"\nVerifying OCSP Response: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}
	} else {
		fprintf(stderr, "missing --load-trust or --load-signer\n");
		app_exit(1);
	}

	printf("\nVerifying OCSP Response: ");
	print_ocsp_verify_res(verify);
	printf(".\n");

	gnutls_ocsp_resp_deinit(resp);

	return verify;
}

#define MAX_CHAIN_SIZE 8

static
unsigned load_chain(gnutls_x509_crt_t chain[MAX_CHAIN_SIZE])
{
	if (HAVE_OPT(LOAD_CHAIN)) {
		common_info_st info;
		size_t list_size;

		memset(&info, 0, sizeof(info));
		gnutls_x509_crt_t *list;
		unsigned i;

		info.verbose = verbose;
		info.cert = OPT_ARG(LOAD_CHAIN);
		info.sort_chain = 1;
		list = load_cert_list(1, &list_size, &info);

		if (list_size > MAX_CHAIN_SIZE) {
			fprintf(stderr, "Too many certificates in chain\n");
			app_exit(1);
		}

		for (i=0;i<list_size;i++)
			chain[i] = list[i];
		gnutls_free(list);
		return list_size;
	} else {
		common_info_st info;

		memset(&info, 0, sizeof(info));
		info.verbose = verbose;
		if (!HAVE_OPT(LOAD_CERT)) {
			fprintf(stderr, "Missing option --load-cert\n");
			app_exit(1);
		}
		info.cert = OPT_ARG(LOAD_CERT);

		chain[0] = load_cert(1, &info);

		memset(&info, 0, sizeof(info));
		info.verbose = verbose;
		if (!HAVE_OPT(LOAD_ISSUER)) {
			fprintf(stderr, "Missing option --load-issuer\n");
			app_exit(1);
		}
		info.cert = OPT_ARG(LOAD_ISSUER);

		chain[1] = load_cert(1, &info);
		return 2;
	}
}

static void verify_response(gnutls_datum_t *nonce)
{
	gnutls_datum_t dat;
	size_t size;
	gnutls_x509_crt_t signer;
	common_info_st info;
	int v;
	gnutls_x509_crt_t chain[MAX_CHAIN_SIZE];
	unsigned chain_size = 0, i;

	if (HAVE_OPT(LOAD_RESPONSE))
		dat.data =
		    (void *) read_binary_file(OPT_ARG(LOAD_RESPONSE),
					      &size);
	else
		dat.data = (void *) fread_file(infile, &size);
	if (dat.data == NULL) {
		fprintf(stderr, "error reading response\n");
		app_exit(1);
	}
	dat.size = size;

	if (HAVE_OPT(LOAD_CHAIN)) {
		chain_size = load_chain(chain);
		if (chain_size < 1) {
			fprintf(stderr, "Empty chain found; cannot verify\n");
			app_exit(1);
		}

		if (chain_size == 1)
			signer = chain[0];
		else
			signer = chain[1];

		v = _verify_response(&dat, nonce, signer, 1);

		for (i=0;i<chain_size;i++)
			gnutls_x509_crt_deinit(chain[i]);
	} else if (HAVE_OPT(LOAD_TRUST)) {
		v = _verify_response(&dat, nonce, NULL, 1);
	} else {
		memset(&info, 0, sizeof(info));
		info.verbose = verbose;
		if (!HAVE_OPT(LOAD_SIGNER)) {
			fprintf(stderr, "Missing option --load-signer or --load-chain\n");
			app_exit(1);
		}
		info.cert = OPT_ARG(LOAD_SIGNER);

		signer = load_cert(1, &info);

		v = _verify_response(&dat, nonce, signer, 1);
		gnutls_x509_crt_deinit(signer);
	}

	free(dat.data);

	if (v && !HAVE_OPT(IGNORE_ERRORS))
		app_exit(1);
}

static void ask_server(const char *url)
{
	gnutls_datum_t resp_data;
	int ret, v = 0, total_v = 0;
	unsigned char noncebuf[23];
	gnutls_datum_t nonce = { noncebuf, sizeof(noncebuf) };
	gnutls_datum_t *n;
	gnutls_x509_crt_t chain[MAX_CHAIN_SIZE];
	unsigned chain_size, counter;
	unsigned idx = 0;
	common_info_st info;

	chain_size = load_chain(chain);

	if (chain_size > 2 && HAVE_OPT(OUTFILE)) {
		if (outcert_format != GNUTLS_X509_FMT_PEM) {
			fprintf(stderr, "error: You cannot combine --outfile when more than 2 certificates are found in a chain\n");
			fprintf(stderr, "Did you mean to use --outpem?\n");
			app_exit(1);
		}
	}

	counter = chain_size;
	while(counter > 1) {
		if (ENABLED_OPT(NONCE)) {
			ret =
			    gnutls_rnd(GNUTLS_RND_NONCE, nonce.data, nonce.size);
			if (ret < 0) {
				fprintf(stderr, "gnutls_rnd: %s\n",
					gnutls_strerror(ret));
				app_exit(1);
			}
			n = &nonce;
		} else {
			n = NULL;
		}

		ret =
		    send_ocsp_request(url, chain[idx], chain[idx+1], &resp_data, n);
		if (ret < 0) {
			fprintf(stderr, "Cannot send OCSP request\n");
			app_exit(1);
		}

		_response_info(&resp_data, 1);

		if (HAVE_OPT(LOAD_TRUST)) {
			v = _verify_response(&resp_data, n, NULL, 0);
		} else if (HAVE_OPT(LOAD_SIGNER)) {
			memset(&info, 0, sizeof(info));
			info.verbose = verbose;
			info.cert = OPT_ARG(LOAD_SIGNER);

			v = _verify_response(&resp_data, n, load_cert(1, &info), 0);
		} else {
			if (!HAVE_OPT(LOAD_CHAIN))
				fprintf(stderr,
					"\nAssuming response's signer = issuer (use --load-signer to override).\n");

			v = _verify_response(&resp_data, n, chain[idx+1], 0);
		}

		total_v += v;

		free(resp_data.data);
		idx++;
		counter--;
		printf("\n");
	}

	for (idx = 0;idx<chain_size;idx++) {
		gnutls_x509_crt_deinit(chain[idx]);
	}

	if (total_v && !HAVE_OPT(IGNORE_ERRORS))
		app_exit(1);
}

int main(int argc, char **argv)
{
	int ret;

	if ((ret = gnutls_global_init()) < 0) {
		fprintf(stderr, "global_init: %s\n", gnutls_strerror(ret));
		app_exit(1);
	}

	optionProcess(&ocsptoolOptions, argc, argv);

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(OPT_VALUE_DEBUG);

	if (ENABLED_OPT(INDER))
		incert_format = GNUTLS_X509_FMT_DER;
	else
		incert_format = GNUTLS_X509_FMT_PEM;

	if (HAVE_OPT(OUTPEM))
		outcert_format = GNUTLS_X509_FMT_PEM;
	else
		outcert_format = GNUTLS_X509_FMT_DER;

	if (HAVE_OPT(VERIFY_ALLOW_BROKEN))
		vflags |= GNUTLS_VERIFY_ALLOW_BROKEN;

	if (HAVE_OPT(OUTFILE)) {
		outfile = fopen(OPT_ARG(OUTFILE), "wb");
		if (outfile == NULL) {
			fprintf(stderr, "%s\n", OPT_ARG(OUTFILE));
			app_exit(1);
		}
		outfile_name = OPT_ARG(OUTFILE);
	} else
		outfile = stdout;

	if (HAVE_OPT(INFILE)) {
		infile = fopen(OPT_ARG(INFILE), "rb");
		if (infile == NULL) {
			fprintf(stderr, "%s\n", OPT_ARG(INFILE));
			app_exit(1);
		}
	} else
		infile = stdin;

	if (ENABLED_OPT(INDER))
		encoding = GNUTLS_X509_FMT_DER;
	else
		encoding = GNUTLS_X509_FMT_PEM;

	if (HAVE_OPT(REQUEST_INFO))
		request_info();
	else if (HAVE_OPT(RESPONSE_INFO))
		response_info();
	else if (HAVE_OPT(GENERATE_REQUEST))
		generate_request(NULL);
	else if (HAVE_OPT(VERIFY_RESPONSE))
		verify_response(NULL);
	else if (HAVE_OPT(ASK)) {
		if ((!HAVE_OPT(LOAD_CERT)) && (!HAVE_OPT(LOAD_CHAIN))) {
			fprintf(stderr, "This option required --load-chain or --load-cert\n");
			app_exit(1);
		}
		ask_server(OPT_ARG(ASK));
	} else {
		USAGE(1);
	}

	if (infile != stdin)
		fclose(infile);
	gnutls_global_deinit();

	return 0;
}
