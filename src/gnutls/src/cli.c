/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2013-2016 Nikos Mavrogiannopoulos
 * Copyright (C) 2015-2017 Red Hat, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#include <sys/select.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <netdb.h>
#include <ctype.h>
#include <assert.h>

/* Get TCP_FASTOPEN */
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/dtls.h>
#include <gnutls/x509.h>
#include <gnutls/pkcs11.h>
#include <gnutls/crypto.h>
#include <gnutls/socket.h>

/* Gnulib portability files. */
#include <read-file.h>
#include <getpass.h>
#include <minmax.h>

#include "sockets.h"
#include "benchmark.h"
#include "inline_cmds.h"

#ifdef HAVE_DANE
#include <gnutls/dane.h>
#endif

#include <common.h>
#include <socket.h>

#include <cli-args.h>
#include <ocsptool-common.h>

#define MAX_BUF 4096

/* global stuff here */
int resume, starttls, insecure, ranges, rehandshake, udp, mtu,
    inline_commands;
unsigned int global_vflags = 0;
char *hostname = NULL;
char service[32]="";
int record_max_size;
int crlf;
int fastopen;
unsigned int verbose = 0;
int print_cert;

const char *srp_passwd = NULL;
const char *srp_username = NULL;
const char *x509_keyfile = NULL;
const char *x509_certfile = NULL;
const char *x509_cafile = NULL;
const char *x509_crlfile = NULL;
static int x509ctype;
const char *rawpk_keyfile = NULL;
const char *rawpk_file = NULL;
static int disable_extensions;
static int disable_sni;
static unsigned int init_flags = GNUTLS_CLIENT | GNUTLS_ENABLE_RAWPK;
static const char *priorities = NULL;
static const char *inline_commands_prefix;

const char *psk_username = NULL;
gnutls_datum_t psk_key = { NULL, 0 };

static gnutls_srp_client_credentials_t srp_cred;
static gnutls_psk_client_credentials_t psk_cred;
static gnutls_anon_client_credentials_t anon_cred;
static gnutls_certificate_credentials_t xcred;

/* end of global stuff */

/* prototypes */

static void check_server_cmd(socket_st * socket, int ret);
static void init_global_tls_stuff(void);
static int cert_verify_ocsp(gnutls_session_t session);

#define MAX_CRT 6
static unsigned int x509_crt_size;
static gnutls_pcert_st x509_crt[MAX_CRT];
static gnutls_privkey_t x509_key = NULL;
static gnutls_pcert_st rawpk;
static gnutls_privkey_t rawpk_key = NULL;


/* Load a PKCS #8, PKCS #12 private key or PKCS #11 URL
 */
static void load_priv_key(gnutls_privkey_t* privkey, const char* key_source)
{
	int ret;
	gnutls_datum_t data = { NULL, 0 };

	ret = gnutls_privkey_init(privkey);

	if (ret < 0) {
		fprintf(stderr, "*** Error initializing key: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	gnutls_privkey_set_pin_function(*privkey, pin_callback,
					NULL);

	if (gnutls_url_is_supported(key_source) != 0) {
		ret = gnutls_privkey_import_url(*privkey, key_source, 0);
		if (ret < 0) {
			fprintf(stderr,
				"*** Error loading url: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		ret = gnutls_load_file(key_source, &data);
		if (ret < 0) {
			fprintf(stderr,
				"*** Error loading key file.\n");
			exit(1);
		}

		ret = gnutls_privkey_import_x509_raw(*privkey, &data,
		                                     x509ctype, NULL, 0);
		if (ret < 0) {
			fprintf(stderr,
				"*** Error importing key: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		gnutls_free(data.data);
	}
}

/* Load the X509 certificate and the private key.
 */
static void load_x509_keys(void)
{
	unsigned int crt_num;
	int ret;
	unsigned int i;
	gnutls_datum_t data = { NULL, 0 };
	gnutls_x509_crt_t crt_list[MAX_CRT];

	if (x509_certfile != NULL && x509_keyfile != NULL) {
#ifdef ENABLE_PKCS11
		if (strncmp(x509_certfile, "pkcs11:", 7) == 0) {
			crt_num = 1;
			ret = gnutls_x509_crt_init(&crt_list[0]);
			if (ret < 0) {
				fprintf(stderr, "Memory error\n");
				exit(1);
			}
			gnutls_x509_crt_set_pin_function(crt_list[0],
							 pin_callback,
							 NULL);

			ret =
			    gnutls_x509_crt_import_pkcs11_url(crt_list[0],
							      x509_certfile,
							      0);

			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				ret =
				    gnutls_x509_crt_import_pkcs11_url
				    (crt_list[0], x509_certfile,
				     GNUTLS_PKCS11_OBJ_FLAG_LOGIN);

			if (ret < 0) {
				fprintf(stderr,
					"*** Error loading cert file.\n");
				exit(1);
			}
			x509_crt_size = 1;
		} else
#endif				/* ENABLE_PKCS11 */
		{

			ret = gnutls_load_file(x509_certfile, &data);
			if (ret < 0) {
				fprintf(stderr,
					"*** Error loading cert file.\n");
				exit(1);
			}

			crt_num = MAX_CRT;
			ret =
			    gnutls_x509_crt_list_import(crt_list, &crt_num,
							&data, x509ctype,
							GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED);
			if (ret < 0) {
				if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER) {
					fprintf(stderr,
						"*** Error loading cert file: Too many certs %d\n",
						crt_num);

				} else {
					fprintf(stderr,
						"*** Error loading cert file: %s\n",
						gnutls_strerror(ret));
				}
				exit(1);
			}
			x509_crt_size = ret;
		}

		for (i = 0; i < x509_crt_size; i++) {
			ret =
			    gnutls_pcert_import_x509(&x509_crt[i],
						     crt_list[i], 0);
			if (ret < 0) {
				fprintf(stderr,
					"*** Error importing crt to pcert: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
			gnutls_x509_crt_deinit(crt_list[i]);
		}

		gnutls_free(data.data);

		load_priv_key(&x509_key, x509_keyfile);

		log_msg(stdout,
			"Processed %d client X.509 certificates...\n",
			x509_crt_size);
	}
}

/* Load the raw public key and corresponding private key.
 */
static void load_rawpk_keys(void)
{
	int ret;
	gnutls_datum_t data = { NULL, 0 };

	if (rawpk_file != NULL && rawpk_keyfile != NULL) {
		// First we load the raw public key
		ret = gnutls_load_file(rawpk_file, &data);
		if (ret < 0) {
			fprintf(stderr,
				"*** Error loading cert file.\n");
			exit(1);
		}

		ret = gnutls_pcert_import_rawpk_raw(&rawpk, &data, x509ctype, 0, 0);
		if (ret < 0) {
			fprintf(stderr,
			        "*** Error importing rawpk to pcert: %s\n",
			        gnutls_strerror(ret));
			exit(1);
		}

		gnutls_free(data.data);

		// Secondly, we load the private key corresponding to the raw pk
		load_priv_key(&rawpk_key, rawpk_keyfile);

		log_msg(stdout,
			"Processed %d client raw public key pair...\n",	1);
	}
}

#define IS_NEWLINE(x) ((x[0] == '\n') || (x[0] == '\r'))
static int read_yesno(const char *input_str)
{
	char input[128];

	fputs(input_str, stderr);
	if (fgets(input, sizeof(input), stdin) == NULL)
		return 0;

	if (IS_NEWLINE(input))
		return 0;

	if (input[0] == 'y' || input[0] == 'Y')
		return 1;

	return 0;
}

static void try_save_cert(gnutls_session_t session)
{
	const gnutls_datum_t *cert_list;
	unsigned int cert_list_size = 0;
	int ret;
	unsigned i;
	gnutls_datum_t t;
	FILE *fp;

	cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
	if (cert_list_size == 0) {
		fprintf(stderr, "no certificates sent by server!\n");
		exit(1);
	}

	fp = fopen(OPT_ARG(SAVE_CERT), "w");
	if (fp == NULL) {
		fprintf(stderr, "could not open %s\n", OPT_ARG(SAVE_CERT));
		exit(1);
	}

	for (i=0;i<cert_list_size;i++) {
		ret = gnutls_pem_base64_encode_alloc("CERTIFICATE", &cert_list[i], &t);
		if (ret < 0) {
			fprintf(stderr, "error[%d]: %s\n", __LINE__,
				gnutls_strerror(ret));
			exit(1);
		}

		fwrite(t.data, t.size, 1, fp);
		gnutls_free(t.data);
	}
	fclose(fp);

	return;
}

static int cert_verify_callback(gnutls_session_t session)
{
	int rc;
	unsigned int status = 0;
	int ssh = ENABLED_OPT(TOFU);
	int strictssh = ENABLED_OPT(STRICT_TOFU);
	int dane = ENABLED_OPT(DANE);
	int ca_verify = ENABLED_OPT(CA_VERIFICATION);
	const char *txt_service;
	gnutls_datum_t oresp;
	const char *host;

	/* On an session with TOFU the PKI/DANE verification
	 * become advisory.
	 */

	if (strictssh) {
		ssh = strictssh;
	}

	if (HAVE_OPT(VERIFY_HOSTNAME)) {
		host = OPT_ARG(VERIFY_HOSTNAME);
		canonicalize_host((char *) host, NULL, 0);
	} else
		host = hostname;

	/* Save certificate and OCSP response */
	if (HAVE_OPT(SAVE_CERT)) {
		try_save_cert(session);
	}

	rc = gnutls_ocsp_status_request_get(session, &oresp);
	if (rc < 0) {
		oresp.data = NULL;
		oresp.size = 0;
	}

	if (HAVE_OPT(SAVE_OCSP) && oresp.data) {
		FILE *fp = fopen(OPT_ARG(SAVE_OCSP), "w");

		if (fp != NULL) {
			fwrite(oresp.data, 1, oresp.size, fp);
			fclose(fp);
		}
	}

	print_cert_info(session, verbose, print_cert);

	if (ca_verify) {
		rc = cert_verify(session, host, GNUTLS_KP_TLS_WWW_SERVER);
		if (rc == 0) {
			log_msg
			    (stdout, "*** PKI verification of server certificate failed...\n");
			if (!insecure && !ssh)
				return -1;
		} else if (ENABLED_OPT(OCSP) && gnutls_ocsp_status_request_is_checked(session, 0) == 0) {	/* off-line verification succeeded. Try OCSP */
			rc = cert_verify_ocsp(session);
			if (rc == -1) {
				log_msg
				    (stdout, "*** Verifying (with OCSP) server certificate chain failed...\n");
				if (!insecure && !ssh)
					return -1;
			} else if (rc == 0)
				log_msg(stdout, "*** OCSP: nothing to check.\n");
			else
				log_msg(stdout, "*** OCSP: verified %d certificate(s).\n", rc);
		}
	}

	if (dane) {		/* try DANE auth */
#ifdef HAVE_DANE
		int port;
		unsigned vflags = 0;
		unsigned int sflags =
		    ENABLED_OPT(LOCAL_DNS) ? 0 :
		    DANE_F_IGNORE_LOCAL_RESOLVER;

		/* if we didn't verify the chain it only makes sense
		 * to check the end certificate using dane. */
		if (ca_verify == 0)
			vflags |= DANE_VFLAG_ONLY_CHECK_EE_USAGE;

		port = service_to_port(service, udp?"udp":"tcp");
		rc = dane_verify_session_crt(NULL, session, host,
					     udp ? "udp" : "tcp", port,
					     sflags, vflags, &status);
		if (rc < 0) {
			fprintf(stderr,
				"*** DANE verification error: %s\n",
				dane_strerror(rc));
			if (!insecure && !ssh)
				return -1;
		} else {
			gnutls_datum_t out;

			rc = dane_verification_status_print(status, &out,
							    0);
			if (rc < 0) {
				fprintf(stderr, "*** DANE error: %s\n",
					dane_strerror(rc));
			} else {
				fprintf(stderr, "- DANE: %s\n", out.data);
				gnutls_free(out.data);
			}

			if (status != 0 && !insecure && !ssh)
				return -1;
		}
#else
		fprintf(stderr, "*** DANE error: GnuTLS is not compiled with DANE support.\n");
		if (!insecure && !ssh)
			return -1;
#endif
	}

	if (ssh) {		/* try ssh auth */
		unsigned int list_size;
		const gnutls_datum_t *cert;

		cert = gnutls_certificate_get_peers(session, &list_size);
		if (cert == NULL) {
			fprintf(stderr,
				"Cannot obtain peer's certificate!\n");
			return -1;
		}

		txt_service = port_to_service(service, udp?"udp":"tcp");

		rc = gnutls_verify_stored_pubkey(NULL, NULL, host,
						 txt_service,
						 GNUTLS_CRT_X509, cert, 0);
		if (rc == GNUTLS_E_NO_CERTIFICATE_FOUND) {
			fprintf(stderr,
				"Host %s (%s) has never been contacted before.\n",
				host, txt_service);
			if (status == 0)
				fprintf(stderr,
					"Its certificate is valid for %s.\n",
					host);

			if (strictssh)
				return -1;

			rc = read_yesno
			    ("Are you sure you want to trust it? (y/N): ");
			if (rc == 0)
				return -1;
		} else if (rc == GNUTLS_E_CERTIFICATE_KEY_MISMATCH) {
			fprintf(stderr,
				"Warning: host %s is known and it is associated with a different key.\n",
				host);
			fprintf(stderr,
				"It might be that the server has multiple keys, or an attacker replaced the key to eavesdrop this connection .\n");
			if (status == 0)
				fprintf(stderr,
					"Its certificate is valid for %s.\n",
					host);

			if (strictssh)
				return -1;

			rc = read_yesno
				("Do you trust the received key? (y/N): ");
			if (rc == 0)
				return -1;
		} else if (rc < 0) {
			fprintf(stderr,
				"gnutls_verify_stored_pubkey: %s\n",
				gnutls_strerror(rc));
			return -1;
		}

		if (rc != 0) {
			rc = gnutls_store_pubkey(NULL, NULL, host,
						 txt_service,
						 GNUTLS_CRT_X509, cert, 0,
						 0);
			if (rc < 0)
				fprintf(stderr,
					"Could not store key: %s\n",
					gnutls_strerror(rc));
		}
	}
	return 0;
}

/* This callback should be associated with a session by calling
 * gnutls_certificate_client_set_retrieve_function( session, cert_callback),
 * before a handshake.
 */

static int
cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * sign_algos,
	      int sign_algos_length, gnutls_pcert_st ** pcert,
	      unsigned int *pcert_length, gnutls_privkey_t * pkey)
{
	char issuer_dn[256];
	int i, ret, cert_type;
	size_t len;

	if (verbose) {
		/* Print the server's trusted CAs
		 */
		if (nreqs > 0)
			log_msg(stdout, "- Server's trusted authorities:\n");
		else
			log_msg
			    (stdout, "- Server did not send us any trusted authorities names.\n");

		/* print the names (if any) */
		for (i = 0; i < nreqs; i++) {
			len = sizeof(issuer_dn);
			ret =
			    gnutls_x509_rdn_get(&req_ca_rdn[i], issuer_dn,
						&len);
			if (ret >= 0) {
				log_msg(stdout, "   [%d]: ", i);
				log_msg(stdout, "%s\n", issuer_dn);
			}
		}
	}

	/* Select a certificate and return it.
	 * The certificate must be of any of the "sign algorithms"
	 * supported by the server.
	 */

	cert_type = gnutls_certificate_type_get2(session, GNUTLS_CTYPE_CLIENT);

	*pcert_length = 0;

	switch (cert_type) {
		case GNUTLS_CRT_X509:
			if (x509_crt_size > 0) {
				if (x509_key != NULL) {
					*pkey = x509_key;
				} else {
					log_msg
					      (stdout, "- Could not find a suitable key to send to server\n");
					return -1;
				}

				*pcert_length = x509_crt_size;
				*pcert = x509_crt;
			}
			break;
		case GNUTLS_CRT_RAWPK:
			if (rawpk_key == NULL || rawpk.type != GNUTLS_CRT_RAWPK) {
				log_msg
				      (stdout, "- Could not find a suitable key to send to server\n");
				return -1;
			}

			*pkey = rawpk_key;
			*pcert = &rawpk;
			*pcert_length = 1;
			break;
		default:
			log_msg(stdout, "- Could not retrieve unsupported certificate type %s.\n",
	       gnutls_certificate_type_get_name(cert_type));
	    return -1;
	}

	log_msg(stdout, "- Successfully sent %u certificate(s) to server.\n",
	       *pcert_length);
	return 0;

}

/* initializes a gnutls_session_t with some defaults.
 */
gnutls_session_t init_tls_session(const char *host)
{
	const char *err;
	int ret;
	unsigned i;
	gnutls_session_t session;

	if (udp) {
		gnutls_init(&session, GNUTLS_DATAGRAM | init_flags);
		if (mtu)
			gnutls_dtls_set_mtu(session, mtu);
	} else
		gnutls_init(&session, init_flags);

	if (priorities == NULL) {
		ret = gnutls_set_default_priority(session);
		if (ret < 0) {
			fprintf(stderr, "Error in setting priorities: %s\n",
					gnutls_strerror(ret));
			exit(1);
		}
	} else {
		ret = gnutls_priority_set_direct(session, priorities, &err);
		if (ret < 0) {
			if (ret == GNUTLS_E_INVALID_REQUEST)
				fprintf(stderr, "Syntax error at: %s\n", err);
			else
				fprintf(stderr, "Error in priorities: %s\n",
					gnutls_strerror(ret));
			exit(1);
		}
	}

	/* allow the use of private ciphersuites.
	 */
	if (disable_extensions == 0 && disable_sni == 0) {
		if (HAVE_OPT(SNI_HOSTNAME)) {
			const char *sni_host = OPT_ARG(SNI_HOSTNAME);

			canonicalize_host((char *) sni_host, NULL, 0);
			gnutls_server_name_set(session, GNUTLS_NAME_DNS, sni_host, strlen(sni_host));
		} else if (host != NULL && is_ip(host) == 0)
			gnutls_server_name_set(session, GNUTLS_NAME_DNS,
					       host, strlen(host));
	}

	if (HAVE_OPT(DH_BITS))
		gnutls_dh_set_prime_bits(session, OPT_VALUE_DH_BITS);

	if (HAVE_OPT(ALPN)) {
		unsigned proto_n = STACKCT_OPT(ALPN);
		char **protos = (void *) STACKLST_OPT(ALPN);

		if (proto_n > 1024) {
			fprintf(stderr, "Number of ALPN protocols too large (%d)\n",
					proto_n);
			exit(1);
		}

		gnutls_datum_t p[1024];
		for (i = 0; i < proto_n; i++) {
			p[i].data = (void *) protos[i];
			p[i].size = strlen(protos[i]);
		}
		gnutls_alpn_set_protocols(session, p, proto_n, 0);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred);
	if (srp_cred)
		gnutls_credentials_set(session, GNUTLS_CRD_SRP, srp_cred);
	if (psk_cred)
		gnutls_credentials_set(session, GNUTLS_CRD_PSK, psk_cred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	gnutls_certificate_set_retrieve_function2(xcred, cert_callback);
	gnutls_certificate_set_verify_function(xcred,
					       cert_verify_callback);

	/* use the max record size extension */
	if (record_max_size > 0 && disable_extensions == 0) {
		if (gnutls_record_set_max_size(session, record_max_size) <
		    0) {
			fprintf(stderr,
				"Cannot set the maximum record size to %d.\n",
				record_max_size);
			fprintf(stderr,
				"Possible values: 512, 1024, 2048, 4096.\n");
			exit(1);
		}
	}

	if (HAVE_OPT(HEARTBEAT))
		gnutls_heartbeat_enable(session,
					GNUTLS_HB_PEER_ALLOWED_TO_SEND);

#ifdef ENABLE_DTLS_SRTP
	if (HAVE_OPT(SRTP_PROFILES)) {
		ret =
		    gnutls_srtp_set_profile_direct(session,
						   OPT_ARG(SRTP_PROFILES),
						   &err);
		if (ret == GNUTLS_E_INVALID_REQUEST)
			fprintf(stderr, "Syntax error at: %s\n", err);
		else if (ret != 0)
			fprintf(stderr, "Error in profiles: %s\n",
				gnutls_strerror(ret));
		else fprintf(stderr,"DTLS profile set to %s\n",
			     OPT_ARG(SRTP_PROFILES));

		if (ret != 0) exit(1);
	}
#endif


	return session;
}

static void cmd_parser(int argc, char **argv);

/* Returns zero if the error code was successfully handled.
 */
static int handle_error(socket_st * hd, int err)
{
	int alert, ret;
	const char *err_type, *str;

	if (err >= 0 || err == GNUTLS_E_AGAIN
	    || err == GNUTLS_E_INTERRUPTED)
		return 0;

	if (gnutls_error_is_fatal(err) == 0) {
		ret = 0;
		err_type = "Non fatal";
	} else {
		ret = err;
		err_type = "Fatal";
	}

	str = gnutls_strerror(err);
	if (str == NULL)
		str = str_unknown;
	fprintf(stderr, "*** %s error: %s\n", err_type, str);

	if (err == GNUTLS_E_WARNING_ALERT_RECEIVED
	    || err == GNUTLS_E_FATAL_ALERT_RECEIVED) {
		alert = gnutls_alert_get(hd->session);
		str = gnutls_alert_get_name(alert);
		if (str == NULL)
			str = str_unknown;
		log_msg(stdout, "*** Received alert [%d]: %s\n", alert, str);
	}

	check_server_cmd(hd, err);

	return ret;
}

int starttls_alarmed = 0;

#ifndef _WIN32
static void starttls_alarm(int signum)
{
	starttls_alarmed = 1;
}
#endif

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

#define IN_NONE 0
#define IN_KEYBOARD 1
#define IN_NET 2
#define IN_TERM 3
/* returns IN_KEYBOARD for keyboard input and IN_NET for network input
 */
static int check_net_or_keyboard_input(socket_st * hd, unsigned user_term)
{
	int maxfd;
	fd_set rset;
	int err;
	struct timeval tv;

	do {
		FD_ZERO(&rset);
		FD_SET(hd->fd, &rset);

#ifndef _WIN32
		if (!user_term) {
			FD_SET(fileno(stdin), &rset);
			maxfd = MAX(fileno(stdin), hd->fd);
		} else {
			maxfd = hd->fd;
		}
#else
		maxfd = hd->fd;
#endif

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		if (hd->secure == 1)
			if (gnutls_record_check_pending(hd->session))
				return IN_NET;

		err = select(maxfd + 1, &rset, NULL, NULL, &tv);
		if (err < 0)
			continue;

		if (FD_ISSET(hd->fd, &rset))
			return IN_NET;

#ifdef _WIN32
		{
			int state;
			state =
			    WaitForSingleObject(GetStdHandle
						(STD_INPUT_HANDLE), 200);

			if (state == WAIT_OBJECT_0)
				return IN_KEYBOARD;
		}
#else
		if (!user_term && FD_ISSET(fileno(stdin), &rset))
			return IN_KEYBOARD;
#endif
		if (err == 0 && user_term)
			return IN_TERM;
	}
	while (err == 0);

	return IN_NONE;
}

static int try_rehandshake(socket_st * hd)
{
	int ret;

	ret = do_handshake(hd);
	if (ret < 0) {
		fprintf(stderr, "*** ReHandshake has failed\n");
		gnutls_perror(ret);
		return ret;
	} else {
		log_msg(stdout, "- ReHandshake was completed\n");
		return 0;
	}
}

static int try_rekey(socket_st * hd, unsigned peer)
{
	int ret;

	do {
		ret = gnutls_session_key_update(hd->session, peer?GNUTLS_KU_PEER:0);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		fprintf(stderr, "*** Rekey has failed: %s\n", gnutls_strerror(ret));
		return ret;
	} else {
		log_msg(stdout, "- Rekey was completed\n");
		return 0;
	}
}

static int try_resume(socket_st * hd)
{
	int ret, socket_flags = SOCKET_FLAG_DONT_PRINT_ERRORS;
	gnutls_datum_t rdata = {NULL, 0};
	gnutls_datum_t edata = {NULL, 0};

	if (gnutls_session_is_resumed(hd->session) == 0) {
		/* not resumed - obtain the session data */
		ret = gnutls_session_get_data2(hd->session, &rdata);
		if (ret < 0) {
			rdata.data = NULL;
		}
	} else {
		/* resumed - try to reuse the previous session data */
		rdata.data = hd->rdata.data;
		rdata.size = hd->rdata.size;
		hd->rdata.data = NULL;
	}

	log_msg(stdout, "- Disconnecting\n");
	socket_bye(hd, 1);

	canonicalize_host(hostname, service, sizeof(service));

	log_msg
	    (stdout, "\n\n- Connecting again- trying to resume previous session\n");
	if (HAVE_OPT(STARTTLS_PROTO))
		socket_flags |= SOCKET_FLAG_STARTTLS;
	else if (fastopen)
		socket_flags |= SOCKET_FLAG_FASTOPEN;

	if (udp)
		socket_flags |= SOCKET_FLAG_UDP;

	if (HAVE_OPT(EARLYDATA)) {
		FILE *fp;
		size_t size;

		fp = fopen(OPT_ARG(EARLYDATA), "r");
		if (fp == NULL) {
			fprintf(stderr, "could not open %s\n", OPT_ARG(EARLYDATA));
			exit(1);
		}
		edata.data = (void *) fread_file(fp, &size);
		edata.size = size;
		fclose(fp);
	}

	socket_open3(hd, hostname, service, OPT_ARG(STARTTLS_PROTO),
		     socket_flags, CONNECT_MSG, &rdata, &edata);

	log_msg(stdout, "- Resume Handshake was completed\n");
	if (gnutls_session_is_resumed(hd->session) != 0)
		log_msg(stdout, "*** This is a resumed session\n");

	return 0;
}

static
bool parse_for_inline_commands_in_buffer(char *buffer, size_t bytes,
					 inline_cmds_st * inline_cmds)
{
	ssize_t local_bytes, match_bytes, prev_bytes_copied, ii;
	unsigned jj;
	char *local_buffer_ptr, *ptr;
	char inline_command_string[MAX_INLINE_COMMAND_BYTES];
	ssize_t l;

	inline_cmds->bytes_to_flush = 0;
	inline_cmds->cmd_found = INLINE_COMMAND_NONE;

	if (inline_cmds->bytes_copied) {
		local_buffer_ptr =
		    &inline_cmds->inline_cmd_buffer[inline_cmds->
						    bytes_copied];

		local_bytes =
		    ((inline_cmds->bytes_copied + bytes) <=
		     MAX_INLINE_COMMAND_BYTES) ? (ssize_t) bytes
		    : (MAX_INLINE_COMMAND_BYTES -
		       inline_cmds->bytes_copied);

		memcpy(local_buffer_ptr, buffer, local_bytes);
		prev_bytes_copied = inline_cmds->bytes_copied;
		inline_cmds->new_buffer_ptr = buffer + local_bytes;
		inline_cmds->bytes_copied += local_bytes;
		local_buffer_ptr = inline_cmds->inline_cmd_buffer;
		local_bytes = inline_cmds->bytes_copied;
	} else {
		prev_bytes_copied = 0;
		local_buffer_ptr = buffer;
		local_bytes = bytes;
		inline_cmds->new_buffer_ptr = buffer + bytes;
	}

	assert(local_buffer_ptr != NULL);

	inline_cmds->current_ptr = local_buffer_ptr;

	if (local_buffer_ptr[0] == inline_commands_prefix[0]
	    && inline_cmds->lf_found) {
		for (jj = 0; jj < NUM_INLINE_COMMANDS; jj++) {
			if (inline_commands_prefix[0] != '^') {	/* refer inline_cmds.h for usage of ^ */
				strcpy(inline_command_string,
				       inline_commands_def[jj].string);
				inline_command_string[strlen
						      (inline_commands_def
						       [jj].string)] =
				    '\0';
				inline_command_string[0] =
				    inline_commands_prefix[0];
				/* Inline commands are delimited by the inline_commands_prefix[0] (default is ^).
				   The inline_commands_def[].string includes a trailing LF */
				inline_command_string[strlen
						      (inline_commands_def
						       [jj].string) - 2] =
				    inline_commands_prefix[0];
				ptr = inline_command_string;
			} else
				ptr = inline_commands_def[jj].string;

			l = strlen(ptr);
			match_bytes = (local_bytes <= l) ? local_bytes : l;
			if (strncmp(ptr, local_buffer_ptr, match_bytes) ==
			    0) {
				if (match_bytes == (ssize_t) strlen(ptr)) {
					inline_cmds->new_buffer_ptr =
					    buffer + match_bytes -
					    prev_bytes_copied;
					inline_cmds->cmd_found =
					    inline_commands_def[jj].
					    command;
					inline_cmds->bytes_copied = 0;	/* reset it */
				} else {
					/* partial command */
					memcpy(&inline_cmds->
					       inline_cmd_buffer
					       [inline_cmds->bytes_copied],
					       buffer, bytes);
					inline_cmds->bytes_copied += bytes;
				}
				return true;
			}
			/* else - if not a match, do nothing here */
		}		/* for */
	}

	for (ii = prev_bytes_copied; ii < local_bytes; ii++) {
		if (ii && local_buffer_ptr[ii] == inline_commands_prefix[0]
		    && inline_cmds->lf_found) {
			/* possible inline command. First, let's flush bytes up to ^ */
			inline_cmds->new_buffer_ptr =
			    buffer + ii - prev_bytes_copied;
			inline_cmds->bytes_to_flush = ii;
			inline_cmds->lf_found = true;

			/* bytes to flush starts at inline_cmds->current_ptr */
			return true;
		} else if (local_buffer_ptr[ii] == '\n') {
			inline_cmds->lf_found = true;
		} else {
			inline_cmds->lf_found = false;
		}
	}			/* for */

	inline_cmds->bytes_copied = 0;	/* reset it */
	return false;		/* not an inline command */
}

static
int run_inline_command(inline_cmds_st * cmd, socket_st * hd)
{
	switch (cmd->cmd_found) {
	case INLINE_COMMAND_RESUME:
		return try_resume(hd);
	case INLINE_COMMAND_REKEY_LOCAL:
		return try_rekey(hd, 0);
	case INLINE_COMMAND_REKEY_BOTH:
		return try_rekey(hd, 1);
	case INLINE_COMMAND_RENEGOTIATE:
		return try_rehandshake(hd);
	default:
		return -1;
	}
}

static
int do_inline_command_processing(char *buffer_ptr, size_t curr_bytes,
				 socket_st * hd,
				 inline_cmds_st * inline_cmds)
{
	int skip_bytes, bytes;
	bool inline_cmd_start_found;

	bytes = curr_bytes;

      continue_inline_processing:
	/* parse_for_inline_commands_in_buffer hunts for start of an inline command
	 * sequence. The function maintains state information in inline_cmds.
	 */
	inline_cmd_start_found =
	    parse_for_inline_commands_in_buffer(buffer_ptr, bytes,
						inline_cmds);
	if (!inline_cmd_start_found)
		return bytes;

	/* inline_cmd_start_found is set */

	if (inline_cmds->bytes_to_flush) {
		/* start of an inline command sequence found, but is not
		 * at the beginning of buffer. So, we flush all preceding bytes.
		 */
		return inline_cmds->bytes_to_flush;
	} else if (inline_cmds->cmd_found == INLINE_COMMAND_NONE) {
		/* partial command found */
		return 0;
	} else {
		/* complete inline command found and is at the start */
		if (run_inline_command(inline_cmds, hd))
			return -1;

		inline_cmds->cmd_found = INLINE_COMMAND_NONE;
		skip_bytes = inline_cmds->new_buffer_ptr - buffer_ptr;

		if (skip_bytes >= bytes)
			return 0;
		else {
			buffer_ptr = inline_cmds->new_buffer_ptr;
			bytes -= skip_bytes;
			goto continue_inline_processing;
		}
	}
}

static void
print_other_info(gnutls_session_t session)
{
	int ret;
	gnutls_datum_t oresp;

	ret = gnutls_ocsp_status_request_get(session, &oresp);
	if (ret < 0) {
		oresp.data = NULL;
		oresp.size = 0;
	}

	if (ENABLED_OPT(VERBOSE) && oresp.data) {
		gnutls_ocsp_resp_t r;
		gnutls_datum_t p;
		unsigned flag;

		ret = gnutls_ocsp_resp_init(&r);
		if (ret < 0) {
			fprintf(stderr, "ocsp_resp_init: %s\n",
				gnutls_strerror(ret));
			return;
		}

		ret = gnutls_ocsp_resp_import(r, &oresp);
		if (ret < 0) {
			fprintf(stderr, "importing response: %s\n",
				gnutls_strerror(ret));
			return;
		}

		if (print_cert != 0)
			flag = GNUTLS_OCSP_PRINT_FULL;
		else
			flag = GNUTLS_OCSP_PRINT_COMPACT;
		ret =
		    gnutls_ocsp_resp_print(r, flag, &p);
		gnutls_ocsp_resp_deinit(r);
		if (ret>=0) {
			log_msg(stdout, "%s", (char*) p.data);
			gnutls_free(p.data);
		}
	}

}

int main(int argc, char **argv)
{
	int ret;
	int ii, inp;
	char buffer[MAX_BUF + 1];
	int user_term = 0, retval = 0;
	socket_st hd;
	ssize_t bytes, keyboard_bytes;
	char *keyboard_buffer_ptr;
	inline_cmds_st inline_cmds;
	int socket_flags = SOCKET_FLAG_DONT_PRINT_ERRORS;
	FILE *server_fp = NULL;
	FILE *client_fp = NULL;
	FILE *logfile = NULL;
#ifndef _WIN32
	struct sigaction new_action;
#endif

	cmd_parser(argc, argv);

	if (HAVE_OPT(LOGFILE)) {
		logfile = fopen(OPT_ARG(LOGFILE), "w+");
		if (!logfile) {
			log_msg(stderr, "Unable to open '%s'!\n", OPT_ARG(LOGFILE));
			exit(1);
		}
		log_set(logfile);
	}

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(OPT_VALUE_DEBUG);

	if ((ret = gnutls_global_init()) < 0) {
		fprintf(stderr, "global_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (hostname == NULL) {
		fprintf(stderr, "No hostname given\n");
		exit(1);
	}

	sockets_init();

	init_global_tls_stuff();

	canonicalize_host(hostname, service, sizeof(service));

	if (udp)
		socket_flags |= SOCKET_FLAG_UDP;
	if (fastopen)
		socket_flags |= SOCKET_FLAG_FASTOPEN;
	if (verbose)
		socket_flags |= SOCKET_FLAG_VERBOSE;
	if (starttls)
		socket_flags |= SOCKET_FLAG_RAW;
	else if (HAVE_OPT(STARTTLS_PROTO))
		socket_flags |= SOCKET_FLAG_STARTTLS;

	if (HAVE_OPT(SAVE_SERVER_TRACE)) {
		server_fp = fopen(OPT_ARG(SAVE_SERVER_TRACE), "wb");
	}
	if (HAVE_OPT(SAVE_CLIENT_TRACE)) {
		client_fp = fopen(OPT_ARG(SAVE_CLIENT_TRACE), "wb");
	}

	socket_open2(&hd, hostname, service, OPT_ARG(STARTTLS_PROTO),
		     socket_flags, CONNECT_MSG, NULL, NULL,
		     server_fp, client_fp);

	hd.verbose = verbose;

	if (hd.secure) {
		log_msg(stdout, "- Handshake was completed\n");

		if (resume != 0)
			if (try_resume(&hd)) {
				retval = 1;
				goto cleanup;
			}

		print_other_info(hd.session);
	}

	/* Warning!  Do not touch this text string, it is used by external
	   programs to search for when gnutls-cli has reached this point. */
	log_msg(stdout, "\n- Simple Client Mode:\n\n");

	if (rehandshake)
		if (try_rehandshake(&hd)) {
			retval = 1;
			goto cleanup;
		}

#ifndef _WIN32
	new_action.sa_handler = starttls_alarm;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;

	sigaction(SIGALRM, &new_action, NULL);
#endif

	fflush(stdout);
	fflush(stderr);

	/* do not buffer */
#ifndef _WIN32
	setbuf(stdin, NULL);
#endif
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	memset(&inline_cmds, 0, sizeof(inline_cmds_st));
	if (inline_commands) {
		inline_cmds.lf_found = true;	/* initially, at start of line */
	}

	for (;;) {
		if (starttls_alarmed && !hd.secure) {
			/* Warning!  Do not touch this text string, it is used by
			   external programs to search for when gnutls-cli has
			   reached this point. */
			fprintf(stderr, "*** Starting TLS handshake\n");
			ret = do_handshake(&hd);
			if (ret < 0) {
				fprintf(stderr,
					"*** Handshake has failed\n");
				retval = 1;
				break;
			}
		}

		inp = check_net_or_keyboard_input(&hd, user_term);
		if (inp == IN_TERM)
			break;

		if (inp == IN_NET) {
			memset(buffer, 0, MAX_BUF + 1);
			ret = socket_recv(&hd, buffer, MAX_BUF);

			if (ret == 0 || (ret == GNUTLS_E_PREMATURE_TERMINATION && user_term)) {
				log_msg
				    (stdout, "- Peer has closed the GnuTLS connection\n");
				break;
			} else if (handle_error(&hd, ret) < 0) {
				fprintf(stderr,
					"*** Server has terminated the connection abnormally.\n");
				retval = 1;
				break;
			} else if (ret > 0) {
				if (verbose != 0)
					log_msg(stdout, "- Received[%d]: ", ret);
				for (ii = 0; ii < ret; ii++) {
					fputc(buffer[ii], stdout);
				}
				fflush(stdout);
			}
		}

		if (inp == IN_KEYBOARD && user_term == 0) {
			if ((bytes =
			    read(fileno(stdin), buffer,
			    MAX_BUF - 1)) <= 0) {
				if (hd.secure == 0) {
					/* Warning!  Do not touch this text string, it is
					   used by external programs to search for when
					   gnutls-cli has reached this point. */
					fprintf(stderr,
						"*** Starting TLS handshake\n");
					ret = do_handshake(&hd);
					clearerr(stdin);
					if (ret < 0) {
						fprintf(stderr,
							"*** Handshake has failed\n");
						retval = 1;
						break;
					}
				} else {
					do {
						ret = gnutls_bye(hd.session, GNUTLS_SHUT_WR);
					} while (ret == GNUTLS_E_INTERRUPTED ||
					         ret == GNUTLS_E_AGAIN);

					user_term = 1;
				}
				continue;
			}
			buffer[bytes] = 0;

			if (crlf != 0) {
				char *b = strchr(buffer, '\n');
				if (b != NULL) {
					strcpy(b, "\r\n");
					bytes++;
				}
			}

			keyboard_bytes = bytes;
			keyboard_buffer_ptr = buffer;

		      inline_command_processing:

			if (inline_commands) {
				keyboard_bytes =
				    do_inline_command_processing
				    (keyboard_buffer_ptr, keyboard_bytes,
				     &hd, &inline_cmds);
				if (keyboard_bytes == 0)
					continue;
				else if (keyboard_bytes < 0) {	/* error processing an inline command */
					retval = 1;
					break;
				} else {
					/* current_ptr could point to either an inline_cmd_buffer
					 * or may point to start or an offset into buffer.
					 */
					keyboard_buffer_ptr =
					    inline_cmds.current_ptr;
				}
			}

			if (ranges
			    && gnutls_record_can_use_length_hiding(hd.
								   session))
			{
				gnutls_range_st range;
				range.low = 0;
				range.high = MAX_BUF;
				ret =
				    socket_send_range(&hd,
						      keyboard_buffer_ptr,
						      keyboard_bytes,
						      &range);
			} else {
				ret =
				    socket_send(&hd, keyboard_buffer_ptr,
						keyboard_bytes);
			}

			if (ret > 0) {
				if (verbose != 0)
					log_msg(stdout, "- Sent: %d bytes\n", ret);
			} else
				handle_error(&hd, ret);

			if (inline_commands &&
			    inline_cmds.new_buffer_ptr < (buffer + bytes))
			{
				keyboard_buffer_ptr =
				    inline_cmds.new_buffer_ptr;
				keyboard_bytes =
				    (buffer + bytes) - keyboard_buffer_ptr;
				goto inline_command_processing;
			}
		}
	}

 cleanup:
	socket_bye(&hd, 0);
	if (logfile) {
		fclose(logfile);
		logfile = NULL;
	}

#ifdef ENABLE_SRP
	if (srp_cred)
		gnutls_srp_free_client_credentials(srp_cred);
#endif
#ifdef ENABLE_PSK
	if (psk_cred)
		gnutls_psk_free_client_credentials(psk_cred);
#endif

	gnutls_certificate_free_credentials(xcred);

#ifdef ENABLE_ANON
	gnutls_anon_free_client_credentials(anon_cred);
#endif

	gnutls_global_deinit();

	return retval;
}

static
void print_priority_list(void)
{
	unsigned int idx;
	const char *str;
	unsigned int lineb = 0;

	log_msg(stdout, "Priority strings in GnuTLS %s:\n", gnutls_check_version(NULL));

	fputs("\t", stdout);
	for (idx=0;;idx++) {
		str = gnutls_priority_string_list(idx, GNUTLS_PRIORITY_LIST_INIT_KEYWORDS);
		if (str == NULL)
			break;
		lineb += log_msg(stdout, "%s ", str);
		if (lineb > 64) {
			lineb = 0;
			log_msg(stdout, "\n\t");
		}
	}

	log_msg(stdout, "\n\nSpecial strings:\n");
	lineb = 0;
	fputs("\t", stdout);
	for (idx=0;;idx++) {
		str = gnutls_priority_string_list(idx, GNUTLS_PRIORITY_LIST_SPECIAL);
		if (str == NULL)
			break;
		if (str[0] == 0)
			continue;
		lineb += log_msg(stdout, "%%%s ", str);
		if (lineb > 64) {
			lineb = 0;
			log_msg(stdout, "\n\t");
		}
	}
	log_msg(stdout, "\n");

	return;
}

static void cmd_parser(int argc, char **argv)
{
	char *rest = NULL;

	int optct = optionProcess(&gnutls_cliOptions, argc, argv);
	argc -= optct;
	argv += optct;

	if (rest == NULL && argc > 0)
		rest = argv[0];


	if (HAVE_OPT(FIPS140_MODE)) {
		if (gnutls_fips140_mode_enabled() != 0) {
			fprintf(stderr, "library is in FIPS140-2 mode\n");
			exit(0);
		}
		fprintf(stderr, "library is NOT in FIPS140-2 mode\n");
		exit(1);
	}

	if (HAVE_OPT(BENCHMARK_CIPHERS)) {
		benchmark_cipher(OPT_VALUE_DEBUG);
		exit(0);
	}

	if (HAVE_OPT(BENCHMARK_TLS_CIPHERS)) {
		benchmark_tls(OPT_VALUE_DEBUG, 1);
		exit(0);
	}

	if (HAVE_OPT(BENCHMARK_TLS_KX)) {
		benchmark_tls(OPT_VALUE_DEBUG, 0);
		exit(0);
	}

	if (HAVE_OPT(PRIORITY)) {
		priorities = OPT_ARG(PRIORITY);
	}
	verbose = HAVE_OPT(VERBOSE);
	if (verbose)
		print_cert = 1;
	else
		print_cert = HAVE_OPT(PRINT_CERT);

	if (HAVE_OPT(LIST)) {
		print_list(priorities, verbose);
		exit(0);
	}

	if (HAVE_OPT(PRIORITY_LIST)) {
		print_priority_list();
		exit(0);
	}

	disable_sni = HAVE_OPT(DISABLE_SNI);
	disable_extensions = HAVE_OPT(DISABLE_EXTENSIONS);
	if (disable_extensions)
		init_flags |= GNUTLS_NO_EXTENSIONS;

	if (HAVE_OPT(SINGLE_KEY_SHARE))
		init_flags |= GNUTLS_KEY_SHARE_TOP;

	if (HAVE_OPT(POST_HANDSHAKE_AUTH))
		init_flags |= GNUTLS_POST_HANDSHAKE_AUTH;

	inline_commands = HAVE_OPT(INLINE_COMMANDS);
	if (HAVE_OPT(INLINE_COMMANDS_PREFIX)) {
		if (strlen(OPT_ARG(INLINE_COMMANDS_PREFIX)) > 1) {
			fprintf(stderr,
				"inline-commands-prefix value is a single US-ASCII character (octets 0 - 127)\n");
			exit(1);
		}
		inline_commands_prefix =
		    (char *) OPT_ARG(INLINE_COMMANDS_PREFIX);
		if (!isascii(inline_commands_prefix[0])) {
			fprintf(stderr,
				"inline-commands-prefix value is a single US-ASCII character (octets 0 - 127)\n");
			exit(1);
		}
	} else
		inline_commands_prefix = "^";

	starttls = HAVE_OPT(STARTTLS);
	resume = HAVE_OPT(RESUME);
	rehandshake = HAVE_OPT(REHANDSHAKE);
	insecure = HAVE_OPT(INSECURE);
	ranges = HAVE_OPT(RANGES);

	if (insecure || HAVE_OPT(VERIFY_ALLOW_BROKEN)) {
		global_vflags |= GNUTLS_VERIFY_ALLOW_BROKEN;
	}

	udp = HAVE_OPT(UDP);
	mtu = OPT_VALUE_MTU;

	if (HAVE_OPT(PORT)) {
		snprintf(service, sizeof(service), "%s", OPT_ARG(PORT));
	} else {
		if (HAVE_OPT(STARTTLS_PROTO))
			snprintf(service, sizeof(service), "%s", starttls_proto_to_service(OPT_ARG(STARTTLS_PROTO)));
		else
			strcpy(service, "443");
	}

	record_max_size = OPT_VALUE_RECORDSIZE;

	if (HAVE_OPT(X509FMTDER))
		x509ctype = GNUTLS_X509_FMT_DER;
	else
		x509ctype = GNUTLS_X509_FMT_PEM;

	if (HAVE_OPT(SRPUSERNAME))
		srp_username = OPT_ARG(SRPUSERNAME);

	if (HAVE_OPT(SRPPASSWD))
		srp_passwd = OPT_ARG(SRPPASSWD);

	if (HAVE_OPT(X509CAFILE))
		x509_cafile = OPT_ARG(X509CAFILE);

	if (HAVE_OPT(X509CRLFILE))
		x509_crlfile = OPT_ARG(X509CRLFILE);

	if (HAVE_OPT(X509KEYFILE))
		x509_keyfile = OPT_ARG(X509KEYFILE);

	if (HAVE_OPT(X509CERTFILE))
		x509_certfile = OPT_ARG(X509CERTFILE);

	if (HAVE_OPT(RAWPKKEYFILE))
		rawpk_keyfile = OPT_ARG(RAWPKKEYFILE);

	if (HAVE_OPT(RAWPKFILE))
		rawpk_file = OPT_ARG(RAWPKFILE);

	if (HAVE_OPT(PSKUSERNAME))
		psk_username = OPT_ARG(PSKUSERNAME);

	if (HAVE_OPT(PSKKEY)) {
		psk_key.data = (unsigned char *) OPT_ARG(PSKKEY);
		psk_key.size = strlen(OPT_ARG(PSKKEY));
	} else
		psk_key.size = 0;

	crlf = HAVE_OPT(CRLF);

#ifdef TCP_FASTOPEN
	fastopen = HAVE_OPT(FASTOPEN);
#else
	if (HAVE_OPT(FASTOPEN)) {
		fprintf(stderr, "Warning: TCP Fast Open not supported on this OS\n");
	}
#endif

	if (rest != NULL)
		hostname = rest;

	if (hostname == NULL) {
		fprintf(stderr, "No hostname specified\n");
		exit(1);
	}
}

static void check_server_cmd(socket_st * socket, int ret)
{
	if (socket->secure) {
		if (ret == GNUTLS_E_REHANDSHAKE) {
			/* There is a race condition here. If application
			 * data is sent after the rehandshake request,
			 * the server thinks we ignored his request.
			 * This is a bad design of this client.
			 */
			log_msg(stdout, "*** Received rehandshake request\n");
			/* gnutls_alert_send( session, GNUTLS_AL_WARNING, GNUTLS_A_NO_RENEGOTIATION); */

			ret = do_handshake(socket);

			if (ret == 0) {
				log_msg(stdout, "*** Rehandshake was performed.\n");
			} else {
				log_msg(stdout, "*** Rehandshake Failed: %s\n", gnutls_strerror(ret));
			}
		} else if (ret == GNUTLS_E_REAUTH_REQUEST) {
			do {
				ret = gnutls_reauth(socket->session, 0);
			} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

			if (ret == 0) {
				log_msg(stdout, "*** Re-auth was performed.\n");
			} else {
				log_msg(stdout, "*** Re-auth failed: %s\n", gnutls_strerror(ret));
			}
		}
	}
}


int do_handshake(socket_st * socket)
{
	int ret;

	if (fastopen && socket->connect_addrlen) {
		gnutls_transport_set_fastopen(socket->session, socket->fd,
					      (struct sockaddr*)&socket->connect_addr,
					      socket->connect_addrlen, 0);
		socket->connect_addrlen = 0;
	} else {
		set_read_funcs(socket->session);
	}

	do {
		gnutls_handshake_set_timeout(socket->session,
					     GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
		ret = gnutls_handshake(socket->session);

		if (ret < 0) {
			handle_error(socket, ret);
		}
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret == 0) {
		/* print some information */
		print_info(socket->session, verbose, HAVE_OPT(X509CERTFILE)?P_WAIT_FOR_CERT:0);
		if (HAVE_OPT(KEYMATEXPORT))
			print_key_material(socket->session,
					   OPT_ARG(KEYMATEXPORT),
					   HAVE_OPT(KEYMATEXPORTSIZE) ?
					   OPT_VALUE_KEYMATEXPORTSIZE : 20);
		socket->secure = 1;
	} else {
		gnutls_alert_send_appropriate(socket->session, ret);
		shutdown(socket->fd, SHUT_RDWR);
	}
	return ret;
}

static int
srp_username_callback(gnutls_session_t session,
		      char **username, char **password)
{
	if (srp_username == NULL || srp_passwd == NULL) {
		return -1;
	}

	*username = gnutls_strdup(srp_username);
	*password = gnutls_strdup(srp_passwd);

	return 0;
}

static int
psk_callback(gnutls_session_t session, char **username,
	     gnutls_datum_t * key)
{
	const char *hint = gnutls_psk_client_get_hint(session);
	char *rawkey;
	char *passwd;
	int ret;
	size_t res_size;
	gnutls_datum_t tmp;

	log_msg(stdout, "- PSK client callback. ");
	if (hint)
		log_msg(stdout, "PSK hint '%s'\n", hint);
	else
		log_msg(stdout, "No PSK hint\n");

	if (HAVE_OPT(PSKUSERNAME))
		*username = gnutls_strdup(OPT_ARG(PSKUSERNAME));
	else {
		char *p = NULL;
		size_t n;

		log_msg(stdout, "Enter PSK identity: ");
		fflush(stdout);
		ret = getline(&p, &n, stdin);

		if (ret == -1 || p == NULL) {
			fprintf(stderr,
				"No username given, aborting...\n");
			return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
		}

		if (p[strlen(p) - 1] == '\n')
			p[strlen(p) - 1] = '\0';
		if (p[strlen(p) - 1] == '\r')
			p[strlen(p) - 1] = '\0';

		*username = gnutls_strdup(p);
		free(p);
	}
	if (!*username)
		return GNUTLS_E_MEMORY_ERROR;

	passwd = getpass("Enter key: ");
	if (passwd == NULL) {
		fprintf(stderr, "No key given, aborting...\n");
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	tmp.data = (void *) passwd;
	tmp.size = strlen(passwd);

	res_size = tmp.size / 2 + 1;
	rawkey = gnutls_malloc(res_size);
	if (rawkey == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	ret = gnutls_hex_decode(&tmp, rawkey, &res_size);
	if (ret < 0) {
		fprintf(stderr, "Error deriving password: %s\n",
			gnutls_strerror(ret));
		gnutls_free(rawkey);
		gnutls_free(*username);
		return ret;
	}

	key->data = (void *) rawkey;
	key->size = res_size;

	if (HAVE_OPT(DEBUG)) {
		char hexkey[41];
		res_size = sizeof(hexkey);
		ret = gnutls_hex_encode(key, hexkey, &res_size);
		if (ret < 0) {
			fprintf(stderr, "Error in hex encoding: %s\n", gnutls_strerror(ret));
			exit(1);
		}
		fprintf(stderr, "PSK username: %s\n", *username);
		fprintf(stderr, "PSK hint: %s\n", hint);
		fprintf(stderr, "PSK key: %s\n", hexkey);
	}

	return 0;
}

static void init_global_tls_stuff(void)
{
	int ret;

#ifdef ENABLE_PKCS11
	if (HAVE_OPT(PROVIDER)) {
		ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
		if (ret < 0)
			fprintf(stderr, "pkcs11_init: %s",
				gnutls_strerror(ret));
		else {
			ret =
			    gnutls_pkcs11_add_provider(OPT_ARG(PROVIDER),
						       NULL);
			if (ret < 0) {
				fprintf(stderr, "pkcs11_add_provider: %s",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	}
#endif

	/* X509 stuff */
	if (gnutls_certificate_allocate_credentials(&xcred) < 0) {
		fprintf(stderr, "Certificate allocation memory error\n");
		exit(1);
	}
	gnutls_certificate_set_pin_function(xcred, pin_callback, NULL);

	gnutls_certificate_set_verify_flags(xcred, global_vflags);
	gnutls_certificate_set_flags(xcred, GNUTLS_CERTIFICATE_VERIFY_CRLS);

	if (x509_cafile != NULL) {
		ret = gnutls_certificate_set_x509_trust_file(xcred,
							     x509_cafile,
							     x509ctype);
	} else {
		if (insecure == 0) {
			ret = gnutls_certificate_set_x509_system_trust(xcred);
			if (ret == GNUTLS_E_UNIMPLEMENTED_FEATURE) {
				fprintf(stderr, "Warning: this system doesn't support a default trust store\n");
				ret = 0;
			}
		} else {
			ret = 0;
		}
	}
	if (ret < 0) {
		fprintf(stderr, "Error setting the x509 trust file: %s\n", gnutls_strerror(ret));
		exit(1);
	} else {
		log_msg(stdout, "Processed %d CA certificate(s).\n", ret);
	}

	if (x509_crlfile != NULL) {
		ret =
		    gnutls_certificate_set_x509_crl_file(xcred,
							 x509_crlfile,
							 x509ctype);
		if (ret < 0) {
			fprintf(stderr,
				"Error setting the x509 CRL file: %s\n", gnutls_strerror(ret));
			exit(1);
		} else {
			log_msg(stdout, "Processed %d CRL(s).\n", ret);
		}
	}

	load_x509_keys();
	load_rawpk_keys();

#ifdef ENABLE_SRP
	if (srp_username && srp_passwd) {
		/* SRP stuff */
		if (gnutls_srp_allocate_client_credentials(&srp_cred) < 0) {
			fprintf(stderr, "SRP authentication error\n");
		}

		gnutls_srp_set_client_credentials_function(srp_cred,
							   srp_username_callback);
	}
#endif

#ifdef ENABLE_PSK
	/* PSK stuff */
	if (gnutls_psk_allocate_client_credentials(&psk_cred) < 0) {
		fprintf(stderr, "PSK authentication error\n");
	}

	if (psk_username && psk_key.data) {
		ret = gnutls_psk_set_client_credentials(psk_cred,
							psk_username,
							&psk_key,
							GNUTLS_PSK_KEY_HEX);
		if (ret < 0) {
			fprintf(stderr,
				"Error setting the PSK credentials: %s\n",
				gnutls_strerror(ret));
		}
	} else
		gnutls_psk_set_client_credentials_function(psk_cred,
							   psk_callback);
#endif

#ifdef ENABLE_ANON
	/* ANON stuff */
	if (gnutls_anon_allocate_client_credentials(&anon_cred) < 0) {
		fprintf(stderr, "Anonymous authentication error\n");
	}
#endif

}

/* OCSP check for the peer's certificate. Should be called
 * only after the certificate list verification is complete.
 * Returns:
 * -1: certificate chain could not be checked fully
 * >=0: number of certificates verified ok
 */
static int cert_verify_ocsp(gnutls_session_t session)
{
	gnutls_x509_crt_t cert, issuer;
	const gnutls_datum_t *cert_list;
	unsigned int cert_list_size = 0, ok = 0;
	unsigned failed = 0;
	int deinit_issuer = 0, deinit_cert = 0;
	gnutls_datum_t resp;
	unsigned char noncebuf[23];
	gnutls_datum_t nonce = { noncebuf, sizeof(noncebuf) };
	int ret;
	unsigned it;

	cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
	if (cert_list_size == 0) {
		fprintf(stderr, "No certificates found!\n");
		return 0;
	}

	for (it = 0; it < cert_list_size; it++) {
		if (deinit_cert)
			gnutls_x509_crt_deinit(cert);

		ret = gnutls_x509_crt_init(&cert);
		if (ret < 0) {
			fprintf(stderr, "Memory error: %s\n", gnutls_strerror(ret));
			goto cleanup;
		}

		deinit_cert = 1;
		ret = gnutls_x509_crt_import(cert, &cert_list[it], GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			fprintf(stderr, "Decoding error: %s\n", gnutls_strerror(ret));
			goto cleanup;
		}

		if (deinit_issuer) {
			gnutls_x509_crt_deinit(issuer);
			deinit_issuer = 0;
		}

		ret = gnutls_certificate_get_issuer(xcred, cert, &issuer, 0);
		if (ret < 0 && cert_list_size - it > 1) {
			ret = gnutls_x509_crt_init(&issuer);
			if (ret < 0) {
				fprintf(stderr, "Memory error: %s\n", gnutls_strerror(ret));
				goto cleanup;
			}
			deinit_issuer = 1;
			ret = gnutls_x509_crt_import(issuer, &cert_list[it + 1], GNUTLS_X509_FMT_DER);
			if (ret < 0) {
				fprintf(stderr, "Decoding error: %s\n", gnutls_strerror(ret));
				goto cleanup;
			}
		} else if (ret < 0) {
			if (it == 0)
				fprintf(stderr, "Cannot find issuer: %s\n", gnutls_strerror(ret));
			goto cleanup;
		}

		ret = gnutls_rnd(GNUTLS_RND_NONCE, nonce.data, nonce.size);
		if (ret < 0) {
			fprintf(stderr, "gnutls_rnd: %s", gnutls_strerror(ret));
			goto cleanup;
		}

		ret = send_ocsp_request(NULL, cert, issuer, &resp, &nonce);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			continue;
		}
		if (ret < 0) {
			fprintf(stderr, "Cannot contact OCSP server\n");
			goto cleanup;
		}

		/* verify and check the response for revoked cert */
		ret = check_ocsp_response(cert, issuer, &resp, &nonce, verbose);
		free(resp.data);
		if (ret == 1)
			ok++;
		else if (ret == 0) {
			failed++;
			break;
		}
	}

cleanup:
	if (deinit_issuer)
		gnutls_x509_crt_deinit(issuer);
	if (deinit_cert)
		gnutls_x509_crt_deinit(cert);

	if (failed > 0)
		return -1;
	return ok >= 1 ? (int) ok : -1;
}
