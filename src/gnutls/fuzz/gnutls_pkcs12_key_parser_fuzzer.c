/*
# Copyright 2016 Nikos Mavrogiannopoulos
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

#include <gnutls/gnutls.h>
#include <gnutls/pkcs12.h>

#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t raw = {(unsigned char*)data, (unsigned int)size};
	gnutls_pkcs12_t p12;
	gnutls_x509_privkey_t key;
	gnutls_x509_crt_t *chain;
	gnutls_x509_crt_t *extras;
	gnutls_x509_crl_t crl;
	unsigned int chain_len = 0, extras_len = 0;
	int ret;

	raw.data = (unsigned char *)data;
	raw.size = size;

	ret = gnutls_pkcs12_init(&p12);
	assert(ret >= 0);

	ret = gnutls_pkcs12_import(p12, &raw, GNUTLS_X509_FMT_DER, 0);
	if (ret < 0) {
		goto cleanup;
	}

	/* catch crashes */
	gnutls_pkcs12_verify_mac(p12, "1234");

	ret = gnutls_pkcs12_simple_parse(p12, "1234", &key, &chain, &chain_len, &extras, &extras_len, &crl, 0);
	if (ret >= 0) {
		gnutls_x509_privkey_deinit(key);
		if (crl)
			gnutls_x509_crl_deinit(crl);
		if (extras_len > 0) {
			for (unsigned i = 0; i < extras_len; i++)
				gnutls_x509_crt_deinit(extras[i]);
			gnutls_free(extras);
		}
		if (chain_len > 0) {
			for (unsigned i = 0; i < chain_len; i++)
				gnutls_x509_crt_deinit(chain[i]);
			gnutls_free(chain);
		}
	}

cleanup:
	gnutls_pkcs12_deinit(p12);
	return 0;
}
