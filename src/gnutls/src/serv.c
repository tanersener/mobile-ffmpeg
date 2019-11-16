/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (C) 2001,2002 Paul Sheer
 * Copyright (C) 2016-2018 Red Hat, Inc.
 * Portions Copyright (C) 2002,2003 Nikos Mavrogiannopoulos
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

/* This server is heavily modified for GnuTLS by Nikos Mavrogiannopoulos
 * (which means it is quite unreadable)
 */

#include <config.h>

#include "common.h"
#include "serv-args.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <list.h>
#include <netdb.h>
#include <unistd.h>
#include <socket.h>

/* Gnulib portability files. */
#include "read-file.h"
#include "minmax.h"
#include "sockets.h"
#include "udp-serv.h"

/* konqueror cannot handle sending the page in multiple
 * pieces.
 */
/* global stuff */
static int generate = 0;
static int http = 0;
static int x509ctype;
static int debug = 0;

unsigned int verbose = 1;
static int nodb;
static int noticket;
static int earlydata;
int require_cert;
int disable_client_cert;

const char *psk_passwd = NULL;
const char *srp_passwd = NULL;
const char *srp_passwd_conf = NULL;
const char **x509_keyfile = NULL;
const char **x509_certfile = NULL;
unsigned x509_certfile_size = 0;
unsigned x509_keyfile_size = 0;
const char *x509_cafile = NULL;
const char *dh_params_file = NULL;
const char *x509_crlfile = NULL;
const char *priorities = NULL;

const char **ocsp_responses = NULL;
unsigned ocsp_responses_size = 0;

const char *sni_hostname = NULL;
int sni_hostname_fatal = 0;

const char **alpn_protos = NULL;
unsigned alpn_protos_size = 0;

gnutls_datum_t session_ticket_key;
gnutls_anti_replay_t anti_replay;
int record_max_size;
const char *http_data_file = NULL;
static void tcp_server(const char *name, int port);

/* end of globals */

/* This is a sample TCP echo server.
 * This will behave as an http server if any argument in the
 * command line is present
 */

#define SMALL_READ_TEST (2147483647)

#define GERR(ret) fprintf(stderr, "Error: %s\n", safe_strerror(ret))

#define HTTP_END  "</BODY></HTML>\n\n"

#define HTTP_UNIMPLEMENTED "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n<HTML><HEAD>\r\n<TITLE>501 Method Not Implemented</TITLE>\r\n</HEAD><BODY>\r\n<H1>Method Not Implemented</H1>\r\n<HR>\r\n</BODY></HTML>\r\n"
#define HTTP_OK "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"

#define HTTP_BEGIN HTTP_OK \
		"\n" \
		"<HTML><BODY>\n" \
		"<CENTER><H1>This is <a href=\"http://www.gnu.org/software/gnutls\">" \
		"GnuTLS</a></H1></CENTER>\n\n"

/* These are global */
gnutls_srp_server_credentials_t srp_cred = NULL;
gnutls_psk_server_credentials_t psk_cred = NULL;
gnutls_anon_server_credentials_t dh_cred = NULL;
gnutls_certificate_credentials_t cert_cred = NULL;

const int ssl_session_cache = 2048;

static void wrap_db_init(void);
static void wrap_db_deinit(void);
static int wrap_db_store(void *dbf, gnutls_datum_t key,
			 gnutls_datum_t data);
static gnutls_datum_t wrap_db_fetch(void *dbf, gnutls_datum_t key);
static int wrap_db_delete(void *dbf, gnutls_datum_t key);
static int anti_replay_db_add(void *dbf, time_t exp, const gnutls_datum_t *key,
			      const gnutls_datum_t *data);

static void cmd_parser(int argc, char **argv);


#define HTTP_STATE_REQUEST	1
#define HTTP_STATE_RESPONSE	2
#define HTTP_STATE_CLOSING	3

LIST_TYPE_DECLARE(listener_item, char *http_request; char *http_response;
		  int request_length; int response_length;
		  int response_written; int http_state;
		  int listen_socket; int fd;
		  gnutls_session_t tls_session;
		  int handshake_ok;
		  int close_ok;
		  time_t start;
		  int earlydata_eof;
    );

static const char *safe_strerror(int value)
{
	const char *ret = gnutls_strerror(value);
	if (ret == NULL)
		ret = str_unknown;
	return ret;
}

static void listener_free(listener_item * j)
{

	free(j->http_request);
	free(j->http_response);
	if (j->fd >= 0) {
		if (j->close_ok)
			gnutls_bye(j->tls_session, GNUTLS_SHUT_WR);
		shutdown(j->fd, 2);
		close(j->fd);
		gnutls_deinit(j->tls_session);
	}
}


/* we use primes up to 1024 in this server.
 * otherwise we should add them here.
 */

gnutls_dh_params_t dh_params = NULL;
gnutls_rsa_params_t rsa_params = NULL;

static int generate_dh_primes(void)
{
	int prime_bits =
	    gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH,
					GNUTLS_SEC_PARAM_MEDIUM);

	if (gnutls_dh_params_init(&dh_params) < 0) {
		fprintf(stderr, "Error in dh parameter initialization\n");
		exit(1);
	}

	/* Generate Diffie-Hellman parameters - for use with DHE
	 * kx algorithms. These should be discarded and regenerated
	 * once a week or once a month. Depends on the
	 * security requirements.
	 */
	printf
	    ("Generating Diffie-Hellman parameters [%d]. Please wait...\n",
	     prime_bits);
	fflush(stdout);

	if (gnutls_dh_params_generate2(dh_params, prime_bits) < 0) {
		fprintf(stderr, "Error in prime generation\n");
		exit(1);
	}

	return 0;
}

static void read_dh_params(void)
{
	char tmpdata[2048];
	int size;
	gnutls_datum_t params;
	FILE *fd;

	if (gnutls_dh_params_init(&dh_params) < 0) {
		fprintf(stderr, "Error in dh parameter initialization\n");
		exit(1);
	}

	/* read the params file
	 */
	fd = fopen(dh_params_file, "r");
	if (fd == NULL) {
		fprintf(stderr, "Could not open %s\n", dh_params_file);
		exit(1);
	}

	size = fread(tmpdata, 1, sizeof(tmpdata) - 1, fd);
	tmpdata[size] = 0;
	fclose(fd);

	params.data = (unsigned char *) tmpdata;
	params.size = size;

	size =
	    gnutls_dh_params_import_pkcs3(dh_params, &params,
					  GNUTLS_X509_FMT_PEM);

	if (size < 0) {
		fprintf(stderr, "Error parsing dh params: %s\n",
			safe_strerror(size));
		exit(1);
	}

	printf("Read Diffie-Hellman parameters.\n");
	fflush(stdout);

}

static int
get_params(gnutls_session_t session, gnutls_params_type_t type,
	   gnutls_params_st * st)
{

	if (type == GNUTLS_PARAMS_DH) {
		if (dh_params == NULL)
			return -1;
		st->params.dh = dh_params;
	} else
		return -1;

	st->type = type;
	st->deinit = 0;

	return 0;
}

LIST_DECLARE_INIT(listener_list, listener_item, listener_free);

static int cert_verify_callback(gnutls_session_t session)
{
listener_item * j = gnutls_session_get_ptr(session);
unsigned int size;
int ret;

	if (gnutls_auth_get_type(session) == GNUTLS_CRD_CERTIFICATE) {
		if (!require_cert && gnutls_certificate_get_peers(session, &size) == NULL)
			return 0;

		if (ENABLED_OPT(VERIFY_CLIENT_CERT)) {
			if (cert_verify(session, NULL, NULL) == 0) {
				do {
					ret = gnutls_alert_send(session, GNUTLS_AL_FATAL, GNUTLS_A_ACCESS_DENIED);
				} while(ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN);

				j->http_state = HTTP_STATE_CLOSING;
				return -1;
			}
		} else {
			printf("- Peer's certificate was NOT verified.\n");
		}
	}
	return 0;
}

/* callback used to verify if the host name advertised in client hello matches
 * the one configured in server
 */
static int
post_client_hello(gnutls_session_t session)
{
	int ret;
	/* DNS names (only type supported) may be at most 256 byte long */
	char *name;
	size_t len = 256;
	unsigned int type;
	int i;

	name = malloc(len);
	if (name == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	for (i=0; ; ) {
		ret = gnutls_server_name_get(session, name, &len, &type, i);
		if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER) {
			char *new_name;
			new_name = realloc(name, len);
			if (new_name == NULL) {
				ret = GNUTLS_E_MEMORY_ERROR;
				goto end;
			}
			name = new_name;
			continue; /* retry call with same index */
		}

		/* check if it is the last entry in list */
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;
		i++;
		if (ret != GNUTLS_E_SUCCESS)
			goto end;
		/* unknown types need to be ignored */
		if (type != GNUTLS_NAME_DNS)
			continue;

		if (strlen(sni_hostname) != len)
			continue;
		/* API guarantees that the name of type DNS will be null terminated */
		if (!strncmp(name, sni_hostname, len)) {
			ret = GNUTLS_E_SUCCESS;
			goto end;
		}
	};
	/* when there is no extension, we can't send the extension specific alert */
	if (i == 0) {
		fprintf(stderr, "Warning: client did not include SNI extension, using default host\n");
		ret = GNUTLS_E_SUCCESS;
		goto end;
	}

	if (sni_hostname_fatal == 1) {
		/* abort the connection, propagate error up the stack */
		ret = GNUTLS_E_UNRECOGNIZED_NAME;
		goto end;
	}

	fprintf(stderr, "Warning: client provided unrecognized host name\n");
	/* since we just want to send an alert, not abort the connection, we
	 * need to send it ourselves
	 */
	do {
		ret = gnutls_alert_send(session,
					GNUTLS_AL_WARNING,
					GNUTLS_A_UNRECOGNIZED_NAME);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	/* continue handshake, fall through */
end:
	free(name);
	return ret;
}

#define MAX_ALPN_PROTOCOLS 16
gnutls_session_t initialize_session(int dtls)
{
	gnutls_session_t session;
	int ret;
	unsigned i;
	const char *err;
	gnutls_datum_t alpn[MAX_ALPN_PROTOCOLS];
	unsigned alpn_size;
	unsigned flags = GNUTLS_SERVER | GNUTLS_POST_HANDSHAKE_AUTH;

	if (dtls)
		flags |= GNUTLS_DATAGRAM;

	if (earlydata)
		flags |= GNUTLS_ENABLE_EARLY_DATA;

	gnutls_init(&session, flags);

	/* allow the use of private ciphersuites.
	 */
	gnutls_handshake_set_private_extensions(session, 1);

	gnutls_handshake_set_timeout(session,
				     GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

	if (nodb == 0) {
		gnutls_db_set_retrieve_function(session, wrap_db_fetch);
		gnutls_db_set_remove_function(session, wrap_db_delete);
		gnutls_db_set_store_function(session, wrap_db_store);
		gnutls_db_set_ptr(session, NULL);
	}

	if (noticket == 0)
		gnutls_session_ticket_enable_server(session,
						    &session_ticket_key);

	if (earlydata) {
		gnutls_anti_replay_enable(session, anti_replay);
		if (HAVE_OPT(MAXEARLYDATA)) {
			ret = gnutls_record_set_max_early_data_size(session, OPT_VALUE_MAXEARLYDATA);
			if (ret < 0) {
				fprintf(stderr, "Could not set max early data size: %s\n", gnutls_strerror(ret));
				exit(1);
			}
		}
	}

	if (sni_hostname != NULL)
		gnutls_handshake_set_post_client_hello_function(session,
								&post_client_hello);

	if (priorities == NULL) {
		ret = gnutls_set_default_priority(session);
		if (ret < 0) {
			fprintf(stderr, "Could not set default policy: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	} else {
		ret = gnutls_priority_set_direct(session, priorities, &err);
		if (ret < 0) {
			fprintf(stderr, "Syntax error at: %s\n", err);
			exit(1);
		}
	}

	alpn_size = MIN(MAX_ALPN_PROTOCOLS,alpn_protos_size);
	for (i=0;i<alpn_size;i++) {
		alpn[i].data = (void*)alpn_protos[i];
		alpn[i].size = strlen(alpn_protos[i]);
	}

	ret = gnutls_alpn_set_protocols(session, alpn, alpn_size, HAVE_OPT(ALPN_FATAL)?GNUTLS_ALPN_MANDATORY:0);
	if (ret < 0) {
		fprintf(stderr, "Error setting ALPN protocols: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, dh_cred);

	if (srp_cred != NULL)
		gnutls_credentials_set(session, GNUTLS_CRD_SRP, srp_cred);

	if (psk_cred != NULL)
		gnutls_credentials_set(session, GNUTLS_CRD_PSK, psk_cred);

	if (cert_cred != NULL) {
		gnutls_certificate_set_verify_function(cert_cred,
					       cert_verify_callback);

		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				       cert_cred);
	}

	if (disable_client_cert)
		gnutls_certificate_server_set_request(session,
						      GNUTLS_CERT_IGNORE);
	else {
		if (require_cert)
			gnutls_certificate_server_set_request(session,
							      GNUTLS_CERT_REQUIRE);
		else
			gnutls_certificate_server_set_request(session,
							      GNUTLS_CERT_REQUEST);
	}

	/* use the record size limit extension */
	if (record_max_size > 0) {
		if (gnutls_record_set_max_recv_size(session, record_max_size) <
		    0) {
			fprintf(stderr,
				"Cannot set the maximum record receive size to %d.\n",
				record_max_size);
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

#include <gnutls/x509.h>

static const char DEFAULT_DATA[] =
    "This is the default message reported by the GnuTLS implementation. "
    "For more information please visit "
    "<a href=\"https://www.gnutls.org/\">https://www.gnutls.org/</a>.";

/* Creates html with the current session information.
 */
#define tmp_buffer &http_buffer[strlen(http_buffer)]
#define tmp_buffer_size len-strlen(http_buffer)
static char *peer_print_info(gnutls_session_t session, int *ret_length,
			     const char *header)
{
	const char *tmp;
	unsigned char sesid[32];
	size_t i, sesid_size;
	char *http_buffer, *desc;
	gnutls_kx_algorithm_t kx_alg;
	size_t len = 20 * 1024 + strlen(header);
	char *crtinfo = NULL, *crtinfo_old = NULL;
	gnutls_protocol_t version;
	size_t ncrtinfo = 0;

	if (verbose == 0) {
		http_buffer = malloc(len);
		if (http_buffer == NULL)
			return NULL;

		strcpy(http_buffer, HTTP_BEGIN);
		strcpy(&http_buffer[sizeof(HTTP_BEGIN) - 1], DEFAULT_DATA);
		strcpy(&http_buffer
		       [sizeof(HTTP_BEGIN) + sizeof(DEFAULT_DATA) - 2],
		       HTTP_END);
		*ret_length =
		    sizeof(DEFAULT_DATA) + sizeof(HTTP_BEGIN) +
		    sizeof(HTTP_END) - 3;
		return http_buffer;
	}

	if (gnutls_certificate_type_get(session) == GNUTLS_CRT_X509) {
		const gnutls_datum_t *cert_list;
		unsigned int cert_list_size = 0;

		cert_list =
		    gnutls_certificate_get_peers(session, &cert_list_size);

		for (i = 0; i < cert_list_size; i++) {
			gnutls_x509_crt_t cert = NULL;
			gnutls_datum_t info;

			if (gnutls_x509_crt_init(&cert) == 0 &&
			    gnutls_x509_crt_import(cert, &cert_list[i],
						   GNUTLS_X509_FMT_DER) ==
			    0
			    && gnutls_x509_crt_print(cert,
						     GNUTLS_CRT_PRINT_FULL,
						     &info) == 0) {
				const char *post = "</PRE><P><PRE>";
				
				crtinfo_old = crtinfo;
				crtinfo =
				    realloc(crtinfo,
					    ncrtinfo + info.size +
					    strlen(post) + 1);
				if (crtinfo == NULL) {
					free(crtinfo_old);
					return NULL;
				}
				memcpy(crtinfo + ncrtinfo, info.data,
				       info.size);
				ncrtinfo += info.size;
				memcpy(crtinfo + ncrtinfo, post,
				       strlen(post));
				ncrtinfo += strlen(post);
				crtinfo[ncrtinfo] = '\0';
				gnutls_free(info.data);
			}
			gnutls_x509_crt_deinit(cert);
		}
	}

	http_buffer = malloc(len);
	if (http_buffer == NULL) {
		free(crtinfo);
		return NULL;
	}

	strcpy(http_buffer, HTTP_BEGIN);

	version = gnutls_protocol_get_version(session);

	/* print session_id */
	sesid_size = sizeof(sesid);
	gnutls_session_get_id(session, sesid, &sesid_size);
	snprintf(tmp_buffer, tmp_buffer_size, "\n<p>Session ID: <i>");
	for (i = 0; i < sesid_size; i++)
		snprintf(tmp_buffer, tmp_buffer_size, "%.2X", sesid[i]);
	snprintf(tmp_buffer, tmp_buffer_size, "</i></p>\n");
	snprintf(tmp_buffer, tmp_buffer_size,
		 "<h5>If your browser supports session resumption, then you should see the "
		 "same session ID, when you press the <b>reload</b> button.</h5>\n");

	/* Here unlike print_info() we use the kx algorithm to distinguish
	 * the functions to call.
	 */
	{
		char dns[256];
		size_t dns_size = sizeof(dns);
		unsigned int type;

		if (gnutls_server_name_get
		    (session, dns, &dns_size, &type, 0) == 0) {
			snprintf(tmp_buffer, tmp_buffer_size,
				 "\n<p>Server Name: %s</p>\n", dns);
		}

	}

	kx_alg = gnutls_kx_get(session);

	/* print srp specific data */
#ifdef ENABLE_SRP
	if (kx_alg == GNUTLS_KX_SRP) {
		snprintf(tmp_buffer, tmp_buffer_size,
			 "<p>Connected as user '%s'.</p>\n",
			 gnutls_srp_server_get_username(session));
	}
#endif

#ifdef ENABLE_PSK
	if (kx_alg == GNUTLS_KX_PSK && gnutls_psk_server_get_username(session)) {
		snprintf(tmp_buffer, tmp_buffer_size,
			 "<p>Connected as user '%s'.</p>\n",
			 gnutls_psk_server_get_username(session));
	}
#endif


	/* print session information */
	strcat(http_buffer, "<P>\n");

	tmp =
	    gnutls_protocol_get_name(version);
	if (tmp == NULL)
		tmp = str_unknown;
	snprintf(tmp_buffer, tmp_buffer_size,
		 "<TABLE border=1><TR><TD>Protocol version:</TD><TD>%s</TD></TR>\n",
		 tmp);

	desc = gnutls_session_get_desc(session);
	if (desc) {
		snprintf(tmp_buffer, tmp_buffer_size,
			"<TR><TD>Description:</TD><TD>%s</TD></TR>\n",
			desc);
		gnutls_free(desc);
	}

	if (gnutls_auth_get_type(session) == GNUTLS_CRD_CERTIFICATE &&
	    gnutls_certificate_type_get(session) != GNUTLS_CRT_X509) {
		tmp =
		    gnutls_certificate_type_get_name
		    (gnutls_certificate_type_get(session));
		if (tmp == NULL)
			tmp = str_unknown;
		snprintf(tmp_buffer, tmp_buffer_size,
			 "<TR><TD>Certificate Type:</TD><TD>%s</TD></TR>\n",
			 tmp);
	}

	if (version < GNUTLS_TLS1_3) {
		tmp = gnutls_kx_get_name(kx_alg);
		if (tmp == NULL)
			tmp = str_unknown;
		snprintf(tmp_buffer, tmp_buffer_size,
			 "<TR><TD>Key Exchange:</TD><TD>%s</TD></TR>\n", tmp);

#ifdef ENABLE_ANON
		if (kx_alg == GNUTLS_KX_ANON_DH) {
			snprintf(tmp_buffer, tmp_buffer_size,
				 "<p> Connect using anonymous DH (prime of %d bits)</p>\n",
				 gnutls_dh_get_prime_bits(session));
		}
#endif

		if (kx_alg == GNUTLS_KX_DHE_RSA || kx_alg == GNUTLS_KX_DHE_DSS) {
			snprintf(tmp_buffer, tmp_buffer_size,
				 "Ephemeral DH using prime of <b>%d</b> bits.<br>\n",
				 gnutls_dh_get_prime_bits(session));
		}

		tmp = gnutls_compression_get_name(gnutls_compression_get(session));
		if (tmp == NULL)
			tmp = str_unknown;
		snprintf(tmp_buffer, tmp_buffer_size,
			 "<TR><TD>Compression</TD><TD>%s</TD></TR>\n", tmp);

		tmp = gnutls_cipher_suite_get_name(kx_alg,
						   gnutls_cipher_get(session),
						   gnutls_mac_get(session));
		if (tmp == NULL)
			tmp = str_unknown;
		snprintf(tmp_buffer, tmp_buffer_size,
			 "<TR><TD>Ciphersuite</TD><TD>%s</TD></TR></p></TABLE>\n",
			 tmp);
	}

	tmp = gnutls_cipher_get_name(gnutls_cipher_get(session));
	if (tmp == NULL)
		tmp = str_unknown;
	snprintf(tmp_buffer, tmp_buffer_size,
		 "<TR><TD>Cipher</TD><TD>%s</TD></TR>\n", tmp);

	tmp = gnutls_mac_get_name(gnutls_mac_get(session));
	if (tmp == NULL)
		tmp = str_unknown;
	snprintf(tmp_buffer, tmp_buffer_size,
		 "<TR><TD>MAC</TD><TD>%s</TD></TR>\n", tmp);


	if (crtinfo) {
		snprintf(tmp_buffer, tmp_buffer_size,
			 "<hr><PRE>%s\n</PRE>\n", crtinfo);
		free(crtinfo);
	}

	snprintf(tmp_buffer, tmp_buffer_size,
		 "<hr><P>Your HTTP header was:<PRE>%s</PRE></P>\n"
		 HTTP_END, header);

	*ret_length = strlen(http_buffer);

	return http_buffer;
}

static char *peer_print_data(gnutls_session_t session, int *ret_length)
{
	gnutls_datum_t data;
	char *http_buffer;
	size_t len;
	int ret;

	ret = gnutls_load_file(http_data_file, &data);
	if (ret < 0) {
		ret = asprintf(&http_buffer,
			       "HTTP/1.0 404 Not Found\r\n"
			       "Content-type: text/html\r\n"
			       "\r\n"
			       "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
			       "<BODY><H1>Couldn't read %s</H1></BODY></HTML>\n\n",
			       http_data_file);
		if (ret < 0)
			return NULL;

		*ret_length = strlen(http_buffer);
		return http_buffer;
	}

	ret = asprintf(&http_buffer,
		       "HTTP/1.0 200 OK\r\n"
		       "Content-Type: application/octet-stream\r\n"
		       "Content-Length: %u\r\n"
		       "\r\n",
		       data.size);
	if (ret < 0)
		return NULL;
	len = ret;
	http_buffer = realloc(http_buffer, len + data.size);
	memcpy(&http_buffer[len], data.data, data.size);
	gnutls_free(data.data);
	*ret_length = len + data.size;
	return http_buffer;
}

const char *human_addr(const struct sockaddr *sa, socklen_t salen,
		       char *buf, size_t buflen)
{
	const char *save_buf = buf;
	size_t l;

	if (!buf || !buflen)
		return "(error)";

	*buf = 0;

	switch (sa->sa_family) {
#if HAVE_IPV6
	case AF_INET6:
		snprintf(buf, buflen, "IPv6 ");
		break;
#endif

	case AF_INET:
		snprintf(buf, buflen, "IPv4 ");
		break;
	}

	l = 5;
	buf += l;
	buflen -= l;

	if (getnameinfo(sa, salen, buf, buflen, NULL, 0, NI_NUMERICHOST) !=
	    0) {	
		return "(error)";
	}

	l = strlen(buf);
	buf += l;
	buflen -= l;

	if (buflen < 8)
		return save_buf;

	strcat(buf, " port ");
	buf += 6;
	buflen -= 6;

	if (getnameinfo(sa, salen, NULL, 0, buf, buflen, NI_NUMERICSERV) !=
	    0) {
		snprintf(buf, buflen, "%s", " unknown");
	}

	return save_buf;
}

int wait_for_connection(void)
{
	listener_item *j;
	fd_set rd, wr;
	int n, sock = -1;

	FD_ZERO(&rd);
	FD_ZERO(&wr);
	n = 0;

	lloopstart(listener_list, j) {
		if (j->listen_socket) {
			FD_SET(j->fd, &rd);
			n = MAX(n, j->fd);
		}
	}
	lloopend(listener_list, j);

	/* waiting part */
	n = select(n + 1, &rd, &wr, NULL, NULL);
	if (n == -1 && errno == EINTR)
		return -1;
	if (n < 0) {
		perror("select()");
		exit(1);
	}

	/* find which one is ready */
	lloopstart(listener_list, j) {
		/* a new connection has arrived */
		if (FD_ISSET(j->fd, &rd) && j->listen_socket) {
			sock = j->fd;
			break;
		}
	}
	lloopend(listener_list, j);
	return sock;
}

int listen_socket(const char *name, int listen_port, int socktype)
{
	struct addrinfo hints, *res, *ptr;
	char portname[6];
	int s = -1;
	int yes;
	listener_item *j = NULL;

	snprintf(portname, sizeof(portname), "%d", listen_port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = socktype;
	hints.ai_flags = AI_PASSIVE
#ifdef AI_ADDRCONFIG
	    | AI_ADDRCONFIG
#endif
	    ;

	if ((s = getaddrinfo(NULL, portname, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo() failed: %s\n",
			gai_strerror(s));
		return -1;
	}

	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		int news;
#ifndef HAVE_IPV6
		if (ptr->ai_family != AF_INET)
			continue;
#endif

		/* Print what we are doing. */
		{
			char topbuf[512];

			fprintf(stderr, "%s listening on %s...",
				name, human_addr(ptr->ai_addr,
						 ptr->ai_addrlen, topbuf,
						 sizeof(topbuf)));
		}

		if ((news = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol)) < 0) {
			perror("socket() failed");
			continue;
		}
		s = news; /* to not overwrite existing s from previous loops */
#if defined(HAVE_IPV6) && !defined(_WIN32)
		if (ptr->ai_family == AF_INET6) {
			yes = 1;
			/* avoid listen on ipv6 addresses failing
			 * because already listening on ipv4 addresses: */
			(void)setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,
				   (const void *) &yes, sizeof(yes));
		}
#endif

		if (socktype == SOCK_STREAM) {
			yes = 1;
			if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
				       (const void *) &yes,
				       sizeof(yes)) < 0) {
				perror("setsockopt() failed");
				close(s);
				continue;
			}
		} else {
#if defined(IP_DONTFRAG)
			yes = 1;
			if (setsockopt(s, IPPROTO_IP, IP_DONTFRAG,
				       (const void *) &yes,
				       sizeof(yes)) < 0)
				perror("setsockopt(IP_DF) failed");
#elif defined(IP_MTU_DISCOVER)
			yes = IP_PMTUDISC_DO;
			if (setsockopt(s, IPPROTO_IP, IP_MTU_DISCOVER,
				       (const void *) &yes,
				       sizeof(yes)) < 0)
				perror("setsockopt(IP_DF) failed");
#endif
		}

		if (bind(s, ptr->ai_addr, ptr->ai_addrlen) < 0) {
			perror("bind() failed");
			close(s);
			continue;
		}

		if (socktype == SOCK_STREAM) {
			if (listen(s, 10) < 0) {
				perror("listen() failed");
				exit(1);
			}
		}

		/* new list entry for the connection */
		lappend(listener_list);
		j = listener_list.tail;
		j->listen_socket = 1;
		j->fd = s;

		/* Complete earlier message. */
		fprintf(stderr, "done\n");
	}

	fflush(stderr);

	freeaddrinfo(res);

	return s;
}

/* strips \r\n from the end of the string 
 */
static void strip(char *data)
{
	int i;
	int len = strlen(data);

	for (i = 0; i < len; i++) {
		if (data[i] == '\r' && data[i + 1] == '\n'
		    && data[i + 2] == 0) {
			data[i] = '\n';
			data[i + 1] = 0;
			break;
		}
	}
}

static void
get_response(gnutls_session_t session, char *request,
	     char **response, int *response_length)
{
	char *p, *h;

	if (http != 0) {
		if (strncmp(request, "GET ", 4))
			goto unimplemented;

		if (!(h = strchr(request, '\n')))
			goto unimplemented;

		*h++ = '\0';
		while (*h == '\r' || *h == '\n')
			h++;

		if (!(p = strchr(request + 4, ' ')))
			goto unimplemented;
		*p = '\0';
	}
/*    *response = peer_print_info(session, request+4, h, response_length); */
	if (http != 0) {
		if (http_data_file == NULL)
			*response = peer_print_info(session, response_length, h);
		else
			*response = peer_print_data(session, response_length);
	} else {
		int ret;
		strip(request);
		fprintf(stderr, "received cmd: %s\n", request);

		ret = check_command(session, request, disable_client_cert);
		if (ret > 0) {
			*response = strdup("Successfully executed command\n");
			if (*response == NULL) {
				fprintf(stderr, "Memory error\n");
				exit(1);
			}
			*response_length = strlen(*response);
			return;
		} else if (ret == 0) {
			*response = strdup(request);
			*response_length = ((*response) ? strlen(*response) : 0);
		} else {
			do {
				ret = gnutls_alert_send(session, GNUTLS_AL_FATAL, GNUTLS_A_UNEXPECTED_MESSAGE);
			} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		}
	}

	return;

      unimplemented:
	*response = strdup(HTTP_UNIMPLEMENTED);
	*response_length = ((*response) ? strlen(*response) : 0);
}

static void terminate(int sig) __attribute__ ((noreturn));

static void terminate(int sig)
{
	fprintf(stderr, "Exiting via signal %d\n", sig);
	exit(1);
}


static void check_alert(gnutls_session_t session, int ret)
{
	if (ret == GNUTLS_E_WARNING_ALERT_RECEIVED
	    || ret == GNUTLS_E_FATAL_ALERT_RECEIVED) {
		int last_alert = gnutls_alert_get(session);
		if (last_alert == GNUTLS_A_NO_RENEGOTIATION &&
		    ret == GNUTLS_E_WARNING_ALERT_RECEIVED)
			printf
			    ("* Received NO_RENEGOTIATION alert. Client does not support renegotiation.\n");
		else
			printf("* Received alert '%d': %s.\n", last_alert,
			       gnutls_alert_get_name(last_alert));
	}
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static void tls_audit_log_func(gnutls_session_t session, const char *str)
{
	fprintf(stderr, "|<%p>| %s", session, str);
}

int main(int argc, char **argv)
{
	int ret, mtu, port;
	char name[256];
	int cert_set = 0;
	unsigned use_static_dh_params = 0;
	unsigned i;

	cmd_parser(argc, argv);

#ifndef _WIN32
	signal(SIGHUP, SIG_IGN);
	signal(SIGTERM, terminate);
	if (signal(SIGINT, terminate) == SIG_IGN)
		signal(SIGINT, SIG_IGN);	/* e.g. background process */
#endif

	sockets_init();

	if (nodb == 0)
		wrap_db_init();

	if (HAVE_OPT(UDP))
		strcpy(name, "UDP ");
	else
		name[0] = 0;

	if (http == 1) {
		strcat(name, "HTTP Server");
	} else {
		strcat(name, "Echo Server");
	}

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_audit_log_function(tls_audit_log_func);
	gnutls_global_set_log_level(debug);

	if ((ret = gnutls_global_init()) < 0) {
		fprintf(stderr, "global_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}
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
	pkcs11_common(NULL);
#endif

	/* Note that servers must generate parameters for
	 * Diffie-Hellman. See gnutls_dh_params_generate(), and
	 * gnutls_dh_params_set().
	 */
	if (generate != 0) {
		generate_dh_primes();
	} else if (dh_params_file) {
		read_dh_params();
	} else {
		use_static_dh_params = 1;
	}

	if (gnutls_certificate_allocate_credentials(&cert_cred) < 0) {
		fprintf(stderr, "memory error\n");
		exit(1);
	}

	if (x509_cafile != NULL) {
		if ((ret = gnutls_certificate_set_x509_trust_file
		     (cert_cred, x509_cafile, x509ctype)) < 0) {
			fprintf(stderr, "Error reading '%s'\n",
				x509_cafile);
			GERR(ret);
			exit(1);
		} else {
			printf("Processed %d CA certificate(s).\n", ret);
		}
	}
	if (x509_crlfile != NULL) {
		if ((ret = gnutls_certificate_set_x509_crl_file
		     (cert_cred, x509_crlfile, x509ctype)) < 0) {
			fprintf(stderr, "Error reading '%s'\n",
				x509_crlfile);
			GERR(ret);
			exit(1);
		} else {
			printf("Processed %d CRL(s).\n", ret);
		}
	}

	if (x509_certfile_size > 0 && x509_keyfile_size > 0) {
		for (i = 0; i < x509_certfile_size; i++) {
			ret = gnutls_certificate_set_x509_key_file
			    (cert_cred, x509_certfile[i], x509_keyfile[i], x509ctype);
			if (ret < 0) {
				fprintf(stderr,
						"Error reading '%s' or '%s'\n",
						x509_certfile[i], x509_keyfile[i]);
				GERR(ret);
				exit(1);
			} else
				cert_set = 1;
		}
	}

	if (cert_set == 0) {
		fprintf(stderr,
			"Warning: no private key and certificate pairs were set.\n");
	}

	/* OCSP status-request TLS extension */
	if (HAVE_OPT(IGNORE_OCSP_RESPONSE_ERRORS))
		gnutls_certificate_set_flags(cert_cred, GNUTLS_CERTIFICATE_SKIP_OCSP_RESPONSE_CHECK);

	for (i = 0; i < ocsp_responses_size; i++ ) {
		ret = gnutls_certificate_set_ocsp_status_request_file
		    (cert_cred, ocsp_responses[i], 0);
		if (ret < 0) {
			fprintf(stderr,
				"Cannot set OCSP status request file: %s: %s\n",
				ocsp_responses[i],
				gnutls_strerror(ret));
			exit(1);
		}
	}

	if (use_static_dh_params) {
		ret = gnutls_certificate_set_known_dh_params(cert_cred, GNUTLS_SEC_PARAM_MEDIUM);
		if (ret < 0) {
			fprintf(stderr, "Error while setting DH parameters: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	} else {
		gnutls_certificate_set_params_function(cert_cred, get_params);
	}

	/* this is a password file (created with the included srpcrypt utility) 
	 * Read README.crypt prior to using SRP.
	 */
#ifdef ENABLE_SRP
	if (srp_passwd != NULL) {
		gnutls_srp_allocate_server_credentials(&srp_cred);

		if ((ret =
		     gnutls_srp_set_server_credentials_file(srp_cred,
							    srp_passwd,
							    srp_passwd_conf))
		    < 0) {
			/* only exit is this function is not disabled 
			 */
			fprintf(stderr,
				"Error while setting SRP parameters\n");
			GERR(ret);
		}
	}
#endif

	/* this is a password file 
	 */
#ifdef ENABLE_PSK
	if (psk_passwd != NULL) {
		gnutls_psk_allocate_server_credentials(&psk_cred);

		if ((ret =
		     gnutls_psk_set_server_credentials_file(psk_cred,
							    psk_passwd)) <
		    0) {
			/* only exit is this function is not disabled 
			 */
			fprintf(stderr,
				"Error while setting PSK parameters\n");
			GERR(ret);
		}

		if (HAVE_OPT(PSKHINT)) {
			ret =
			    gnutls_psk_set_server_credentials_hint
			    (psk_cred, OPT_ARG(PSKHINT));
			if (ret) {
				fprintf(stderr,
					"Error setting PSK identity hint.\n");
				GERR(ret);
			}
		}

		if (use_static_dh_params) {
			ret = gnutls_psk_set_server_known_dh_params(psk_cred, GNUTLS_SEC_PARAM_MEDIUM);
			if (ret < 0) {
				fprintf(stderr, "Error while setting DH parameters: %s\n", gnutls_strerror(ret));
				exit(1);
			}
		} else {
			gnutls_psk_set_server_params_function(psk_cred,
							      get_params);
		}
	}
#endif

#ifdef ENABLE_ANON
	gnutls_anon_allocate_server_credentials(&dh_cred);

	if (use_static_dh_params) {
		ret = gnutls_anon_set_server_known_dh_params(dh_cred, GNUTLS_SEC_PARAM_MEDIUM);
		if (ret < 0) {
			fprintf(stderr, "Error while setting DH parameters: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	} else {
		gnutls_anon_set_server_params_function(dh_cred, get_params);
	}
#endif

	if (noticket == 0)
		gnutls_session_ticket_key_generate(&session_ticket_key);

	if (earlydata) {
		ret = gnutls_anti_replay_init(&anti_replay);
		if (ret < 0) {
			fprintf(stderr, "Error while initializing anti-replay: %s\n", gnutls_strerror(ret));
			exit(1);
		}
		gnutls_anti_replay_set_add_function(anti_replay, anti_replay_db_add);
		gnutls_anti_replay_set_ptr(anti_replay, NULL);
	}

	if (HAVE_OPT(MTU))
		mtu = OPT_VALUE_MTU;
	else
		mtu = 1300;

	if (HAVE_OPT(PORT))
		port = OPT_VALUE_PORT;
	else
		port = 5556;

	if (HAVE_OPT(UDP))
		udp_server(name, port, mtu);
	else
		tcp_server(name, port);

	return 0;
}

static void retry_handshake(listener_item *j)
{
	int r, ret;

	r = gnutls_handshake(j->tls_session);
	if (r < 0 && gnutls_error_is_fatal(r) == 0) {
		check_alert(j->tls_session, r);
		/* nothing */
	} else if (r < 0) {
		j->http_state = HTTP_STATE_CLOSING;
		check_alert(j->tls_session, r);
		fprintf(stderr, "Error in handshake: %s\n", gnutls_strerror(r));

		do {
			ret = gnutls_alert_send_appropriate(j->tls_session, r);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		j->close_ok = 0;
	} else if (r == 0) {
		if (gnutls_session_is_resumed(j->tls_session) != 0 && verbose != 0)
			printf("*** This is a resumed session\n");

		if (verbose != 0) {
#if 0
			printf("- connection from %s\n",
			     human_addr((struct sockaddr *)
				&client_address,
				calen,
				topbuf,
				sizeof(topbuf)));
#endif

			print_info(j->tls_session, verbose, verbose);

			if (HAVE_OPT(KEYMATEXPORT))
				print_key_material(j->tls_session,
						   OPT_ARG(KEYMATEXPORT),
						   HAVE_OPT(KEYMATEXPORTSIZE) ?
						   OPT_VALUE_KEYMATEXPORTSIZE :
						   20);
		}

		j->close_ok = 1;
		j->handshake_ok = 1;
	}
}

static void try_rehandshake(listener_item *j)
{
	int r, ret;
	fprintf(stderr, "*** Received hello message\n");

	do {
		r = gnutls_handshake(j->tls_session);
	} while (r == GNUTLS_E_INTERRUPTED || r == GNUTLS_E_AGAIN);

	if (r < 0) {
		do {
			ret = gnutls_alert_send_appropriate(j->tls_session, r);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		fprintf(stderr, "Error in rehandshake: %s\n", gnutls_strerror(r));
		j->http_state = HTTP_STATE_CLOSING;
	} else {
		j->close_ok = 1;
		j->http_state = HTTP_STATE_REQUEST;
	}
}

static void tcp_server(const char *name, int port)
{
	int n, s;
	char topbuf[512];
	int accept_fd;
	struct sockaddr_storage client_address;
	socklen_t calen;
	struct timeval tv;

	s = listen_socket(name, port, SOCK_STREAM);
	if (s < 0)
		exit(1);

	for (;;) {
		listener_item *j;
		fd_set rd, wr;
		time_t now = time(0);
#ifndef _WIN32
		int val;
#endif

		FD_ZERO(&rd);
		FD_ZERO(&wr);
		n = 0;

/* flag which connections we are reading or writing to within the fd sets */
		lloopstart(listener_list, j) {

#ifndef _WIN32
			val = fcntl(j->fd, F_GETFL, 0);
			if ((val == -1)
			    || (fcntl(j->fd, F_SETFL, val | O_NONBLOCK) <
				0)) {
				perror("fcntl()");
				exit(1);
			}
#endif
			if (j->start != 0 && now - j->start > 30) {
				if (verbose != 0) {
					fprintf(stderr, "Scheduling inactive connection for close\n");
				}
				j->http_state = HTTP_STATE_CLOSING;
			}

			if (j->listen_socket) {
				FD_SET(j->fd, &rd);
				n = MAX(n, j->fd);
			}
			if (j->http_state == HTTP_STATE_REQUEST) {
				FD_SET(j->fd, &rd);
				n = MAX(n, j->fd);
			}
			if (j->http_state == HTTP_STATE_RESPONSE) {
				FD_SET(j->fd, &wr);
				n = MAX(n, j->fd);
			}
		}
		lloopend(listener_list, j);

/* core operation */
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		n = select(n + 1, &rd, &wr, NULL, &tv);
		if (n == -1 && errno == EINTR)
			continue;
		if (n < 0) {
			perror("select()");
			exit(1);
		}

/* read or write to each connection as indicated by select()'s return argument */
		lloopstart(listener_list, j) {

			/* a new connection has arrived */
			if (FD_ISSET(j->fd, &rd) && j->listen_socket) {
				calen = sizeof(client_address);
				memset(&client_address, 0, calen);
				accept_fd =
				    accept(j->fd,
					   (struct sockaddr *)
					   &client_address, &calen);

				if (accept_fd < 0) {
					perror("accept()");
				} else {
					char timebuf[SIMPLE_CTIME_BUF_SIZE];
					time_t tt = time(0);
					char *ctt;

					/* new list entry for the connection */
					lappend(listener_list);
					j = listener_list.tail;
					j->http_request =
					    (char *) strdup("");
					j->http_state = HTTP_STATE_REQUEST;
					j->fd = accept_fd;
					j->start = tt;

					j->tls_session = initialize_session(0);
					gnutls_session_set_ptr(j->tls_session, j);
					gnutls_transport_set_int
					    (j->tls_session, accept_fd);
					set_read_funcs(j->tls_session);
					j->handshake_ok = 0;
					j->close_ok = 0;

					if (verbose != 0) {
						ctt = simple_ctime(&tt, timebuf);
						ctt[strlen(ctt) - 1] = 0;

						printf
						    ("\n* Accepted connection from %s on %s\n",
						     human_addr((struct
								 sockaddr
								 *)
								&client_address,
								calen,
								topbuf,
								sizeof
								(topbuf)),
						     ctt);
					}
				}
			}

			if (FD_ISSET(j->fd, &rd) && !j->listen_socket) {
/* read partial GET request */
				char buf[16*1024];
				int r;

				if (j->handshake_ok == 0) {
					retry_handshake(j);
				}

				if (j->handshake_ok == 1) {
					int earlydata_read = 0;
					if (earlydata && !j->earlydata_eof) {
						r = gnutls_record_recv_early_data(j->
										  tls_session,
										  buf,
										  MIN(sizeof(buf),
										      SMALL_READ_TEST));
						if (r == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
							j->earlydata_eof = 1;
						}
						if (r == 0) {
							earlydata_read = 1;
						}
					}
					if (!earlydata_read) {
						r = gnutls_record_recv(j->
								       tls_session,
								       buf,
								       MIN(sizeof(buf),
									   SMALL_READ_TEST));
					}
					if (r == GNUTLS_E_INTERRUPTED || r == GNUTLS_E_AGAIN) {
						/* do nothing */
					} else if (r <= 0) {
						if (r == GNUTLS_E_HEARTBEAT_PING_RECEIVED) {
							gnutls_heartbeat_pong(j->tls_session, 0);
						} else if (r == GNUTLS_E_REHANDSHAKE) {
							try_rehandshake(j);
						} else {
							j->http_state = HTTP_STATE_CLOSING;
							if (r < 0) {
								int ret;
								check_alert(j->tls_session, r);
								fprintf(stderr,
								     "Error while receiving data\n");
								do {
									ret = gnutls_alert_send_appropriate(j->tls_session, r);
								} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
								GERR(r);
								j->close_ok = 0;
							}
						}
					} else {
						j->http_request =
						    realloc(j->
							    http_request,
							    j->
							    request_length
							    + r + 1);
						if (j->http_request !=
						    NULL) {
							memcpy(j->
							       http_request
							       +
							       j->
							       request_length,
							       buf, r);
							j->request_length
							    += r;
							j->http_request[j->
									request_length]
							    = '\0';
						} else {
							j->http_state =
							    HTTP_STATE_CLOSING;
						}
					}
/* check if we have a full HTTP header */

					j->http_response = NULL;
					if (j->http_state == HTTP_STATE_REQUEST && j->http_request != NULL) {
						if ((http == 0
						     && strchr(j->
							       http_request,
							       '\n'))
						    || strstr(j->
							      http_request,
							      "\r\n\r\n")
						    || strstr(j->
							      http_request,
							      "\n\n")) {
							get_response(j->
								     tls_session,
								     j->
								     http_request,
								     &j->
								     http_response,
								     &j->
								     response_length);
							j->http_state =
							    HTTP_STATE_RESPONSE;
							j->response_written
							    = 0;
						}
					}
				}
			}

			if (FD_ISSET(j->fd, &wr)) {
/* write partial response request */
				int r;

				if (j->handshake_ok == 0) {
					retry_handshake(j);
				}

				if (j->handshake_ok == 1 && j->http_response == NULL) {
					j->http_state = HTTP_STATE_CLOSING;
				} else if (j->handshake_ok == 1 && j->http_response != NULL) {
					r = gnutls_record_send(j->tls_session,
							       j->http_response
							       +
							       j->response_written,
							       MIN(j->response_length
								   -
								   j->response_written,
								   SMALL_READ_TEST));
					if (r == GNUTLS_E_INTERRUPTED || r == GNUTLS_E_AGAIN) {
						/* do nothing */
					} else if (r <= 0) {
						j->http_state = HTTP_STATE_CLOSING;
						if (r < 0) {
							fprintf(stderr,
								"Error while sending data\n");
							GERR(r);
						}
						check_alert(j->tls_session,
							    r);
					} else {
						j->response_written += r;
/* check if we have written a complete response */
						if (j->response_written ==
						    j->response_length) {
							if (http != 0)
								j->http_state = HTTP_STATE_CLOSING;
							else {
								j->http_state = HTTP_STATE_REQUEST;
								free(j->
								     http_response);
								j->http_response = NULL;
								j->response_length = 0;
								j->request_length = 0;
								j->http_request[0] = 0;
							}
						}
					}
				} else {
					j->request_length = 0;
					j->http_request[0] = 0;
					j->http_state = HTTP_STATE_REQUEST;
				}
			}
		}
		lloopend(listener_list, j);

/* loop through all connections, closing those that are in error */
		lloopstart(listener_list, j) {
			if (j->http_state == HTTP_STATE_CLOSING) {
				ldeleteinc(listener_list, j);
			}
		}
		lloopend(listener_list, j);
	}


	gnutls_certificate_free_credentials(cert_cred);

#ifdef ENABLE_SRP
	if (srp_cred)
		gnutls_srp_free_server_credentials(srp_cred);
#endif

#ifdef ENABLE_PSK
	if (psk_cred)
		gnutls_psk_free_server_credentials(psk_cred);
#endif

#ifdef ENABLE_ANON
	gnutls_anon_free_server_credentials(dh_cred);
#endif

	if (noticket == 0)
		gnutls_free(session_ticket_key.data);

	if (earlydata)
		gnutls_anti_replay_deinit(anti_replay);

	if (nodb == 0)
		wrap_db_deinit();
	gnutls_global_deinit();

}

static void cmd_parser(int argc, char **argv)
{
	optionProcess(&gnutls_servOptions, argc, argv);

	disable_client_cert = HAVE_OPT(DISABLE_CLIENT_CERT);
	require_cert = ENABLED_OPT(REQUIRE_CLIENT_CERT);
	if (HAVE_OPT(DEBUG))
		debug = OPT_VALUE_DEBUG;

	if (HAVE_OPT(QUIET))
		verbose = 0;

	if (HAVE_OPT(PRIORITY))
		priorities = OPT_ARG(PRIORITY);

	if (HAVE_OPT(LIST)) {
		print_list(priorities, verbose);
		exit(0);
	}

	nodb = HAVE_OPT(NODB);
	noticket = HAVE_OPT(NOTICKET);
	earlydata = HAVE_OPT(EARLYDATA);

	if (HAVE_OPT(ECHO))
		http = 0;
	else
		http = 1;

	record_max_size = OPT_VALUE_RECORDSIZE;

	if (HAVE_OPT(X509FMTDER))
		x509ctype = GNUTLS_X509_FMT_DER;
	else
		x509ctype = GNUTLS_X509_FMT_PEM;

	generate = HAVE_OPT(GENERATE);

	if (HAVE_OPT(DHPARAMS))
		dh_params_file = OPT_ARG(DHPARAMS);

	if (HAVE_OPT(ALPN)) {
		alpn_protos = STACKLST_OPT(ALPN);
		alpn_protos_size = STACKCT_OPT(ALPN);
	}

	if (HAVE_OPT(X509KEYFILE)) {
		x509_keyfile = STACKLST_OPT(X509KEYFILE);
		x509_keyfile_size = STACKCT_OPT(X509KEYFILE);
	}

	if (HAVE_OPT(X509CERTFILE)) {
		x509_certfile = STACKLST_OPT(X509CERTFILE);
		x509_certfile_size = STACKCT_OPT(X509CERTFILE);
	}

	if (x509_certfile_size != x509_keyfile_size) {
		fprintf(stderr, "The certificate number provided (%u) doesn't match the keys (%u)\n",
			x509_certfile_size, x509_keyfile_size);
	}

	if (HAVE_OPT(X509CAFILE))
		x509_cafile = OPT_ARG(X509CAFILE);
	if (HAVE_OPT(X509CRLFILE))
		x509_crlfile = OPT_ARG(X509CRLFILE);

	if (HAVE_OPT(SRPPASSWD))
		srp_passwd = OPT_ARG(SRPPASSWD);
	if (HAVE_OPT(SRPPASSWDCONF))
		srp_passwd_conf = OPT_ARG(SRPPASSWDCONF);

	if (HAVE_OPT(PSKPASSWD))
		psk_passwd = OPT_ARG(PSKPASSWD);

	if (HAVE_OPT(OCSP_RESPONSE)) {
		ocsp_responses = STACKLST_OPT(OCSP_RESPONSE);
		ocsp_responses_size = STACKCT_OPT(OCSP_RESPONSE);
	}

	if (HAVE_OPT(SNI_HOSTNAME))
		sni_hostname = OPT_ARG(SNI_HOSTNAME);

	if (HAVE_OPT(SNI_HOSTNAME_FATAL))
		sni_hostname_fatal = 1;

	if (HAVE_OPT(HTTPDATA))
		http_data_file = OPT_ARG(HTTPDATA);

}

/* session resuming support */

#define SESSION_ID_SIZE 128
#define SESSION_DATA_SIZE (16*1024)

typedef struct {
	unsigned char session_id[SESSION_ID_SIZE];
	unsigned int session_id_size;

	gnutls_datum_t session_data;
} CACHE;

static CACHE *cache_db;
static int cache_db_ptr;
static int cache_db_alloc;

static void wrap_db_init(void)
{
}

static void wrap_db_deinit(void)
{
	int i;

	for (i = 0; i < cache_db_ptr; i++)
		free(cache_db[i].session_data.data);
	free(cache_db);
}

static int
wrap_db_store(void *dbf, gnutls_datum_t key, gnutls_datum_t data)
{
	int i;
	time_t now = time(0);

	if (key.size > SESSION_ID_SIZE)
		return GNUTLS_E_DB_ERROR;
	if (data.size > SESSION_DATA_SIZE)
		return GNUTLS_E_DB_ERROR;

	if (cache_db_ptr < cache_db_alloc)
		i = cache_db_ptr++;
	else {
		/* find empty or expired slot to store the new entry */
		for (i = 0; i < cache_db_ptr; i++)
			if (cache_db[i].session_id_size == 0 ||
			    !(now <
			      gnutls_db_check_entry_expire_time(&cache_db[i].
								session_data)))
				break;

		if (i == cache_db_ptr) {
			/* try to allocate additional slots */
			if (cache_db_ptr == ssl_session_cache) {
				fprintf(stderr,
					"Error: too many sessions\n");
				return GNUTLS_E_DB_ERROR;
			}
			cache_db_alloc = cache_db_alloc * 2 + 1;
			cache_db = realloc(cache_db,
					   cache_db_alloc * sizeof(CACHE));
			if (!cache_db)
				return GNUTLS_E_MEMORY_ERROR;
			memset(cache_db + cache_db_ptr, 0,
			       (cache_db_alloc - cache_db_ptr) * sizeof(CACHE));
			cache_db_ptr++;
		}
	}

	memcpy(cache_db[i].session_id, key.data, key.size);
	cache_db[i].session_id_size = key.size;

	/* resize the data slot if needed */
	if (cache_db[i].session_data.size < data.size) {
		cache_db[i].session_data.data =
			realloc(cache_db[i].session_data.data,
				data.size);
		if (!cache_db[i].session_data.data)
			return GNUTLS_E_MEMORY_ERROR;
	}
	memcpy(cache_db[i].session_data.data, data.data, data.size);
	cache_db[i].session_data.size = data.size;

	return 0;
}

static gnutls_datum_t wrap_db_fetch(void *dbf, gnutls_datum_t key)
{
	gnutls_datum_t res = { NULL, 0 };
	time_t now = time(0);
	int i;

	for (i = 0; i < cache_db_ptr; i++) {
		if (key.size == cache_db[i].session_id_size &&
		    memcmp(key.data, cache_db[i].session_id,
			   key.size) == 0 &&
		    now < gnutls_db_check_entry_expire_time(&cache_db[i].
							    session_data)) {
			res.size = cache_db[i].session_data.size;

			res.data = malloc(res.size);
			if (res.data == NULL)
				return res;

			memcpy(res.data, cache_db[i].session_data.data,
			       res.size);

			return res;
		}
	}
	return res;
}

static int wrap_db_delete(void *dbf, gnutls_datum_t key)
{
	int i;

	for (i = 0; i < cache_db_ptr; i++) {
		if (key.size == cache_db[i].session_id_size &&
		    memcmp(key.data, cache_db[i].session_id,
			   key.size) == 0) {

			cache_db[i].session_id_size = 0;
			free(cache_db[i].session_data.data);
			cache_db[i].session_data.data = NULL;
			cache_db[i].session_data.size = 0;

			return 0;
		}
	}

	return GNUTLS_E_DB_ERROR;
}

static int
anti_replay_db_add(void *dbf, time_t exp, const gnutls_datum_t *key, const gnutls_datum_t *data)
{
	time_t now = time(0);
	int i;

	for (i = 0; i < cache_db_ptr; i++) {
		if (key->size == cache_db[i].session_id_size &&
		    memcmp(key->data, cache_db[i].session_id,
			   key->size) == 0 &&
		    now < gnutls_db_check_entry_expire_time(&cache_db[i].
							    session_data))
			return GNUTLS_E_DB_ENTRY_EXISTS;
	}

	return wrap_db_store(dbf, *key, *data);
}
