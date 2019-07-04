/* minip12.c - A mini pkcs-12 implementation (modified for gnutls)
 *
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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

#include <mpi.h>
#include "errors.h"
#include <x509_int.h>
#include <algorithms.h>

#define MAX_PASS_LEN 256
#define MAX_V_SIZE 128
/* ID should be:
 * 3 for MAC
 * 2 for IV
 * 1 for encryption key
 *
 * Note that this function produces different key for the
 * NULL password, and for the password with zero length.
 */
int
_gnutls_pkcs12_string_to_key(const mac_entry_st * me,
			     unsigned int id, const uint8_t * salt,
			     unsigned int salt_size, unsigned int iter,
			     const char *pw, unsigned int req_keylen,
			     uint8_t * keybuf)
{
	int rc;
	unsigned int i, j;
	digest_hd_st md;
	bigint_t num_b1 = NULL, num_ij = NULL;
	bigint_t v_mpi = NULL;
	unsigned int pwlen;
	uint8_t hash[MAX_HASH_SIZE], buf_b[MAX_V_SIZE], buf_i[MAX_PASS_LEN + MAX_V_SIZE], *p;
	uint8_t d[MAX_V_SIZE];
	size_t cur_keylen;
	size_t n, m, plen, i_size;
	size_t slen;
	gnutls_datum_t ucs2 = {NULL, 0};
	unsigned mac_len;
	uint8_t v_val[MAX_V_SIZE+1];
	unsigned v_size = 0;

	switch (me->id) {
		case GNUTLS_DIG_GOSTR_94:
			v_size = 32;
			break;
		case GNUTLS_DIG_SHA1:
		case GNUTLS_DIG_SHA224:
		case GNUTLS_DIG_SHA256:
		case GNUTLS_DIG_STREEBOG_256:
		case GNUTLS_DIG_STREEBOG_512:
			v_size = 64;
			break;
		case GNUTLS_DIG_SHA384:
		case GNUTLS_DIG_SHA512:
			v_size = 128;
			break;
		default:
			break;
	}

	if (v_size == 0 || v_size > MAX_V_SIZE)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	memset(v_val, 0, sizeof(v_val));
	v_val[0] = 0x01; /* make it be 2^64 or 2^128 */

	cur_keylen = 0;

	if (pw) {
		pwlen = strlen(pw);

		if (pwlen == 0) {
			ucs2.data = gnutls_calloc(1, 2);
			if (ucs2.data == NULL)
				return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
			ucs2.size = 2;
		} else {
			rc = _gnutls_utf8_to_ucs2(pw, pwlen, &ucs2, 1);
			if (rc < 0)
				return gnutls_assert_val(rc);

			/* include terminating zero */
			ucs2.size+=2;
		}
		pwlen = ucs2.size;
		pw = (char*)ucs2.data;
	} else {
		pwlen = 0;
	}

	if (pwlen > MAX_PASS_LEN) {
		rc = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	rc = _gnutls_mpi_init_scan(&v_mpi, v_val, v_size+1);
	if (rc < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Store salt and password in BUF_I */
	slen = ((salt_size+v_size-1)/v_size) * v_size;
	plen = ((pwlen+v_size-1)/v_size) * v_size;
	i_size = slen + plen;

	if (i_size > sizeof(buf_i)) {
		rc = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	p = buf_i;
	for (i = 0; i < slen; i++)
		*p++ = salt[i % salt_size];

	if (pw) {
		for (i = j = 0; i < plen; i += 2) {
			*p++ = pw[j];
			*p++ = pw[j+1];
			j+=2;
			if (j >= pwlen)
				j = 0;
		}
	} else {
		memset(p, 0, plen);
	}

	mac_len = _gnutls_mac_get_algo_len(me);
	assert(mac_len != 0);

	for (;;) {
		rc = _gnutls_hash_init(&md, me);
		if (rc < 0) {
			gnutls_assert();
			goto cleanup;
		}
		memset(d, id & 0xff, v_size);
		_gnutls_hash(&md, d, v_size);
		_gnutls_hash(&md, buf_i, i_size);
		_gnutls_hash_deinit(&md, hash);
		for (i = 1; i < iter; i++) {
			rc = _gnutls_hash_fast((gnutls_digest_algorithm_t)me->id,
					       hash, mac_len, hash);
			if (rc < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}
		for (i = 0; i < mac_len && cur_keylen < req_keylen; i++)
			keybuf[cur_keylen++] = hash[i];
		if (cur_keylen == req_keylen) {
			rc = 0;	/* ready */
			goto cleanup;
		}

		/* need more bytes. */
		for (i = 0; i < v_size; i++)
			buf_b[i] = hash[i % mac_len];
		n = v_size;
		rc = _gnutls_mpi_init_scan(&num_b1, buf_b, n);
		if (rc < 0) {
			gnutls_assert();
			goto cleanup;
		}

		rc = _gnutls_mpi_add_ui(num_b1, num_b1, 1);
		if (rc < 0) {
			gnutls_assert();
			goto cleanup;
		}

		for (i = 0; i < i_size; i += v_size) {
			n = v_size;
			rc = _gnutls_mpi_init_scan(&num_ij, buf_i + i, n);
			if (rc < 0) {
				gnutls_assert();
				goto cleanup;
			}

			rc = _gnutls_mpi_addm(num_ij, num_ij, num_b1, v_mpi);
			if (rc < 0) {
				gnutls_assert();
				goto cleanup;
			}

			n = v_size;
			m = (_gnutls_mpi_get_nbits(num_ij) + 7) / 8;

			memset(buf_i + i, 0, n - m);
			rc = _gnutls_mpi_print(num_ij, buf_i + i + n - m,
					       &n);
			if (rc < 0) {
				gnutls_assert();
				goto cleanup;
			}
			_gnutls_mpi_release(&num_ij);
		}
	}
      cleanup:
	_gnutls_mpi_release(&num_ij);
	_gnutls_mpi_release(&num_b1);
	_gnutls_mpi_release(&v_mpi);
	gnutls_free(ucs2.data);

	return rc;
}
