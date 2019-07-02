#ifndef GNUTLS_LIB_ACCELERATED_X86_AES_PADLOCK_H
#define GNUTLS_LIB_ACCELERATED_X86_AES_PADLOCK_H

#include "gnutls_int.h"
#include <aes-x86.h>

struct padlock_cipher_data {
	unsigned char iv[16];	/* Initialization vector */
	union {
		unsigned int pad[4];
		struct {
			unsigned rounds:4;
			unsigned dgst:1;	/* n/a in C3 */
			unsigned align:1;	/* n/a in C3 */
			unsigned ciphr:1;	/* n/a in C3 */
			unsigned int keygen:1;
			unsigned interm:1;
			unsigned int encdec:1;
			unsigned ksize:2;
		} b;
	} cword;		/* Control word */
	AES_KEY ks;		/* Encryption key */
};

struct padlock_ctx {
	struct padlock_cipher_data expanded_key;
	int enc;
};

extern const gnutls_crypto_cipher_st _gnutls_aes_padlock;
extern const gnutls_crypto_cipher_st _gnutls_aes_gcm_padlock;

extern const gnutls_crypto_mac_st _gnutls_hmac_sha_padlock;
extern const gnutls_crypto_digest_st _gnutls_sha_padlock;

int padlock_aes_cipher_setkey(void *_ctx, const void *userkey,
			      size_t keysize);

/* asm */
unsigned int padlock_capability(void);
void padlock_reload_key(void);
int padlock_ecb_encrypt(void *out, const void *inp,
			struct padlock_cipher_data *ctx, size_t len);
int padlock_cbc_encrypt(void *out, const void *inp,
			struct padlock_cipher_data *ctx, size_t len);
#endif /* GNUTLS_LIB_ACCELERATED_X86_AES_PADLOCK_H */
