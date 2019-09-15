/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#else
#include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <tests.h>

void _gnutls_record_set_default_version(gnutls_session_t session,
					unsigned char major,
					unsigned char minor);

void _gnutls_hello_set_default_version(gnutls_session_t session,
					unsigned char major,
					unsigned char minor);


extern gnutls_srp_client_credentials_t srp_cred;
extern gnutls_anon_client_credentials_t anon_cred;
extern gnutls_certificate_credentials_t xcred;

extern unsigned int verbose;

const char *ext_text = "";
int tls_ext_ok = 1;
int tls1_ok = 0;
int ssl3_ok = 0;
int tls1_1_ok = 0;
int tls1_2_ok = 0;
int tls1_3_ok = 0;
int send_record_ok = 0;

/* keep session info */
static char *session_data = NULL;
static char session_id[32];
static size_t session_data_size = 0, session_id_size = 0;
static int sfree = 0;
static int handshake_output = 0;

static int test_do_handshake(gnutls_session_t session)
{
	int ret, alert;

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	handshake_output = ret;

	if (ret < 0 && verbose > 1) {
		if (ret == GNUTLS_E_FATAL_ALERT_RECEIVED) {
			alert = gnutls_alert_get(session);
			printf("\n");
			printf("*** Received alert [%d]: %s\n",
			       alert, gnutls_alert_get_name(alert));
		}
	}

	if (ret < 0)
		return TEST_FAILED;
	gnutls_session_get_data(session, NULL, &session_data_size);

	if (sfree != 0) {
		free(session_data);
		sfree = 0;
	}
	session_data = malloc(session_data_size);
	sfree = 1;
	if (session_data == NULL) {
		fprintf(stderr, "Memory error\n");
		exit(1);
	}
	gnutls_session_get_data(session, session_data, &session_data_size);

	session_id_size = sizeof(session_id);
	gnutls_session_get_id(session, session_id, &session_id_size);

	return TEST_SUCCEED;
}

char protocol_str[] =
    "+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:+VERS-SSL3.0";
char protocol_all_str[] =
    "+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:+VERS-SSL3.0";
char prio_str[512] = "";

#define ALL_CIPHERS "+CIPHER-ALL:+ARCFOUR-128:+3DES-CBC"
#define BLOCK_CIPHERS "+3DES-CBC:+AES-128-CBC:+CAMELLIA-128-CBC:+AES-256-CBC:+CAMELLIA-256-CBC"
#define ALL_COMP "+COMP-NULL"
#define ALL_MACS "+MAC-ALL:+MD5:+SHA1"
#define ALL_KX "+RSA:+DHE-RSA:+DHE-DSS:+ANON-DH:+ECDHE-RSA:+ECDHE-ECDSA:+ANON-ECDH"
#define INIT_STR "NONE:"
char rest[128] = "%UNSAFE_RENEGOTIATION:+SIGN-ALL:+GROUP-ALL";

#define _gnutls_priority_set_direct(s, str) __gnutls_priority_set_direct(s, str, __LINE__)

static inline void
__gnutls_priority_set_direct(gnutls_session_t session, const char *str, int line)
{
	const char *err;
	int ret = gnutls_priority_set_direct(session, str, &err);

	if (ret < 0) {
		fprintf(stderr, "Error at %d with string %s\n", line, str);
		fprintf(stderr, "Error at %s: %s\n", err,
			gnutls_strerror(ret));
		exit(1);
	}
}

test_code_t test_server(gnutls_session_t session)
{
	int ret, i = 0;
	static char buf[5 * 1024];
	char *p;
	const char snd_buf[] = "GET / HTTP/1.0\r\n\r\n";

	buf[sizeof(buf) - 1] = 0;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":" ALL_KX ":" "%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret != TEST_SUCCEED)
		return TEST_FAILED;

	gnutls_record_send(session, snd_buf, sizeof(snd_buf) - 1);
	ret = gnutls_record_recv(session, buf, sizeof(buf) - 1);
	if (ret < 0)
		return TEST_FAILED;

	ext_text = "unknown";
	p = strstr(buf, "Server:");
	if (p != NULL) {
		p+=7;
		if (*p == ' ') p++;
		ext_text = p;
		while (*p != 0 && *p != '\r' && *p != '\n') {
			p++;
			i++;
			if (i > 128)
				break;
		}
		*p = 0;
	}

	return TEST_SUCCEED;
}

static gnutls_datum_t pubkey = { NULL, 0 };

test_code_t test_dhe(gnutls_session_t session)
{
#ifdef ENABLE_DHE
	int ret;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":+DHE-RSA:+DHE-DSS:%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	gnutls_dh_get_pubkey(session, &pubkey);

	return ret;
#endif
	return TEST_IGNORE;
}

test_code_t test_rfc7919(gnutls_session_t session)
{
#ifdef ENABLE_DHE
	int ret;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":+DHE-RSA:+DHE-DSS:+GROUP-ALL:%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	if (ret != TEST_FAILED && (gnutls_session_get_flags(session) & GNUTLS_SFLAGS_RFC7919))
		return TEST_SUCCEED;
	else
		return TEST_FAILED;
#endif
	return TEST_IGNORE;
}

test_code_t test_ecdhe(gnutls_session_t session)
{
	int ret;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":+ECDHE-RSA:+ECDHE-ECDSA:+CURVE-ALL:%s", protocol_all_str,
		rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	if (ret < 0)
		return TEST_FAILED;

	return ret;
}

test_code_t test_rsa(gnutls_session_t session)
{
	int ret;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":+RSA:%s", protocol_all_str,
		rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	if (ret < 0)
		return TEST_FAILED;

	return ret;
}

static
test_code_t test_ecdhe_curve(gnutls_session_t session, const char *curve, unsigned id)
{
	int ret;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	/* We always enable all the curves but set our selected as first. That is
	 * because list of curves may be also used by the server to select a cert. */
	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":+ECDHE-RSA:+ECDHE-ECDSA:%s:%s", protocol_all_str, curve, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	if (ret < 0)
		return TEST_FAILED;

	if (gnutls_ecc_curve_get(session) != id)
		return TEST_FAILED;

	return TEST_SUCCEED;
}

test_code_t test_ecdhe_secp256r1(gnutls_session_t session)
{
	return test_ecdhe_curve(session, "+CURVE-SECP256R1", GNUTLS_ECC_CURVE_SECP256R1);
}

test_code_t test_ecdhe_secp384r1(gnutls_session_t session)
{
	return test_ecdhe_curve(session, "+CURVE-SECP384R1", GNUTLS_ECC_CURVE_SECP384R1);
}

test_code_t test_ecdhe_secp521r1(gnutls_session_t session)
{
	return test_ecdhe_curve(session, "+CURVE-SECP521R1", GNUTLS_ECC_CURVE_SECP521R1);
}

test_code_t test_ecdhe_x25519(gnutls_session_t session)
{
	return test_ecdhe_curve(session, "+CURVE-X25519", GNUTLS_ECC_CURVE_X25519);
}

test_code_t test_rfc7507(gnutls_session_t session)
{
	int ret;
	const char *pstr = NULL;

	if (tls1_2_ok && tls1_1_ok)
		pstr = "-VERS-TLS-ALL:+VERS-TLS1.1:%FALLBACK_SCSV";
	else if (tls1_1_ok && tls1_ok)
		pstr = "-VERS-TLS-ALL:+VERS-TLS1.0:%FALLBACK_SCSV";
#ifdef ENABLE_SSL3
	else if (tls1_ok && ssl3_ok)
		pstr = "-VERS-TLS-ALL:+VERS-SSL3.0:%FALLBACK_SCSV";
#endif
	else
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":"ALL_KX":%s", pstr, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret < 0)
		return TEST_IGNORE2;

	if (handshake_output < 0)
		return TEST_SUCCEED;

	return TEST_FAILED;
}


test_code_t test_safe_renegotiation(gnutls_session_t session)
{
	int ret;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":" ALL_KX ":%s:%%SAFE_RENEGOTIATION", rest, protocol_str);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	return ret;
}

#ifdef ENABLE_OCSP
test_code_t test_ocsp_status(gnutls_session_t session)
{
	int ret;
	gnutls_datum_t resp;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":" ALL_KX":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_ocsp_status_request_enable_client(session, NULL, 0, NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	if (ret < 0)
		return TEST_FAILED;

	ret = gnutls_ocsp_status_request_get(session, &resp);
	if (ret == 0)
		return TEST_SUCCEED;


	return TEST_FAILED;
}

#endif

test_code_t test_ext_master_secret(gnutls_session_t session)
{
	int ret;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":%s:" ALL_KX, rest, protocol_str);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	if (ret < 0)
		return TEST_FAILED;

	if (gnutls_session_ext_master_secret_status(session) != 0)
		return TEST_SUCCEED;

	return TEST_FAILED;
}

test_code_t test_etm(gnutls_session_t session)
{
	int ret;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		"+AES-128-CBC:+AES-256-CBC:" ALL_COMP ":%s:" ALL_MACS
		":%s:" ALL_KX, rest, protocol_str);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	if (ret < 0)
		return TEST_FAILED;

	if (gnutls_session_etm_status(session) != 0)
		return TEST_SUCCEED;

	return TEST_FAILED;
}

test_code_t test_safe_renegotiation_scsv(gnutls_session_t session)
{
	int ret;

	if (ssl3_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":+VERS-TLS1.0:"
		ALL_MACS ":" ALL_KX ":%%SAFE_RENEGOTIATION");
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	return ret;
}

test_code_t test_dhe_group(gnutls_session_t session)
{
	int ret, ret2;
	gnutls_datum_t gen, prime, pubkey2;
	const char *print;
	FILE *fp;

	(void)remove("debug-dh.out");

	if (verbose == 0 || pubkey.data == NULL)
		return TEST_IGNORE;

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":+DHE-RSA:+DHE-DSS:%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);

	ret2 = gnutls_dh_get_group(session, &gen, &prime);
	if (ret2 >= 0) {

		fp = fopen("debug-dh.out", "w");
		if (fp == NULL)
			return TEST_FAILED;

		ext_text = "saved in debug-dh.out";

		print = raw_to_string(gen.data, gen.size);
		if (print) {
			fprintf(fp, " Generator [%d bits]: %s\n", gen.size * 8,
				print);
		}

		print = raw_to_string(prime.data, prime.size);
		if (print) {
			fprintf(fp, " Prime [%d bits]: %s\n", prime.size * 8,
				print);
		}

		gnutls_dh_get_pubkey(session, &pubkey2);
		print = raw_to_string(pubkey2.data, pubkey2.size);
		if (print) {
			fprintf(fp, " Pubkey [%d bits]: %s\n", pubkey2.size * 8,
				print);
		}

		if (pubkey2.data && pubkey2.size == pubkey.size &&
		    memcmp(pubkey.data, pubkey2.data, pubkey.size) == 0) {
			fprintf
			    (fp, " (public key seems to be static among sessions)\n");
		}

		{
			/* save the PKCS #3 params */
			gnutls_dh_params_t dhp;
			gnutls_datum_t p3;
			
			ret2 = gnutls_dh_params_init(&dhp);
			if (ret2 < 0)
				return TEST_FAILED;

			ret2 = gnutls_dh_params_import_raw(dhp, &prime, &gen);
			if (ret2 < 0)
				return TEST_FAILED;

			ret2 = gnutls_dh_params_export2_pkcs3(dhp, GNUTLS_X509_FMT_PEM, &p3);
			if (ret2 < 0)
				return TEST_FAILED;

			fprintf(fp, "\n%s\n", p3.data);
			gnutls_free(p3.data);
		}

		fclose(fp);
	}
	return ret;
}

test_code_t test_ssl3(gnutls_session_t session)
{
	int ret;
	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":+VERS-SSL3.0:"
		ALL_MACS ":" ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED)
		ssl3_ok = 1;

	return ret;
}

static int alrm = 0;
static void got_alarm(int k)
{
	alrm = 1;
}

test_code_t test_bye(gnutls_session_t session)
{
	int ret;
	char data[20];
	int secs = 6;
#ifndef _WIN32
	int old;

	signal(SIGALRM, got_alarm);
#endif

	sprintf(prio_str, INIT_STR
		ALL_CIPHERS ":" ALL_COMP ":%s:" ALL_MACS
		":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED)
		return ret;

	ret = gnutls_bye(session, GNUTLS_SHUT_WR);
	if (ret < 0)
		return TEST_FAILED;

#ifndef _WIN32
	old = siginterrupt(SIGALRM, 1);
	alarm(secs);
#else
	setsockopt((int) gnutls_transport_get_ptr(session), SOL_SOCKET,
		   SO_RCVTIMEO, (char *) &secs, sizeof(int));
#endif

	do {
		ret = gnutls_record_recv(session, data, sizeof(data));
	}
	while (ret > 0);

#ifndef _WIN32
	siginterrupt(SIGALRM, old);
#else
	if (WSAGetLastError() == WSAETIMEDOUT ||
	    WSAGetLastError() == WSAECONNABORTED)
		alrm = 1;
#endif
	if (ret == 0)
		return TEST_SUCCEED;

	if (alrm == 0)
		return TEST_UNSURE;

	return TEST_FAILED;
}



test_code_t test_aes(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str, INIT_STR
		"+AES-128-CBC:+AES-256-CBC:" ALL_COMP ":%s:" ALL_MACS
		":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_aes_gcm(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str, INIT_STR
		"+AES-128-GCM:+AES-256-GCM:" ALL_COMP
		":%s:" ALL_MACS ":" ALL_KX ":%s",
		protocol_all_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_aes_ccm(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str, INIT_STR
		"+AES-128-CCM:+AES-256-CCM:" ALL_COMP
		":%s:" ALL_MACS ":" ALL_KX ":%s",
		protocol_all_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_aes_ccm_8(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str, INIT_STR
		"+AES-128-CCM-8:+AES-256-CCM-8:" ALL_COMP
		":%s:" ALL_MACS ":" ALL_KX ":%s",
		protocol_all_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_camellia_cbc(gnutls_session_t session)
{
	int ret;

	if (gnutls_fips140_mode_enabled())
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR "+CAMELLIA-128-CBC:+CAMELLIA-256-CBC:" ALL_COMP
		":%s:" ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);


	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_camellia_gcm(gnutls_session_t session)
{
	int ret;

	if (gnutls_fips140_mode_enabled())
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR "+CAMELLIA-128-GCM:+CAMELLIA-256-GCM:" ALL_COMP
		":%s:" ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_unknown_ciphersuites(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":%s:" ALL_MACS ":" ALL_KX ":%s",
		protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_md5(gnutls_session_t session)
{
	int ret;

	if (gnutls_fips140_mode_enabled())
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":%s:+MD5:" ALL_KX ":%s", protocol_str,
		rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_sha(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR  ALL_CIPHERS ":" ALL_COMP
		":%s:+SHA1:" ALL_KX ":%s", protocol_str,
		rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_sha256(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":%s:+SHA256:" ALL_KX ":%s",
		protocol_all_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_3des(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR "+3DES-CBC:" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_arcfour(gnutls_session_t session)
{
	int ret;

	if (gnutls_fips140_mode_enabled())
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR "+ARCFOUR-128:" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_chacha20(gnutls_session_t session)
{
	int ret;

	if (gnutls_fips140_mode_enabled())
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR "+CHACHA20-POLY1305:" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_tls1(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.0:%%SSL3_RECORD_VERSION:" ALL_MACS ":" ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED)
		tls1_ok = 1;

	return ret;

}

test_code_t test_tls1_nossl3(gnutls_session_t session)
{
	int ret;

	if (tls1_ok != 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.0:%%LATEST_RECORD_VERSION:" ALL_MACS ":" ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED) {
		strcat(rest, ":%LATEST_RECORD_VERSION");
		tls1_ok = 1;
	}

	return ret;

}

test_code_t test_record_padding(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR BLOCK_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:-VERS-SSL3.0:" ALL_MACS ":" ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED) {
		tls1_ok = 1;
	} else {
		sprintf(prio_str,
			INIT_STR BLOCK_CIPHERS ":" ALL_COMP
			":+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:-VERS-SSL3.0:" ALL_MACS ":" ALL_KX ":%%COMPAT:%s", rest);
		_gnutls_priority_set_direct(session, prio_str);

		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
		ret = test_do_handshake(session);
		if (ret == TEST_SUCCEED) {
			tls1_ok = 1;
			strcat(rest, ":%COMPAT");
		}
	}

	return ret;
}

test_code_t test_no_extensions(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	gnutls_record_set_max_size(session, 4096);

	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED) {
		tls_ext_ok = 1;
	} else {
		sprintf(prio_str,
			INIT_STR BLOCK_CIPHERS ":" ALL_COMP
			":+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:-VERS-SSL3.0:" ALL_MACS ":" ALL_KX ":%%NO_EXTENSIONS:%s", rest);
		_gnutls_priority_set_direct(session, prio_str);

		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
		ret = test_do_handshake(session);
		if (ret == TEST_SUCCEED) {
			tls_ext_ok = 0;
			strcat(rest, ":%NO_EXTENSIONS");
		}
	}

	return ret;
}

test_code_t test_tls1_2(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.2:" ALL_MACS ":" ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED)
		tls1_2_ok = 1;

	return ret;

}

test_code_t test_tls1_3(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.3:" ALL_MACS ":" ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED)
		tls1_3_ok = 1;

	return ret;

}

test_code_t test_tls1_1(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.1:" ALL_MACS ":" ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_SUCCEED)
		tls1_1_ok = 1;

	return ret;

}

test_code_t test_tls1_1_fallback(gnutls_session_t session)
{
	int ret;
	if (tls1_1_ok)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.1:+VERS-TLS1.0:+VERS-SSL3.0:" ALL_MACS ":"
		ALL_KX ":%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret != TEST_SUCCEED)
		return TEST_FAILED;

	if (gnutls_protocol_get_version(session) == GNUTLS_TLS1)
		return TEST_SUCCEED;
	else if (gnutls_protocol_get_version(session) == GNUTLS_SSL3)
		return TEST_UNSURE;

	return TEST_FAILED;

}

test_code_t test_tls1_6_fallback(gnutls_session_t session)
{
	int ret;

	/* we remove RSA as there is a version check in the key exchange
	 * message we do not properly set in this test */
	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP
		":+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:+VERS-SSL3.0:" ALL_MACS ":"
		ALL_KX ":-RSA:%s", rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	_gnutls_hello_set_default_version(session, 3, 7);

	ret = test_do_handshake(session);
	if (ret != TEST_SUCCEED)
		return TEST_FAILED;

	ext_text = gnutls_protocol_get_name(gnutls_protocol_get_version(session));
	return TEST_SUCCEED;
}

/* Advertize both TLS 1.0 and SSL 3.0. If the connection fails,
 * but the previous SSL 3.0 test succeeded then disable TLS 1.0.
 */
test_code_t test_tls_disable0(gnutls_session_t session)
{
	int ret;
	if (tls1_ok != 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED) {
		/* disable TLS 1.0 */
		if (ssl3_ok != 0) {
			strcpy(protocol_str, "+VERS-SSL3.0");
		}
	}
	return ret;

}

test_code_t test_tls_disable1(gnutls_session_t session)
{
	int ret;

	if (tls1_1_ok != 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED) {
		/* disable TLS 1.1 */
		snprintf(protocol_str, sizeof(protocol_str), "+VERS-TLS1.0:+VERS-SSL3.0");
	}
	return ret;
}

test_code_t test_tls_disable2(gnutls_session_t session)
{
	int ret;

	if (tls1_2_ok != 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED) {
		/* disable TLS 1.2 */
		snprintf(protocol_str, sizeof(protocol_str), "+VERS-TLS1.1:+VERS-TLS1.0:+VERS-SSL3.0");
	}
	return ret;
}


test_code_t test_rsa_pms(gnutls_session_t session)
{
	int ret;

	/* here we enable both SSL 3.0 and TLS 1.0
	 * and try to connect and use rsa authentication.
	 * If the server is old, buggy and only supports
	 * SSL 3.0 then the handshake will fail.
	 */
	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":+RSA:%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED)
		return TEST_FAILED;

	if (gnutls_protocol_get_version(session) == GNUTLS_TLS1)
		return TEST_SUCCEED;
	return TEST_UNSURE;
}

test_code_t test_max_record_size(gnutls_session_t session)
{
	int ret;

	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	gnutls_record_set_max_size(session, 512);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED)
		return ret;

	ret = gnutls_record_get_max_size(session);
	if (ret == 512)
		return TEST_SUCCEED;

	return TEST_FAILED;
}

test_code_t test_heartbeat_extension(gnutls_session_t session)
{
	if (tls_ext_ok == 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	gnutls_record_set_max_size(session, 4096);

	gnutls_heartbeat_enable(session, GNUTLS_HB_PEER_ALLOWED_TO_SEND);
	test_do_handshake(session);

	switch (gnutls_heartbeat_allowed(session, GNUTLS_HB_LOCAL_ALLOWED_TO_SEND)) {
	case 0:
		return TEST_FAILED;
	default:
		return TEST_SUCCEED;
	}
}

test_code_t test_small_records(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	gnutls_record_set_max_size(session, 512);

	ret = test_do_handshake(session);
	return ret;
}

test_code_t test_version_rollback(gnutls_session_t session)
{
	int ret;
	if (tls1_ok == 0)
		return TEST_IGNORE;

	/* here we enable both SSL 3.0 and TLS 1.0
	 * and we connect using a 3.1 client hello version,
	 * and a 3.0 record version. Some implementations
	 * are buggy (and vulnerable to man in the middle
	 * attacks which allow a version downgrade) and this 
	 * connection will fail.
	 */
	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	_gnutls_record_set_default_version(session, 3, 0);

	ret = test_do_handshake(session);
	if (ret != TEST_SUCCEED)
		return ret;

	if (tls1_ok != 0
	    && gnutls_protocol_get_version(session) == GNUTLS_SSL3)
		return TEST_FAILED;

	return TEST_SUCCEED;
}

/* See if the server tolerates out of bounds
 * record layer versions in the first client hello
 * message.
 */
test_code_t test_version_oob(gnutls_session_t session)
{
	int ret;
	/* here we enable both SSL 3.0 and TLS 1.0
	 * and we connect using a 5.5 record version.
	 */
	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	_gnutls_record_set_default_version(session, 5, 5);

	ret = test_do_handshake(session);
	return ret;
}

void _gnutls_rsa_pms_set_version(gnutls_session_t session,
				 unsigned char major, unsigned char minor);

test_code_t test_rsa_pms_version_check(gnutls_session_t session)
{
	int ret;
	/* here we use an arbitary version in the RSA PMS
	 * to see whether to server will check this version.
	 *
	 * A normal server would abort this handshake.
	 */
	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	_gnutls_rsa_pms_set_version(session, 5, 5);	/* use SSL 5.5 version */

	ret = test_do_handshake(session);
	return ret;

}

#ifdef ENABLE_ANON
test_code_t test_anonymous(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":+ANON-DH:+ANON-ECDH:+CURVE-ALL:%s",
		protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred);

	ret = test_do_handshake(session);

	if (ret == TEST_SUCCEED)
		gnutls_dh_get_pubkey(session, &pubkey);

	return ret;
}
#endif

test_code_t test_session_resume2(gnutls_session_t session)
{
	int ret;
	char tmp_session_id[32];
	size_t tmp_session_id_size;

	if (session == NULL)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred);

	gnutls_session_set_data(session, session_data, session_data_size);

	memcpy(tmp_session_id, session_id, session_id_size);
	tmp_session_id_size = session_id_size;

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED)
		return ret;

	/* check if we actually resumed the previous session */

	session_id_size = sizeof(session_id);
	gnutls_session_get_id(session, session_id, &session_id_size);

	if (session_id_size == 0)
		return TEST_FAILED;

	if (gnutls_session_is_resumed(session))
		return TEST_SUCCEED;

	if (tmp_session_id_size == session_id_size &&
	    memcmp(tmp_session_id, session_id, tmp_session_id_size) == 0)
		return TEST_SUCCEED;
	else
		return TEST_FAILED;
}

extern char *hostname;

test_code_t test_certificate(gnutls_session_t session)
{
	int ret;
	FILE *fp;

	(void)remove("debug-certs.out");

	if (verbose == 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED)
		return ret;

	fp = fopen("debug-certs.out", "w");
	if (fp != NULL) {
		fprintf(fp, "\n");
		print_cert_info2(session, GNUTLS_CRT_PRINT_FULL, fp, verbose);
		fclose(fp);
		ext_text = "saved in debug-certs.out";
		return TEST_SUCCEED;
	}
	return TEST_FAILED;
}

test_code_t test_chain_order(gnutls_session_t session)
{
	int ret;
	const gnutls_datum_t *cert_list;
	unsigned int cert_list_size = 0;
	unsigned int i;
	unsigned p_size;
	gnutls_datum_t t;
	gnutls_x509_crt_t *certs;
	char *p, *pos;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake(session);
	if (ret == TEST_FAILED)
		return ret;

	if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509)
		return TEST_IGNORE2;

	cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
	if (cert_list_size == 0) {
		ext_text = "No certificates found!";
		return TEST_IGNORE2;
	}

	if (cert_list_size == 1)
		return TEST_SUCCEED;

	p = 0;
	p_size = 0;
	pos = NULL;
	for (i=0;i<cert_list_size;i++) {
		t.data = NULL;
		ret = gnutls_pem_base64_encode_alloc("CERTIFICATE", &cert_list[i], &t);
		if (ret < 0) {
			free(p);
			return TEST_FAILED;
		}

		p = realloc(p, p_size+t.size+1);
		pos = p + p_size;

		memcpy(pos, t.data, t.size);
		p_size += t.size;
		pos += t.size;

		gnutls_free(t.data);
	}
	*pos = 0;

	t.size = p_size;
	t.data = (void*)p;

	p_size = 0;
	ret = gnutls_x509_crt_list_import2(&certs, &p_size, &t, GNUTLS_X509_FMT_PEM, GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED);
	if (ret < 0) {
		return TEST_FAILED;
	}

	for (i=0;i<p_size;i++) {
		gnutls_x509_crt_deinit(certs[i]);
	}
	gnutls_free(certs);
	free(p);

	return TEST_SUCCEED;
}

/* A callback function to be used at the certificate selection time.
 */
static int
cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * sign_algos,
	      int sign_algos_length, gnutls_retr2_st * st)
{
	char issuer_dn[256];
	int i, ret;
	size_t len;
	FILE *fp;

	if (verbose == 0)
		return -1;

	fp = fopen("debug-cas.out", "w");
	if (fp == NULL)
		return -1;

	/* Print the server's trusted CAs
	 */
	printf("\n");
	if (nreqs > 0)
		fprintf(fp, "- Server's trusted authorities:\n");
	else
		fprintf
		    (fp, "- Server did not send us any trusted authorities names.\n");

	/* print the names (if any) */
	for (i = 0; i < nreqs; i++) {
		len = sizeof(issuer_dn);
		ret = gnutls_x509_rdn_get(&req_ca_rdn[i], issuer_dn, &len);
		if (ret >= 0) {
			fprintf(fp, "   [%d]: ", i);
			fprintf(fp, "%s\n", issuer_dn);
		}
	}
	fclose(fp);

	return -1;

}

/* Prints the trusted server's CAs. This is only
 * if the server sends a certificate request packet.
 */
test_code_t test_server_cas(gnutls_session_t session)
{
	int ret;

	(void)remove("debug-cas.out");
	if (verbose == 0)
		return TEST_IGNORE;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	gnutls_certificate_set_retrieve_function(xcred, cert_callback);

	ret = test_do_handshake(session);
	gnutls_certificate_set_retrieve_function(xcred, NULL);

	if (ret == TEST_FAILED)
		return ret;

	if (access("debug-cas.out", R_OK) == 0)
		ext_text = "saved in debug-cas.out";
	else
		ext_text = "none";
	return TEST_SUCCEED;
}

static test_code_t
test_do_handshake_and_send_record(gnutls_session_t session)
{
	int ret;
	/* This will be padded to 512 bytes. */
	const char snd_buf[] = "GET / HTTP/1.0\r\n\r\n";
	static char buf[5 * 1024];

	ret = test_do_handshake(session);
	if (ret != TEST_SUCCEED)
		return ret;

	gnutls_record_send(session, snd_buf, sizeof(snd_buf) - 1);
	ret = gnutls_record_recv(session, buf, sizeof(buf) - 1);
	if (ret < 0)
		return TEST_FAILED;

	return TEST_SUCCEED;
}

/* These tests shall be sent in this order to check if the server
 * advertises smaller limits than our default 512. and we can work it
 * around with %ALLOW_SMALL_RECORDS. */
test_code_t test_send_record(gnutls_session_t session)
{
	int ret;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%s", protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake_and_send_record(session);
	if (ret == TEST_SUCCEED)
		send_record_ok = 1;
	return ret;
}

test_code_t test_send_record_with_allow_small_records(gnutls_session_t session)
{
	int ret;

	/* If test_send_record succeeded, we don't need to check. */
	if (send_record_ok)
		return TEST_FAILED;

	sprintf(prio_str,
		INIT_STR ALL_CIPHERS ":" ALL_COMP ":%s:"
		ALL_MACS ":" ALL_KX ":%%ALLOW_SMALL_RECORDS:%s",
		protocol_str, rest);
	_gnutls_priority_set_direct(session, prio_str);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	ret = test_do_handshake_and_send_record(session);
	if (ret == TEST_SUCCEED)
		strcat(rest, ":%ALLOW_SMALL_RECORDS");
	return ret;
}
