/*
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
 * Copyright (C) 2019 Tom Vrancken (dev@tomvrancken.nl)
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


/***
 * This fuzzer tests the behavior of the GnuTLS library in client mode,
 * specifically dealing with raw public keys during the handshake.
 * 
 * The fuzzer corpus generated as first input was generated with the
 * following parameters for gnutls-serv and gnutls-cli:
 * 
 * gnutls-serv --priority NORMAL:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK
 * gnutls-cli localhost:5556 --priority NORMAL:-CTYPE-ALL:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK --no-ca-verification
 * 
 * The above yields a handshake where both the client and server present
 * a raw public-key to eachother.
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

#include "certs.h"
#include "mem.h"
#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
	IGNORE_CERTS;
	int res;
	gnutls_session_t session;
	gnutls_certificate_credentials_t rawpk_cred;
	struct mem_st memdata;

	res = gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_ENABLE_RAWPK);
	assert(res >= 0);

	res = gnutls_certificate_allocate_credentials(&rawpk_cred);
	assert(res >= 0);
	
	res =
		gnutls_certificate_set_rawpk_key_mem(rawpk_cred,
		                                     &rawpk_public_key1,
		                                     &rawpk_private_key1,
		                                     GNUTLS_X509_FMT_PEM,
		                                     NULL, 0, NULL, 0, 0);
	assert(res >= 0);
	
	gnutls_certificate_set_known_dh_params(rawpk_cred, GNUTLS_SEC_PARAM_MEDIUM);
	
	res = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, rawpk_cred);
	assert(res >= 0);

	res = gnutls_priority_set_direct(session, "NORMAL:"VERS_STR":-CTYPE-ALL:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK", NULL);
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
	gnutls_certificate_free_credentials(rawpk_cred);
	return 0;
}
