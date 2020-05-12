#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/self-test.h>

#ifndef HAVE_LIBNETTLE
int main(int argc, char **argv)
{
	exit(77);
}
#else

# include <nettle/aes.h>
# include <nettle/cbc.h>
# include <nettle/gcm.h>
# include <assert.h>

/* this tests whether the API to override ciphers works sanely.
 */
static int used = 0;
static int aead_used = 0;
static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

struct myaes_ctx {
	struct aes128_ctx aes;
	unsigned char iv[16];
	int enc;
};

static int
myaes_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_CBC)
		return GNUTLS_E_INVALID_REQUEST;

	*_ctx = calloc(1, sizeof(struct myaes_ctx));
	if (*_ctx == NULL) {
		return GNUTLS_E_MEMORY_ERROR;
	}

	((struct myaes_ctx *) (*_ctx))->enc = enc;

	return 0;
}

static int
myaes_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct myaes_ctx *ctx = _ctx;

	assert(keysize == 16);

	if (ctx->enc)
		aes128_set_encrypt_key(&ctx->aes, userkey);
	else
		aes128_set_decrypt_key(&ctx->aes, userkey);

	return 0;
}

static int myaes_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct myaes_ctx *ctx = _ctx;

	memcpy(ctx->iv, iv, 16);
	return 0;
}

static int
myaes_encrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct myaes_ctx *ctx = _ctx;

	used++;
	cbc_encrypt(&ctx->aes, (nettle_cipher_func*)aes128_encrypt, 16, ctx->iv, src_size, dst, src);
	return 0;
}

static int
myaes_decrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct myaes_ctx *ctx = _ctx;

	used++;
	cbc_decrypt(&ctx->aes, (nettle_cipher_func*)aes128_decrypt, 16, ctx->iv, src_size, dst, src);

	return 0;
}

static void myaes_deinit(void *_ctx)
{
	free(_ctx);
}

/* AES-GCM */
struct myaes_gcm_ctx {
	struct gcm_aes128_ctx aes;
};

static int
myaes_gcm_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_GCM
	    && algorithm != GNUTLS_CIPHER_AES_256_GCM)
		return GNUTLS_E_INVALID_REQUEST;

	*_ctx = calloc(1, sizeof(struct myaes_gcm_ctx));
	if (*_ctx == NULL) {
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

static int
myaes_gcm_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct myaes_gcm_ctx *ctx = _ctx;

	gcm_aes128_set_key(&ctx->aes, userkey);

	return 0;
}

static void myaes_gcm_deinit(void *_ctx)
{
	free(_ctx);
}

static int
myaes_gcm_encrypt(void *_ctx,
		  const void *nonce, size_t nonce_size,
		  const void *auth, size_t auth_size,
		  size_t tag_size,
		  const void *plain, size_t plain_size,
		  void *encr, size_t encr_size)
{
	/* proper AEAD cipher */
	struct myaes_gcm_ctx *ctx = _ctx;
	if (encr_size < plain_size + tag_size)
		return GNUTLS_E_SHORT_MEMORY_BUFFER;

	aead_used++;
	gcm_aes128_set_iv(&ctx->aes, nonce_size, nonce);
	gcm_aes128_update(&ctx->aes, auth_size, auth);

	gcm_aes128_encrypt(&ctx->aes, plain_size, encr, plain);

	gcm_aes128_digest(&ctx->aes, tag_size, ((uint8_t*)encr) + plain_size);
	return 0;
}

static int
myaes_gcm_decrypt(void *_ctx,
		  const void *nonce, size_t nonce_size,
		  const void *auth, size_t auth_size,
		  size_t tag_size,
		  const void *encr, size_t encr_size,
		  void *plain, size_t plain_size)
{
	uint8_t tag[16];
	struct myaes_gcm_ctx *ctx = _ctx;

	if (encr_size < tag_size)
		return GNUTLS_E_DECRYPTION_FAILED;

	aead_used++;
	gcm_aes128_set_iv(&ctx->aes, nonce_size, nonce);
	gcm_aes128_update(&ctx->aes, auth_size, auth);

	encr_size -= tag_size;
	gcm_aes128_decrypt(&ctx->aes, encr_size, plain, encr);

	gcm_aes128_digest(&ctx->aes, tag_size, tag);

	if (gnutls_memcmp(((uint8_t*)encr)+encr_size, tag, tag_size) != 0)
		return GNUTLS_E_DECRYPTION_FAILED;

	return 0;
}



int main(int argc, char **argv)
{
	int ret;

	gnutls_global_set_log_function(tls_log_func);
	if (argc > 1)
		gnutls_global_set_log_level(4711);

	ret = gnutls_crypto_register_cipher(GNUTLS_CIPHER_AES_128_CBC, 1,
		myaes_init,
		myaes_setkey,
		myaes_setiv,
		myaes_encrypt,
		myaes_decrypt,
		myaes_deinit);
	if (ret < 0) {
		fprintf(stderr, "%d: cannot register cipher\n", __LINE__);
		exit(1);
	}

	ret = gnutls_crypto_register_aead_cipher(GNUTLS_CIPHER_AES_128_GCM, 1,
		myaes_gcm_init,
		myaes_gcm_setkey,
		myaes_gcm_encrypt,
		myaes_gcm_decrypt,
		myaes_gcm_deinit);
	if (ret < 0) {
		fprintf(stderr, "%d: cannot register cipher\n", __LINE__);
		exit(1);
	}

	global_init();

	if (gnutls_cipher_self_test(GNUTLS_SELF_TEST_FLAG_ALL|GNUTLS_SELF_TEST_FLAG_NO_COMPAT, 0) < 0)
		return 1;

	if (used == 0) {
		fprintf(stderr, "The CBC cipher was not used\n");
		exit(1);
	}

	if (aead_used == 0) {
		fprintf(stderr, "The AEAD cipher was not used\n");
		exit(1);
	}

	gnutls_global_deinit();
	return 0;
}

#endif
