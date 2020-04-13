/*
# Copyright 2017 Red Hat, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
*/

#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gnutls/gnutls.h>

#include "certs.h"
#include "mem.h"
#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
	int res;
	gnutls_datum_t rsa_cert, rsa_key;
	gnutls_datum_t ecdsa_cert, ecdsa_key;
	gnutls_datum_t ed25519_cert, ed25519_key;
	gnutls_datum_t ed448_cert, ed448_key;
	gnutls_session_t session;
	gnutls_certificate_credentials_t xcred;
	struct mem_st memdata;

	res = gnutls_init(&session, GNUTLS_SERVER);
	assert(res >= 0);

	res = gnutls_certificate_allocate_credentials(&xcred);
	assert(res >= 0);

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

	ed448_cert.data = (unsigned char *)kEd448CertificateDER;
	ed448_cert.size = sizeof(kEd448CertificateDER);
	ed448_key.data = (unsigned char *)kEd448PrivateKeyDER;
	ed448_key.size = sizeof(kEd448PrivateKeyDER);

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

	res =
		gnutls_certificate_set_x509_key_mem(xcred, &ed448_cert, &ed448_key,
		GNUTLS_X509_FMT_DER);
	assert(res >= 0);

	gnutls_certificate_set_known_dh_params(xcred, GNUTLS_SEC_PARAM_MEDIUM);

	res = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	assert(res >= 0);

	/*res = gnutls_set_default_priority(session);*/
	res = gnutls_priority_set_direct(session, "NORMAL:+SIGN-EDDSA-ED448:"VERS_STR, NULL);
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
	return 0;
}
