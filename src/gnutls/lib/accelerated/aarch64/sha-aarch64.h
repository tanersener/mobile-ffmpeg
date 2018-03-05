#ifndef SHA_ARM_H
#define SHA_ARM_H

#include <nettle/sha.h>

extern const struct nettle_hash aarch64_sha1;
extern const struct nettle_hash aarch64_sha224;
extern const struct nettle_hash aarch64_sha256;
extern const struct nettle_hash aarch64_sha384;
extern const struct nettle_hash aarch64_sha512;

extern const gnutls_crypto_digest_st _gnutls_sha_aarch64;
extern const gnutls_crypto_mac_st _gnutls_hmac_sha_aarch64;

void aarch64_sha1_update(struct sha1_ctx *ctx, size_t length, const uint8_t * data);
void aarch64_sha256_update(struct sha256_ctx *ctx, size_t length, const uint8_t * data);
void aarch64_sha512_update(struct sha512_ctx *ctx, size_t length, const uint8_t * data);

#endif
