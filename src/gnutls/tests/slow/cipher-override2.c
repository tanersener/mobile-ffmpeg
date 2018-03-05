#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/self-test.h>

/* this tests whether the API to override ciphers works sanely,
 * when GNUTLS_E_NEED_FALLBACK is used.
 */
static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#ifndef ENABLE_SELF_CHECKS
# define AVOID_INTERNALS
# include "../../lib/crypto-selftests.c"
#endif

struct myaes_ctx {
	unsigned char iv[16];
};

static int
myaes_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	return GNUTLS_E_NEED_FALLBACK;
}

static int
myaes_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	abort();
}

static int myaes_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	abort();
}

static int
myaes_encrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	abort();
}

static int
myaes_decrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	abort();
}

static void myaes_deinit(void *_ctx)
{
	abort();
}

/* AES-GCM */
struct myaes_gcm_ctx {
	char xx[32];
};

static int
myaes_gcm_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	return GNUTLS_E_NEED_FALLBACK;
}

static int
myaes_gcm_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	abort();
}

static void myaes_gcm_deinit(void *_ctx)
{
	abort();
}

static int
myaes_gcm_encrypt(void *_ctx,
		  const void *nonce, size_t nonce_size,
		  const void *auth, size_t auth_size,
		  size_t tag_size,
		  const void *plain, size_t plain_size,
		  void *encr, size_t encr_size)
{
	abort();
}

static int
myaes_gcm_decrypt(void *_ctx,
		  const void *nonce, size_t nonce_size,
		  const void *auth, size_t auth_size,
		  size_t tag_size,
		  const void *encr, size_t encr_size,
		  void *plain, size_t plain_size)
{
	abort();
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

	if (gnutls_cipher_self_test(1, 0) < 0)
		return 1;

	gnutls_global_deinit();
	return 0;
}
