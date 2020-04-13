/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include <stdint.h>

#include <gnutls/gnutls.h>

#include "fuzzer.h"

static
int cb(void *ctx, unsigned tls_id, const unsigned char *data, unsigned data_size)
{
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t raw;

	raw.data = (unsigned char *)data;
	raw.size = size;

	gnutls_ext_raw_parse(NULL, cb, &raw, 0);

	gnutls_ext_raw_parse(NULL, cb, &raw, GNUTLS_EXT_RAW_FLAG_TLS_CLIENT_HELLO);

	gnutls_ext_raw_parse(NULL, cb, &raw, GNUTLS_EXT_RAW_FLAG_DTLS_CLIENT_HELLO);

	return 0;
}
