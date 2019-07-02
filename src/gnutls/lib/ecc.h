/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_ECC_H
#define GNUTLS_LIB_ECC_H

int _gnutls_ecc_ansi_x962_import(const uint8_t * in, unsigned long inlen,
				 bigint_t * x, bigint_t * y);
int _gnutls_ecc_ansi_x962_export(gnutls_ecc_curve_t curve, bigint_t x,
				 bigint_t y, gnutls_datum_t * out);

#endif /* GNUTLS_LIB_ECC_H */
