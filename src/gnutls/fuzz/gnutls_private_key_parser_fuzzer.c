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
#include <gnutls/x509.h>

#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t raw;
	gnutls_x509_privkey_t key;
	int ret;

	raw.data = (unsigned char *)data;
	raw.size = size;

	ret = gnutls_x509_privkey_init(&key);
	assert(ret >= 0);

	ret = gnutls_x509_privkey_import(key, &raw, GNUTLS_X509_FMT_DER);

	gnutls_x509_privkey_deinit(key);
	return 0;
}
