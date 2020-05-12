/*
 * Copyright (C) 2020 Dmitry Baryshkov
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

#include <assert.h>
#include <stdint.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	gnutls_datum_t raw;
	gnutls_datum_t out;
	gnutls_x509_crq_t crq;
	int ret;

	raw.data = (unsigned char *)data;
	raw.size = size;

	ret = gnutls_x509_crq_init(&crq);
	assert(ret >= 0);

	ret = gnutls_x509_crq_import(crq, &raw, GNUTLS_X509_FMT_DER);
	if (ret >= 0) {
		ret = gnutls_x509_crq_print(crq, GNUTLS_CRT_PRINT_FULL, &out);
		assert(ret >= 0);
		gnutls_free(out.data);
	}
	gnutls_x509_crq_deinit(crq);

	return 0;
}
