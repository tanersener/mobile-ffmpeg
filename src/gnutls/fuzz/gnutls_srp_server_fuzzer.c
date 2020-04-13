/*
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gnutls/gnutls.h>

#include "certs.h"
#include "srp.h"
#include "mem.h"
#include "fuzzer.h"

static int
srp_cb(gnutls_session_t session, const char *username,
	gnutls_datum_t *salt, gnutls_datum_t *verifier, gnutls_datum_t *generator, gnutls_datum_t *prime)
{
	int ret;

	salt->data = (unsigned char*)gnutls_malloc(SALT_SIZE);
	memcpy(salt->data, (unsigned char*)SALT, SALT_SIZE);
	salt->size = SALT_SIZE;

	generator->data = (unsigned char*)gnutls_malloc(gnutls_srp_1024_group_generator.size);
	memcpy(generator->data, gnutls_srp_1024_group_generator.data, gnutls_srp_1024_group_generator.size);
	generator->size = gnutls_srp_1024_group_generator.size;

	prime->data = (unsigned char*)gnutls_malloc(gnutls_srp_1024_group_prime.size);
	memcpy(prime->data, gnutls_srp_1024_group_prime.data, gnutls_srp_1024_group_prime.size);
	prime->size = gnutls_srp_1024_group_prime.size;

	ret = gnutls_srp_verifier(USERNAME, PASSWORD, salt, generator, prime, verifier);
	if (ret < 0)
		return -1;

	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
	IGNORE_CERTS;
	int res;
	gnutls_datum_t rsa_cert, rsa_key;
	gnutls_datum_t ecdsa_cert, ecdsa_key;
	gnutls_datum_t ed25519_cert, ed25519_key;
	gnutls_session_t session;
	gnutls_certificate_credentials_t xcred;
	gnutls_srp_server_credentials_t pcred;
	struct mem_st memdata;

	res = gnutls_init(&session, GNUTLS_SERVER);
	assert(res >= 0);

	res = gnutls_certificate_allocate_credentials(&xcred);
	assert(res >= 0);

	res = gnutls_srp_allocate_server_credentials(&pcred);
	assert(res >= 0);

	gnutls_srp_set_server_credentials_function(pcred, srp_cb);

	rsa_cert.data = (unsigned char *)kRSACertificateDER;
	rsa_cert.size = sizeof(kRSACertificateDER);
	rsa_key.data = (unsigned char *)kRSAPrivateKeyDER;
	rsa_key.size = sizeof(kRSAPrivateKeyDER);

	ecdsa_cert.data = (unsigned char *)kECDSACertificateDER;
	ecdsa_cert.size = sizeof(kECDSACertificateDER);
	ecdsa_key.data = (unsigned char *)kECDSAPrivateKeyDER;
	ecdsa_key.size = sizeof(kECDSAPrivateKeyDER);

	ed25519_cert.data = (unsigned char *)kEd25519CertificateDER;
	ed25519_cert.size = sizeof(kEd25519CertificateDER);
	ed25519_key.data = (unsigned char *)kEd25519PrivateKeyDER;
	ed25519_key.size = sizeof(kEd25519PrivateKeyDER);

	res =
		gnutls_certificate_set_x509_key_mem(xcred, &rsa_cert, &rsa_key,
		GNUTLS_X509_FMT_DER);
	assert(res >= 0);

	res =
		gnutls_certificate_set_x509_key_mem(xcred, &ecdsa_cert, &ecdsa_key,
		GNUTLS_X509_FMT_DER);
	assert(res >= 0);

	res =
		gnutls_certificate_set_x509_key_mem(xcred, &ed25519_cert, &ed25519_key,
		GNUTLS_X509_FMT_DER);
	assert(res >= 0);

	gnutls_certificate_set_known_dh_params(xcred, GNUTLS_SEC_PARAM_MEDIUM);

	res = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	assert(res >= 0);

	res = gnutls_credentials_set(session, GNUTLS_CRD_SRP, pcred);
	assert(res >= 0);

	res = gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+SRP:+SRP-RSA:+SRP-DSS", NULL);
	assert(res >= 0);

	memdata.data = data;
	memdata.size = size;

	gnutls_transport_set_push_function(session, mem_push);
	gnutls_transport_set_pull_function(session, mem_pull);
	gnutls_transport_set_pull_timeout_function(session, mem_pull_timeout);
	gnutls_transport_set_ptr(session, &memdata);

	do {
		res = gnutls_handshake(session);
	} while (res < 0 && gnutls_error_is_fatal(res) == 0);
	if (res >= 0) {
		for (;;) {
			char buf[16384];
			res = gnutls_record_recv(session, buf, sizeof(buf));
			if (res <= 0) {
				break;
			}
		}
	}

	gnutls_deinit(session);
	gnutls_certificate_free_credentials(xcred);
	gnutls_srp_free_server_credentials(pcred);
	return 0;
}
