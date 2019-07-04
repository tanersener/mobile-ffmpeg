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

#include <stdint.h>

#include <gnutls/gnutls.h>
#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t raw = {.data = (unsigned char *)data, .size = size};
	gnutls_datum_t out;
	unsigned char result[50];
	size_t result_size = sizeof(result);
	int ret;

	ret = gnutls_pem_base64_decode2(NULL, &raw, &out);
	if (ret >= 0)
		gnutls_free(out.data);

	gnutls_pem_base64_decode(NULL, &raw, result, &result_size);

	ret = gnutls_base64_decode2(&raw, &out);
	if (ret >= 0)
		gnutls_free(out.data);

	return 0;
}
