/*
 * Copyright (C) 2017 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_X509_PROV_SEED_H
#define GNUTLS_LIB_X509_PROV_SEED_H

int _x509_encode_provable_seed(gnutls_x509_privkey_t pkey, gnutls_datum_t *der);
int _x509_decode_provable_seed(gnutls_x509_privkey_t pkey, const gnutls_datum_t *der);

#endif /* GNUTLS_LIB_X509_PROV_SEED_H */
