/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef GNUTLS_LIB_ACCELERATED_AARCH64_AARCH64_COMMON_H
# define GNUTLS_LIB_ACCELERATED_AARCH64_AARCH64_COMMON_H

#if !__ASSEMBLER__
#define NN_HASH(name, update_func, digest_func, NAME) {	\
 #name,						\
 sizeof(struct name##_ctx),			\
 NAME##_DIGEST_SIZE,				\
 NAME##_DATA_SIZE,				\
 (nettle_hash_init_func *) name##_init,		\
 (nettle_hash_update_func *) update_func,	\
 (nettle_hash_digest_func *) digest_func	\
} 

void register_aarch64_crypto(void);
#endif

#define ARMV7_NEON      (1<<0)
#define ARMV7_TICK      (1<<1)
#define ARMV8_AES       (1<<2)
#define ARMV8_SHA1      (1<<3)
#define ARMV8_SHA256    (1<<4)
#define ARMV8_PMULL     (1<<5)
#define ARMV8_SHA512    (1<<6)

#endif /* GNUTLS_LIB_ACCELERATED_AARCH64_AARCH64_COMMON_H */
