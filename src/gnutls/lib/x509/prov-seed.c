/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "gnutls_int.h"
#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <x509.h>
#include <x509_int.h>
#include <mpi.h>
#include "prov-seed.h"

/* This function encodes a seed value and a hash algorithm OID to the format
 * described in RFC8479. The output is the DER encoded form.
 */
int _x509_encode_provable_seed(gnutls_x509_privkey_t pkey, gnutls_datum_t *der)
{

	ASN1_TYPE c2;
	int ret, result;
	const char *oid;

	oid = gnutls_digest_get_oid(pkey->params.palgo);
	if (oid == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.ProvableSeed",
				 &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_write_value(c2, "seed", pkey->params.seed, pkey->params.seed_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = asn1_write_value(c2, "algorithm", oid, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	ret = _gnutls_x509_der_encode(c2, "", der, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	asn1_delete_structure2(&c2, ASN1_DELETE_FLAG_ZEROIZE);
	return ret;
}

/* This function decodes a DER encoded form of seed and a hash algorithm, as in
 * RFC8479.
 */
int _x509_decode_provable_seed(gnutls_x509_privkey_t pkey, const gnutls_datum_t *der)
{

	ASN1_TYPE c2;
	int ret, result;
	char oid[MAX_OID_SIZE];
	int oid_size;
	gnutls_datum_t seed = {NULL, 0};

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.ProvableSeed",
				 &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, der->data, der->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	ret = _gnutls_x509_read_value(c2, "seed", &seed);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (seed.size <= sizeof(pkey->params.seed)) {
		memcpy(pkey->params.seed, seed.data, seed.size);
		pkey->params.seed_size = seed.size;
	} else {
		ret = 0; /* ignore struct */
		_gnutls_debug_log("%s: ignoring ProvableSeed due to very long params\n", __func__);
		goto cleanup;
	}

	oid_size = sizeof(oid);
	result = asn1_read_value(c2, "algorithm", oid, &oid_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	pkey->params.palgo = gnutls_oid_to_digest(oid);
	pkey->params.pkflags |= GNUTLS_PK_FLAG_PROVABLE;

	ret = 0;

 cleanup:
	gnutls_free(seed.data);
	asn1_delete_structure2(&c2, ASN1_DELETE_FLAG_ZEROIZE);
	return ret;
}
