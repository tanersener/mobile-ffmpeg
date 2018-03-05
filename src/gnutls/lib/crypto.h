/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef CRYPTO_H
#define CRYPTO_H

const gnutls_crypto_cipher_st
    * _gnutls_get_crypto_cipher(gnutls_cipher_algorithm_t algo);
const gnutls_crypto_digest_st
    * _gnutls_get_crypto_digest(gnutls_digest_algorithm_t algo);
const gnutls_crypto_mac_st *_gnutls_get_crypto_mac(gnutls_mac_algorithm_t
						   algo);
void _gnutls_crypto_deregister(void);

#endif				/* CRYPTO_H */
