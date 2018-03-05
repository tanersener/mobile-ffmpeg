#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/self-test.h>

#if !defined(HAVE_LIBNETTLE) || defined(WIN32)
int main(int argc, char **argv)
{
	exit(77);
}
#else

# include <nettle/sha1.h>
# include <nettle/sha2.h>
# include <nettle/hmac.h>
# include <nettle/macros.h>

/* this tests whether the API to override ciphers works sanely.
 */
static int used = 0;
static int used_mac = 0;
static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#ifndef ENABLE_SELF_CHECKS
# define AVOID_INTERNALS
# include "../../lib/crypto-selftests.c"
#endif

struct myhash_ctx {
	struct sha1_ctx sha1;
};

static int myhash_init(gnutls_digest_algorithm_t algo,
		     void **_ctx)
{
	struct myhash_ctx *ctx;
	ctx = malloc(sizeof(struct myhash_ctx));
	if (ctx == NULL) {
		return GNUTLS_E_MEMORY_ERROR;
	}

	sha1_init(&ctx->sha1);
	*_ctx = ctx;

	return 0;
}

static void myhash_deinit(void *ctx)
{
	free(ctx);
}

static int
myhash_output(void *src_ctx, void *digest, size_t digestsize)
{
	struct myhash_ctx *ctx;
	ctx = src_ctx;

	used = 1;
	sha1_digest(&ctx->sha1, digestsize, digest);
	return 0;
}

static
int myhash_fast(gnutls_digest_algorithm_t algo,
			   const void *text, size_t text_size,
			   void *digest)
{
	struct sha1_ctx ctx;

	if (algo != GNUTLS_DIG_SHA1)
		return -1;

	used = 1;
	sha1_init(&ctx);
	sha1_update(&ctx, text_size, text);
	sha1_digest(&ctx, 20, digest);
	return 0;
}

static int
myhash_update(void *_ctx, const void * data, size_t length)
{
	struct myhash_ctx *ctx = _ctx;

	sha1_update(&ctx->sha1, length, data);
	return 0;
}

/* MAC */
struct mymac_ctx {
	struct hmac_sha256_ctx sha256;
};

static int mymac_init(gnutls_mac_algorithm_t algo,
		     void **ctx)
{
	*ctx = malloc(sizeof(struct mymac_ctx));
	if (*ctx == NULL) {
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

static void mymac_deinit(void *ctx)
{
	free(ctx);
}

static int
mymac_output(void *src_ctx, void *digest, size_t digestsize)
{
	struct mymac_ctx *ctx;
	ctx = src_ctx;

	hmac_sha256_digest(&ctx->sha256, digestsize, digest);
	return 0;
}

static
int mymac_fast(gnutls_mac_algorithm_t algo,
	       const void *nonce, size_t nonce_size, const void *key, size_t keysize,
	       const void *text, size_t text_size,
	       void *digest)
{
	struct hmac_sha256_ctx ctx;

	if (algo != GNUTLS_MAC_SHA256)
		return -1;

	used_mac = 1;
	hmac_sha256_set_key(&ctx, keysize, key);
	hmac_sha256_update(&ctx, text_size, text);
	hmac_sha256_digest(&ctx, 32, digest);
	return 0;
}

static int
mymac_update(void *_ctx, const void * data, size_t length)
{
	struct mymac_ctx *ctx;
	ctx = _ctx;

	used_mac = 1;
	hmac_sha256_update(&ctx->sha256, length, data);
	return 0;
}

static int
mymac_setkey(void *_ctx, const void * key, size_t length)
{
	struct mymac_ctx *ctx;
	ctx = _ctx;

	hmac_sha256_set_key(&ctx->sha256, length, key);
	return 0;
}


int main(int argc, char **argv)
{
	int ret;

	gnutls_global_set_log_function(tls_log_func);
	if (argc > 1)
		gnutls_global_set_log_level(4711);

	ret = gnutls_crypto_register_digest(GNUTLS_DIG_SHA1, 1,
		myhash_init,
		myhash_update,
		myhash_output,
		myhash_deinit,
		myhash_fast);
	if (ret < 0) {
		fprintf(stderr, "%d: cannot register hash\n", __LINE__);
		exit(1);
	}

	ret = gnutls_crypto_register_mac(GNUTLS_MAC_SHA256, 1,
		mymac_init,
		mymac_setkey,
		NULL,
		mymac_update,
		mymac_output,
		mymac_deinit,
		mymac_fast);
	if (ret < 0) {
		fprintf(stderr, "%d: cannot register hash\n", __LINE__);
		exit(1);
	}

	global_init();

	if (gnutls_digest_self_test(0, GNUTLS_DIG_SHA1) < 0)
		return 1;

	if (used == 0) {
		fprintf(stderr, "The hash algorithm was not used\n");
		exit(1);
	}

	if (gnutls_mac_self_test(0, GNUTLS_MAC_SHA256) < 0)
		return 1;

	if (used_mac == 0) {
		fprintf(stderr, "The MAC algorithm was not used\n");
		exit(1);
	}

	gnutls_global_deinit();
	return 0;
}

#endif
