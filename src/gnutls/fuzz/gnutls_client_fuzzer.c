/*
# Copyright 2016 Google Inc.
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
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <gnutls/gnutls.h>
#include "mem.h"
#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
	int res;
	gnutls_session_t session;
	gnutls_certificate_credentials_t xcred;
	struct mem_st memdata;

	res = gnutls_init(&session, GNUTLS_CLIENT);
	assert(res >= 0);

	res = gnutls_certificate_allocate_credentials(&xcred);
	assert(res >= 0);
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
		while (true) {
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
