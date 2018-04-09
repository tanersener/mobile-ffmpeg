#ifndef CRYPTO_H_
#define CRYPTO_H_

#include "global.h"
#include "../cfg.h"

#include <stdio.h>
#include <math.h>

#ifdef KVZ_SEL_ENCRYPTION
#define STUBBED extern
#else
#define STUBBED static
#endif

#define AESEncryptionStreamMode 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct crypto_handle_t crypto_handle_t;

STUBBED crypto_handle_t* kvz_crypto_create(const kvz_config *cfg);
STUBBED void kvz_crypto_decrypt(crypto_handle_t* hdl,
                                const uint8_t *in_stream,
                                int size_bits,
                                uint8_t *out_stream);
STUBBED void kvz_crypto_delete(crypto_handle_t **hdl);

#if AESEncryptionStreamMode
STUBBED unsigned kvz_crypto_get_key(crypto_handle_t *hdl, int num_bits);
#endif

#ifdef __cplusplus
}
#endif

#undef STUBBED


#ifndef KVZ_SEL_ENCRYPTION
// Provide static stubs to allow linking without libcryptopp and allows us
// to avoid sprinkling ifdefs everywhere and having a bunch of code that's
// not compiled during normal development.
// Provide them in the header so we can avoid compiling the cpp file, which
// means we don't need a C++ compiler when crypto is not enabled.

static INLINE crypto_handle_t* kvz_crypto_create(const kvz_config *cfg)
{
  return NULL;
}

static INLINE void kvz_crypto_decrypt(crypto_handle_t* hdl,
                                const uint8_t *in_stream,
                                int size_bits,
                                uint8_t *out_stream)
{}

static INLINE void kvz_crypto_delete(crypto_handle_t **hdl)
{}

#if AESEncryptionStreamMode
static INLINE unsigned kvz_crypto_get_key(crypto_handle_t *hdl, int num_bits)
{
  return 0;
}
#endif

#endif // KVZ_SEL_ENCRYPTION

#endif // CRYPTO_H_
