/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Timo Schulz
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* Functions on keyring parsing
 */

#include "gnutls_int.h"
#include <datum.h>
#include <global.h>
#include "errors.h"
#include <openpgp_int.h>
#include <openpgp.h>
#include <num.h>

/* Keyring stuff.
 */

/**
 * gnutls_openpgp_keyring_init:
 * @keyring: A pointer to the type to be initialized
 *
 * This function will initialize an keyring structure.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int gnutls_openpgp_keyring_init(gnutls_openpgp_keyring_t * keyring)
{
	*keyring = gnutls_calloc(1, sizeof(gnutls_openpgp_keyring_int));

	if (*keyring)
		return 0;	/* success */
	return GNUTLS_E_MEMORY_ERROR;
}


/**
 * gnutls_openpgp_keyring_deinit:
 * @keyring: A pointer to the type to be initialized
 *
 * This function will deinitialize a keyring structure.
 **/
void gnutls_openpgp_keyring_deinit(gnutls_openpgp_keyring_t keyring)
{
	if (!keyring)
		return;

	if (keyring->db) {
		cdk_keydb_free(keyring->db);
		keyring->db = NULL;
	}

	gnutls_free(keyring);
}

/**
 * gnutls_openpgp_keyring_check_id:
 * @ring: holds the keyring to check against
 * @keyid: will hold the keyid to check for.
 * @flags: unused (should be 0)
 *
 * Check if a given key ID exists in the keyring.
 *
 * Returns: %GNUTLS_E_SUCCESS on success (if keyid exists) and a
 *   negative error code on failure.
 **/
int
gnutls_openpgp_keyring_check_id(gnutls_openpgp_keyring_t ring,
				const gnutls_openpgp_keyid_t keyid,
				unsigned int flags)
{
	cdk_pkt_pubkey_t pk;
	uint32_t id[2];

	id[0] = _gnutls_read_uint32(keyid);
	id[1] = _gnutls_read_uint32(&keyid[4]);

	if (!cdk_keydb_get_pk(ring->db, id, &pk)) {
		cdk_pk_release(pk);
		return 0;
	}

	_gnutls_debug_log("PGP: key not found %08lX\n",
			  (unsigned long) id[1]);
	return GNUTLS_E_NO_CERTIFICATE_FOUND;
}

/**
 * gnutls_openpgp_keyring_import:
 * @keyring: The structure to store the parsed key.
 * @data: The RAW or BASE64 encoded keyring.
 * @format: One of #gnutls_openpgp_keyring_fmt elements.
 *
 * This function will convert the given RAW or Base64 encoded keyring
 * to the native #gnutls_openpgp_keyring_t format.  The output will be
 * stored in 'keyring'.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_keyring_import(gnutls_openpgp_keyring_t keyring,
			      const gnutls_datum_t * data,
			      gnutls_openpgp_crt_fmt_t format)
{
	cdk_error_t err;
	cdk_stream_t input = NULL;
	size_t raw_len = 0;
	uint8_t *raw_data = NULL;
	unsigned free_data = 0;

	if (data->data == NULL || data->size == 0) {
		gnutls_assert();
		return GNUTLS_E_OPENPGP_GETKEY_FAILED;
	}

	_gnutls_debug_log("PGP: keyring import format '%s'\n",
			  format ==
			  GNUTLS_OPENPGP_FMT_RAW ? "raw" : "base64");

	/* Create a new stream from the given data, decode it, and import
	 * the raw database. This to avoid using opencdk streams which are
	 * not thread safe.
	 */
	if (format == GNUTLS_OPENPGP_FMT_BASE64) {
		size_t seen = 0;

		err =
		    cdk_stream_tmp_from_mem(data->data, data->size,
					    &input);
		if (err == 0)
			err = cdk_stream_set_armor_flag(input, 0);
		if (err) {
			gnutls_assert();
			err = _gnutls_map_cdk_rc(err);
			goto error;
		}

		raw_len = cdk_stream_get_length(input);
		if (raw_len == 0) {
			gnutls_assert();
			err = GNUTLS_E_BASE64_DECODING_ERROR;
			goto error;
		}

		raw_data = gnutls_malloc(raw_len);
		if (raw_data == NULL) {
			gnutls_assert();
			err = GNUTLS_E_MEMORY_ERROR;
			goto error;
		}

		do {
			err =
			    cdk_stream_read(input, raw_data + seen,
					    raw_len - seen);

			if (err > 0)
				seen += err;
		}
		while (seen < raw_len && err != EOF && err > 0);

		raw_len = seen;
		if (raw_len == 0) {
			gnutls_assert();
			err = GNUTLS_E_BASE64_DECODING_ERROR;
			goto error;
		}

		free_data = 1;
	} else {		/* RAW */
		raw_len = data->size;
		raw_data = data->data;
	}

	err =
	    cdk_keydb_new_from_mem(&keyring->db, 0, 0, raw_data, raw_len);
	if (err)
		gnutls_assert();

	if (free_data) {
		err = _gnutls_map_cdk_rc(err);
		goto error;
	}

	return _gnutls_map_cdk_rc(err);

      error:
	gnutls_free(raw_data);
	cdk_stream_close(input);

	return err;
}

#define knode_is_pkey(node) \
  cdk_kbnode_find_packet (node, CDK_PKT_PUBLIC_KEY)!=NULL

/**
 * gnutls_openpgp_keyring_get_crt_count:
 * @ring: is an OpenPGP key ring
 *
 * This function will return the number of OpenPGP certificates
 * present in the given keyring.
 *
 * Returns: the number of subkeys, or a negative error code on error.
 **/
int gnutls_openpgp_keyring_get_crt_count(gnutls_openpgp_keyring_t ring)
{
	cdk_kbnode_t knode;
	cdk_error_t err;
	cdk_keydb_search_t st;
	int ret = 0;

	err =
	    cdk_keydb_search_start(&st, ring->db, CDK_DBSEARCH_NEXT, NULL);
	if (err != CDK_Success) {
		gnutls_assert();
		return _gnutls_map_cdk_rc(err);
	}

	do {
		err = cdk_keydb_search(st, ring->db, &knode);
		if (err != CDK_Error_No_Key && err != CDK_Success) {
			gnutls_assert();
			cdk_keydb_search_release(st);
			return _gnutls_map_cdk_rc(err);
		}

		if (knode_is_pkey(knode))
			ret++;

		cdk_kbnode_release(knode);

	}
	while (err != CDK_Error_No_Key);

	cdk_keydb_search_release(st);
	return ret;
}

/**
 * gnutls_openpgp_keyring_get_crt:
 * @ring: Holds the keyring.
 * @idx: the index of the certificate to export
 * @cert: An uninitialized #gnutls_openpgp_crt_t type
 *
 * This function will extract an OpenPGP certificate from the given
 * keyring.  If the index given is out of range
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned. The
 * returned structure needs to be deinited.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_keyring_get_crt(gnutls_openpgp_keyring_t ring,
			       unsigned int idx,
			       gnutls_openpgp_crt_t * cert)
{
	cdk_kbnode_t knode;
	cdk_error_t err;
	int ret = 0;
	unsigned int count = 0;
	cdk_keydb_search_t st;

	err =
	    cdk_keydb_search_start(&st, ring->db, CDK_DBSEARCH_NEXT, NULL);
	if (err != CDK_Success) {
		gnutls_assert();
		return _gnutls_map_cdk_rc(err);
	}

	do {
		err = cdk_keydb_search(st, ring->db, &knode);
		if (err != CDK_EOF && err != CDK_Success) {
			gnutls_assert();
			cdk_keydb_search_release(st);
			return _gnutls_map_cdk_rc(err);
		}

		if (idx == count && err == CDK_Success) {
			ret = gnutls_openpgp_crt_init(cert);
			if (ret == 0)
				(*cert)->knode = knode;
			cdk_keydb_search_release(st);
			return ret;
		}

		if (knode_is_pkey(knode))
			count++;

		cdk_kbnode_release(knode);

	}
	while (err != CDK_EOF);

	cdk_keydb_search_release(st);
	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
}
