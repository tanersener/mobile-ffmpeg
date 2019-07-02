#include "testutils.h"
#include "nettle-internal.h"
#include "cmac.h"

#define test_cmac_aes128(key, msg, ref)					\
  test_cmac_hash ((nettle_set_key_func*) cmac_aes128_set_key,		\
		  (nettle_hash_update_func*) cmac_aes128_update,	\
		  (nettle_hash_digest_func*) cmac_aes128_digest,	\
		  sizeof(struct cmac_aes128_ctx),			\
		  key, msg, ref)

#define test_cmac_aes256(key, msg, ref)					\
  test_cmac_hash ((nettle_set_key_func*) cmac_aes256_set_key,		\
		  (nettle_hash_update_func*) cmac_aes256_update,	\
		  (nettle_hash_digest_func*) cmac_aes256_digest,	\
		  sizeof(struct cmac_aes256_ctx),			\
		  key, msg, ref)

static void
test_cmac_hash (nettle_set_key_func *set_key,
		nettle_hash_update_func *update,
		nettle_hash_digest_func *digest, size_t ctx_size,
		const struct tstring *key, const struct tstring *msg,
		const struct tstring *ref)
{
  void *ctx;
  uint8_t hash[16];
  unsigned i;

  ctx = xalloc(ctx_size);

  ASSERT (ref->length == sizeof(hash));
  ASSERT (key->length == 16 || key->length == 32);
  set_key (ctx, key->data);
  update (ctx, msg->length, msg->data);
  digest (ctx, sizeof(hash), hash);
  if (!MEMEQ (ref->length, ref->data, hash))
    {
      fprintf (stderr, "cmac_hash failed, msg: ");
      print_hex (msg->length, msg->data);
      fprintf(stderr, "Output:");
      print_hex (16, hash);
      fprintf(stderr, "Expected:");
      tstring_print_hex(ref);
      fprintf(stderr, "\n");
      FAIL();
    }

  /* attempt to re-use the structure */
  update (ctx, msg->length, msg->data);
  digest (ctx, sizeof(hash), hash);
  if (!MEMEQ (ref->length, ref->data, hash))
    {
      fprintf (stderr, "cmac_hash failed on re-use, msg: ");
      print_hex (msg->length, msg->data);
      fprintf(stderr, "Output:");
      print_hex (16, hash);
      fprintf(stderr, "Expected:");
      tstring_print_hex(ref);
      fprintf(stderr, "\n");
      FAIL();
    }

  /* attempt byte-by-byte hashing */
  set_key (ctx, key->data);
  for (i=0;i<msg->length;i++)
    update (ctx, 1, msg->data+i);
  digest (ctx, sizeof(hash), hash);
  if (!MEMEQ (ref->length, ref->data, hash))
    {
      fprintf (stderr, "cmac_hash failed on byte-by-byte, msg: ");
      print_hex (msg->length, msg->data);
      fprintf(stderr, "Output:");
      print_hex (16, hash);
      fprintf(stderr, "Expected:");
      tstring_print_hex(ref);
      fprintf(stderr, "\n");
      FAIL();
    }
  free (ctx);
}

void
test_main(void)
{
  /*
   * CMAC-AES Test Vectors from RFC4493.
   */
  test_cmac_aes128 (SHEX("2b7e151628aed2a6abf7158809cf4f3c"),
		  SDATA(""),
		  SHEX("bb1d6929e95937287fa37d129b756746"));

  test_cmac_aes128 (SHEX("2b7e151628aed2a6abf7158809cf4f3c"),
		  SHEX("6bc1bee22e409f96e93d7e117393172a"),
		  SHEX("070a16b46b4d4144f79bdd9dd04a287c"));

  test_cmac_aes128 (SHEX("2b7e151628aed2a6abf7158809cf4f3c"),
		  SHEX("6bc1bee22e409f96e93d7e117393172a"
		       "ae2d8a571e03ac9c9eb76fac45af8e51"
		       "30c81c46a35ce411"),
		  SHEX("dfa66747de9ae63030ca32611497c827"));

  test_cmac_aes128 (SHEX("2b7e151628aed2a6abf7158809cf4f3c"),
		  SHEX("6bc1bee22e409f96e93d7e117393172a"
		       "ae2d8a571e03ac9c9eb76fac45af8e51"
		       "30c81c46a35ce411e5fbc1191a0a52ef"
		       "f69f2445df4f9b17ad2b417be66c3710"),
		  SHEX("51f0bebf7e3b9d92fc49741779363cfe"));

  /* Additional tests with different keys (same plaintext)
   * to check all variants of set_key() */
  test_cmac_aes128 (SHEX("2b7e151628aed2a8abf7158809cf4f3c"),
		  SHEX("6bc1bee22e409f96e93d7e117393172a"
		       "ae2d8a571e03ac9c9eb76fac45af8e51"
		       "30c81c46a35ce411"),
		  SHEX("87dd33c2945a4e228028690ae8954945"));

  test_cmac_aes128 (SHEX("2b7e1ab628aed2a8abf7158809cf4f3c"),
		  SHEX("6bc1bee22e409f96e93d7e117393172a"
		       "ae2d8a571e03ac9c9eb76fac45af8e51"
		       "30c81c46a35ce411"),
		  SHEX("f0dc613a88886c7ed76eeb51f1c5e8d3"));

  test_cmac_aes128 (SHEX("2b7e1ab628aed2a8abf7158809cf4f3d"),
		  SHEX("6bc1bee22e409f96e93d7e117393172a"
		       "ae2d8a571e03ac9c9eb76fac45af8e51"
		       "30c81c46a35ce411"),
		  SHEX("b9d092dc387a9e42cdfeb9f9930cf567"));

  /* CMAC-AES256 vectors taken from phplib */
  test_cmac_aes256 (SHEX("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"),
		  SDATA(""),
		  SHEX("028962f61b7bf89efc6b551f4667d983"));

  test_cmac_aes256 (SHEX("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"),
		  SHEX("6bc1bee22e409f96e93d7e117393172a"),
		  SHEX("28a7023f452e8f82bd4bf28d8c37c35c"));

  test_cmac_aes256 (SHEX("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"),
		  SHEX("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411"),
		  SHEX("aaf3d8f1de5640c232f5b169b9c911e6"));

  test_cmac_aes256 (SHEX("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"),
		  SHEX("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710"),
		  SHEX("e1992190549f6ed5696a2c056c315410"));

}
