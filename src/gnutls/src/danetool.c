/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/openpgp.h>
#include <gnutls/pkcs12.h>
#include <gnutls/pkcs11.h>
#include <gnutls/abstract.h>
#include <gnutls/crypto.h>

#ifdef HAVE_DANE
#include <gnutls/dane.h>
#endif

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
#include <minmax.h>

#include <common.h>
#include "danetool-args.h"
#include "certtool-common.h"
#include "socket.h"

static const char *obtain_cert(const char *hostname, const char *proto, const char *service,
				const char *app_proto, unsigned quiet);
static void cmd_parser(int argc, char **argv);
static void dane_info(const char *host, const char *proto,
		      const char *service, unsigned int ca,
		      unsigned int domain, common_info_st * cinfo);

static void dane_check(const char *host, const char *proto,
		       const char *service, common_info_st * cinfo);

FILE *outfile;
static const char *outfile_name = NULL;
static gnutls_digest_algorithm_t default_dig;

/* non interactive operation if set
 */
int batch = 0;
int ask_pass = 0;

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

int main(int argc, char **argv)
{
	fix_lbuffer(0);
	cmd_parser(argc, argv);

	return 0;
}


static void cmd_parser(int argc, char **argv)
{
	int ret, privkey_op = 0;
	common_info_st cinfo;
	const char *proto = "tcp";
	char service[32] = "443";

	optionProcess(&danetoolOptions, argc, argv);

	if (HAVE_OPT(OUTFILE)) {
		outfile = safe_open_rw(OPT_ARG(OUTFILE), privkey_op);
		if (outfile == NULL) {
			fprintf(stderr, "%s", OPT_ARG(OUTFILE));
			app_exit(1);
		}
		outfile_name = OPT_ARG(OUTFILE);
	} else
		outfile = stdout;

	default_dig = GNUTLS_DIG_UNKNOWN;
	if (HAVE_OPT(HASH)) {
		if (strcasecmp(OPT_ARG(HASH), "md5") == 0) {
			fprintf(stderr,
				"Warning: MD5 is broken, and should not be used any more for digital signatures.\n");
			default_dig = GNUTLS_DIG_MD5;
		} else if (strcasecmp(OPT_ARG(HASH), "sha1") == 0)
			default_dig = GNUTLS_DIG_SHA1;
		else if (strcasecmp(OPT_ARG(HASH), "sha256") == 0)
			default_dig = GNUTLS_DIG_SHA256;
		else if (strcasecmp(OPT_ARG(HASH), "sha224") == 0)
			default_dig = GNUTLS_DIG_SHA224;
		else if (strcasecmp(OPT_ARG(HASH), "sha384") == 0)
			default_dig = GNUTLS_DIG_SHA384;
		else if (strcasecmp(OPT_ARG(HASH), "sha512") == 0)
			default_dig = GNUTLS_DIG_SHA512;
		else if (strcasecmp(OPT_ARG(HASH), "rmd160") == 0)
			default_dig = GNUTLS_DIG_RMD160;
		else {
			fprintf(stderr, "invalid hash: %s", OPT_ARG(HASH));
			app_exit(1);
		}
	}

	gnutls_global_set_log_function(tls_log_func);

	if (HAVE_OPT(DEBUG)) {
		gnutls_global_set_log_level(OPT_VALUE_DEBUG);
		printf("Setting log level to %d\n", (int) OPT_VALUE_DEBUG);
	}

	if ((ret = gnutls_global_init()) < 0) {
		fprintf(stderr, "global_init: %s", gnutls_strerror(ret));
		app_exit(1);
	}
#ifdef ENABLE_PKCS11
	pkcs11_common(NULL);
#endif

	memset(&cinfo, 0, sizeof(cinfo));

	if (HAVE_OPT(INDER) || HAVE_OPT(INRAW))
		cinfo.incert_format = GNUTLS_X509_FMT_DER;
	else
		cinfo.incert_format = GNUTLS_X509_FMT_PEM;

	if (HAVE_OPT(VERBOSE))
		cinfo.verbose = 1;

	if (HAVE_OPT(LOAD_PUBKEY))
		cinfo.pubkey = OPT_ARG(LOAD_PUBKEY);

	if (HAVE_OPT(LOAD_CERTIFICATE))
		cinfo.cert = OPT_ARG(LOAD_CERTIFICATE);

	if (HAVE_OPT(PORT)) {
		snprintf(service, sizeof(service), "%s", OPT_ARG(PORT));
	} else {
		if (HAVE_OPT(STARTTLS_PROTO))
			snprintf(service, sizeof(service), "%s", starttls_proto_to_service(OPT_ARG(STARTTLS_PROTO)));
	}

	if (HAVE_OPT(PROTO))
		proto = OPT_ARG(PROTO);

	if (HAVE_OPT(TLSA_RR))
		dane_info(OPT_ARG(HOST), proto, service,
			  HAVE_OPT(CA), ENABLED_OPT(DOMAIN), &cinfo);
	else if (HAVE_OPT(CHECK))
		dane_check(OPT_ARG(CHECK), proto, service, &cinfo);
	else
		USAGE(1);

	fclose(outfile);

#ifdef ENABLE_PKCS11
	gnutls_pkcs11_deinit();
#endif
	gnutls_global_deinit();
}

#define MAX_CLIST_SIZE 32
static void dane_check(const char *host, const char *proto,
		       const char *service, common_info_st * cinfo)
{
#ifdef HAVE_DANE
	dane_state_t s;
	dane_query_t q;
	int ret, retcode = 1;
	unsigned entries;
	unsigned int flags = DANE_F_IGNORE_LOCAL_RESOLVER, i;
	unsigned int usage, type, match;
	gnutls_datum_t data, file;
	size_t size;
	unsigned del = 0;
	unsigned vflags = DANE_VFLAG_FAIL_IF_NOT_CHECKED;
	const char *cstr;
	char *str;
	gnutls_x509_crt_t *clist = NULL;
	unsigned int clist_size = 0;
	gnutls_datum_t certs[MAX_CLIST_SIZE];
	int port = service_to_port(service, proto);

	if (ENABLED_OPT(LOCAL_DNS))
		flags = 0;

	if (HAVE_OPT(INSECURE))
		flags |= DANE_F_INSECURE;

	if (HAVE_OPT(CHECK_EE))
		vflags |= DANE_VFLAG_ONLY_CHECK_EE_USAGE;

	if (HAVE_OPT(CHECK_CA))
		vflags |= DANE_VFLAG_ONLY_CHECK_CA_USAGE;

	if (!cinfo->cert) {
		const char *app_proto = NULL;
		if (HAVE_OPT(STARTTLS_PROTO))
			app_proto = OPT_ARG(STARTTLS_PROTO);

		cinfo->cert = obtain_cert(host, proto, service, app_proto, HAVE_OPT(QUIET));
		del = 1;
	}

	if (!HAVE_OPT(QUIET))
		fprintf(stderr, "Querying DNS for %s (%s:%d)...\n", host, proto, port);
	ret = dane_state_init(&s, flags);
	if (ret < 0) {
		fprintf(stderr, "dane_state_init: %s\n",
			dane_strerror(ret));
		retcode = 1;
		goto error;
	}

	if (HAVE_OPT(DLV)) {
		ret = dane_state_set_dlv_file(s, OPT_ARG(DLV));
		if (ret < 0) {
			fprintf(stderr, "dane_state_set_dlv_file: %s\n",
				dane_strerror(ret));
			retcode = 1;
			goto error;
		}
	}

	ret = dane_query_tlsa(s, &q, host, proto, port);
	if (ret < 0) {
		fprintf(stderr, "dane_query_tlsa: %s\n",
			dane_strerror(ret));
		retcode = 1;
		goto error;
	}

	if (ENABLED_OPT(PRINT_RAW)) {
		gnutls_datum_t t;
		char **dane_data;
		int *dane_data_len;
		int secure;
		int bogus;

		ret = dane_query_to_raw_tlsa(q, &entries, &dane_data,
			&dane_data_len, &secure, &bogus);
		if (ret < 0) {
			fprintf(stderr, "dane_query_to_raw_tlsa: %s\n",
				dane_strerror(ret));
			retcode = 1;
			goto error;
		}

		for (i=0;i<entries;i++) {
			size_t str_size;
			t.data = (void*)dane_data[i];
			t.size = dane_data_len[i];

			str_size = t.size * 2 + 1;
			str = gnutls_malloc(str_size);

			ret = gnutls_hex_encode(&t, str, &str_size);
			if (ret < 0) {
				fprintf(stderr, "gnutls_hex_encode: %s\n",
					dane_strerror(ret));
				retcode = 1;
				goto error;
			}
			fprintf(outfile, "[%u]: %s\n", i, str);
			gnutls_free(str);
		}
		fprintf(outfile, "\n");
	}

	if (cinfo->cert) {
		ret = gnutls_load_file(cinfo->cert, &file);
		if (ret < 0) {
			fprintf(stderr, "gnutls_load_file: %s\n",
				gnutls_strerror(ret));
			retcode = 1;
			goto error;
		}

		ret =
		    gnutls_x509_crt_list_import2(&clist,
						 &clist_size,
						 &file,
						 cinfo->
						 incert_format, 0);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crt_list_import2: %s\n",
				gnutls_strerror(ret));
			retcode = 1;
			goto error;
		}

		if (clist_size > 0) {
			for (i = 0; i < MIN(MAX_CLIST_SIZE,clist_size); i++) {
				ret =
				    gnutls_x509_crt_export2(clist
							    [i],
							    GNUTLS_X509_FMT_DER,
							    &certs
							    [i]);
				if (ret < 0) {
					fprintf(stderr,
						"gnutls_x509_crt_export2: %s\n",
						gnutls_strerror
						(ret));
					retcode = 1;
					goto error;
				}
			}
		}
	}

	entries = dane_query_entries(q);
	for (i = 0; i < entries; i++) {
		ret = dane_query_data(q, i, &usage, &type, &match, &data);
		if (ret < 0) {
			fprintf(stderr, "dane_query_data: %s\n",
				dane_strerror(ret));
			retcode = 1;
			goto error;
		}

		size = lbuffer_size;
		ret = gnutls_hex_encode(&data, (void *) lbuffer, &size);
		if (ret < 0) {
			fprintf(stderr, "gnutls_hex_encode: %s\n",
				dane_strerror(ret));
			retcode = 1;
			goto error;
		}

		if (entries > 1 && !HAVE_OPT(QUIET))
			fprintf(outfile, "\n==== Entry %d ====\n", i + 1);

		fprintf(outfile,
			"_%u._%s.%s. IN TLSA ( %.2x %.2x %.2x %s )\n",
			port, proto, host, usage, type, match, lbuffer);

		if (!HAVE_OPT(QUIET)) {
			cstr = dane_cert_usage_name(usage);
			if (cstr == NULL) cstr= "Unknown";
			fprintf(outfile, "Certificate usage: %s (%.2x)\n", cstr, usage);

			cstr = dane_cert_type_name(type);
			if (cstr == NULL) cstr= "Unknown";
			fprintf(outfile, "Certificate type:  %s (%.2x)\n", cstr, type);

			cstr = dane_match_type_name(match);
			if (cstr == NULL) cstr= "Unknown";
			fprintf(outfile, "Contents:	  %s (%.2x)\n", cstr, match);
			fprintf(outfile, "Data:	      %s\n", lbuffer);
		}

		/* Verify the DANE data */
		if (cinfo->cert) {
			unsigned int status;
			gnutls_datum_t out;

			ret =
			    dane_verify_crt(s, certs, clist_size,
					    GNUTLS_CRT_X509, host,
					    proto, port, 0, vflags,
					    &status);
			if (ret < 0) {
				fprintf(stderr,
					"dane_verify_crt: %s\n",
					dane_strerror(ret));
				retcode = 1;
				goto error;
			}

			ret =
			    dane_verification_status_print(status,
							   &out,
							   0);
			if (ret < 0) {
				fprintf(stderr,
					"dane_verification_status_print: %s\n",
					dane_strerror(ret));
				retcode = 1;
				goto error;
			}

			if (!HAVE_OPT(QUIET))
				fprintf(outfile, "\nVerification: %s\n", out.data);
			gnutls_free(out.data);

			/* if there is at least one correct accept */
			if (status == 0)
				retcode = 0;
		} else {
			fprintf(stderr,
				"\nCertificate could not be obtained. You can explicitly load the certificate using --load-certificate.\n");
		}
	}

	if (clist_size > 0) {
		for (i = 0; i < clist_size; i++) {
			gnutls_free(certs[i].data);
			gnutls_x509_crt_deinit(clist[i]);
		}
		gnutls_free(clist);
	}



	dane_query_deinit(q);
	dane_state_deinit(s);

 error:
	if (del != 0 && cinfo->cert) {
		(void)remove(cinfo->cert);
	}

	app_exit(retcode);
#else
	fprintf(stderr,
		"This functionality is disabled (GnuTLS was not compiled with support for DANE).\n");
	return;
#endif
}

static void dane_info(const char *host, const char *proto,
		      const char *service, unsigned int ca,
		      unsigned int domain, common_info_st * cinfo)
{
	gnutls_pubkey_t pubkey;
	gnutls_x509_crt_t crt;
	unsigned char digest[64];
	gnutls_datum_t t;
	int ret;
	unsigned int usage, selector, type;
	size_t size;
	int port = service_to_port(service, proto);

	if (proto == NULL)
		proto = "tcp";

	crt = load_cert(0, cinfo);
	if (crt != NULL && HAVE_OPT(X509)) {
		selector = 0;	/* X.509 */

		size = lbuffer_size;
		ret =
		    gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_DER,
					   lbuffer, &size);
		if (ret < 0) {
			fprintf(stderr, "export error: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		gnutls_x509_crt_deinit(crt);
	} else {		/* use public key only */

		selector = 1;

		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0) {
			fprintf(stderr, "pubkey_init: %s\n",
				gnutls_strerror(ret));
			app_exit(1);
		}

		if (crt != NULL) {

			ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
			if (ret < 0) {
				fprintf(stderr, "pubkey_import_x509: %s\n",
					gnutls_strerror(ret));
				app_exit(1);
			}

			size = lbuffer_size;
			ret =
			    gnutls_pubkey_export(pubkey,
						 GNUTLS_X509_FMT_DER,
						 lbuffer, &size);
			if (ret < 0) {
				fprintf(stderr, "pubkey_export: %s\n",
					gnutls_strerror(ret));
				app_exit(1);
			}

			gnutls_x509_crt_deinit(crt);
		} else {
			pubkey = load_pubkey(1, cinfo);

			size = lbuffer_size;
			ret =
			    gnutls_pubkey_export(pubkey,
						 GNUTLS_X509_FMT_DER,
						 lbuffer, &size);
			if (ret < 0) {
				fprintf(stderr, "export error: %s\n",
					gnutls_strerror(ret));
				app_exit(1);
			}
		}

		gnutls_pubkey_deinit(pubkey);
	}

	if (default_dig != GNUTLS_DIG_SHA256
	    && default_dig != GNUTLS_DIG_SHA512) {
		if (default_dig != GNUTLS_DIG_UNKNOWN)
			fprintf(stderr,
				"Unsupported digest. Assuming SHA256.\n");
		default_dig = GNUTLS_DIG_SHA256;
	}

	ret = gnutls_hash_fast(default_dig, lbuffer, size, digest);
	if (ret < 0) {
		fprintf(stderr, "hash error: %s\n", gnutls_strerror(ret));
		app_exit(1);
	}

	if (default_dig == GNUTLS_DIG_SHA256)
		type = 1;
	else
		type = 2;

	/* DANE certificate classification crap */
	if (domain == 0) {
		if (ca)
			usage = 0;
		else
			usage = 1;
	} else {
		if (ca)
			usage = 2;
		else
			usage = 3;
	}

	t.data = digest;
	t.size = gnutls_hash_get_len(default_dig);

	size = lbuffer_size;
	ret = gnutls_hex_encode(&t, (void *) lbuffer, &size);
	if (ret < 0) {
		fprintf(stderr, "hex encode error: %s\n",
			gnutls_strerror(ret));
		app_exit(1);
	}

	fprintf(outfile, "_%u._%s.%s. IN TLSA ( %.2x %.2x %.2x %s )\n",
		port, proto, host, usage, selector, type, lbuffer);

}


struct priv_st {
	int fd;
	int found;
};

#ifdef HAVE_DANE
static int cert_callback(gnutls_session_t session)
{
	const gnutls_datum_t *cert_list;
	unsigned int cert_list_size = 0;
	int ret;
	unsigned i;
	gnutls_datum_t t;
	struct priv_st *priv;

	cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
	if (cert_list_size == 0) {
		fprintf(stderr, "no certificates sent by server!\n");
		return -1;
	}

	priv = gnutls_session_get_ptr(session);

	for (i=0;i<cert_list_size;i++) {
		ret = gnutls_pem_base64_encode_alloc("CERTIFICATE", &cert_list[i], &t);
		if (ret < 0) {
			fprintf(stderr, "error[%d]: %s\n", __LINE__,
				gnutls_strerror(ret));
			app_exit(1);
		}

		write(priv->fd, t.data, t.size);
		gnutls_free(t.data);
	}
	priv->found = 1;

	return -1;
}

static gnutls_certificate_credentials_t xcred;
static int file_fd = -1;
static unsigned udp = 0;

gnutls_session_t init_tls_session(const char *hostname)
{
	gnutls_session_t session;
	int ret;
	static struct priv_st priv;

	priv.found = 0;
	priv.fd = file_fd;

	ret = gnutls_init(&session, (udp?GNUTLS_DATAGRAM:0)|GNUTLS_CLIENT);
	if (ret < 0) {
		fprintf(stderr, "error[%d]: %s\n", __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}
	gnutls_session_set_ptr(session, &priv);

	ret = gnutls_set_default_priority(session);
	if (ret < 0) {
		fprintf(stderr, "error[%d]: %s\n", __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}

	if (hostname && is_ip(hostname)==0) {
		gnutls_server_name_set(session, GNUTLS_NAME_DNS, hostname, strlen(hostname));
	}
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	return session;
}

int do_handshake(socket_st * socket)
{
	int ret;

	do {
		ret = gnutls_handshake(socket->session);
	} while(ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_WARNING_ALERT_RECEIVED);
	/* we don't care on the result */

	return 0;
}


static const char *obtain_cert(const char *hostname, const char *proto, const char *service,
				const char *app_proto, unsigned quiet)
{
	socket_st hd;
	const char *txt_service;
	static char tmpfile[32];
	int ret;
	const char *str = "Obtaining certificate from";
	int socket_flags = 0;
	struct priv_st *priv;

	ret = gnutls_certificate_allocate_credentials(&xcred);
	if (ret < 0) {
		fprintf(stderr, "error[%d]: %s\n", __LINE__,
			gnutls_strerror(ret));
		app_exit(1);
	}
	gnutls_certificate_set_verify_function(xcred, cert_callback);

	if (strcmp(proto, "udp") == 0)
		udp = 1;
	else if (strcmp(proto, "tcp") != 0) {
		/* we cannot handle this protocol */
		return NULL;
	}

	strcpy(tmpfile, "danetool-certXXXXXX");

	sockets_init();
	txt_service = port_to_service(service, proto);

	if (quiet)
		str = NULL;

	if (app_proto == NULL) app_proto = txt_service;

	if (udp)
		socket_flags |= SOCKET_FLAG_UDP;
	

	umask(066);
	file_fd = mkstemp(tmpfile);
	if (file_fd == -1) {
		int e = errno;
		fprintf(stderr, "error[%d]: %s\n", __LINE__,
			strerror(e));
		app_exit(1);
	}

	socket_open(&hd, hostname, txt_service, app_proto, socket_flags|SOCKET_FLAG_STARTTLS, str, NULL);

	close(file_fd);

	ret = 0;
	priv = gnutls_session_get_ptr(hd.session);
	if (priv->found == 0)
		ret = -1;

	socket_bye(&hd, 1);
	gnutls_certificate_free_credentials(xcred);

	if (ret == -1)
		return NULL;
	else
		return tmpfile;
}
#endif
