/* GOST 28147-89 (Magma) implementation
 *
 * Copyright: 2015, 2016 Dmitry Eremin-Solenikov <dbaryshkov@gmail.com>
 * Copyright: 2009-2012 Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnutls_int.h>

#include <string.h>

#include <nettle/macros.h>
#include "gost28147.h"
#include <nettle/cfb.h>
#include <nettle/memops.h>

void
gost28147_kdf_cryptopro(const struct gost28147_param *param,
		       const uint8_t *in,
		       const uint8_t *ukm,
		       uint8_t *out)
{
	struct gost28147_ctx ctx;
	int i;

	memcpy(out, in, GOST28147_KEY_SIZE);
	for (i = 0; i < 8; i++) {
		uint8_t mask;
		uint8_t *p;
		uint8_t iv[GOST28147_BLOCK_SIZE];
		uint32_t block[2] = {0, 0};
		uint32_t t;

		for (p = out, mask = 1; mask; mask <<= 1) {
			t = LE_READ_UINT32(p);
			p += 4;
			if (mask & ukm[i])
				block[0] += t;
			else
				block[1] += t;
		}

		LE_WRITE_UINT32(iv + 0, block[0]);
		LE_WRITE_UINT32(iv + 4, block[1]);

		gost28147_set_key(&ctx, out);
		gost28147_set_param(&ctx, param);
		cfb_encrypt(&ctx,
			    (nettle_cipher_func*)gost28147_encrypt_for_cfb,
			    GOST28147_BLOCK_SIZE, iv,
			    GOST28147_KEY_SIZE, out, out);
	}
}

void
gost28147_key_wrap_cryptopro(const struct gost28147_param *param,
			     const uint8_t *kek,
			     const uint8_t *ukm, size_t ukm_size,
			     const uint8_t *cek,
			     uint8_t *enc,
			     uint8_t *imit)
{
	uint8_t kd[GOST28147_KEY_SIZE];
	struct gost28147_ctx ctx;
	struct gost28147_imit_ctx ictx;

	assert(ukm_size >= GOST28147_IMIT_BLOCK_SIZE);

	gost28147_kdf_cryptopro(param, kek, ukm, kd);
	gost28147_set_key(&ctx, kd);
	gost28147_set_param(&ctx, param);
	gost28147_encrypt(&ctx, GOST28147_KEY_SIZE, enc, cek);

	gost28147_imit_init(&ictx);
	gost28147_imit_set_key(&ictx, GOST28147_KEY_SIZE, kd);
	gost28147_imit_set_param(&ictx, param);
	gost28147_imit_set_nonce(&ictx, ukm);
	gost28147_imit_update(&ictx, GOST28147_KEY_SIZE, cek);
	gost28147_imit_digest(&ictx, GOST28147_IMIT_DIGEST_SIZE, imit);
}

int
gost28147_key_unwrap_cryptopro(const struct gost28147_param *param,
			       const uint8_t *kek,
			       const uint8_t *ukm, size_t ukm_size,
			       const uint8_t *enc,
			       const uint8_t *imit,
			       uint8_t *cek)
{
	uint8_t kd[GOST28147_KEY_SIZE];
	uint8_t mac[GOST28147_IMIT_DIGEST_SIZE];
	struct gost28147_ctx ctx;
	struct gost28147_imit_ctx ictx;

	assert(ukm_size >= GOST28147_IMIT_BLOCK_SIZE);

	gost28147_kdf_cryptopro(param, kek, ukm, kd);
	gost28147_set_key(&ctx, kd);
	gost28147_set_param(&ctx, param);
	gost28147_decrypt(&ctx, GOST28147_KEY_SIZE, cek, enc);

	gost28147_imit_init(&ictx);
	gost28147_imit_set_key(&ictx, GOST28147_KEY_SIZE, kd);
	gost28147_imit_set_param(&ictx, param);
	gost28147_imit_set_nonce(&ictx, ukm);
	gost28147_imit_update(&ictx, GOST28147_KEY_SIZE, cek);
	gost28147_imit_digest(&ictx, GOST28147_IMIT_DIGEST_SIZE, mac);

	return memeql_sec(mac, imit, GOST28147_IMIT_DIGEST_SIZE);
}
