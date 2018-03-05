/*
 * Copyright (C) 2011-2014 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gnutls/gnutls.h>
#include <gnutls/ocsp.h>
#include <gnutls/x509.h>
#include <gnutls/crypto.h>

/* Gnulib portability files. */
#include <read-file.h>
#include <socket.h>

#include <ocsptool-common.h>
#include <ocsptool-args.h>

FILE *outfile;
FILE *infile;
static unsigned int encoding;
unsigned int verbose = 0;

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
	gnutls_datum_t dat;
	size_t size;

	ret = gnutls_ocsp_req_init(&req);
	if (ret < 0) {
		fprintf(stderr, "ocsp_req_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (HAVE_OPT(LOAD_REQUEST))
		dat.data =
		    (void *) read_binary_file(OPT_ARG(LOAD_REQUEST),
					      &size);
	else
		dat.data = (void *) fread_file(infile, &size);
	if (dat.data == NULL) {
		fprintf(stderr, "error reading request\n");
		exit(1);
	}
	dat.size = size;

	ret = gnutls_ocsp_req_import(req, &dat);
	free(dat.data);
	if (ret < 0) {
		fprintf(stderr, "error importing request: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_ocsp_req_print(req, GNUTLS_OCSP_PRINT_FULL, &dat);
	if (ret != 0) {
		fprintf(stderr, "ocsp_req_print: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	printf("%.*s", dat.size, dat.data);
	gnutls_free(dat.data);

	gnutls_ocsp_req_deinit(req);
}

static void _response_info(const gnutls_datum_t * data)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	gnutls_datum_t buf;

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret < 0) {
		fprintf(stderr, "ocsp_resp_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_ocsp_resp_import(resp, data);
	if (ret < 0) {
		fprintf(stderr, "importing response: %s\n",
			gnutls_strerror(ret));
		exit(1);
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
		exit(1);
	}

	printf("%.*s", buf.size, buf.data);
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
		exit(1);
	}
	dat.size = size;

	_response_info(&dat);
	gnutls_free(dat.data);
}

static gnutls_x509_crt_t load_issuer(void)
{
	gnutls_x509_crt_t crt;
	int ret;
	gnutls_datum_t dat;
	size_t size;

	if (!HAVE_OPT(LOAD_ISSUER)) {
		fprintf(stderr, "missing --load-issuer\n");
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr, "crt_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	dat.data = (void *) read_binary_file(OPT_ARG(LOAD_ISSUER), &size);
	dat.size = size;

	if (!dat.data) {
		fprintf(stderr, "error reading --load-issuer: %s\n",
			OPT_ARG(LOAD_ISSUER));
		exit(1);
	}

	ret = gnutls_x509_crt_import(crt, &dat, encoding);
	free(dat.data);
	if (ret < 0) {
		fprintf(stderr, "error importing --load-issuer: %s: %s\n",
			OPT_ARG(LOAD_ISSUER), gnutls_strerror(ret));
		exit(1);
	}

	return crt;
}

static gnutls_x509_crt_t load_signer(void)
{
	gnutls_x509_crt_t crt;
	int ret;
	gnutls_datum_t dat;
	size_t size;

	if (!HAVE_OPT(LOAD_SIGNER)) {
		fprintf(stderr, "missing --load-signer\n");
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr, "crt_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	dat.data = (void *) read_binary_file(OPT_ARG(LOAD_SIGNER), &size);
	dat.size = size;

	if (!dat.data) {
		fprintf(stderr, "error reading --load-signer: %s\n",
			OPT_ARG(LOAD_SIGNER));
		exit(1);
	}

	ret = gnutls_x509_crt_import(crt, &dat, encoding);
	free(dat.data);
	if (ret < 0) {
		fprintf(stderr, "importing --load-signer: %s: %s\n",
			OPT_ARG(LOAD_SIGNER), gnutls_strerror(ret));
		exit(1);
	}

	return crt;
}

static gnutls_x509_crt_t load_cert(void)
{
	gnutls_x509_crt_t crt;
	int ret;
	gnutls_datum_t dat;
	size_t size;

	if (!HAVE_OPT(LOAD_CERT)) {
		fprintf(stderr, "missing --load-cert\n");
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr, "crt_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	dat.data = (void *) read_binary_file(OPT_ARG(LOAD_CERT), &size);
	dat.size = size;

	if (!dat.data) {
		fprintf(stderr, "error reading --load-cert: %s\n",
			OPT_ARG(LOAD_CERT));
		exit(1);
	}

	ret = gnutls_x509_crt_import(crt, &dat, encoding);
	free(dat.data);
	if (ret < 0) {
		fprintf(stderr, "importing --load-cert: %s: %s\n",
			OPT_ARG(LOAD_CERT), gnutls_strerror(ret));
		exit(1);
	}

	return crt;
}

static void generate_request(gnutls_datum_t *nonce)
{
	gnutls_datum_t dat;
	gnutls_x509_crt_t cert, issuer;

	cert = load_cert();
	issuer = load_issuer();

	_generate_request(cert, issuer, &dat, nonce);

	gnutls_x509_crt_deinit(cert);
	gnutls_x509_crt_deinit(issuer);
	fwrite(dat.data, 1, dat.size, outfile);

	gnutls_free(dat.data);
}


static int _verify_response(gnutls_datum_t * data, gnutls_datum_t * nonce,
	gnutls_x509_crt_t signer)
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
		exit(1);
	}

	ret = gnutls_ocsp_resp_import(resp, data);
	if (ret < 0) {
		fprintf(stderr, "importing response: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	if (nonce) {
		gnutls_datum_t rnonce;

		ret = gnutls_ocsp_resp_get_nonce(resp, NULL, &rnonce);
		if (ret < 0) {
			fprintf(stderr, "could not read response's nonce: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		if (rnonce.size != nonce->size || memcmp(nonce->data, rnonce.data,
			nonce->size) != 0) {
			fprintf(stderr, "nonce in the response doesn't match\n");
			exit(1);
		}

		gnutls_free(rnonce.data);
	}

	if (HAVE_OPT(LOAD_TRUST)) {
		dat.data =
		    (void *) read_binary_file(OPT_ARG(LOAD_TRUST), &size);
		if (dat.data == NULL) {
			fprintf(stderr, "error reading --load-trust: %s\n",
				OPT_ARG(LOAD_TRUST));
			exit(1);
		}
		dat.size = size;

		ret = gnutls_x509_trust_list_init(&list, 0);
		if (ret < 0) {
			fprintf(stderr, "gnutls_x509_trust_list_init: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		ret =
		    gnutls_x509_crt_list_import2(&x509_ca_list, &x509_ncas,
						 &dat, GNUTLS_X509_FMT_PEM,
						 0);
		if (ret < 0 || x509_ncas < 1) {
			fprintf(stderr, "error parsing CAs: %s\n",
				gnutls_strerror(ret));
			exit(1);
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
					exit(1);
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
			exit(1);
		}

		if (HAVE_OPT(VERBOSE))
			fprintf(stdout, "Loaded %d trust anchors\n",
				x509_ncas);

		ret = gnutls_ocsp_resp_verify(resp, list, &verify, 0);
		if (ret < 0) {
			fprintf(stderr, "gnutls_ocsp_resp_verify: %s\n",
				gnutls_strerror(ret));
			exit(1);
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
				exit(1);
			}

			printf("Signer: %.*s\n", out.size, out.data);
			gnutls_free(out.data);
			printf("\n");
		}

		ret =
		    gnutls_ocsp_resp_verify_direct(resp, signer, &verify,
						   0);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_ocsp_resp_verify_direct: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		fprintf(stderr, "missing --load-trust or --load-signer\n");
		exit(1);
	}

	printf("Verifying OCSP Response: ");
	print_ocsp_verify_res(verify);
	printf(".\n");

	gnutls_ocsp_resp_deinit(resp);

	return verify;
}

static void verify_response(gnutls_datum_t *nonce)
{
	gnutls_datum_t dat;
	size_t size;
	gnutls_x509_crt_t signer;
	int v;

	if (HAVE_OPT(LOAD_RESPONSE))
		dat.data =
		    (void *) read_binary_file(OPT_ARG(LOAD_RESPONSE),
					      &size);
	else
		dat.data = (void *) fread_file(infile, &size);
	if (dat.data == NULL) {
		fprintf(stderr, "error reading response\n");
		exit(1);
	}
	dat.size = size;

	signer = load_signer();

	v = _verify_response(&dat, nonce, signer);

	gnutls_x509_crt_deinit(signer);
	free(dat.data);

	if (v && !HAVE_OPT(IGNORE_ERRORS))
		exit(1);
}

static void ask_server(const char *url)
{
	gnutls_datum_t resp_data;
	int ret, v = 0;
	gnutls_x509_crt_t cert, issuer;
	unsigned char noncebuf[23];
	gnutls_datum_t nonce = { noncebuf, sizeof(noncebuf) };
	gnutls_datum_t *n;

	cert = load_cert();
	issuer = load_issuer();

	if (ENABLED_OPT(NONCE)) {
		ret =
		    gnutls_rnd(GNUTLS_RND_NONCE, nonce.data, nonce.size);
		if (ret < 0) {
			fprintf(stderr, "gnutls_rnd: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
		n = &nonce;

	} else {
		n = NULL;
	}

	ret =
	    send_ocsp_request(url, cert, issuer, &resp_data, n);
	if (ret < 0) {
		fprintf(stderr, "Cannot send OCSP request\n");
		exit(1);
	}

	_response_info(&resp_data);

	if (HAVE_OPT(LOAD_TRUST)) {
		v = _verify_response(&resp_data, n, NULL);
	} else if (HAVE_OPT(LOAD_SIGNER)) {
		v = _verify_response(&resp_data, n, load_signer());
	} else {
		fprintf(stderr,
			"\nAssuming response's signer = issuer (use --load-signer to override).\n");

		v = _verify_response(&resp_data, n, issuer);
	}

	if (HAVE_OPT(OUTFILE) && (v == 0 || HAVE_OPT(IGNORE_ERRORS))) {
		fwrite(resp_data.data, 1, resp_data.size, outfile);
	}

	free(resp_data.data);
	gnutls_x509_crt_deinit(issuer);
	gnutls_x509_crt_deinit(cert);

	if (v && !HAVE_OPT(IGNORE_ERRORS))
		exit(1);
}

int main(int argc, char **argv)
{
	int ret;

	if ((ret = gnutls_global_init()) < 0) {
		fprintf(stderr, "global_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	optionProcess(&ocsptoolOptions, argc, argv);

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(OPT_VALUE_DEBUG);

	if (HAVE_OPT(OUTFILE)) {
		outfile = fopen(OPT_ARG(OUTFILE), "wb");
		if (outfile == NULL) {
			fprintf(stderr, "%s\n", OPT_ARG(OUTFILE));
			exit(1);
		}
	} else
		outfile = stdout;

	if (HAVE_OPT(INFILE)) {
		infile = fopen(OPT_ARG(INFILE), "rb");
		if (infile == NULL) {
			fprintf(stderr, "%s\n", OPT_ARG(INFILE));
			exit(1);
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
	else if (HAVE_OPT(ASK))
		ask_server(OPT_ARG(ASK));
	else {
		USAGE(1);
	}

	if (infile != stdin)
		fclose(infile);
	gnutls_global_deinit();

	return 0;
}
