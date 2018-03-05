/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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

/* This program tests functionality in gnutls_dh_params structure */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cert-common.h"
#include "utils.h"

static int compare(gnutls_datum_t *d1, gnutls_datum_t *d2)
{
	gnutls_datum_t t1, t2;
	t1.data = d1->data;
	t1.size = d1->size;
	t2.data = d2->data;
	t2.size = d2->size;

	/* skip any differences due to zeros */
	while (t1.data[0] == 0) {
		t1.data++;
		t1.size--;
	}

	while (t2.data[0] == 0) {
		t2.data++;
		t2.size--;
	}

	if (t1.size != t2.size)
		return -1;
	if (memcmp(t1.data, t2.data, t1.size) != 0)
		return -1;
	return 0;
}

void doit(void)
{
	gnutls_dh_params_t dh_params;
	gnutls_x509_privkey_t privkey;
	gnutls_datum_t p1, g1, p2, g2, q;
	unsigned bits = 0;
	int ret;

	/* import DH parameters from DSA key and verify they are the same */
	gnutls_dh_params_init(&dh_params);
	gnutls_x509_privkey_init(&privkey);

	ret = gnutls_x509_privkey_import(privkey, &dsa_key, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("error in %s: %d\n", __FILE__, __LINE__);

	ret = gnutls_dh_params_import_dsa(dh_params, privkey);
	if (ret < 0)
		fail("error in %s: %d\n", __FILE__, __LINE__);

	ret = gnutls_dh_params_export_raw(dh_params, &p1, &g1, &bits);
	if (ret < 0)
		fail("error in %s: %d\n", __FILE__, __LINE__);

	ret = gnutls_x509_privkey_export_dsa_raw(privkey, &p2, &q, &g2, NULL, NULL);
	if (ret < 0)
		fail("error in %s: %d\n", __FILE__, __LINE__);

	if (bits > q.size*8  || bits < q.size*8-8)
		fail("error in %s: %d\n", __FILE__, __LINE__);

	if (compare(&p1, &p2) != 0)
		fail("error in %s: %d\n", __FILE__, __LINE__);

	if (compare(&g1, &g2) != 0)
		fail("error in %s: %d\n", __FILE__, __LINE__);

	gnutls_free(p1.data);
	gnutls_free(g1.data);
	gnutls_free(p2.data);
	gnutls_free(g2.data);
	gnutls_free(q.data);

	gnutls_dh_params_deinit(dh_params);
	gnutls_x509_privkey_deinit(privkey);
	success("all ok\n");
}
