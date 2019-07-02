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
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gnutls/gnutls.h>

#include "psk.h"
#include "mem.h"
#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
	int res;
	gnutls_session_t session;
	gnutls_psk_client_credentials_t pcred;
	struct mem_st memdata;
	gnutls_datum_t psk_key;

	psk_key.data = (unsigned char*)psk_key16;
	psk_key.size = 16;

	res = gnutls_init(&session, GNUTLS_CLIENT);
	assert(res >= 0);

	res = gnutls_psk_allocate_client_credentials(&pcred);
	assert(res >= 0);

	res = gnutls_psk_set_client_credentials(pcred, "test", &psk_key, GNUTLS_PSK_KEY_RAW);
	assert(res >= 0);

	res = gnutls_credentials_set(session, GNUTLS_CRD_PSK, pcred);
	assert(res >= 0);

	res = gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+ECDHE-PSK:+DHE-PSK:+PSK:"VERS_STR, NULL);
	assert(res >= 0);

	memdata.data = data;
	memdata.size = size;

	gnutls_transport_set_push_function(session, mem_push);
	gnutls_transport_set_pull_function(session, mem_pull);
	gnutls_transport_set_pull_timeout_function(session,
		mem_pull_timeout);
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
	gnutls_psk_free_client_credentials(pcred);
	return 0;
}
