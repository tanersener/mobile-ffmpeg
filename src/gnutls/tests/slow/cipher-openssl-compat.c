#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <stdlib.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>

/* This does check the AES and CHACHA20 implementations for compatibility
 * with openssl.
 */

#define BSIZE (64*1024+12)
#define B2SIZE (1024+7)
static unsigned char buffer_auth[B2SIZE];
static unsigned char orig_plain_data[BSIZE];
static unsigned char enc_data[BSIZE + 32];	/* allow for tag */
static unsigned char dec_data[BSIZE];

static int cipher_test(const char *ocipher, gnutls_cipher_algorithm_t gcipher,
		       unsigned tag_size)
{
	int ret;
	gnutls_aead_cipher_hd_t hd;
	gnutls_datum_t dkey, dnonce;
	unsigned char key[32];
	unsigned char nonce[32];
	size_t enc_data_size, dec_data_size;
	int dec_data_size2;
	EVP_CIPHER_CTX *ctx;
	const EVP_CIPHER *evp_cipher;
	unsigned char tag[64];
	unsigned i;

	success("cipher: %s\n", ocipher);

	/* decrypt with openssl */
	evp_cipher = EVP_get_cipherbyname(ocipher);
	if (!evp_cipher) {
		/* XXX: fix version check later when LibreSSL fixes support for aes-ccm and chacha20-poly1305 */
#ifdef LIBRESSL_VERSION_NUMBER
		fprintf(stderr, "EVP_get_cipherbyname failed for %s\n", ocipher);
		return -1;
#else
		/* OpenSSL should always work! */
		fail("EVP_get_cipherbyname failed for %s\n", ocipher);
#endif
	}

	for (i = 0; i < 32; i++) {	/* try with multiple keys and nonces */
		assert(gnutls_rnd
		       (GNUTLS_RND_NONCE, orig_plain_data,
			sizeof(orig_plain_data)) >= 0);
		assert(gnutls_rnd
		       (GNUTLS_RND_NONCE, buffer_auth,
			sizeof(buffer_auth)) >= 0);
		assert(gnutls_rnd(GNUTLS_RND_NONCE, key, sizeof(key)) >= 0);
		assert(gnutls_rnd(GNUTLS_RND_NONCE, nonce, sizeof(nonce)) >= 0);

		dkey.data = (void *)key;
		dkey.size = gnutls_cipher_get_key_size(gcipher);
		assert(gnutls_aead_cipher_init(&hd, gcipher, &dkey) >= 0);

		dnonce.data = (void *)nonce;
		dnonce.size = gnutls_cipher_get_iv_size(gcipher);

		enc_data_size = sizeof(enc_data);
		assert(gnutls_aead_cipher_encrypt(hd, dnonce.data, dnonce.size,
						  buffer_auth,
						  sizeof(buffer_auth), tag_size,
						  orig_plain_data,
						  sizeof(orig_plain_data),
						  enc_data,
						  &enc_data_size) >= 0);

		if (debug)
			success("encrypted %d bytes, to %d\n",
				(int)sizeof(orig_plain_data),
				(int)enc_data_size);

		dec_data_size = sizeof(dec_data);
		ret = gnutls_aead_cipher_decrypt(hd, dnonce.data, dnonce.size,
						 buffer_auth,
						 sizeof(buffer_auth), tag_size,
						 enc_data, enc_data_size,
						 dec_data, &dec_data_size);
		if (ret < 0) {
			fail("error in gnutls_aead_cipher_decrypt for %s: %s\n",
			     ocipher, gnutls_strerror(ret));
		}

		if (dec_data_size != sizeof(orig_plain_data)
		    || memcmp(dec_data, orig_plain_data,
			      sizeof(orig_plain_data)) != 0) {
			fail("gnutls encrypt-decrypt failed (got: %d, expected: %d)\n", (int)dec_data_size, (int)sizeof(orig_plain_data));
		}

		gnutls_aead_cipher_deinit(hd);

		ctx = EVP_CIPHER_CTX_new();

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
		if (gcipher == GNUTLS_CIPHER_AES_128_CCM
		    || gcipher == GNUTLS_CIPHER_AES_256_CCM) {
			assert(EVP_CipherInit_ex(ctx, evp_cipher, 0, 0, 0, 0) >
			       0);

			assert(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, dnonce.size, 0)==1);
			assert(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, tag_size,
					    enc_data + enc_data_size - tag_size) == 1);

			assert(EVP_CipherInit_ex(ctx, 0, 0, key, nonce, 0) >
			       0);

			dec_data_size2 = sizeof(dec_data);
			/* Add plain size */
			assert(EVP_CipherUpdate
			       (ctx, NULL, &dec_data_size2, NULL,
				enc_data_size - tag_size) > 0);

			/* Add AAD */
			assert(EVP_CipherUpdate
			       (ctx, NULL, &dec_data_size2, buffer_auth,
				sizeof(buffer_auth)) > 0);

			/* Decrypt */
			assert(EVP_CipherUpdate
			       (ctx, dec_data, &dec_data_size2, enc_data,
				enc_data_size - tag_size) > 0);

			dec_data_size = dec_data_size2;
			dec_data_size2 = tag_size;

			if (dec_data_size != sizeof(orig_plain_data)
			    || memcmp(dec_data, orig_plain_data,
				      sizeof(orig_plain_data)) != 0) {
				fail("openssl decrypt failed for %s\n", ocipher);
			}
		} else
#endif
		{
			assert(EVP_CipherInit_ex(ctx, evp_cipher, NULL, key, nonce, 0) >
			       0);

			EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_size,
					    enc_data + enc_data_size - tag_size);

			dec_data_size2 = sizeof(dec_data);

			/* Add AAD */
			assert(EVP_CipherUpdate
			       (ctx, NULL, &dec_data_size2, buffer_auth,
				sizeof(buffer_auth)) > 0);

			/* Decrypt */
			assert(EVP_CipherUpdate
			       (ctx, dec_data, &dec_data_size2, enc_data,
				enc_data_size - tag_size) > 0);

			dec_data_size = dec_data_size2;
			dec_data_size2 = tag_size;
			assert(EVP_CipherFinal_ex(ctx, tag, &dec_data_size2) > 0);

			if (dec_data_size != sizeof(orig_plain_data)
			    || memcmp(dec_data, orig_plain_data,
				      sizeof(orig_plain_data)) != 0) {
				fail("openssl decrypt failed for %s\n", ocipher);
			}

		}
		EVP_CIPHER_CTX_free(ctx);

	}

	return 0;
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

void doit(void)
{
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	global_init();
	OpenSSL_add_all_algorithms();

	/* ciphers */
	cipher_test("aes-128-gcm", GNUTLS_CIPHER_AES_128_GCM, 16);
	cipher_test("aes-256-gcm", GNUTLS_CIPHER_AES_256_GCM, 16);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	if (!gnutls_fips140_mode_enabled()) {
		cipher_test("chacha20-poly1305", GNUTLS_CIPHER_CHACHA20_POLY1305, 16);
	}
	cipher_test("aes-128-ccm", GNUTLS_CIPHER_AES_128_CCM, 16);
	cipher_test("aes-256-ccm", GNUTLS_CIPHER_AES_256_CCM, 16);
#endif

	gnutls_global_deinit();
	return;
}
