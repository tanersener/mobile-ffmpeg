#ifndef GNUTLS_LIB_ACCELERATED_X86_SHA_PADLOCK_H
#define GNUTLS_LIB_ACCELERATED_X86_SHA_PADLOCK_H

#include <nettle/sha.h>

void padlock_sha1_oneshot(void *ctx, const void *inp, size_t len);
void padlock_sha256_oneshot(void *ctx, const void *inp, size_t len);

void padlock_sha1_blocks(unsigned int *ctx, const void *inp,
			 size_t blocks);
void padlock_sha256_blocks(unsigned int *ctx, const void *inp,
			   size_t blocks);
void padlock_sha512_blocks(unsigned int *ctx, const void *inp,
			   size_t blocks);

int wrap_padlock_hash_fast(gnutls_digest_algorithm_t algo,
			   const void *text, size_t text_size,
			   void *digest);

void padlock_sha1_update(struct sha1_ctx *ctx,
			 size_t length, const uint8_t * data);
void padlock_sha256_update(struct sha256_ctx *ctx,
			   size_t length, const uint8_t * data);
void padlock_sha512_update(struct sha512_ctx *ctx,
			   size_t length, const uint8_t * data);

extern const struct nettle_hash padlock_sha1;
extern const struct nettle_hash padlock_sha224;
extern const struct nettle_hash padlock_sha256;
extern const struct nettle_hash padlock_sha384;
extern const struct nettle_hash padlock_sha512;

extern const gnutls_crypto_mac_st _gnutls_hmac_sha_padlock_nano;
extern const gnutls_crypto_digest_st _gnutls_sha_padlock_nano;

#endif /* GNUTLS_LIB_ACCELERATED_X86_SHA_PADLOCK_H */
