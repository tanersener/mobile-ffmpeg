#ifndef SHA_X86_H
#define SHA_X86_H

#include <nettle/sha.h>

/* nettle's SHA512 is faster than openssl's */
#undef ENABLE_SHA512

extern const struct nettle_hash x86_sha1;
extern const struct nettle_hash x86_sha224;
extern const struct nettle_hash x86_sha256;
extern const struct nettle_hash x86_sha384;
extern const struct nettle_hash x86_sha512;

void x86_sha1_update(struct sha1_ctx *ctx, size_t length, const uint8_t * data);
void x86_sha256_update(struct sha256_ctx *ctx, size_t length, const uint8_t * data);
void x86_sha512_update(struct sha512_ctx *ctx, size_t length, const uint8_t * data);

extern const gnutls_crypto_digest_st _gnutls_sha_x86_ssse3;
extern const gnutls_crypto_mac_st _gnutls_hmac_sha_x86_ssse3;

#endif
