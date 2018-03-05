/*
 * Copyright (C) 2009-2013 Free Software Foundation, Inc.
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
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
#include <cipher_int.h>
#include <datum.h>
#include <gnutls/crypto.h>
#include <crypto.h>
#include <fips.h>
#include <algorithms.h>

#define SR_FB(x, cleanup) ret=(x); if ( ret<0 ) { \
  if (ret == GNUTLS_E_NEED_FALLBACK) { \
    if (handle->handle) \
	handle->deinit(handle->handle); \
    goto fallback; \
  } \
  gnutls_assert(); \
  ret = GNUTLS_E_INTERNAL_ERROR; \
  goto cleanup; \
  }

#define SR(x, cleanup) if ( (x)<0 ) { \
  gnutls_assert(); \
  ret = GNUTLS_E_INTERNAL_ERROR; \
  goto cleanup; \
  }

/* Returns true(non-zero) or false(0) if the 
 * provided cipher exists
 */
int _gnutls_cipher_exists(gnutls_cipher_algorithm_t cipher)
{
	const gnutls_crypto_cipher_st *cc;
	int ret;

	/* All the other ciphers are disabled on the back-end library.
	 * The NULL needs to be detected here as it is not a cipher
	 * that is provided by the back-end.
	 */
	if (cipher == GNUTLS_CIPHER_NULL) {
		if (_gnutls_fips_mode_enabled() == 0)
			return 1;
		else
			return 0;
	}

	cc = _gnutls_get_crypto_cipher(cipher);
	if (cc != NULL)
		return 1;

	ret = _gnutls_cipher_ops.exists(cipher);
	return ret;
}

int
_gnutls_cipher_init(cipher_hd_st *handle, const cipher_entry_st *e,
		    const gnutls_datum_t *key, const gnutls_datum_t *iv,
		    int enc)
{
	int ret = GNUTLS_E_INTERNAL_ERROR;
	const gnutls_crypto_cipher_st *cc = NULL;

	if (unlikely(e == NULL || e->id == GNUTLS_CIPHER_NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	FAIL_IF_LIB_ERROR;

	handle->e = e;
	handle->handle = NULL;

	/* check if a cipher has been registered
	 */
	cc = _gnutls_get_crypto_cipher(e->id);
	if (cc != NULL) {
		handle->encrypt = cc->encrypt;
		handle->decrypt = cc->decrypt;
		handle->aead_encrypt = cc->aead_encrypt;
		handle->aead_decrypt = cc->aead_decrypt;
		handle->deinit = cc->deinit;
		handle->auth = cc->auth;
		handle->tag = cc->tag;
		handle->setiv = cc->setiv;

		/* if cc->init() returns GNUTLS_E_NEED_FALLBACK we
		 * use the default ciphers */
		SR_FB(cc->init(e->id, &handle->handle, enc), cc_cleanup);
		SR_FB(cc->setkey(handle->handle, key->data, key->size),
		   cc_cleanup);
		if (iv) {
			if (unlikely(cc->setiv == NULL)) /* the API doesn't accept IV */
				return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
			SR(cc->setiv(handle->handle, iv->data, iv->size),
			   cc_cleanup);
		}

		return 0;
	}

 fallback:
	handle->encrypt = _gnutls_cipher_ops.encrypt;
	handle->decrypt = _gnutls_cipher_ops.decrypt;
	handle->aead_encrypt = _gnutls_cipher_ops.aead_encrypt;
	handle->aead_decrypt = _gnutls_cipher_ops.aead_decrypt;
	handle->deinit = _gnutls_cipher_ops.deinit;
	handle->auth = _gnutls_cipher_ops.auth;
	handle->tag = _gnutls_cipher_ops.tag;
	handle->setiv = _gnutls_cipher_ops.setiv;

	/* otherwise use generic cipher interface
	 */
	ret = _gnutls_cipher_ops.init(e->id, &handle->handle, enc);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_cipher_ops.setkey(handle->handle, key->data,
				      key->size);
	if (ret < 0) {
		gnutls_assert();
		goto cc_cleanup;
	}

	if (iv) {
		ret =
		    _gnutls_cipher_ops.setiv(handle->handle, iv->data,
					     iv->size);
		if (ret < 0) {
			gnutls_assert();
			goto cc_cleanup;
		}
	}

	return 0;

      cc_cleanup:

	if (handle->handle)
		handle->deinit(handle->handle);

	return ret;
}

/* Auth_cipher API 
 */
int _gnutls_auth_cipher_init(auth_cipher_hd_st * handle,
			     const cipher_entry_st * e,
			     const gnutls_datum_t * cipher_key,
			     const gnutls_datum_t * iv,
			     const mac_entry_st * me,
			     const gnutls_datum_t * mac_key,
			     unsigned etm,
#ifdef ENABLE_SSL3
			     unsigned ssl_hmac,
#endif
			     int enc)
{
	int ret;

	if (unlikely(e == NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	FAIL_IF_LIB_ERROR;

	memset(handle, 0, sizeof(*handle));
	handle->etm = etm;

	if (e->id != GNUTLS_CIPHER_NULL) {
		handle->non_null = 1;
		ret =
		    _gnutls_cipher_init(&handle->cipher, e, cipher_key, iv,
					enc);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else
		handle->non_null = 0;

	if (me->id != GNUTLS_MAC_AEAD) {
		handle->is_mac = 1;
#ifdef ENABLE_SSL3
		handle->ssl_hmac = ssl_hmac;

		if (ssl_hmac)
			ret =
			    _gnutls_mac_init_ssl3(&handle->mac.dig, me,
						  mac_key->data,
						  mac_key->size);
		else
#endif
			ret =
			    _gnutls_mac_init(&handle->mac.mac, me,
					     mac_key->data, mac_key->size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		handle->tag_size = _gnutls_mac_get_algo_len(me);
	} else if (_gnutls_cipher_algo_is_aead(e)) {
		handle->tag_size = _gnutls_cipher_get_tag_size(e);
	} else {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	return 0;
      cleanup:
	if (handle->non_null != 0)
		_gnutls_cipher_deinit(&handle->cipher);
	return ret;

}

#ifdef ENABLE_SSL3
# define MAC(handle, text, textlen) \
		if (handle->ssl_hmac) { \
			ret = \
			    _gnutls_hash(&handle->mac.dig, text, textlen); \
		} else { \
			ret = _gnutls_mac(&handle->mac.mac, text, textlen); \
		} \
		if (unlikely(ret < 0)) \
			return gnutls_assert_val(ret)
#else
# define MAC(handle, text, textlen) \
		ret = _gnutls_mac(&handle->mac.mac, text, textlen); \
		if (unlikely(ret < 0)) \
			return gnutls_assert_val(ret)
#endif

int _gnutls_auth_cipher_add_auth(auth_cipher_hd_st * handle,
				 const void *text, int textlen)
{
	int ret;

	if (handle->is_mac) {
		MAC(handle, text, textlen);
	} else if (_gnutls_cipher_is_aead(&handle->cipher))
		return _gnutls_cipher_auth(&handle->cipher, text, textlen);
	return 0;
}


/* The caller must make sure that textlen+pad_size+tag_size is divided by the block size of the cipher */
int _gnutls_auth_cipher_encrypt2_tag(auth_cipher_hd_st * handle,
				     const uint8_t * text, int textlen,
				     void *_ciphertext, int ciphertextlen,
				     int pad_size)
{
	int ret;
	uint8_t *ciphertext = _ciphertext;
	unsigned blocksize =
	    _gnutls_cipher_get_block_size(handle->cipher.e);
	unsigned l;

	if (handle->is_mac) { /* cipher + mac */
		if (handle->non_null == 0) { /* NULL cipher + MAC */
			MAC(handle, text, textlen);

			if (unlikely(textlen + pad_size + handle->tag_size) >
			    ciphertextlen)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			if (text != ciphertext)
				memcpy(ciphertext, text, textlen);
			ret =
			    _gnutls_auth_cipher_tag(handle,
						    ciphertext + textlen,
						    handle->tag_size);
			if (ret < 0)
				return gnutls_assert_val(ret);

		} else {
			uint8_t *orig_ciphertext = ciphertext;

			if (handle->etm == 0 || handle->cipher.e->type != CIPHER_BLOCK) {
				MAC(handle, text, textlen);
			}

			if (unlikely(textlen + pad_size + handle->tag_size) >
			    ciphertextlen)
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			l = (textlen / blocksize) * blocksize;
			if (l > 0) {
				ret =
					_gnutls_cipher_encrypt2(&handle->cipher, text,
							    l, ciphertext,
							    ciphertextlen);
				if (ret < 0)
					return gnutls_assert_val(ret);

				textlen -= l;
				text += l;
				ciphertext += l;
				ciphertextlen -= l;
			}

			if (ciphertext != text && textlen > 0)
				memcpy(ciphertext, text, textlen);

			if (handle->etm == 0 || handle->cipher.e->type != CIPHER_BLOCK) {
				ret =
				    _gnutls_auth_cipher_tag(handle,
							    ciphertext + textlen,
							    handle->tag_size);
				if (ret < 0)
					return gnutls_assert_val(ret);
				textlen += handle->tag_size;
			}

			/* TLS 1.0 style padding */
			if (pad_size > 0) {
				memset(ciphertext + textlen, pad_size - 1,
				       pad_size);
				textlen += pad_size;
			}

			ret =
			    _gnutls_cipher_encrypt2(&handle->cipher,
						    ciphertext, textlen,
						    ciphertext,
						    ciphertextlen);
			if (ret < 0)
				return gnutls_assert_val(ret);

			if (handle->etm != 0 && handle->cipher.e->type == CIPHER_BLOCK) {
				MAC(handle, orig_ciphertext, l);
				MAC(handle, ciphertext, textlen);

				ret =
					_gnutls_auth_cipher_tag(handle,
							ciphertext + textlen,
							handle->tag_size);
				if (ret < 0)
					return gnutls_assert_val(ret);
			}
		}
	} else if (_gnutls_cipher_is_aead(&handle->cipher)) {
		ret =
		    _gnutls_cipher_encrypt2(&handle->cipher, text, textlen,
					    ciphertext, ciphertextlen);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);

		ret =
		    _gnutls_auth_cipher_tag(handle, ciphertext + textlen,
					    handle->tag_size);
		if (unlikely(ret < 0))
			return gnutls_assert_val(ret);
	} else if (handle->non_null == 0 && text != ciphertext) /* NULL cipher - no MAC */
		memcpy(ciphertext, text, textlen);

	return 0;
}

int _gnutls_auth_cipher_decrypt2(auth_cipher_hd_st * handle,
				 const void *ciphertext, int ciphertextlen,
				 void *text, int textlen)
{
	int ret;

	if (unlikely(ciphertextlen > textlen))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (handle->is_mac && (handle->etm != 0 && handle->cipher.e->type == CIPHER_BLOCK)) {
		/* The MAC is not to be hashed */
		ciphertextlen -= handle->tag_size;

		MAC(handle, ciphertext, ciphertextlen);
	}

	if (handle->non_null != 0) {
		ret =
		    _gnutls_cipher_decrypt2(&handle->cipher, ciphertext,
					    ciphertextlen, text, textlen);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else if (handle->non_null == 0 && text != ciphertext)
		memcpy(text, ciphertext, ciphertextlen);

	if (handle->is_mac && (handle->etm == 0 || handle->cipher.e->type != CIPHER_BLOCK)) {
		/* The MAC is not to be hashed */
		ciphertextlen -= handle->tag_size;

		MAC(handle, text, ciphertextlen);
	}

	return 0;
}

int _gnutls_auth_cipher_tag(auth_cipher_hd_st * handle, void *tag,
			    int tag_size)
{
	if (handle->is_mac) {
#ifdef ENABLE_SSL3
		int ret;

		if (handle->ssl_hmac) {
			ret =
			    _gnutls_mac_output_ssl3(&handle->mac.dig, tag);
			if (ret < 0)
				return gnutls_assert_val(ret);
		} else
#endif
			_gnutls_mac_output(&handle->mac.mac, tag);
	} else if (_gnutls_cipher_is_aead(&handle->cipher)) {
		_gnutls_cipher_tag(&handle->cipher, tag, tag_size);
	} else
		memset(tag, 0, tag_size);

	return 0;
}

void _gnutls_auth_cipher_deinit(auth_cipher_hd_st * handle)
{
	if (handle->is_mac) {
#ifdef ENABLE_SSL3
		if (handle->ssl_hmac)	/* failure here doesn't matter */
			_gnutls_mac_deinit_ssl3(&handle->mac.dig, NULL);
		else
#endif
			_gnutls_mac_deinit(&handle->mac.mac, NULL);
	}
	if (handle->non_null != 0)
		_gnutls_cipher_deinit(&handle->cipher);
}
