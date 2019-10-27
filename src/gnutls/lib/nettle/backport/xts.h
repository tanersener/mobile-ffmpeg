/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Simo Sorce
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_NETTLE_BACKPORT_XTS_H
#define GNUTLS_LIB_NETTLE_BACKPORT_XTS_H

#ifdef HAVE_NETTLE_XTS_ENCRYPT_MESSAGE
#include <nettle/xts.h>

#else /* Nettle version is old, use a vendored version instead */

#ifndef NETTLE_XTS_H_INCLUDED
#define NETTLE_XTS_H_INCLUDED

#include <nettle/nettle-types.h>
#include <nettle/aes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define xts_encrypt_message nettle_xts_encrypt_message
#define xts_decrypt_message nettle_xts_decrypt_message
#define xts_aes128_set_encrypt_key nettle_xts_aes128_set_encrypt_key
#define xts_aes128_set_decrypt_key nettle_xts_aes128_set_decrypt_key
#define xts_aes128_encrypt_message nettle_xts_aes128_encrypt_message
#define xts_aes128_decrypt_message nettle_xts_aes128_decrypt_message
#define xts_aes256_set_encrypt_key nettle_xts_aes256_set_encrypt_key
#define xts_aes256_set_decrypt_key nettle_xts_aes256_set_decrypt_key
#define xts_aes256_encrypt_message nettle_xts_aes256_encrypt_message
#define xts_aes256_decrypt_message nettle_xts_aes256_decrypt_message

#define XTS_BLOCK_SIZE 16

void
xts_encrypt_message(const void *enc_ctx, const void *twk_ctx,
                nettle_cipher_func *encf,
                const uint8_t *tweak, size_t length,
                uint8_t *dst, const uint8_t *src);
void
xts_decrypt_message(const void *dec_ctx, const void *twk_ctx,
                    nettle_cipher_func *decf, nettle_cipher_func *encf,
                    const uint8_t *tweak, size_t length,
                    uint8_t *dst, const uint8_t *src);

/* XTS Mode with AES-128 */
struct xts_aes128_key {
    struct aes128_ctx cipher;
    struct aes128_ctx tweak_cipher;
};

void
xts_aes128_set_encrypt_key(struct xts_aes128_key *xts_key,
                           const uint8_t *key);

void
xts_aes128_set_decrypt_key(struct xts_aes128_key *xts_key,
                           const uint8_t *key);

void
xts_aes128_encrypt_message(struct xts_aes128_key *xtskey,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src);

void
xts_aes128_decrypt_message(struct xts_aes128_key *xts_key,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src);

/* XTS Mode with AES-256 */
struct xts_aes256_key {
    struct aes256_ctx cipher;
    struct aes256_ctx tweak_cipher;
};

void
xts_aes256_set_encrypt_key(struct xts_aes256_key *xts_key,
                           const uint8_t *key);

void
xts_aes256_set_decrypt_key(struct xts_aes256_key *xts_key,
                           const uint8_t *key);

void
xts_aes256_encrypt_message(struct xts_aes256_key *xts_key,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src);

void
xts_aes256_decrypt_message(struct xts_aes256_key *xts_key,
                           const uint8_t *tweak, size_t length,
                           uint8_t *dst, const uint8_t *src);

#ifdef __cplusplus
}
#endif

#endif /* NETTLE_XTS_H_INCLUDED */

#endif /* HAVE_NETTLE_XTS_ENCRYPT_MESSAGE */

#endif /* GNUTLS_LIB_NETTLE_BACKPORT_XTS_H */
