/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Simo Sorce
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

/* This program tests functionality of DH exchanges */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnutls/gnutls.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"

#ifdef ENABLE_FIPS140
int _gnutls_ecdh_compute_key(gnutls_ecc_curve_t curve,
			   const gnutls_datum_t *x, const gnutls_datum_t *y,
			   const gnutls_datum_t *k,
			   const gnutls_datum_t *peer_x, const gnutls_datum_t *peer_y,
			   gnutls_datum_t *Z);

int _gnutls_ecdh_generate_key(gnutls_ecc_curve_t curve,
			      gnutls_datum_t *x, gnutls_datum_t *y,
			      gnutls_datum_t *k);

static void genkey(gnutls_ecc_curve_t curve, gnutls_datum_t *x,
                   gnutls_datum_t *y, gnutls_datum_t *key)
{
	int ret;

	ret = _gnutls_ecdh_generate_key(curve, x, y, key);
	if (ret != 0)
		fail("error\n");
}

static void compute_key(gnutls_ecc_curve_t curve, gnutls_datum_t *x,
			gnutls_datum_t *y, gnutls_datum_t *key,
			const gnutls_datum_t *peer_x,
			const gnutls_datum_t *peer_y,
                        int expect_error,
			gnutls_datum_t *result, bool expect_success)
{
	gnutls_datum_t Z = { 0 };
	bool success;
	int ret;

	ret = _gnutls_ecdh_compute_key(curve, x, y, key, peer_x, peer_y, &Z);
	if (expect_error != ret)
		fail("error (%d)\n", ret);

	if (result) {
		success = (Z.size != result->size &&
			   memcmp(Z.data, result->data, Z.size));
		if (success != expect_success)
			fail("error\n");
	}
	gnutls_free(Z.data);
}

struct dh_test_data {
	gnutls_ecc_curve_t curve;
	const gnutls_datum_t x;
	const gnutls_datum_t y;
	const gnutls_datum_t key;
	const gnutls_datum_t peer_x;
	const gnutls_datum_t peer_y;
	int expected_error;
};

void doit(void)
{
	struct dh_test_data test_data[] = {
		{
                        /* x == 0, y == 0 */
			GNUTLS_ECC_CURVE_SECP256R1,
                        { 0 }, { 0 }, { 0 },
			{ (void *)"\x00", 1 },
			{ (void *)"\x00", 1 },
			/* Should be GNUTLS_E_PK_INVALID_PUBKEY but mpi scan
                         * balks on values of 0 */
                        GNUTLS_E_MPI_SCAN_FAILED,
		},
		{
                        /* x > p -1 */
			GNUTLS_ECC_CURVE_SECP256R1,
                        { 0 }, { 0 }, { 0 },
			{ (void *)"\xff\xff\xff\xff\x00\x00\x00\x01"
                                  "\x00\x00\x00\x00\x00\x00\x00\x00"
                                  "\x00\x00\x00\x00\xff\xff\xff\xff"
                                  "\xff\xff\xff\xff\xff\xff\xff\xff", 1 },
			{ (void *)"\x02", 1 },
			GNUTLS_E_PK_INVALID_PUBKEY,
		},
		{
                        /* y > p -1 */
			GNUTLS_ECC_CURVE_SECP256R1,
                        { 0 }, { 0 }, { 0 },
			{ (void *)"\x02", 1 },
			{ (void *)"\xff\xff\xff\xff\x00\x00\x00\x01"
                                  "\x00\x00\x00\x00\x00\x00\x00\x00"
                                  "\x00\x00\x00\x00\xff\xff\xff\xff"
                                  "\xff\xff\xff\xff\xff\xff\xff\xff", 1 },
			GNUTLS_E_PK_INVALID_PUBKEY,
		},
		{
                        /* From CAVS tests */
			GNUTLS_ECC_CURVE_SECP521R1,
                        { (void *)"\xac\xbe\x4a\xd4\xf6\x73\x44\x0a"
                                  "\xfc\x31\xf0\xb0\x3d\x28\xd4\xd5"
                                  "\x14\xbe\x7b\xdd\x7a\x31\xb0\x32"
                                  "\xec\x27\x27\x17\xa5\x7d\xc2\x6c"
                                  "\xc4\xc9\x56\x29\xdb\x2d\x8c\x05"
                                  "\x86\x2b\xe6\x15\xc6\x06\x28\xa3"
                                  "\x24\xf2\x01\x7f\x98\xbd\xf9\x11"
                                  "\xcc\xf8\x83\x5e\x43\x9e\xb2\xc1"
                                  "\x88", 65 },
                        { (void *)"\xd6\x9b\x29\xa2\x37\x82\x36\x92"
                                  "\xe8\xdb\x90\xa3\x25\x68\x67\x6c"
                                  "\x92\xff\x3d\x23\x85\xe2\xfd\x13"
                                  "\x16\x12\x72\xb3\x4b\x55\x88\x72"
                                  "\xb0\x35\xab\xb5\x10\x89\x52\x5f"
                                  "\x42\x9f\x53\x02\x60\x80\xc3\xd5"
                                  "\x36\x6e\xe9\xdd\x28\xae\xd2\x38"
                                  "\xab\xbe\x68\x6a\x54\x3e\x19\xf2"
                                  "\x77", 65 },
                        { (void *)"\xd7\xdd\x17\x7c\xb9\x7f\x19\x09"
                                  "\xbe\x56\x79\xba\x38\x7b\xee\x64"
                                  "\xf7\xb4\x08\x4a\x4f\xaa\x6c\x31"
                                  "\x8b\x82\xe9\xf2\xf7\x50\xc5\xc1"
                                  "\x82\x26\x20\xd4\x88\x25\x0b\xf6"
                                  "\xb4\x14\xea\x9b\x2c\x07\x93\x50"
                                  "\xb9\xad\x78\x0a\x5e\xc6\xa6\xf8"
                                  "\xb2\x9f\xa1\xc4\x76\xce\x1d\xa9"
                                  "\xf5", 65 },
                        { (void *)"\x01\x41\xbe\x1a\xfa\x21\x99\xc9"
                                  "\xb2\x2d\xaa\x0a\xff\x90\xb2\x67"
                                  "\x18\xa2\x67\x04\x7e\xae\x28\x40"
                                  "\xe8\xbc\xa0\xbd\x0c\x75\x41\x51"
                                  "\xf1\xa0\x4d\xcf\x09\xa5\x4f\x1e"
                                  "\x13\x5e\xa0\xdd\x13\xed\x86\x74"
                                  "\x05\xc0\xcb\x6d\xac\x14\x6a\x24"
                                  "\xb8\xdc\xf3\x78\xed\xed\x5d\xcd"
                                  "\x57\x5b", 66 },
                        { (void *)"\x19\x52\xbd\x5d\xe6\x26\x40\xc3"
                                  "\xfc\x8c\xc1\x55\xe2\x9c\x71\x14"
                                  "\x5e\xdc\x62\x1c\x3a\x94\x4e\x55"
                                  "\x56\x75\xf7\x45\x6e\xa4\x9e\x94"
                                  "\xb8\xfe\xda\xd4\xac\x7d\x76\xc5"
                                  "\xb4\x65\xed\xb4\x49\x34\x71\x14"
                                  "\xdb\x8f\x10\x90\xa3\x05\x02\xdc"
                                  "\x86\x92\x6c\xbe\x9b\x57\x32\xe3"
                                  "\x2c", 65 },
			0,
		},
		{ 0 }
	};

	for (int i = 0; test_data[i].curve != 0; i++) {
		gnutls_datum_t x, y, key;

                if (test_data[i].key.data == NULL) {
		        genkey(test_data[i].curve, &x, &y, &key);
                } else {
                        x = test_data[i].x;
                        y = test_data[i].y;
                        key = test_data[i].key;
                }

		compute_key(test_data[i].curve, &x, &y, &key,
			    &test_data[i].peer_x,
			    &test_data[i].peer_y,
			    test_data[i].expected_error,
			    NULL, 0);

                if (test_data[i].key.data == NULL) {
		        gnutls_free(x.data);
		        gnutls_free(y.data);
		        gnutls_free(key.data);
                }
	}

	success("all ok\n");
}
#else
void doit(void)
{
	return;
}
#endif
