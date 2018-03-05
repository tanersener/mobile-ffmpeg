/*
 * Copyright (C) 2015 Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || !defined(ENABLE_ALPN)

int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>

#include "utils.h"

static void terminate(void);

/* This program tests whether the gnutls_prf() works as
 * expected.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

/* These are global */
static pid_t child;

static unsigned char server_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDIzCCAgugAwIBAgIMUz8PCR2sdRK56V6OMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
"BgNVBAMTBENBLTEwIhgPMjAxNDA0MDQxOTU5MDVaGA85OTk5MTIzMTIzNTk1OVow\n"
"EzERMA8GA1UEAxMIc2VydmVyLTIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQDZ3dCzh9gOTOiOb2dtrPu91fYYgC/ey0ACYjQxaru7FZwnuXPhQK9KHsIV\n"
"YRIyo49wjKZddkHet2sbpFAAeETZh8UUWLRb/mupyaSJMycaYCNjLZCUJTztvXxJ\n"
"CCNfbtgvKC+Vu1mu94KBPatslgvnsamH7AiL5wmwRRqdH/Z93XaEvuRG6Zk0Sh9q\n"
"ZMdCboGfjtmGEJ1V+z5CR+IyH4sckzd8WJW6wBSEwgliGaXnc75xKtFWBZV2njNr\n"
"8V1TOYOdLEbiF4wduVExL5TKq2ywNkRpUfK2I1BcWS5D9Te/QT7aSdE08rL6ztmZ\n"
"IhILSrMOfoLnJ4lzXspz3XLlEuhnAgMBAAGjdzB1MAwGA1UdEwEB/wQCMAAwFAYD\n"
"VR0RBA0wC4IJbG9jYWxob3N0MA8GA1UdDwEB/wQFAwMHoAAwHQYDVR0OBBYEFJXR\n"
"raRS5MVhEqaRE42A3S2BIj7UMB8GA1UdIwQYMBaAFP6S7AyMRO2RfkANgo8YsCl8\n"
"JfJkMA0GCSqGSIb3DQEBCwUAA4IBAQCQ62+skMVZYrGbpab8RI9IG6xH8kEndvFj\n"
"J7wBBZCOlcjOj+HQ7a2buF5zGKRwAOSznKcmvZ7l5DPdsd0t5/VT9LKSbQ6+CfGr\n"
"Xs5qPaDJnRhZkOILCvXJ9qyO+79WNMsg9pWnxkTK7aWR5OYE+1Qw1jG681HMkWTm\n"
"nt7et9bdiNNpvA+L55569XKbdtJLs3hn5gEQFgS7EaEj59aC4vzSTFcidowCoa43\n"
"7JmfSfC9YaAIFH2vriyU0QNf2y7cG5Hpkge+U7uMzQrsT77Q3SDB9WkyPAFNSB4Q\n"
"B/r+OtZXOnQhLlMV7h4XGlWruFEaOBVjFHSdMGUh+DtaLvd1bVXI\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDATCCAemgAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwNDA0MTk1OTA1WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDvhyQfsUm3T0xK\n"
"jiBXO3H6Y27b7lmCRYZQCmXCl2sUsGDL7V9biavTt3+sorWtH542/cTGDh5n8591\n"
"7rVxAB/VASmN55O3fjZyFGrjusjhXBla0Yxe5rZ/7/Pjrq84T7gc/IXiX9Sums/c\n"
"o9AeoykfhsjV2ubhh4h+8uPsHDTcAFTxq3mQaoldwnW2nmjDFzaKLtQdnyFf41o6\n"
"nsJCK/J9PtpdCID5Zb+eQfu5Yhk1iUHe8a9TOstCHtgBq61YzufDHUQk3zsT+VZM\n"
"20lDvSBnHdWLjxoea587JbkvtH8xRR8ThwABSb98qPnhJ8+A7mpO89QO1wxZM85A\n"
"xEweQlMHAgMBAAGjZDBiMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQU/pLsDIxE7ZF+QA2CjxiwKXwl8mQwHwYDVR0jBBgwFoAUGD0R\n"
"Yr2H7kfjQUcBMxSTCDQnhu0wDQYJKoZIhvcNAQELBQADggEBANEXLUV+Z1PGTn7M\n"
"3rPT/m/EamcrZJ3vFWrnfN91ws5llyRUKNhx6222HECh3xRSxH9YJONsbv2zY6sd\n"
"ztY7lvckL4xOgWAjoCVTx3hqbZjDxpLRsvraw1PlqBHlRQVWLKlEQ55+tId2zgMX\n"
"Z+wxM7FlU/6yWVPODIxrqYQd2KqaEp4aLIklw6Hi4HD6DnQJikjsJ6Noe0qyX1Tx\n"
"uZ8mgP/G47Fe2d2H29kJ1iJ6hp1XOqyWrVIh/jONcnTvWS8aMqS3MU0EJH2Pb1Qa\n"
"KGIvbd/3H9LykFTP/b7Imdv2fZxXIK8jC+jbF1w6rdBCVNA0p30X/jonoC3vynEK\n"
"5cK0cgs=\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEA2d3Qs4fYDkzojm9nbaz7vdX2GIAv3stAAmI0MWq7uxWcJ7lz\n"
"4UCvSh7CFWESMqOPcIymXXZB3rdrG6RQAHhE2YfFFFi0W/5rqcmkiTMnGmAjYy2Q\n"
"lCU87b18SQgjX27YLygvlbtZrveCgT2rbJYL57Gph+wIi+cJsEUanR/2fd12hL7k\n"
"RumZNEofamTHQm6Bn47ZhhCdVfs+QkfiMh+LHJM3fFiVusAUhMIJYhml53O+cSrR\n"
"VgWVdp4za/FdUzmDnSxG4heMHblRMS+UyqtssDZEaVHytiNQXFkuQ/U3v0E+2knR\n"
"NPKy+s7ZmSISC0qzDn6C5yeJc17Kc91y5RLoZwIDAQABAoIBAQCRXAu5HPOsZufq\n"
"0K2DYZz9BdqSckR+M8HbVUZZiksDAeIUJwoHyi6qF2eK+B86JiK4Bz+gsBw2ys3t\n"
"vW2bQqM9N/boIl8D2fZfbCgZWkXGtUonC+mgzk+el4Rq/cEMFVqr6/YDwuKNeJpc\n"
"PJc5dcsvpTvlcjgpj9bJAvJEz2SYiIUpvtG4WNMGGapVZZPDvWn4/isY+75T5oDf\n"
"1X5jG0lN9uoUjcuGuThN7gxjwlRkcvEOPHjXc6rxfrWIDdiz/91V46PwpqVDpRrg\n"
"ig6U7+ckS0Oy2v32x0DaDhwAfDJ2RNc9az6Z+11lmY3LPkjG/p8Klcmgvt4/lwkD\n"
"OYRC5QGRAoGBAPFdud6nmVt9h1DL0o4R6snm6P3K81Ds765VWVmpzJkK3+bwe4PQ\n"
"GQQ0I0zN4hXkDMwHETS+EVWllqkK/d4dsE3volYtyTti8zthIATlgSEJ81x/ChAQ\n"
"vvXxgx+zPUnb1mUwy+X+6urTHe4bxN2ypg6ROIUmT+Hx1ITG40LRRiPTAoGBAOcT\n"
"WR8DTrj42xbxAUpz9vxJ15ZMwuIpk3ShE6+CWqvaXHF22Ju4WFwRNlW2zVLH6UMt\n"
"nNfOzyDoryoiu0+0mg0wSmgdJbtCSHoI2GeiAnjGn5i8flQlPQ8bdwwmU6g6I/EU\n"
"QRbGK/2XLmlrGN52gVy9UX0NsAA5fEOsAJiFj1CdAoGBAN9i3nbq6O2bNVSa/8mL\n"
"XaD1vGe/oQgh8gaIaYSpuXlfbjCAG+C4BZ81XgJkfj3CbfGbDNqimsqI0fKsAJ/F\n"
"HHpVMgrOn3L+Np2bW5YMj0Fzwy+1SCvsQ8C+gJwjOLMV6syGp/+6udMSB55rRv3k\n"
"rPnIf+YDumUke4tTw9wAcgkPAoGASHMkiji7QfuklbjSsslRMyDj21gN8mMevH6U\n"
"cX7pduBsA5dDqu9NpPAwnQdHsSDE3i868d8BykuqQAfLut3hPylY6vPYlLHfj4Oe\n"
"dj+xjrSX7YeMBE34qvfth32s1R4FjtzO25keyc/Q2XSew4FcZftlxVO5Txi3AXC4\n"
"bxnRKXECgYEAva+og7/rK+ZjboJVNxhFrwHp9bXhz4tzrUaWNvJD2vKJ5ZcThHcX\n"
"zCig8W7eXHLPLDhi9aWZ3kUZ1RLhrFc/6dujtVtU9z2w1tmn1I+4Zi6D6L4DzKdg\n"
"nMRLFoXufs/qoaJTqa8sQvKa+ceJAF04+gGtw617cuaZdZ3SYRLR2dk=\n"
"-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

static const
gnutls_datum_t hrnd = {(void*)"\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32};
static const
gnutls_datum_t hsrnd = {(void*)"\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32};

static void dump(const char *name, const uint8_t *data, unsigned data_size)
{
	unsigned i;

	fprintf(stderr, "%s", name);
	for (i=0;i<data_size;i++)
		fprintf(stderr, "\\x%.2x", (unsigned)data[i]);
	fprintf(stderr, "\n");
}

static gnutls_datum_t master =
    { (void*)"\x44\x66\x44\xa9\xb6\x29\xed\x6e\xd6\x93\x15\xdb\xf0\x7d\x4b\x2e\x18\xb1\x9d\xed\xff\x6a\x86\x76\xc9\x0e\x16\xab\xc2\x10\xbb\x17\x99\x24\xb1\xd9\xb9\x95\xe7\xea\xea\xea\xea\xea\xff\xaa\xac", 48};
static gnutls_datum_t sess_id =
    { (void*)"\xd9\xb9\x95\xe7\xea", 5};

#define TRY(label_size, label, extra_size, extra, size, exp) \
	{ \
	ret = gnutls_prf_rfc5705(session, label_size, label, extra_size, extra, size, \
			 (void*)key_material); \
	if (ret < 0) { \
		fprintf(stderr, "gnutls_prf_rfc5705: error in %d\n", __LINE__); \
		gnutls_perror(ret); \
		exit(1); \
	} \
	if (memcmp(key_material, exp, size) != 0) { \
		fprintf(stderr, "gnutls_prf_rfc5705: output doesn't match for '%s'\n", label); \
		dump("got ", key_material, size); \
		dump("expected ", exp, size); \
		exit(1); \
	} \
	}

#define TRY_OLD(label_size, label, extra_size, extra, size, exp) \
	{ \
	ret = gnutls_prf(session, label_size, label, 1, extra_size, extra, size, \
			 (void*)key_material); \
	if (ret < 0) { \
		fprintf(stderr, "gnutls_prf: error in %d\n", __LINE__); \
		gnutls_perror(ret); \
		exit(1); \
	} \
	if (memcmp(key_material, exp, size) != 0) { \
		fprintf(stderr, "gnutls_prf: output doesn't match for '%s'\n", label); \
		dump("got ", key_material, size); \
		dump("expected ", exp, size); \
		exit(1); \
	} \
	}
static void check_prfs(gnutls_session_t session)
{
	unsigned char key_material[512];
	unsigned char key_material2[512];
	int ret;

	TRY(13, "key expansion", 0, NULL, 34, (uint8_t*)"\xcf\x3e\x1c\x03\x47\x1a\xdf\x4a\x8e\x74\xc6\xda\xcd\xda\x22\xa4\x8e\xa5\xf7\x62\xef\xd6\x47\xe7\x41\x20\xea\x44\xb8\x5d\x66\x87\x0a\x61");
	TRY(6, "hello", 0, NULL, 31, (uint8_t*)"\x83\x6c\xc7\x8e\x1b\x62\xc7\x06\x17\x99\x37\x95\x2e\xb8\x42\x5c\x42\xcd\x75\x65\x2c\xa3\x16\x2b\xab\x0a\xcf\xfc\xc8\x90\x30");
	TRY(7, "context", 5, "abcd\xfa", 31, (uint8_t*)"\x5b\xc7\x72\xe9\xda\xe4\x79\x3e\xfe\x9a\xc5\x6f\xf4\x8d\x5a\xfe\x4c\x8d\x16\xa7\xf0\x13\x13\xf1\x93\xdd\x4b\x43\x65\xc1\x94");
	TRY(12, "null-context", 0, "", 31, (uint8_t*)"\xd7\xb6\xff\x3d\xf7\xbe\x0e\xf2\xd0\xbf\x55\x0b\x56\xac\xfb\x3c\x1d\x5c\xaa\xa8\x71\x45\xf5\xd5\x71\x35\xa2\x35\x83\xc2\xe0");

	TRY_OLD(6, "hello", 0, NULL, 31, (uint8_t*)"\x53\x35\x9b\x1c\xbf\xf2\x50\x85\xa1\xbc\x42\xfb\x45\x92\xc3\xbe\x20\x24\x24\xe2\xeb\x6e\xf7\x4f\xc0\xee\xe3\xaa\x46\x36\xfd");
	TRY_OLD(7, "context", 5, "abcd\xfa", 31, (uint8_t*)"\x5f\x75\xb7\x61\x76\x4c\x1e\x86\x4b\x7d\x2e\x6c\x09\x91\xfd\x1e\xe6\xe8\xee\xf9\x86\x6a\x80\xfe\xf3\xbe\x96\xd0\x47\xf5\x9e");

	/* check whether gnutls_prf matches gnutls_prf_rfc5705 when no context is given */
	ret = gnutls_prf(session, 4, "aaaa", 0, 0, NULL, 64,
			 (void*)key_material);
	if (ret < 0) {
		fprintf(stderr, "gnutls_prf: error in %d\n", __LINE__);
		gnutls_perror(ret);
		exit(1);
	}

	ret = gnutls_prf_rfc5705(session, 4, "aaaa", 0, NULL, 64,
			 (void*)key_material2);
	if (ret < 0) {
		fprintf(stderr, "gnutls_prf_rfc5705: error in %d\n", __LINE__);
		gnutls_perror(ret);
		exit(1);
	}

	if (memcmp(key_material, key_material2, 64) != 0) {
		fprintf(stderr, "gnutls_prf: output doesn't match in cross-check\n");
		dump("got1 ", key_material, 64);
		dump("got2 ", key_material2, 64);
		exit(1);
	}
}

static void client(int fd)
{
	gnutls_session_t session;
	int ret;
	gnutls_certificate_credentials_t clientx509cred;
	const char *err;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&clientx509cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	ret = gnutls_session_set_premaster(session, GNUTLS_CLIENT,
		GNUTLS_TLS1_0, GNUTLS_KX_RSA,
		GNUTLS_CIPHER_AES_128_CBC, GNUTLS_MAC_SHA1,
		GNUTLS_COMP_NULL, &master, &sess_id);
	if (ret < 0) {
		fail("client: gnutls_session_set_premaster failed: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	/* Use default priorities */
	ret = gnutls_priority_set_direct(session,
				   "NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA",
				   &err);
	if (ret < 0) {
		fail("client: priority set failed (%s): %s\n",
		     gnutls_strerror(ret), err);
		exit(1);
	}

	ret = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_handshake_set_random(session, &hrnd);
	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", strerror(ret));
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	ret = gnutls_cipher_get(session);
	if (ret != GNUTLS_CIPHER_AES_128_CBC) {
		fprintf(stderr, "negotiated unexpected cipher: %s\n", gnutls_cipher_get_name(ret));
		exit(1);
	}

	ret = gnutls_mac_get(session);
	if (ret != GNUTLS_MAC_SHA1) {
		fprintf(stderr, "negotiated unexpected mac: %s\n", gnutls_mac_get_name(ret));
		exit(1);
	}

	check_prfs(session);

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void terminate(void)
{
	int status = 0;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void server(int fd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t serverx509cred;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&serverx509cred);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	ret = gnutls_priority_set_direct(session,
				   "NORMAL:-KX-ALL:+RSA:%NO_SESSION_HASH", NULL);
	if (ret < 0) {
		fail("server: priority set failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}

	ret = gnutls_session_set_premaster(session, GNUTLS_SERVER,
		GNUTLS_TLS1_0, GNUTLS_KX_RSA,
		GNUTLS_CIPHER_AES_128_CBC, GNUTLS_MAC_SHA1,
		GNUTLS_COMP_NULL, &master, &sess_id);
	if (ret < 0) {
		fail("server: gnutls_session_set_premaster failed: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_handshake_set_random(session, &hsrnd);
	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(void)
{
	int fd[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		exit(1);
	}

	if (child) {
		int status;
		/* parent */

		server(fd[0]);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1]);
		exit(0);
	}
}

void doit(void)
{
	start();
}

#endif				/* _WIN32 */
