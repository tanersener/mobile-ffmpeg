#include <extras/crypto.h>

#ifndef KVZ_SEL_ENCRYPTION
int kvz_make_vs_ignore_crypto_not_having_symbols = 0;
#else

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#if AESEncryptionStreamMode
  typedef CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cipher_t;
#else
  typedef CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cipher_t;
#endif

struct crypto_handle_t {
  cipher_t *cipher;
  byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
  byte iv[CryptoPP::AES::BLOCKSIZE];
  byte out_stream_counter[CryptoPP::AES::BLOCKSIZE];
  byte counter[CryptoPP::AES::BLOCKSIZE];
  int couter_avail;
  int counter_index;
  int counter_index_pos;
};


static uint8_t default_IV[16] = {201, 75, 219, 152, 6, 245, 237, 107, 179, 194, 81, 29, 66, 98, 198, 0};
static uint8_t default_key[16] = {16, 213, 27, 56, 255, 127, 242, 112, 97, 126, 197, 204, 25, 59, 38, 30};


crypto_handle_t* kvz_crypto_create(const kvz_config *cfg)
{
  crypto_handle_t* hdl = (crypto_handle_t*)calloc(1, sizeof(crypto_handle_t));

  uint8_t *key;
  if(cfg->optional_key!=NULL)
    key = cfg->optional_key;
  else
    key = default_key;

  for (int i = 0; i < 16; i++) {
    hdl->iv [i]     = default_IV[i];
    hdl->counter[i] = (i<11)? default_IV[5+i] : key[i-11];
    hdl->key[i]     = key[i];
  }

  hdl->cipher = new cipher_t(hdl->key, CryptoPP::AES::DEFAULT_KEYLENGTH, hdl->iv);

  hdl->couter_avail      = 0;
  hdl->counter_index     = 0;
  hdl->counter_index_pos = 0;

  return hdl;
}

void kvz_crypto_delete(crypto_handle_t **hdl)
{
  if (*hdl) {
    delete (*hdl)->cipher;
    (*hdl)->cipher = NULL;
  }
  FREE_POINTER(*hdl);
}

void kvz_crypto_decrypt(crypto_handle_t* hdl,
                        const uint8_t *in_stream,
                        int size_bits,
                        uint8_t *out_stream)
{
  int num_bytes = ceil((double)size_bits/8);
  hdl->cipher->ProcessData(out_stream, in_stream, num_bytes);
  if (size_bits & 7) {
    hdl->cipher->SetKeyWithIV(hdl->key, CryptoPP::AES::DEFAULT_KEYLENGTH, hdl->iv);
  }
}
#if AESEncryptionStreamMode
static void increment_counter(unsigned char *counter)
{
  counter[0]++;
}

static void decrypt_counter(crypto_handle_t *hdl)
{
  hdl->cipher->ProcessData(hdl->out_stream_counter, hdl->counter, 16);
  hdl->couter_avail      = 128;
  hdl->counter_index     = 15;
  hdl->counter_index_pos = 8;
  increment_counter(hdl->counter);
}

unsigned kvz_crypto_get_key(crypto_handle_t *hdl, int nb_bits)
{
  unsigned key = 0;
  if (nb_bits > 32) {
      fprintf(stderr, "The generator cannot generate %d bits (max 32 bits)\n", nb_bits);
      return 0;
  }
  if (nb_bits == 0) return 0;

  if (!hdl->couter_avail) {
    decrypt_counter(hdl);
  }

  if(hdl->couter_avail >= nb_bits) {
      hdl->couter_avail -= nb_bits;
  } else {
      hdl->couter_avail = 0;
  }

  int nb = 0;
  while (nb_bits) {
    if (nb_bits >= hdl->counter_index_pos) {
      nb = hdl->counter_index_pos;
    } else {
      nb = nb_bits;
    }

    key <<= nb;
    key += hdl->out_stream_counter[hdl->counter_index] & ((1 << nb) - 1);
    hdl->out_stream_counter[hdl->counter_index] >>= nb;
    nb_bits -= nb;

    if (hdl->counter_index && nb == hdl->counter_index_pos) {
      hdl->counter_index--;
      hdl->counter_index_pos = 8;
    } else {
      hdl->counter_index_pos -= nb;
      if (nb_bits) {
        decrypt_counter(hdl);
        hdl->couter_avail -=  nb_bits;
      }
    }
  }
  return key;
}
#endif // AESEncryptionStreamMode

#endif // KVZ_SEL_ENCRYPTION
