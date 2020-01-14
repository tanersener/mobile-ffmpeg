/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2016 Dmitry Eremin-Solenikov
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

#ifndef GNUTLS_LIB_VKO_H
#define GNUTLS_LIB_VKO_H

int
_gnutls_gost_keytrans_encrypt(gnutls_pk_params_st *pub,
			      gnutls_pk_params_st *priv,
			      gnutls_datum_t *cek,
			      gnutls_datum_t *ukm,
			      gnutls_datum_t *out);

int
_gnutls_gost_keytrans_decrypt(gnutls_pk_params_st *priv,
			      gnutls_datum_t *cek,
			      gnutls_datum_t *ukm,
			      gnutls_datum_t *out);

#endif /* GNUTLS_LIB_VKO_H */
