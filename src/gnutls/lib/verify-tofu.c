/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include <libtasn1.h>
#include <global.h>
#include <num.h>		/* MAX */
#include <tls-sig.h>
#include "str.h"
#include <datum.h>
#include "x509_int.h"
#include <nettle/base64.h>
#include <common.h>
#include <gnutls/abstract.h>
#include <system.h>
#include <locks.h>

struct gnutls_tdb_int {
	gnutls_tdb_store_func store;
	gnutls_tdb_store_commitment_func cstore;
	gnutls_tdb_verify_func verify;
};

static int raw_pubkey_to_base64(const gnutls_datum_t * raw,
				gnutls_datum_t * b64);
static int pgp_crt_to_raw_pubkey(const gnutls_datum_t * cert,
				 gnutls_datum_t * rpubkey);
static int verify_pubkey(const char *file, const char *host,
			 const char *service, const gnutls_datum_t * skey);

static
int store_commitment(const char *db_name, const char *host,
		     const char *service, time_t expiration,
		     gnutls_digest_algorithm_t hash_algo,
		     const gnutls_datum_t * hash);
static
int store_pubkey(const char *db_name, const char *host,
		 const char *service, time_t expiration,
		 const gnutls_datum_t * pubkey);

static int find_config_file(char *file, size_t max_size);

extern void *_gnutls_file_mutex;

struct gnutls_tdb_int default_tdb = {
	store_pubkey,
	store_commitment,
	verify_pubkey
};


/**
 * gnutls_verify_stored_pubkey:
 * @db_name: A file specifying the stored keys (use NULL for the default)
 * @tdb: A storage structure or NULL to use the default
 * @host: The peer's name
 * @service: non-NULL if this key is specific to a service (e.g. http)
 * @cert_type: The type of the certificate
 * @cert: The raw (der) data of the certificate
 * @flags: should be 0.
 *
 * This function will try to verify the provided (raw or DER-encoded) certificate 
 * using a list of stored public keys.  The @service field if non-NULL should
 * be a port number.
 *
 * The @retrieve variable if non-null specifies a custom backend for
 * the retrieval of entries. If it is NULL then the
 * default file backend will be used. In POSIX-like systems the
 * file backend uses the $HOME/.gnutls/known_hosts file.
 *
 * Note that if the custom storage backend is provided the
 * retrieval function should return %GNUTLS_E_CERTIFICATE_KEY_MISMATCH
 * if the host/service pair is found but key doesn't match,
 * %GNUTLS_E_NO_CERTIFICATE_FOUND if no such host/service with
 * the given key is found, and 0 if it was found. The storage
 * function should return 0 on success.
 *
 * Returns: If no associated public key is found
 * then %GNUTLS_E_NO_CERTIFICATE_FOUND will be returned. If a key
 * is found but does not match %GNUTLS_E_CERTIFICATE_KEY_MISMATCH
 * is returned. On success, %GNUTLS_E_SUCCESS (0) is returned, 
 * or a negative error value on other errors.
 *
 * Since: 3.0.13
 **/
int
gnutls_verify_stored_pubkey(const char *db_name,
			    gnutls_tdb_t tdb,
			    const char *host,
			    const char *service,
			    gnutls_certificate_type_t cert_type,
			    const gnutls_datum_t * cert,
			    unsigned int flags)
{
	gnutls_datum_t pubkey = { NULL, 0 };
	int ret;
	char local_file[MAX_FILENAME];

	if (cert_type != GNUTLS_CRT_X509
	    && cert_type != GNUTLS_CRT_OPENPGP)
		return
		    gnutls_assert_val
		    (GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE);

	if (db_name == NULL && tdb == NULL) {
		ret = find_config_file(local_file, sizeof(local_file));
		if (ret < 0)
			return gnutls_assert_val(ret);
		db_name = local_file;
	}

	if (tdb == NULL)
		tdb = &default_tdb;

	if (cert_type == GNUTLS_CRT_X509)
		ret = x509_raw_crt_to_raw_pubkey(cert, &pubkey);
	else
		ret = pgp_crt_to_raw_pubkey(cert, &pubkey);

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = tdb->verify(db_name, host, service, &pubkey);
	if (ret < 0 && ret != GNUTLS_E_CERTIFICATE_KEY_MISMATCH)
		ret = gnutls_assert_val(GNUTLS_E_NO_CERTIFICATE_FOUND);

      cleanup:
	gnutls_free(pubkey.data);
	return ret;
}

static int parse_commitment_line(char *line,
				 const char *host, size_t host_len,
				 const char *service, size_t service_len,
				 time_t now, const gnutls_datum_t * skey)
{
	char *p, *kp;
	char *savep = NULL;
	size_t kp_len, phash_size;
	time_t expiration;
	int ret;
	const mac_entry_st *hash_algo;
	uint8_t phash[MAX_HASH_SIZE];
	uint8_t hphash[MAX_HASH_SIZE * 2 + 1];

	/* read host */
	p = strtok_r(line, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	if (p[0] != '*' && host != NULL && strcmp(p, host) != 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	/* read service */
	p = strtok_r(NULL, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	if (p[0] != '*' && service != NULL && strcmp(p, service) != 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	/* read expiration */
	p = strtok_r(NULL, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	expiration = (time_t) atol(p);
	if (expiration > 0 && now > expiration)
		return gnutls_assert_val(GNUTLS_E_EXPIRED);

	/* read hash algorithm */
	p = strtok_r(NULL, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	hash_algo = mac_to_entry(atol(p));
	if (_gnutls_digest_get_name(hash_algo) == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	/* read hash */
	kp = strtok_r(NULL, "|", &savep);
	if (kp == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	p = strpbrk(kp, "\n \r\t|");
	if (p != NULL)
		*p = 0;

	/* hash and hex encode */
	ret =
	    _gnutls_hash_fast((gnutls_digest_algorithm_t)hash_algo->id, 
				skey->data, skey->size, phash);
	if (ret < 0)
		return gnutls_assert_val(ret);

	phash_size = _gnutls_hash_get_algo_len(hash_algo);

	p = _gnutls_bin2hex(phash, phash_size, (void *) hphash,
			    sizeof(hphash), NULL);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	kp_len = strlen(kp);
	if (kp_len != phash_size * 2)
		return
		    gnutls_assert_val(GNUTLS_E_CERTIFICATE_KEY_MISMATCH);

	if (memcmp(kp, hphash, kp_len) != 0)
		return
		    gnutls_assert_val(GNUTLS_E_CERTIFICATE_KEY_MISMATCH);

	/* key found and matches */
	return 0;
}


static int parse_line(char *line,
		      const char *host, size_t host_len,
		      const char *service, size_t service_len,
		      time_t now,
		      const gnutls_datum_t * rawkey,
		      const gnutls_datum_t * b64key)
{
	char *p, *kp;
	char *savep = NULL;
	size_t kp_len;
	time_t expiration;

	/* read version */
	p = strtok_r(line, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	if (strncmp(p, "c0", 2) == 0)
		return parse_commitment_line(p + 3, host, host_len,
					     service, service_len, now,
					     rawkey);

	if (strncmp(p, "g0", 2) != 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	/* read host */
	p = strtok_r(NULL, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	if (p[0] != '*' && host != NULL && strcmp(p, host) != 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	/* read service */
	p = strtok_r(NULL, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	if (p[0] != '*' && service != NULL && strcmp(p, service) != 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	/* read expiration */
	p = strtok_r(NULL, "|", &savep);
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	expiration = (time_t) atol(p);
	if (expiration > 0 && now > expiration)
		return gnutls_assert_val(GNUTLS_E_EXPIRED);

	/* read key */
	kp = strtok_r(NULL, "|", &savep);
	if (kp == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	p = strpbrk(kp, "\n \r\t|");
	if (p != NULL)
		*p = 0;

	kp_len = strlen(kp);
	if (kp_len != b64key->size)
		return
		    gnutls_assert_val(GNUTLS_E_CERTIFICATE_KEY_MISMATCH);

	if (memcmp(kp, b64key->data, b64key->size) != 0)
		return
		    gnutls_assert_val(GNUTLS_E_CERTIFICATE_KEY_MISMATCH);

	/* key found and matches */
	return 0;
}

/* Returns the base64 key if found 
 */
static int verify_pubkey(const char *file,
			 const char *host, const char *service,
			 const gnutls_datum_t * pubkey)
{
	FILE *fd;
	char *line = NULL;
	size_t line_size = 0;
	int ret, l2, mismatch = 0;
	size_t host_len = 0, service_len = 0;
	time_t now = gnutls_time(0);
	gnutls_datum_t b64key = { NULL, 0 };

	ret = raw_pubkey_to_base64(pubkey, &b64key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (host != NULL)
		host_len = strlen(host);
	if (service != NULL)
		service_len = strlen(service);

	fd = fopen(file, "rb");
	if (fd == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_FILE_ERROR);
		goto cleanup;
	}

	do {
		l2 = getline(&line, &line_size, fd);
		if (l2 > 0) {
			ret =
			    parse_line(line, host, host_len, service,
				       service_len, now, pubkey, &b64key);
			if (ret == 0) {	/* found */
				goto cleanup;
			} else if (ret ==
				   GNUTLS_E_CERTIFICATE_KEY_MISMATCH)
				mismatch = 1;
		}
	}
	while (l2 >= 0);

	if (mismatch)
		ret = GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
	else
		ret = GNUTLS_E_NO_CERTIFICATE_FOUND;

      cleanup:
	free(line);
	if (fd != NULL)
		fclose(fd);
	gnutls_free(b64key.data);

	return ret;
}

static int raw_pubkey_to_base64(const gnutls_datum_t * raw,
				gnutls_datum_t * b64)
{
	size_t size;

	size = BASE64_ENCODE_RAW_LENGTH(raw->size);

	b64->data = gnutls_malloc(size);
	if (b64->data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	base64_encode_raw(b64->data, raw->size, raw->data);
	b64->size = size;

	return 0;
}

static int pgp_crt_to_raw_pubkey(const gnutls_datum_t * cert,
				 gnutls_datum_t * rpubkey)
{
#ifdef ENABLE_OPENPGP
	gnutls_openpgp_crt_t crt = NULL;
	gnutls_pubkey_t pubkey = NULL;
	size_t size;
	int ret;

	ret = gnutls_openpgp_crt_init(&crt);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_openpgp_crt_import(crt, cert, GNUTLS_OPENPGP_FMT_RAW);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_import_openpgp(pubkey, crt, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	size = 0;
	ret =
	    gnutls_pubkey_export(pubkey, GNUTLS_X509_FMT_DER, NULL, &size);
	if (ret < 0 && ret != GNUTLS_E_SHORT_MEMORY_BUFFER) {
		gnutls_assert();
		goto cleanup;
	}

	rpubkey->data = gnutls_malloc(size);
	if (rpubkey->data == NULL) {
		ret = GNUTLS_E_MEMORY_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_pubkey_export(pubkey, GNUTLS_X509_FMT_DER,
				 rpubkey->data, &size);
	if (ret < 0) {
		gnutls_free(rpubkey->data);
		gnutls_assert();
		goto cleanup;
	}

	rpubkey->size = size;
	ret = 0;

      cleanup:
	gnutls_openpgp_crt_deinit(crt);
	gnutls_pubkey_deinit(pubkey);

	return ret;
#else
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
#endif
}

static
int store_pubkey(const char *db_name, const char *host,
		 const char *service, time_t expiration,
		 const gnutls_datum_t * pubkey)
{
	FILE *fd = NULL;
	gnutls_datum_t b64key = { NULL, 0 };
	int ret;

	ret = gnutls_mutex_lock(&_gnutls_file_mutex);
	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_LOCKING_ERROR);

	ret = raw_pubkey_to_base64(pubkey, &b64key);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	fd = fopen(db_name, "ab+");
	if (fd == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_FILE_ERROR);
		goto cleanup;
	}

	if (service == NULL)
		service = "*";
	if (host == NULL)
		host = "*";

	fprintf(fd, "|g0|%s|%s|%lu|%.*s\n", host, service,
		(unsigned long) expiration, b64key.size, b64key.data);

	ret = 0;

      cleanup:
	if (fd != NULL)
		fclose(fd);

	gnutls_mutex_unlock(&_gnutls_file_mutex);
	gnutls_free(b64key.data);

	return ret;
}

static
int store_commitment(const char *db_name, const char *host,
		     const char *service, time_t expiration,
		     gnutls_digest_algorithm_t hash_algo,
		     const gnutls_datum_t * hash)
{
	FILE *fd;
	char buffer[MAX_HASH_SIZE * 2 + 1];

	fd = fopen(db_name, "ab+");
	if (fd == NULL)
		return gnutls_assert_val(GNUTLS_E_FILE_ERROR);

	if (service == NULL)
		service = "*";
	if (host == NULL)
		host = "*";

	fprintf(fd, "|c0|%s|%s|%lu|%u|%s\n", host, service,
		(unsigned long) expiration, (unsigned) hash_algo,
		_gnutls_bin2hex(hash->data, hash->size, buffer,
				sizeof(buffer), NULL));

	fclose(fd);

	return 0;
}

/**
 * gnutls_store_pubkey:
 * @db_name: A file specifying the stored keys (use NULL for the default)
 * @tdb: A storage structure or NULL to use the default
 * @host: The peer's name
 * @service: non-NULL if this key is specific to a service (e.g. http)
 * @cert_type: The type of the certificate
 * @cert: The data of the certificate
 * @expiration: The expiration time (use 0 to disable expiration)
 * @flags: should be 0.
 *
 * This function will store the provided (raw or DER-encoded) certificate to 
 * the list of stored public keys. The key will be considered valid until 
 * the provided expiration time.
 *
 * The @store variable if non-null specifies a custom backend for
 * the storage of entries. If it is NULL then the
 * default file backend will be used.
 *
 * Unless an alternative @tdb is provided, the storage format is a textual format
 * consisting of a line for each host with fields separated by '|'. The contents of
 * the fields are a format-identifier which is set to 'g0', the hostname that the
 * rest of the data applies to, the numeric port or host name, the expiration
 * time in seconds since the epoch (0 for no expiration), and a base64
 * encoding of the raw (DER) public key information (SPKI) of the peer.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0.13
 **/
int
gnutls_store_pubkey(const char *db_name,
		    gnutls_tdb_t tdb,
		    const char *host,
		    const char *service,
		    gnutls_certificate_type_t cert_type,
		    const gnutls_datum_t * cert,
		    time_t expiration, unsigned int flags)
{
	gnutls_datum_t pubkey = { NULL, 0 };
	int ret;
	char local_file[MAX_FILENAME];

	if (cert_type != GNUTLS_CRT_X509
	    && cert_type != GNUTLS_CRT_OPENPGP)
		return
		    gnutls_assert_val
		    (GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE);

	if (db_name == NULL && tdb == NULL) {
		ret =
		    _gnutls_find_config_path(local_file,
					     sizeof(local_file));
		if (ret < 0)
			return gnutls_assert_val(ret);

		_gnutls_debug_log("Configuration path: %s\n", local_file);
		mkdir(local_file, 0700);

		ret = find_config_file(local_file, sizeof(local_file));
		if (ret < 0)
			return gnutls_assert_val(ret);
		db_name = local_file;
	}

	if (tdb == NULL)
		tdb = &default_tdb;

	if (cert_type == GNUTLS_CRT_X509)
		ret = x509_raw_crt_to_raw_pubkey(cert, &pubkey);
	else
		ret = pgp_crt_to_raw_pubkey(cert, &pubkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	_gnutls_debug_log("Configuration file: %s\n", db_name);

	tdb->store(db_name, host, service, expiration, &pubkey);

	ret = 0;

      cleanup:
	gnutls_free(pubkey.data);

	return ret;
}

/**
 * gnutls_store_commitment:
 * @db_name: A file specifying the stored keys (use NULL for the default)
 * @tdb: A storage structure or NULL to use the default
 * @host: The peer's name
 * @service: non-NULL if this key is specific to a service (e.g. http)
 * @hash_algo: The hash algorithm type
 * @hash: The raw hash
 * @expiration: The expiration time (use 0 to disable expiration)
 * @flags: should be 0 or %GNUTLS_SCOMMIT_FLAG_ALLOW_BROKEN.
 *
 * This function will store the provided hash commitment to 
 * the list of stored public keys. The key with the given
 * hash will be considered valid until the provided expiration time.
 *
 * The @store variable if non-null specifies a custom backend for
 * the storage of entries. If it is NULL then the
 * default file backend will be used.
 *
 * Note that this function is not thread safe with the default backend.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_store_commitment(const char *db_name,
			gnutls_tdb_t tdb,
			const char *host,
			const char *service,
			gnutls_digest_algorithm_t hash_algo,
			const gnutls_datum_t * hash,
			time_t expiration, unsigned int flags)
{
	int ret;
	char local_file[MAX_FILENAME];
	const mac_entry_st *me = hash_to_entry(hash_algo);

	if (me == NULL)
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	if (!(flags & GNUTLS_SCOMMIT_FLAG_ALLOW_BROKEN) && _gnutls_digest_is_secure(me) == 0)
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_SECURITY);

	if (_gnutls_hash_get_algo_len(me) != hash->size)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (db_name == NULL && tdb == NULL) {
		ret =
		    _gnutls_find_config_path(local_file,
					     sizeof(local_file));
		if (ret < 0)
			return gnutls_assert_val(ret);

		_gnutls_debug_log("Configuration path: %s\n", local_file);
		mkdir(local_file, 0700);

		ret = find_config_file(local_file, sizeof(local_file));
		if (ret < 0)
			return gnutls_assert_val(ret);
		db_name = local_file;
	}

	if (tdb == NULL)
		tdb = &default_tdb;

	_gnutls_debug_log("Configuration file: %s\n", db_name);

	tdb->cstore(db_name, host, service, expiration, 
		(gnutls_digest_algorithm_t)me->id, hash);

	ret = 0;

	return ret;
}

#define CONFIG_FILE "known_hosts"

static int find_config_file(char *file, size_t max_size)
{
	char path[MAX_FILENAME];
	int ret;

	ret = _gnutls_find_config_path(path, sizeof(path));
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (path[0] == 0)
		snprintf(file, max_size, "%s", CONFIG_FILE);
	else
		snprintf(file, max_size, "%s/%s", path, CONFIG_FILE);

	return 0;
}

/**
 * gnutls_tdb_init:
 * @tdb: A pointer to the type to be initialized
 *
 * This function will initialize a public key trust storage structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_tdb_init(gnutls_tdb_t * tdb)
{
	*tdb = gnutls_calloc(1, sizeof(struct gnutls_tdb_int));

	if (!*tdb)
		return GNUTLS_E_MEMORY_ERROR;

	return 0;
}

/**
 * gnutls_set_store_func:
 * @tdb: The trust storage
 * @store: The storage function
 *
 * This function will associate a storage function with the
 * trust storage structure. The function is of the following form.
 *
 * int gnutls_tdb_store_func(const char* db_name, const char* host,
 *		       const char* service, time_t expiration,
 *		       const gnutls_datum_t* pubkey);
 *
 * The @db_name should be used to pass any private data to this function.
 *
 **/
void gnutls_tdb_set_store_func(gnutls_tdb_t tdb,
			       gnutls_tdb_store_func store)
{
	tdb->store = store;
}

/**
 * gnutls_set_store_commitment_func:
 * @tdb: The trust storage
 * @cstore: The commitment storage function
 *
 * This function will associate a commitment (hash) storage function with the
 * trust storage structure. The function is of the following form.
 *
 * int gnutls_tdb_store_commitment_func(const char* db_name, const char* host,
 *		       const char* service, time_t expiration,
 *		       gnutls_digest_algorithm_t, const gnutls_datum_t* hash);
 *
 * The @db_name should be used to pass any private data to this function.
 *
 **/
void gnutls_tdb_set_store_commitment_func(gnutls_tdb_t tdb,
					  gnutls_tdb_store_commitment_func
					  cstore)
{
	tdb->cstore = cstore;
}

/**
 * gnutls_set_verify_func:
 * @tdb: The trust storage
 * @verify: The verification function
 *
 * This function will associate a retrieval function with the
 * trust storage structure. The function is of the following form.
 *
 * int gnutls_tdb_verify_func(const char* db_name, const char* host,
 *		      const char* service, const gnutls_datum_t* pubkey);
 *
 * The verify function should return zero on a match, %GNUTLS_E_CERTIFICATE_KEY_MISMATCH
 * if there is a mismatch and any other negative error code otherwise.
 *
 * The @db_name should be used to pass any private data to this function.
 *
 **/
void gnutls_tdb_set_verify_func(gnutls_tdb_t tdb,
				gnutls_tdb_verify_func verify)
{
	tdb->verify = verify;
}

/**
 * gnutls_tdb_deinit:
 * @tdb: The structure to be deinitialized
 *
 * This function will deinitialize a public key trust storage structure.
 **/
void gnutls_tdb_deinit(gnutls_tdb_t tdb)
{
	gnutls_free(tdb);
}
