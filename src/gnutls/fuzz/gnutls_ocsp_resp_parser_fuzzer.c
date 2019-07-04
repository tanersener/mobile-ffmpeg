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

#include <gnutls/gnutls.h>
#include <gnutls/ocsp.h>

#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t raw;
	gnutls_datum_t out;
	gnutls_ocsp_resp_t resp;
	int ret;

	raw.data = (unsigned char *)data;
	raw.size = size;

	ret = gnutls_ocsp_resp_init(&resp);
	assert(ret >= 0);

	ret = gnutls_ocsp_resp_import(resp, &raw);
	if (ret >= 0) {
		ret = gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_FULL, &out);
		assert(ret >= 0);
		gnutls_free(out.data);
	}

	gnutls_ocsp_resp_deinit(resp);
	return 0;
}
