/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz, Nikos Mavrogiannopoulos
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

#include "gnutls_int.h"
#include "errors.h"
#include "mpi.h"
#include "num.h"
#include "datum.h"
#include "global.h"
#include "openpgp.h"
#include "read-file.h"
#include <gnutls/abstract.h>
#include <str.h>
#include <tls-sig.h>
#include <stdio.h>
#include <sys/stat.h>

/* Map an OpenCDK error type to a GnuTLS error type. */
int _gnutls_map_cdk_rc(int rc)
{
	switch (rc) {
	case CDK_Success:
		return 0;
	case CDK_EOF:
		return GNUTLS_E_PARSING_ERROR;
	case CDK_Too_Short:
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	case CDK_General_Error:
		return GNUTLS_E_INTERNAL_ERROR;
	case CDK_File_Error:
		return GNUTLS_E_FILE_ERROR;
	case CDK_MPI_Error:
		return GNUTLS_E_MPI_SCAN_FAILED;
	case CDK_Error_No_Key:
		return GNUTLS_E_OPENPGP_GETKEY_FAILED;
	case CDK_Armor_Error:
		return GNUTLS_E_BASE64_DECODING_ERROR;
	case CDK_Inv_Value:
		return GNUTLS_E_INVALID_REQUEST;
	default:
		return GNUTLS_E_INTERNAL_ERROR;
	}
}

/**
 * gnutls_certificate_set_openpgp_key:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @crt: contains an openpgp public key
 * @pkey: is an openpgp private key
 *
 * This function sets a certificate/private key pair in the
 * gnutls_certificate_credentials_t type.  This function may be
 * called more than once (in case multiple keys/certificates exist
 * for the server).
 *
 * Note that this function requires that the preferred key ids have
 * been set and be used. See gnutls_openpgp_crt_set_preferred_key_id().
 * Otherwise the master key will be used.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_certificate_set_openpgp_key(gnutls_certificate_credentials_t res,
				   gnutls_openpgp_crt_t crt,
				   gnutls_openpgp_privkey_t pkey)
{
	int ret, ret2, i;
	gnutls_privkey_t privkey;
	gnutls_pcert_st *ccert = NULL;
	char name[MAX_CN];
	size_t max_size;
	gnutls_str_array_t names;

	_gnutls_str_array_init(&names);

	/* this should be first */

	ret = gnutls_privkey_init(&privkey);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    gnutls_privkey_import_openpgp(privkey, pkey,
					  GNUTLS_PRIVKEY_IMPORT_COPY);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ccert = gnutls_calloc(1, sizeof(gnutls_pcert_st));
	if (ccert == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	max_size = sizeof(name);
	ret = 0;
	for (i = 0; !(ret < 0); i++) {
		ret = gnutls_openpgp_crt_get_name(crt, i, name, &max_size);
		if (ret >= 0) {
			ret2 =
			    _gnutls_str_array_append(&names, name,
						     max_size);
			if (ret2 < 0) {
				gnutls_assert();
				ret = ret2;
				goto cleanup;
			}
		}
	}

	ret = gnutls_pcert_import_openpgp(ccert, crt, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = certificate_credentials_append_pkey(res, privkey);
	if (ret >= 0)
		ret =
		    certificate_credential_append_crt_list(res, names,
							   ccert, 1);

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	res->ncerts++;

	ret = _gnutls_check_key_cert_match(res);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;

      cleanup:
	gnutls_privkey_deinit(privkey);
	gnutls_free(ccert);
	_gnutls_str_array_clear(&names);
	return ret;
}

/**
 * gnutls_certificate_get_openpgp_key:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @index: The index of the key to obtain.
 * @key: Location to store the key.
 *
 * Obtains a OpenPGP private key that has been stored in @res with one of
 * gnutls_certificate_set_openpgp_key(),
 * gnutls_certificate_set_openpgp_key_file(),
 * gnutls_certificate_set_openpgp_key_file2(),
 * gnutls_certificate_set_openpgp_key_mem(), or
 * gnutls_certificate_set_openpgp_key_mem2().
 * The returned key must be deallocated with gnutls_openpgp_privkey_deinit()
 * when no longer needed.
 *
 * If there is no key with the given index,
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned. If the key with the
 * given index is not a X.509 key, %GNUTLS_E_INVALID_REQUEST is returned.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success, or a negative error code.
 *
 * Since: 3.4.0
 */
int
gnutls_certificate_get_openpgp_key(gnutls_certificate_credentials_t res,
				   unsigned index,
				   gnutls_openpgp_privkey_t *key)
{
	if (index >= res->ncerts) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	return gnutls_privkey_export_openpgp(res->pkey[index], key);
}

/**
 * gnutls_certificate_get_openpgp_crt:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @index: The index of the certificate list to obtain.
 * @crt_list: Where to store the certificate list.
 * @crt_list_size: Will hold the number of certificates.
 *
 * Obtains a X.509 certificate list that has been stored in @res with one of
 * gnutls_certificate_set_openpgp_key(),
 * gnutls_certificate_set_openpgp_key_file(),
 * gnutls_certificate_set_openpgp_key_file2(),
 * gnutls_certificate_set_openpgp_key_mem(), or
 * gnutls_certificate_set_openpgp_key_mem2().  Each certificate in the
 * returned certificate list must be deallocated with
 * gnutls_openpgp_crt_deinit(), and the list itself must be freed with
 * gnutls_free().
 *
 * If there is no certificate with the given index,
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned. If the certificate
 * with the given index is not a X.509 certificate, %GNUTLS_E_INVALID_REQUEST
 * is returned.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success, or a negative error code.
 *
 * Since: 3.4.0
 */
int
gnutls_certificate_get_openpgp_crt(gnutls_certificate_credentials_t res,
				   unsigned index,
				   gnutls_openpgp_crt_t **crt_list,
				   unsigned *crt_list_size)
{
	int ret;
	unsigned i;

	if (index >= res->ncerts) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	*crt_list_size = res->certs[index].cert_list_length;
	*crt_list = gnutls_malloc(
		res->certs[index].cert_list_length * sizeof (gnutls_openpgp_crt_t));
	if (*crt_list == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	for (i = 0; i < res->certs[index].cert_list_length; ++i) {
		ret = gnutls_pcert_export_openpgp(&res->certs[index].cert_list[i], crt_list[i]);
		if (ret < 0) {
			while (i--)
				gnutls_openpgp_crt_deinit(*crt_list[i]);
			gnutls_free(*crt_list);
			*crt_list = NULL;

			return gnutls_assert_val(ret);
		}
	}

	return 0;
}

/*-
 * gnutls_openpgp_get_key:
 * @key: the destination context to save the key.
 * @keyring: the datum struct that contains all keyring information.
 * @attr: The attribute (keyid, fingerprint, ...).
 * @by: What attribute is used.
 *
 * This function can be used to retrieve keys by different pattern
 * from a binary or a file keyring.
 -*/
int
gnutls_openpgp_get_key(gnutls_datum_t * key,
		       gnutls_openpgp_keyring_t keyring, key_attr_t by,
		       uint8_t * pattern)
{
	cdk_kbnode_t knode = NULL;
	unsigned long keyid[2];
	unsigned char *buf;
	void *desc;
	size_t len;
	int rc = 0;
	cdk_keydb_search_t st;

	if (!key || !keyring || by == KEY_ATTR_NONE) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	memset(key, 0, sizeof *key);

	if (by == KEY_ATTR_SHORT_KEYID) {
		keyid[0] = _gnutls_read_uint32(pattern);
		desc = keyid;
	} else if (by == KEY_ATTR_KEYID) {
		keyid[0] = _gnutls_read_uint32(pattern);
		keyid[1] = _gnutls_read_uint32(pattern + 4);
		desc = keyid;
	} else
		desc = pattern;
	rc = cdk_keydb_search_start(&st, keyring->db, by, desc);
	if (!rc)
		rc = cdk_keydb_search(st, keyring->db, &knode);

	cdk_keydb_search_release(st);

	if (rc) {
		rc = _gnutls_map_cdk_rc(rc);
		goto leave;
	}

	if (!cdk_kbnode_find(knode, CDK_PKT_PUBLIC_KEY)) {
		rc = GNUTLS_E_OPENPGP_GETKEY_FAILED;
		goto leave;
	}

	/* We let the function allocate the buffer to avoid
	   to call the function twice. */
	rc = cdk_kbnode_write_to_mem_alloc(knode, &buf, &len);
	if (!rc)
		_gnutls_datum_append(key, buf, len);
	gnutls_free(buf);

      leave:
	cdk_kbnode_release(knode);
	return rc;
}

/**
 * gnutls_certificate_set_openpgp_key_mem:
 * @res: the destination context to save the data.
 * @cert: the datum that contains the public key.
 * @key: the datum that contains the secret key.
 * @format: the format of the keys
 *
 * This function is used to load OpenPGP keys into the GnuTLS credential 
 * structure. The datum should contain at least one valid non encrypted subkey.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_certificate_set_openpgp_key_mem(gnutls_certificate_credentials_t
				       res, const gnutls_datum_t * cert,
				       const gnutls_datum_t * key,
				       gnutls_openpgp_crt_fmt_t format)
{
	return gnutls_certificate_set_openpgp_key_mem2(res, cert, key,
						       NULL, format);
}

/**
 * gnutls_certificate_set_openpgp_key_file:
 * @res: the destination context to save the data.
 * @certfile: the file that contains the public key.
 * @keyfile: the file that contains the secret key.
 * @format: the format of the keys
 *
 * This function is used to load OpenPGP keys into the GnuTLS
 * credentials structure. The file should contain at least one valid non encrypted subkey.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_certificate_set_openpgp_key_file(gnutls_certificate_credentials_t
					res, const char *certfile,
					const char *keyfile,
					gnutls_openpgp_crt_fmt_t format)
{
	return gnutls_certificate_set_openpgp_key_file2(res, certfile,
							keyfile, NULL,
							format);
}

static int get_keyid(gnutls_openpgp_keyid_t keyid, const char *str)
{
	size_t keyid_size = GNUTLS_OPENPGP_KEYID_SIZE;
	size_t len = strlen(str);
	gnutls_datum_t tmp;
	int ret;

	if (len != 16) {
		_gnutls_debug_log
		    ("The OpenPGP subkey ID has to be 16 hexadecimal characters.\n");
		return GNUTLS_E_INVALID_REQUEST;
	}

	tmp.data = (void*)str;
	tmp.size = len;
	ret = gnutls_hex_decode(&tmp, keyid, &keyid_size);
	if (ret < 0) {
		_gnutls_debug_log("Error converting hex string: %s.\n",
				  str);
		return GNUTLS_E_INVALID_REQUEST;
	}

	return 0;
}

/**
 * gnutls_certificate_set_openpgp_key_mem2:
 * @res: the destination context to save the data.
 * @cert: the datum that contains the public key.
 * @key: the datum that contains the secret key.
 * @subkey_id: a hex encoded subkey id
 * @format: the format of the keys
 *
 * This function is used to load OpenPGP keys into the GnuTLS
 * credentials structure. The datum should contain at least one valid non encrypted subkey.
 *
 * The special keyword "auto" is also accepted as @subkey_id.  In that
 * case the gnutls_openpgp_crt_get_auth_subkey() will be used to
 * retrieve the subkey.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.4.0
 **/
int
gnutls_certificate_set_openpgp_key_mem2(gnutls_certificate_credentials_t
					res, const gnutls_datum_t * cert,
					const gnutls_datum_t * key,
					const char *subkey_id,
					gnutls_openpgp_crt_fmt_t format)
{
	gnutls_openpgp_privkey_t pkey;
	gnutls_openpgp_crt_t crt;
	int ret;
	uint8_t keyid[GNUTLS_OPENPGP_KEYID_SIZE];

	ret = gnutls_openpgp_privkey_init(&pkey);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_openpgp_privkey_import(pkey, key, format, NULL, 0);
	if (ret < 0) {
		gnutls_assert();
		gnutls_openpgp_privkey_deinit(pkey);
		return ret;
	}

	ret = gnutls_openpgp_crt_init(&crt);
	if (ret < 0) {
		gnutls_assert();
		gnutls_openpgp_privkey_deinit(pkey);
		return ret;
	}

	ret = gnutls_openpgp_crt_import(crt, cert, format);
	if (ret < 0) {
		gnutls_assert();
		gnutls_openpgp_privkey_deinit(pkey);
		gnutls_openpgp_crt_deinit(crt);
		return ret;
	}

	if (subkey_id != NULL) {
		if (strcasecmp(subkey_id, "auto") == 0)
			ret =
			    gnutls_openpgp_crt_get_auth_subkey(crt, keyid,
							       1);
		else
			ret = get_keyid(keyid, subkey_id);

		if (ret < 0)
			gnutls_assert();

		if (ret >= 0) {
			ret =
			    gnutls_openpgp_crt_set_preferred_key_id(crt,
								    keyid);
			if (ret >= 0)
				ret =
				    gnutls_openpgp_privkey_set_preferred_key_id
				    (pkey, keyid);
		}

		if (ret < 0) {
			gnutls_assert();
			gnutls_openpgp_privkey_deinit(pkey);
			gnutls_openpgp_crt_deinit(crt);
			return ret;
		}
	}

	ret = gnutls_certificate_set_openpgp_key(res, crt, pkey);

	gnutls_openpgp_crt_deinit(crt);
	gnutls_openpgp_privkey_deinit(pkey);

	return ret;
}

/**
 * gnutls_certificate_set_openpgp_key_file2:
 * @res: the destination context to save the data.
 * @certfile: the file that contains the public key.
 * @keyfile: the file that contains the secret key.
 * @subkey_id: a hex encoded subkey id
 * @format: the format of the keys
 *
 * This function is used to load OpenPGP keys into the GnuTLS credential 
 * structure. The file should contain at least one valid non encrypted subkey.
 *
 * The special keyword "auto" is also accepted as @subkey_id.  In that
 * case the gnutls_openpgp_crt_get_auth_subkey() will be used to
 * retrieve the subkey.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.4.0
 **/
int
gnutls_certificate_set_openpgp_key_file2(gnutls_certificate_credentials_t
					 res, const char *certfile,
					 const char *keyfile,
					 const char *subkey_id,
					 gnutls_openpgp_crt_fmt_t format)
{
	struct stat statbuf;
	gnutls_datum_t key, cert;
	int rc;
	size_t size;

	if (!res || !keyfile || !certfile) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (stat(certfile, &statbuf) || stat(keyfile, &statbuf)) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	cert.data = (void *) read_binary_file(certfile, &size);
	cert.size = (unsigned int) size;
	if (cert.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	key.data = (void *) read_binary_file(keyfile, &size);
	key.size = (unsigned int) size;
	if (key.data == NULL) {
		gnutls_assert();
		free(cert.data);
		return GNUTLS_E_FILE_ERROR;
	}

	rc = gnutls_certificate_set_openpgp_key_mem2(res, &cert, &key,
						     subkey_id, format);

	free(cert.data);
	free(key.data);

	if (rc < 0) {
		gnutls_assert();
		return rc;
	}

	return 0;
}


int gnutls_openpgp_count_key_names(const gnutls_datum_t * cert)
{
	cdk_kbnode_t knode, p, ctx;
	cdk_packet_t pkt;
	int nuids;

	if (cert == NULL) {
		gnutls_assert();
		return 0;
	}

	if (cdk_kbnode_read_from_mem(&knode, 0, cert->data, cert->size, 1)) {
		gnutls_assert();
		return 0;
	}

	ctx = NULL;
	for (nuids = 0;;) {
		p = cdk_kbnode_walk(knode, &ctx, 0);
		if (!p)
			break;
		pkt = cdk_kbnode_get_packet(p);
		if (pkt->pkttype == CDK_PKT_USER_ID)
			nuids++;
	}

	cdk_kbnode_release(knode);
	return nuids;
}

/**
 * gnutls_certificate_set_openpgp_keyring_file:
 * @c: A certificate credentials structure
 * @file: filename of the keyring.
 * @format: format of keyring.
 *
 * The function is used to set keyrings that will be used internally
 * by various OpenPGP functions. For example to find a key when it
 * is needed for an operations. The keyring will also be used at the
 * verification functions.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_certificate_set_openpgp_keyring_file
(gnutls_certificate_credentials_t c, const char *file,
 gnutls_openpgp_crt_fmt_t format)
{
	gnutls_datum_t ring;
	size_t size;
	int rc;

	if (!c || !file) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ring.data = (void *) read_binary_file(file, &size);
	ring.size = (unsigned int) size;
	if (ring.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	rc = gnutls_certificate_set_openpgp_keyring_mem(c, ring.data,
							ring.size, format);

	free(ring.data);

	return rc;
}

/**
 * gnutls_certificate_set_openpgp_keyring_mem:
 * @c: A certificate credentials structure
 * @data: buffer with keyring data.
 * @dlen: length of data buffer.
 * @format: the format of the keyring
 *
 * The function is used to set keyrings that will be used internally
 * by various OpenPGP functions. For example to find a key when it
 * is needed for an operations. The keyring will also be used at the
 * verification functions.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_certificate_set_openpgp_keyring_mem(gnutls_certificate_credentials_t
					   c, const uint8_t * data,
					   size_t dlen,
					   gnutls_openpgp_crt_fmt_t format)
{
	gnutls_datum_t ddata;
	int rc;

	ddata.data = (void *) data;
	ddata.size = dlen;

	if (!c || !data || !dlen) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	rc = gnutls_openpgp_keyring_init(&c->keyring);
	if (rc < 0) {
		gnutls_assert();
		return rc;
	}

	rc = gnutls_openpgp_keyring_import(c->keyring, &ddata, format);
	if (rc < 0) {
		gnutls_assert();
		gnutls_openpgp_keyring_deinit(c->keyring);
		return rc;
	}

	return 0;
}

/*-
 * _gnutls_openpgp_request_key - Receives a key from a database, key server etc
 * @ret - a pointer to gnutls_datum_t type.
 * @cred - a gnutls_certificate_credentials_t type.
 * @key_fingerprint - The keyFingerprint
 * @key_fingerprint_size - the size of the fingerprint
 *
 * Retrieves a key from a local database, keyring, or a key server. The
 * return value is locally allocated.
 *
 -*/
int
_gnutls_openpgp_request_key(gnutls_session_t session, gnutls_datum_t * ret,
			    const gnutls_certificate_credentials_t cred,
			    uint8_t * key_fpr, int key_fpr_size)
{
	int rc = 0;

	if (!ret || !cred || !key_fpr) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key_fpr_size != 16 && key_fpr_size != 20)
		return GNUTLS_E_HASH_FAILED;	/* only MD5 and SHA1 are supported */

	rc = gnutls_openpgp_get_key(ret, cred->keyring, KEY_ATTR_FPR,
				    key_fpr);

	if (rc >= 0) {		/* key was found */
		rc = 0;
		goto error;
	} else
		rc = GNUTLS_E_OPENPGP_GETKEY_FAILED;

	/* If the callback function was set, then try this one. */
	if (session->internals.openpgp_recv_key_func != NULL) {
		rc = session->internals.openpgp_recv_key_func(session,
							      key_fpr,
							      key_fpr_size,
							      ret);
		if (rc < 0) {
			gnutls_assert();
			rc = GNUTLS_E_OPENPGP_GETKEY_FAILED;
			goto error;
		}
	}

      error:

	return rc;
}

/**
 * gnutls_openpgp_set_recv_key_function:
 * @session: a TLS session
 * @func: the callback
 *
 * This function will set a key retrieval function for OpenPGP keys. This
 * callback is only useful in server side, and will be used if the peer
 * sent a key fingerprint instead of a full key.
 *
 * The retrieved key must be allocated using gnutls_malloc().
 *
 **/
void
gnutls_openpgp_set_recv_key_function(gnutls_session_t session,
				     gnutls_openpgp_recv_key_func func)
{
	session->internals.openpgp_recv_key_func = func;
}
