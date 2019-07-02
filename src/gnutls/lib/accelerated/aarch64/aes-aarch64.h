#ifndef GNUTLS_LIB_ACCELERATED_AARCH64_AES_AARCH64_H
#define GNUTLS_LIB_ACCELERATED_AARCH64_AES_AARCH64_H

#include "gnutls_int.h"

#define ALIGN16(x) \
        ((void *)(((ptrdiff_t)(x)+(ptrdiff_t)0x0f)&~((ptrdiff_t)0x0f)))

#define AES_KEY_ALIGN_SIZE 4
#define AES_MAXNR 14
typedef struct {
	/* We add few more integers to allow alignment 
	 * on a 16-byte boundary.
	 */
	uint32_t rd_key[4 * (AES_MAXNR + 1) + AES_KEY_ALIGN_SIZE];
	uint32_t rounds;
} AES_KEY;

#define CHECK_AES_KEYSIZE(s) \
	if (s != 16 && s != 24 && s != 32) \
		return GNUTLS_E_INVALID_REQUEST

int aes_v8_set_encrypt_key(const unsigned char *userKey, int bits, AES_KEY *key);  
int aes_v8_set_decrypt_key(const unsigned char *userKey, int bits, AES_KEY *key);
void aes_v8_cbc_encrypt(const unsigned char *in, unsigned char *out,
                       size_t length, const AES_KEY *key, unsigned char *ivec, int enc);
void aes_v8_encrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);
void aes_v8_decrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);

extern const gnutls_crypto_cipher_st _gnutls_aes_gcm_aarch64;
extern const gnutls_crypto_cipher_st _gnutls_aes_cbc_aarch64;
extern const gnutls_crypto_cipher_st _gnutls_aes_ccm_aarch64;

#endif /* GNUTLS_LIB_ACCELERATED_AARCH64_AES_AARCH64_H */
