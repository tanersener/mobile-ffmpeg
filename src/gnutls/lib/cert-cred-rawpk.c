/*
 * Copyright (C) 2017 - 2018 ARPA2 project
 *
 * Author: Tom Vrancken (dev@tomvrancken.nl)
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
#include <gnutls/gnutls.h>
#include "datum.h"
#include "auth/cert.h"
#include "x509.h"
#include "cert-cred.h"
#include "read-file.h"
#include <stdint.h>


/**
 * gnutls_certificate_set_rawpk_key_mem:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @spki: contains a raw public key in
 *   PKIX.SubjectPublicKeyInfo format.
 * @pkey: contains a raw private key.
 * @format: encoding of the keys. DER or PEM.
 * @pass: an optional password to unlock the private key pkey.
 * @key_usage: An ORed sequence of %GNUTLS_KEY_* flags.
 * @names: is an array of DNS names belonging to the public-key (NULL if none).
 * @names_length: holds the length of the names list.
 * @flags: an ORed sequence of #gnutls_pkcs_encrypt_flags_t.
 *   These apply to the private key pkey.
 *
 * This function sets a public/private keypair in the
 * #gnutls_certificate_credentials_t type to be used for authentication
 * and/or encryption. @spki and @privkey should match otherwise set
 * signatures cannot be validated. In case of no match this function
 * returns %GNUTLS_E_CERTIFICATE_KEY_MISMATCH. This function should
 * be called once for the client because there is currently no mechanism
 * to determine which raw public-key to select for the peer when there
 * are multiple present. Multiple raw public keys for the server can be
 * distinghuished by setting the @names.
 *
 * Note here that @spki is a raw public-key as defined
 * in RFC7250. It means that there is no surrounding certificate that
 * holds the public key and that there is therefore no direct mechanism
 * to prove the authenticity of this key. The keypair can be used during
 * a TLS handshake but its authenticity should be established via a
 * different mechanism (e.g. TOFU or known fingerprint).
 *
 * The supported formats are basic unencrypted key, PKCS8, PKCS12,
 * and the openssl format and will be autodetected.
 *
 * If the raw public-key and the private key are given in PEM encoding
 * then the strings that hold their values must be null terminated.
 *
 * Key usage (as defined by X.509 extension (2.5.29.15)) can be explicitly
 * set because there is no certificate structure around the key to define
 * this value. See for more info gnutls_x509_crt_get_key_usage().
 *
 * Note that, this function by default returns zero on success and a
 * negative value on error. Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2
 * is set using gnutls_certificate_set_flags() it returns an index
 * (greater or equal to zero). That index can be used in other functions
 * to refer to the added key-pair.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, in case the
 *   key pair does not match %GNUTLS_E_CERTIFICATE_KEY_MISMATCH is returned,
 *   in other erroneous cases a different negative error code is returned.
 *
 * Since: 3.6.6
 **/
int gnutls_certificate_set_rawpk_key_mem(gnutls_certificate_credentials_t cred,
				    const gnutls_datum_t* spki,
				    const gnutls_datum_t* pkey,
				    gnutls_x509_crt_fmt_t format,
				    const char* pass,
				    unsigned int key_usage,
				    const char **names,
				    unsigned int names_length,
				    unsigned int flags)
{
	int ret;
	gnutls_privkey_t privkey;
	gnutls_pcert_st* pcert;
	gnutls_str_array_t str_names;
	unsigned int i;

	if (pkey == NULL || spki == NULL) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	/* Import our private key. This function does all the necessary
	 * inits, checks and imports. */
	ret = _gnutls_read_key_mem(cred, pkey->data, pkey->size,
				format, pass, flags, &privkey);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	/* We now convert our raw public key to a parsed certificate (pcert) structure */
	pcert = gnutls_calloc(1, sizeof(*pcert));
	if (pcert == NULL) {
		gnutls_privkey_deinit(privkey);

		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}
	// Import our raw public key to the pcert structure
	ret = gnutls_pcert_import_rawpk_raw(pcert, spki,
					format, key_usage, 0);
	if (ret < 0) {
		gnutls_privkey_deinit(privkey);

		return gnutls_assert_val(ret);
	}

	/* Process the names, if any */
	_gnutls_str_array_init(&str_names);

	if (names != NULL && names_length > 0) {
		for (i = 0; i < names_length; i++) {
			ret =
			    _gnutls_str_array_append_idna(&str_names, names[i],
						     strlen(names[i]));
			if (ret < 0) {
				gnutls_privkey_deinit(privkey);
				_gnutls_str_array_clear(&str_names);

				return gnutls_assert_val(ret);
			}
		}
	}

	/* Now that we have converted the key material to our internal structures
	 * we can now add them to the credentials structure */
	ret = _gnutls_certificate_credential_append_keypair(cred, privkey, str_names, pcert, 1);
	// Check for errors
	if (ret < 0) {
		gnutls_privkey_deinit(privkey);
		gnutls_pcert_deinit(pcert);
		gnutls_free(pcert);

		return gnutls_assert_val(ret);
	}
	// Successfully added a certificate
	cred->ncerts++;

	/* Check whether the key pair matches.
	 * After this point we do not deinitialize anything on failure to avoid
	 * double freeing. We intentionally keep everything as the credentials state
	 * is documented to be in undefined state. */
	if ((ret = _gnutls_check_key_cert_match(cred)) < 0) {
		return gnutls_assert_val(ret);
	}

	CRED_RET_SUCCESS(cred);
}


/**
 * gnutls_certificate_set_rawpk_key_file:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @rawpkfile: contains a raw public key in
 *   PKIX.SubjectPublicKeyInfo format.
 * @privkeyfile: contains a file path to a private key.
 * @format: encoding of the keys. DER or PEM.
 * @pass: an optional password to unlock the private key privkeyfile.
 * @key_usage: an ORed sequence of %GNUTLS_KEY_* flags.
 * @names: is an array of DNS names belonging to the public-key (NULL if none).
 * @names_length: holds the length of the names list.
 * @privkey_flags: an ORed sequence of #gnutls_pkcs_encrypt_flags_t.
 *   These apply to the private key pkey.
 * @pkcs11_flags: one of gnutls_pkcs11_obj_flags. These apply to URLs.
 *
 * This function sets a public/private keypair read from file in the
 * #gnutls_certificate_credentials_t type to be used for authentication
 * and/or encryption. @spki and @privkey should match otherwise set
 * signatures cannot be validated. In case of no match this function
 * returns %GNUTLS_E_CERTIFICATE_KEY_MISMATCH. This function should
 * be called once for the client because there is currently no mechanism
 * to determine which raw public-key to select for the peer when there
 * are multiple present. Multiple raw public keys for the server can be
 * distinghuished by setting the @names.
 *
 * Note here that @spki is a raw public-key as defined
 * in RFC7250. It means that there is no surrounding certificate that
 * holds the public key and that there is therefore no direct mechanism
 * to prove the authenticity of this key. The keypair can be used during
 * a TLS handshake but its authenticity should be established via a
 * different mechanism (e.g. TOFU or known fingerprint).
 *
 * The supported formats are basic unencrypted key, PKCS8, PKCS12,
 * and the openssl format and will be autodetected.
 *
 * If the raw public-key and the private key are given in PEM encoding
 * then the strings that hold their values must be null terminated.
 *
 * Key usage (as defined by X.509 extension (2.5.29.15)) can be explicitly
 * set because there is no certificate structure around the key to define
 * this value. See for more info gnutls_x509_crt_get_key_usage().
 *
 * Note that, this function by default returns zero on success and a
 * negative value on error. Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2
 * is set using gnutls_certificate_set_flags() it returns an index
 * (greater or equal to zero). That index can be used in other functions
 * to refer to the added key-pair.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, in case the
 *   key pair does not match %GNUTLS_E_CERTIFICATE_KEY_MISMATCH is returned,
 *   in other erroneous cases a different negative error code is returned.
 *
 * Since: 3.6.6
 */
int gnutls_certificate_set_rawpk_key_file(gnutls_certificate_credentials_t cred,
				      const char* rawpkfile,
				      const char* privkeyfile,
				      gnutls_x509_crt_fmt_t format,
				      const char *pass,
				      unsigned int key_usage,
				      const char **names,
				      unsigned int names_length,
				      unsigned int privkey_flags,
				      unsigned int pkcs11_flags)
{
	int ret;
	gnutls_privkey_t privkey;
	gnutls_pubkey_t pubkey;
	gnutls_pcert_st* pcert;
	gnutls_datum_t rawpubkey = { NULL, 0 }; // to hold rawpk data from file
	size_t key_size;
	gnutls_str_array_t str_names;
	unsigned int i;

	if (rawpkfile == NULL || privkeyfile == NULL) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	/* Import our private key. This function does all the necessary
	 * inits, checks and imports. */
	ret = _gnutls_read_key_file(cred, privkeyfile, format, pass, privkey_flags, &privkey);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	pcert = gnutls_calloc(1, sizeof(*pcert));
	if (pcert == NULL) {
		gnutls_privkey_deinit(privkey);

		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	/* Check whether we are importing our raw public-key from a URL
	 * or from a regular file.
	 */
	if (gnutls_url_is_supported(rawpkfile)) {

		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0) {
			gnutls_privkey_deinit(privkey);

			return gnutls_assert_val(ret);
		}

		ret = gnutls_pubkey_import_url(pubkey, rawpkfile, pkcs11_flags);
		if (ret < 0) {
			gnutls_privkey_deinit(privkey);
			gnutls_pubkey_deinit(pubkey);

			return gnutls_assert_val(ret);
		}

		ret = gnutls_pcert_import_rawpk(pcert, pubkey, 0);
		if (ret < 0) {
			gnutls_privkey_deinit(privkey);
			gnutls_pubkey_deinit(pubkey);

			return gnutls_assert_val(ret);
		}

	} else {
		/* Read our raw public-key into memory from file */
		rawpubkey.data = (void*) read_binary_file(rawpkfile, &key_size);
		if (rawpubkey.data == NULL) {
			gnutls_privkey_deinit(privkey);

			return gnutls_assert_val(GNUTLS_E_FILE_ERROR);
		}
		rawpubkey.size = key_size; // Implicit type casting

		/* We now convert our raw public key that we've loaded into memory to
		 * a parsed certificate (pcert) structure. Note that rawpubkey will
		 * be copied into pcert. Therefore we can directly cleanup rawpubkey.
		 */
		ret = gnutls_pcert_import_rawpk_raw(pcert, &rawpubkey,
						format, key_usage, 0);

		_gnutls_free_datum(&rawpubkey);

		if (ret < 0) {
			gnutls_privkey_deinit(privkey);

			return gnutls_assert_val(ret);
		}

	}

	/* Process the names, if any */
	_gnutls_str_array_init(&str_names);

	if (names != NULL && names_length > 0) {
		for (i = 0; i < names_length; i++) {
			ret =
			    _gnutls_str_array_append_idna(&str_names, names[i],
						     strlen(names[i]));
			if (ret < 0) {
				gnutls_privkey_deinit(privkey);
				_gnutls_str_array_clear(&str_names);

				return gnutls_assert_val(ret);
			}
		}
	}

	/* Now that we have converted the key material to our internal structures
	 * we can now add them to the credentials structure */
	ret = _gnutls_certificate_credential_append_keypair(cred, privkey, str_names, pcert, 1);
	if (ret < 0) {
		gnutls_privkey_deinit(privkey);
		gnutls_pcert_deinit(pcert);
		gnutls_free(pcert);

		return gnutls_assert_val(ret);
	}
	// Successfully added a certificate
	cred->ncerts++;

	/* Check whether the key pair matches.
	 * After this point we do not deinitialize anything on failure to avoid
	 * double freeing. We intentionally keep everything as the credentials state
	 * is documented to be in undefined state. */
	if ((ret = _gnutls_check_key_cert_match(cred)) < 0) {
		return gnutls_assert_val(ret);
	}

	CRED_RET_SUCCESS(cred);
}

