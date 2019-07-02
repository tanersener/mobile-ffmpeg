/*
# Copyright 2016 Google Inc.
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
#include <gnutls/pkcs7.h>

#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t raw;
	gnutls_datum_t out;
	gnutls_pkcs7_t pkcs7;
	int ret;

	raw.data = (unsigned char *)data;
	raw.size = size;

	ret = gnutls_pkcs7_init(&pkcs7);
	assert(ret >= 0);

	ret = gnutls_pkcs7_import(pkcs7, &raw, GNUTLS_X509_FMT_DER);
	if (ret >= 0) {
		ret = gnutls_pkcs7_print(pkcs7, GNUTLS_CRT_PRINT_FULL, &out);
		assert(ret >= 0);
		gnutls_free(out.data);
	}

	gnutls_pkcs7_deinit(pkcs7);
	return 0;
}
