/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>
#include <assert.h>
#include "utils.h"

unsigned char der_feat_long[] = 
	"\x30\x82\x01\x80\x02\x01\x00\x02\x01\x01\x02\x01\x02\x02\x01\x03"
	"\x02\x01\x04\x02\x01\x05\x02\x01\x06\x02\x01\x07\x02\x01\x08\x02"
	"\x01\x09\x02\x01\x0A\x02\x01\x0B\x02\x01\x0C\x02\x01\x0D\x02\x01"
	"\x0E\x02\x01\x0F\x02\x01\x10\x02\x01\x11\x02\x01\x12\x02\x01\x13"
	"\x02\x01\x14\x02\x01\x15\x02\x01\x16\x02\x01\x17\x02\x01\x18\x02"
	"\x01\x19\x02\x01\x1A\x02\x01\x1B\x02\x01\x1C\x02\x01\x1D\x02\x01"
	"\x1E\x02\x01\x1F\x02\x01\x20\x02\x01\x21\x02\x01\x22\x02\x01\x23"
	"\x02\x01\x24\x02\x01\x25\x02\x01\x26\x02\x01\x27\x02\x01\x28\x02"
	"\x01\x29\x02\x01\x2A\x02\x01\x2B\x02\x01\x2C\x02\x01\x2D\x02\x01"
	"\x2E\x02\x01\x2F\x02\x01\x30\x02\x01\x31\x02\x01\x32\x02\x01\x33"
	"\x02\x01\x34\x02\x01\x35\x02\x01\x36\x02\x01\x37\x02\x01\x38\x02"
	"\x01\x39\x02\x01\x3A\x02\x01\x3B\x02\x01\x3C\x02\x01\x3D\x02\x01"
	"\x3E\x02\x01\x3F\x02\x01\x40\x02\x01\x41\x02\x01\x42\x02\x01\x43"
	"\x02\x01\x44\x02\x01\x45\x02\x01\x46\x02\x01\x47\x02\x01\x48\x02"
	"\x01\x49\x02\x01\x4A\x02\x01\x4B\x02\x01\x4C\x02\x01\x4D\x02\x01"
	"\x4E\x02\x01\x4F\x02\x01\x50\x02\x01\x51\x02\x01\x52\x02\x01\x53"
	"\x02\x01\x54\x02\x01\x55\x02\x01\x56\x02\x01\x57\x02\x01\x58\x02"
	"\x01\x59\x02\x01\x5A\x02\x01\x5B\x02\x01\x5C\x02\x01\x5D\x02\x01"
	"\x5E\x02\x01\x5F\x02\x01\x60\x02\x01\x61\x02\x01\x62\x02\x01\x63"
	"\x02\x01\x64\x02\x01\x65\x02\x01\x66\x02\x01\x67\x02\x01\x68\x02"
	"\x01\x69\x02\x01\x6A\x02\x01\x6B\x02\x01\x6C\x02\x01\x6D\x02\x01"
	"\x6E\x02\x01\x6F\x02\x01\x70\x02\x01\x71\x02\x01\x72\x02\x01\x73"
	"\x02\x01\x74\x02\x01\x75\x02\x01\x76\x02\x01\x77\x02\x01\x78\x02"
	"\x01\x79\x02\x01\x7A\x02\x01\x7B\x02\x01\x7C\x02\x01\x7D\x02\x01"
	"\x7E\x02\x01\x7F";

static gnutls_datum_t der_long = { der_feat_long, sizeof(der_feat_long)-1};

void doit(void)
{
	int ret;
	gnutls_x509_tlsfeatures_t feat;
	unsigned int out;
	gnutls_datum_t der;
	unsigned i;

	ret = global_init();
	if (ret < 0)
		fail("init %d\n", ret);

	/* init and write >1 features
	 */
	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);

	assert(gnutls_x509_tlsfeatures_add(feat, 2) >= 0);
	assert(gnutls_x509_tlsfeatures_add(feat, 3) >= 0);
	assert(gnutls_x509_tlsfeatures_add(feat, 5) >= 0);
	assert(gnutls_x509_tlsfeatures_add(feat, 7) >= 0);
	assert(gnutls_x509_tlsfeatures_add(feat, 11) >= 0);

	assert(gnutls_x509_ext_export_tlsfeatures(feat, &der) >= 0);

	gnutls_x509_tlsfeatures_deinit(feat);

	/* re-load and read
	 */
	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);

	assert(gnutls_x509_ext_import_tlsfeatures(&der, feat, 0) >= 0);

	assert(gnutls_x509_tlsfeatures_get(feat, 0, &out) >= 0);
	assert(out == 2);

	assert(gnutls_x509_tlsfeatures_get(feat, 1, &out) >= 0);
	assert(out == 3);

	assert(gnutls_x509_tlsfeatures_get(feat, 2, &out) >= 0);
	assert(out == 5);

	assert(gnutls_x509_tlsfeatures_get(feat, 3, &out) >= 0);
	assert(out == 7);

	assert(gnutls_x509_tlsfeatures_get(feat, 4, &out) >= 0);
	assert(out == 11);

	gnutls_x509_tlsfeatures_deinit(feat);
	gnutls_free(der.data);

	/* check whether no feature is acceptable */
	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);

	assert(gnutls_x509_ext_export_tlsfeatures(feat, &der) >= 0);

	gnutls_x509_tlsfeatures_deinit(feat);

	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);

	assert(gnutls_x509_ext_import_tlsfeatures(&der, feat, 0) >= 0);

	assert(gnutls_x509_tlsfeatures_get(feat, 0, &out) == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	gnutls_x509_tlsfeatures_deinit(feat);

	gnutls_free(der.data);

	/* check whether we can add a reasonable number of features */
	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);

	for (i=0;i<128;i++) {
		ret = gnutls_x509_tlsfeatures_add(feat, i);
		if (ret < 0) {
			assert(i>=32);
			assert(ret == GNUTLS_E_INTERNAL_ERROR);
		}
	}

	gnutls_x509_tlsfeatures_deinit(feat);

	/* check whether we can import a very long list */
	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);

	assert(gnutls_x509_ext_import_tlsfeatures(&der_long, feat, 0) == GNUTLS_E_INTERNAL_ERROR);

	gnutls_x509_tlsfeatures_deinit(feat);

	gnutls_global_deinit();
}

