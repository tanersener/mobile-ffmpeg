/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <math.h>

#define fail(...) \
	{ \
		fprintf(stderr, __VA_ARGS__); \
		exit(1); \
	}

#include "../tests/eagain-common.h"
#include "benchmark.h"

const char *side = "";

#define PRIO_DH "NONE:+VERS-TLS1.2:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+DHE-RSA"
#define PRIO_ECDH "NONE:+VERS-TLS1.2:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+ECDHE-RSA:+CURVE-SECP256R1"
#define PRIO_ECDHX "NONE:+VERS-TLS1.2:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+ECDHE-RSA:+CURVE-X25519"
#define PRIO_ECDHE_ECDSA "NONE:+VERS-TLS1.2:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+ECDHE-ECDSA:+CURVE-SECP256R1"
#define PRIO_ECDHX_ECDSA "NONE:+VERS-TLS1.2:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+ECDHE-ECDSA:+CURVE-X25519"
#define PRIO_RSA "NONE:+VERS-TLS1.2:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA"


#define PRIO_ARCFOUR_128_SHA1 "NONE:+VERS-TLS1.0:+ARCFOUR-128:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA"

#define PRIO_AES_CBC_SHA1 "NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_ARCFOUR_128_MD5 "NONE:+VERS-TLS1.0:+ARCFOUR-128:+MD5:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_AES_GCM "NONE:+VERS-TLS1.2:+AES-128-GCM:+AEAD:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_AES_CCM "NONE:+VERS-TLS1.2:+AES-128-CCM:+AEAD:+SIGN-ALL:+COMP-NULL:+RSA"
#define PRIO_CHACHA_POLY1305 "NONE:+VERS-TLS1.2:+CHACHA20-POLY1305:+AEAD:+SIGN-ALL:+COMP-NULL:+ECDHE-RSA:+CURVE-ALL"
#define PRIO_CAMELLIA_CBC_SHA1 "NONE:+VERS-TLS1.0:+CAMELLIA-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA"

static const int rsa_bits = 3072, ec_bits = 256;

/* DH of 3072 bits that is pretty close equivalent to 256 bits of ECDH.
 */
const char *pkcs3 =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIIDDQKCAYEAtnlQsMzw6EdzVgv59IvDCNXDz+V5F6S95ies6VuP2najcePLCPa4\n"
    "yLCcQabhjV+rSpYxvqEo1hHMhAZPPsrHP3CCzFlqkSY2mmryC5LfWnoJnJCA5RSs\n"
    "kWNlxyJ/fkXWseFKDm+E3W/yZXxBJxf3BevlcF7hMXuOrv5tGOdiltWsCrZglEMC\n"
    "IO3NcvEwLp7Y/OuHk4J2upJSLJqL2mUoYgOUAwhoM9oh6ucjPJ0Ha/HqNRe0zdup\n"
    "0wnwSbjBR0xa2HdHv5hr0OPk6sma0Zj1cVNi3u5xlMeiirbtEBuRPfM4mrMkhK8F\n"
    "YBhVV7YRf+WMw8v9VhfeX+GYuE4oMdv6tJBwWoj0RdhgpD6BMG7uHwM7WOn5ZukA\n"
    "sn9eGsXRog2gCmckUfOGn5oQWXRk1sv2myeu75GAaIPIsXMWBsJNCfxVBbi7pEU9\n"
    "IQgi6JoLlRnvXVa2GaoVEdAuH0dl6QSIRmNeZ3VKa0ZCx1DHn/WVIt2ooMec5lCY\n"
    "JGCqIT3tQUUzAoIBgFYzCrFBoleurEimohHxnFKMY0E0feGA0qLPDUa+Ys/4wsr6\n"
    "SabuE9X69EHVDu4xGlbS4w9k5sMfXTqgVGIN43jbWuoN1FAdPp8YdbXACB3k+IoN\n"
    "cCj/Ju90Tc/NOTwHN/4Axsy0LpeP+eknb48eQw6mYsHCvN9ytmLqC8AG11G+aTrF\n"
    "boVeI7pCbfuls/cRNl4POuSyv+R12Evs1qXLoSW4crPEDvVpbIrgirjQNJbosfZY\n"
    "5Pxf2Ofpidy1slINQqx8zhILTikl0AdfYAlnBVFEOKg1HF+EnvNbcXW0QDxxnFF/\n"
    "W+Yv0xQpFw9UDa+hdwEVvdrDopqvuvg9BCwCfxT3vGN300RDqWAVGJUknXN4T5MZ\n"
    "+fZrtZMhbWDCsOHMcVcUPqul7V5uQX7EAhUnfBKxE1I5NK9J8wtHeUEYioI8f7XY\n"
    "Be6/w7WHHspV4fwIOfWUD5G0c++NxED+JwDyc8aU/qVOXVikOXwVTB/2oyatkoBX\n"
    "r8Y+1FUiZGhRCT9dbgICAQA=\n"
    "-----END DH PARAMETERS-----\n";

static unsigned char server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEOjCCAqKgAwIBAgIMU+I+KjQZpH+ZdjOlMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
    "BgNVBAMTBENBLTAwIhgPMjAxNDA4MDYxNDM5MzhaGA85OTk5MTIzMTIzNTk1OVow\n"
    "EzERMA8GA1UEAxMIc2VydmVyLTEwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGK\n"
    "AoIBgQDswF+JIWGcyu+JfjTcM8UDRKaxOuLVY0SODV1uaXPB5ZW9nEX/FFYIG+ld\n"
    "SKCyz5JF5ThrdvwqO+GVByuvETJdM7N4i8fzGHU8WIsj/CABAV+SaDT/xb+h1ar9\n"
    "dIehKelBmXQADVFX+xvu9OM5Ft3P/wyO9gWWrR7e/MU/SVzWzMT69+5YoE4QkrYY\n"
    "CuEBtlVHDo2mmNWGSQ5tUVIWARgXbqsmj4voWkutE/CiT0+g6GQilMARkROElIhO\n"
    "5NH+u3/Lt2wRQO5tEP1JmSoqvrMOmF16txze8qMzvKg1Eafijv9DR4NcCc6s8+g+\n"
    "CZbyODSdAybiyKsC7JCIrQjsnAjgPKKBLuZ1NTmu5liuXO05XsdcBoKDbKNAQdJC\n"
    "z4uxfqTr4CGFgHQk48Nhmq01EGmpwAeA/BOCB5qsWzqURtMX8EVB1Zdo3LD5Vwz1\n"
    "8mm+ZdeLPlYy3L/FBpVPDbYoZlFgINUNCQvGgvzqGJAQrKR4w8X/Y6HH9R8sv+U8\n"
    "kNtQI90CAwEAAaOBjTCBijAMBgNVHRMBAf8EAjAAMBQGA1UdEQQNMAuCCWxvY2Fs\n"
    "aG9zdDATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8BAf8EBQMDB6AAMB0GA1Ud\n"
    "DgQWBBTVObJSuRlmfjIx/hqxXk4qrxtnWDAfBgNVHSMEGDAWgBQ5vvRl/1WhIqpf\n"
    "ZFiHs89kf3N3OTANBgkqhkiG9w0BAQsFAAOCAYEAC0KQNPASZ7adSMMM3qx0Ny8Z\n"
    "AkcVAtohkjlwCwhoutcavZVyTjdpGydte6nfyTWOjs6ATBV2GhpyH+nvRJaYQFAh\n"
    "7uksjJxptSlaQuJqUI12urzx6BX0kenwh7nNwnLOngSBRqYwQqQdbnZf0w1DAdac\n"
    "vSa/Y1PrDpcXyPHpk7pDrtI9Mj24rIbvjeWM1RfgkNQYLPkZBDQqKkc5UrCA5y3v\n"
    "3motWyTdfvVYL7KWcEmGeKsWaTDkahd8Xhx29WvE4P740AOvXm/nkrE+PkHODbXi\n"
    "iD0a4cO2FPjjVt5ji+iaJTaXBEd9GHklKE6ZTZhj5az9ygQj1m6HZ2i3shWtG2ks\n"
    "AjgnGzsA8Wm/5X6YyR8UND41rS/lAc9yx8Az9Hqzfg8aOyvixYVPNKoTEPAMmypA\n"
    "oQT6g4b989lZFcjrwnLCrwz83jPD683p5IenCnRI5yhuFoQauy2tgHIbC1FRgs0C\n"
    "dyiOeDh80u1fekMVjRztIAwavuwxI6XgRzPSHhWR\n"
    "-----END CERTIFICATE-----\n";

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "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\n"
    "-----END RSA PRIVATE KEY-----\n";

static unsigned char server_ecc_key_pem[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHgCAQEEIQDrAKCAbdMKPngHu4zdSQ2Pghob8PhyrbUpWAR8V07E+qAKBggqhkjO\n"
    "PQMBB6FEA0IABDfo4YLPkO4pBpQamtObIV3J6l92vI+RkyNtaQ9gtSWDj20w/aBC\n"
    "WlbcTsRZ2itEpJ6GdLsGOW4RRfmiubzC9JU=\n"
    "-----END EC PRIVATE KEY-----\n";

static unsigned char server_ecc_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBrjCCAVSgAwIBAgIMU+I+axGZmBD/YL96MAoGCCqGSM49BAMCMA8xDTALBgNV\n"
    "BAMTBENBLTAwIhgPMjAxNDA4MDYxNDQwNDNaGA85OTk5MTIzMTIzNTk1OVowEzER\n"
    "MA8GA1UEAxMIc2VydmVyLTEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQ36OGC\n"
    "z5DuKQaUGprTmyFdyepfdryPkZMjbWkPYLUlg49tMP2gQlpW3E7EWdorRKSehnS7\n"
    "BjluEUX5orm8wvSVo4GNMIGKMAwGA1UdEwEB/wQCMAAwFAYDVR0RBA0wC4IJbG9j\n"
    "YWxob3N0MBMGA1UdJQQMMAoGCCsGAQUFBwMBMA8GA1UdDwEB/wQFAwMHgAAwHQYD\n"
    "VR0OBBYEFOuSntH2To0gJLH79Ow4wNpBuhmEMB8GA1UdIwQYMBaAFMZ1miRvZAYr\n"
    "nBEymOtPjbfTrnblMAoGCCqGSM49BAMCA0gAMEUCIQCMP3aBcCxSPbCUhihOsUmH\n"
    "G04AgT1PKw8z4LgZ4VGTVAIgYw3IFwS5sSYEAHRZAH8eaTXTz7XFmWmnkve9EBkN\n"
    "cBE=\n"
    "-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

const gnutls_datum_t server_ecc_cert = { server_ecc_cert_pem,
	sizeof(server_ecc_cert_pem)
};

const gnutls_datum_t server_ecc_key = { server_ecc_key_pem,
	sizeof(server_ecc_key_pem)
};

char buffer[64 * 1024];

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static void test_ciphersuite(const char *cipher_prio, int size)
{
	/* Server stuff. */
	gnutls_anon_server_credentials_t s_anoncred;
	gnutls_certificate_credentials_t c_certcred, s_certcred;
	const gnutls_datum_t p3 = { (void *) pkcs3, strlen(pkcs3) };
	static gnutls_dh_params_t dh_params;
	gnutls_session_t server;
	int sret, cret;
	const char *str;
	/* Client stuff. */
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_session_t client;
	/* Need to enable anonymous KX specifically. */
	int ret;
	struct benchmark_st st;
	gnutls_packet_t packet;

	/* Init server */
	gnutls_anon_allocate_server_credentials(&s_anoncred);
	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_anon_set_server_dh_params(s_anoncred, dh_params);

	gnutls_certificate_allocate_credentials(&s_certcred);
	gnutls_certificate_set_dh_params(s_certcred, dh_params);

	gnutls_certificate_set_x509_key_mem(s_certcred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_x509_key_mem(s_certcred, &server_ecc_cert,
					    &server_ecc_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&server, GNUTLS_SERVER);
	ret = gnutls_priority_set_direct(server, cipher_prio, &str);
	if (ret < 0) {
		fprintf(stderr, "Error in %s\n", str);
		exit(1);
	}
	gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, s_certcred);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, (gnutls_transport_ptr_t) server);
	reset_buffers();

	/* Init client */
	gnutls_anon_allocate_client_credentials(&c_anoncred);
	gnutls_certificate_allocate_credentials(&c_certcred);
	gnutls_init(&client, GNUTLS_CLIENT);

	ret = gnutls_priority_set_direct(client, cipher_prio, &str);
	if (ret < 0) {
		fprintf(stderr, "Error in %s\n", str);
		exit(1);
	}
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, c_certcred);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, (gnutls_transport_ptr_t) client);

	HANDSHAKE(client, server);

	fprintf(stdout, "%38s  ",
		gnutls_cipher_suite_get_name(gnutls_kx_get(server),
					     gnutls_cipher_get(server),
					     gnutls_mac_get(server)));
	fflush(stdout);

	gnutls_rnd(GNUTLS_RND_NONCE, buffer, sizeof(buffer));

	start_benchmark(&st);

	do {
		do {
			ret = gnutls_record_send(client, buffer, size);
		}
		while (ret == GNUTLS_E_AGAIN);

		if (ret < 0) {
			fprintf(stderr, "Failed sending to server\n");
			exit(1);
		}

		do {
			ret =
			    gnutls_record_recv_packet(server, &packet);
		}
		while (ret == GNUTLS_E_AGAIN);

		if (ret < 0) {
			fprintf(stderr, "Failed receiving from client: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		st.size += size;
		gnutls_packet_deinit(packet);
	}
	while (benchmark_must_finish == 0);

	stop_benchmark(&st, NULL, 1);

	gnutls_bye(client, GNUTLS_SHUT_WR);
	gnutls_bye(server, GNUTLS_SHUT_WR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_anon_free_server_credentials(s_anoncred);

	gnutls_dh_params_deinit(dh_params);

}

static
double calc_avg(unsigned int *diffs, unsigned int diffs_size)
{
	double avg = 0;
	unsigned int i;

	for (i = 0; i < diffs_size; i++)
		avg += diffs[i];

	avg /= diffs_size;

	return avg;
}

static
double calc_sstdev(unsigned int *diffs, unsigned int diffs_size,
		   double avg)
{
	double sum = 0, d;
	unsigned int i;

	for (i = 0; i < diffs_size; i++) {
		d = ((double) diffs[i] - avg);
		d *= d;

		sum += d;
	}
	sum /= diffs_size - 1;

	return sum;
}


unsigned int diffs[32 * 1024];
unsigned int diffs_size = 0;

static void test_ciphersuite_kx(const char *cipher_prio)
{
	/* Server stuff. */
	gnutls_anon_server_credentials_t s_anoncred;
	const gnutls_datum_t p3 = { (void *) pkcs3, strlen(pkcs3) };
	static gnutls_dh_params_t dh_params;
	gnutls_session_t server;
	int sret, cret;
	const char *str;
	char *suite = NULL;
	/* Client stuff. */
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_certificate_credentials_t c_certcred, s_certcred;
	gnutls_session_t client;
	/* Need to enable anonymous KX specifically. */
	int ret;
	struct benchmark_st st;
	struct timespec tr_start, tr_stop;
	double avg, sstddev;

	diffs_size = 0;

	/* Init server */
	gnutls_certificate_allocate_credentials(&s_certcred);
	gnutls_anon_allocate_server_credentials(&s_anoncred);
	gnutls_dh_params_init(&dh_params);

	ret =
	     gnutls_dh_params_import_pkcs3(dh_params, &p3,
					   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "Error importing the PKCS #3 params: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	gnutls_anon_set_server_dh_params(s_anoncred, dh_params);
	gnutls_certificate_set_dh_params(s_certcred, dh_params);

	ret = gnutls_certificate_set_x509_key_mem(s_certcred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "Error in %d: %s\n", __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_certificate_set_x509_key_mem(s_certcred, &server_ecc_cert,
					    &server_ecc_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "Error in %d: %s\n", __LINE__,
			gnutls_strerror(ret));
		exit(1);
	}

	/* Init client */
	gnutls_anon_allocate_client_credentials(&c_anoncred);
	gnutls_certificate_allocate_credentials(&c_certcred);

	start_benchmark(&st);

	do {

		gnutls_init(&server, GNUTLS_SERVER);
		ret =
		    gnutls_priority_set_direct(server, cipher_prio, &str);
		if (ret < 0) {
			fprintf(stderr, "Error in %s\n", str);
			exit(1);
		}
		gnutls_credentials_set(server, GNUTLS_CRD_ANON,
				       s_anoncred);
		gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				       s_certcred);
		gnutls_transport_set_push_function(server, server_push);
		gnutls_transport_set_pull_function(server, server_pull);
		gnutls_transport_set_ptr(server,
					 (gnutls_transport_ptr_t) server);
		reset_buffers();

		gnutls_init(&client, GNUTLS_CLIENT);

		ret =
		    gnutls_priority_set_direct(client, cipher_prio, &str);
		if (ret < 0) {
			fprintf(stderr, "Error in %s\n", str);
			exit(1);
		}
		gnutls_credentials_set(client, GNUTLS_CRD_ANON,
				       c_anoncred);
		gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				       c_certcred);

		gnutls_transport_set_push_function(client, client_push);
		gnutls_transport_set_pull_function(client, client_pull);
		gnutls_transport_set_ptr(client,
					 (gnutls_transport_ptr_t) client);

		gettime(&tr_start);

		HANDSHAKE(client, server);

		gettime(&tr_stop);

		if (suite == NULL)
			suite =
			    gnutls_session_get_desc(server);

		gnutls_deinit(client);
		gnutls_deinit(server);

		diffs[diffs_size++] = timespec_sub_ms(&tr_stop, &tr_start);
		if (diffs_size > sizeof(diffs))
			abort();

		st.size += 1;
	}
	while (benchmark_must_finish == 0);

	fprintf(stdout, "%38s  ", suite);
	gnutls_free(suite);
	stop_benchmark(&st, "transactions", 1);

	avg = calc_avg(diffs, diffs_size);
	sstddev = calc_sstdev(diffs, diffs_size, avg);

	printf("%32s %.2f ms, sample variance: %.2f)\n",
	       "(avg. handshake time:", avg, sstddev);

	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_anon_free_server_credentials(s_anoncred);

	gnutls_dh_params_deinit(dh_params);

}

void benchmark_tls(int debug_level, int ciphers)
{
	int size;

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_log_level(debug_level);
	gnutls_global_init();

	if (ciphers != 0) {
		size = 1400;
		printf
		    ("Testing throughput in cipher/MAC combinations (payload: %d bytes)\n",
		     size);

		test_ciphersuite(PRIO_ARCFOUR_128_SHA1, size);
		test_ciphersuite(PRIO_ARCFOUR_128_MD5, size);
		test_ciphersuite(PRIO_AES_GCM, size);
		test_ciphersuite(PRIO_AES_CCM, size);
		test_ciphersuite(PRIO_CHACHA_POLY1305, size);
		test_ciphersuite(PRIO_AES_CBC_SHA1, size);
		test_ciphersuite(PRIO_CAMELLIA_CBC_SHA1, size);

		size = 15 * 1024;
		printf
		    ("\nTesting throughput in cipher/MAC combinations (payload: %d bytes)\n",
		     size);
		test_ciphersuite(PRIO_ARCFOUR_128_SHA1, size);
		test_ciphersuite(PRIO_ARCFOUR_128_MD5, size);
		test_ciphersuite(PRIO_AES_GCM, size);
		test_ciphersuite(PRIO_AES_CCM, size);
		test_ciphersuite(PRIO_CHACHA_POLY1305, size);
		test_ciphersuite(PRIO_AES_CBC_SHA1, size);
		test_ciphersuite(PRIO_CAMELLIA_CBC_SHA1, size);
	} else {
		printf
		    ("Testing key exchanges (RSA/DH bits: %d, EC bits: %d)\n",
		     rsa_bits, ec_bits);
		test_ciphersuite_kx(PRIO_DH);
		test_ciphersuite_kx(PRIO_ECDH);
		test_ciphersuite_kx(PRIO_ECDHX);
		test_ciphersuite_kx(PRIO_ECDHE_ECDSA);
		test_ciphersuite_kx(PRIO_ECDHX_ECDSA);
		test_ciphersuite_kx(PRIO_RSA);
	}

	gnutls_global_deinit();

}
