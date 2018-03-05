/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_MPI_H
#define GNUTLS_MPI_H

#include "gnutls_int.h"

#include <crypto-backend.h>

extern int crypto_bigint_prio;
extern gnutls_crypto_bigint_st _gnutls_mpi_ops;

bigint_t _gnutls_mpi_random_modp(bigint_t, bigint_t p,
			       gnutls_rnd_level_t level);

#define _gnutls_mpi_init _gnutls_mpi_ops.bigint_init
#define _gnutls_mpi_init_multi _gnutls_mpi_ops.bigint_init_multi
#define _gnutls_mpi_clear _gnutls_mpi_ops.bigint_clear
#define _gnutls_mpi_cmp _gnutls_mpi_ops.bigint_cmp
#define _gnutls_mpi_cmp_ui _gnutls_mpi_ops.bigint_cmp_ui
#define _gnutls_mpi_mod _gnutls_mpi_ops.bigint_mod
#define _gnutls_mpi_modm _gnutls_mpi_ops.bigint_modm
#define _gnutls_mpi_set _gnutls_mpi_ops.bigint_set
#define _gnutls_mpi_set_ui _gnutls_mpi_ops.bigint_set_ui
#define _gnutls_mpi_get_nbits _gnutls_mpi_ops.bigint_get_nbits
#define _gnutls_mpi_powm _gnutls_mpi_ops.bigint_powm
#define _gnutls_mpi_addm _gnutls_mpi_ops.bigint_addm
#define _gnutls_mpi_subm _gnutls_mpi_ops.bigint_subm
#define _gnutls_mpi_mulm _gnutls_mpi_ops.bigint_mulm
#define _gnutls_mpi_add _gnutls_mpi_ops.bigint_add
#define _gnutls_mpi_sub _gnutls_mpi_ops.bigint_sub
#define _gnutls_mpi_mul _gnutls_mpi_ops.bigint_mul
#define _gnutls_mpi_div _gnutls_mpi_ops.bigint_div
#define _gnutls_mpi_add_ui _gnutls_mpi_ops.bigint_add_ui
#define _gnutls_mpi_sub_ui _gnutls_mpi_ops.bigint_sub_ui
#define _gnutls_mpi_mul_ui _gnutls_mpi_ops.bigint_mul_ui
#define _gnutls_prime_check _gnutls_mpi_ops.bigint_prime_check
#define _gnutls_mpi_print(x,y,z) _gnutls_mpi_ops.bigint_print(x,y,z,GNUTLS_MPI_FORMAT_USG)
#define _gnutls_mpi_print_lz(x,y,z) _gnutls_mpi_ops.bigint_print(x,y,z,GNUTLS_MPI_FORMAT_STD)
#define _gnutls_mpi_print_pgp(x,y,z) _gnutls_mpi_ops.bigint_print(x,y,z,GNUTLS_MPI_FORMAT_PGP)
#define _gnutls_mpi_copy _gnutls_mpi_ops.bigint_copy
#define _gnutls_mpi_scan(r, b, s) _gnutls_mpi_ops.bigint_scan(r, b, s, GNUTLS_MPI_FORMAT_USG)
#define _gnutls_mpi_scan_pgp(r, b, s) _gnutls_mpi_ops.bigint_scan(r, b, s, GNUTLS_MPI_FORMAT_PGP)

inline static
void _gnutls_mpi_release(bigint_t * x)
{
	if (*x == NULL)
		return;

	_gnutls_mpi_ops.bigint_release(*x);
	*x = NULL;
}

int _gnutls_mpi_init_scan(bigint_t * ret_mpi, const void *buffer,
		     size_t nbytes);
int _gnutls_mpi_init_scan_nz(bigint_t * ret_mpi, const void *buffer,
			size_t nbytes);
int _gnutls_mpi_init_scan_pgp(bigint_t * ret_mpi, const void *buffer,
			 size_t nbytes);

int _gnutls_mpi_dprint_lz(const bigint_t a, gnutls_datum_t * dest);
int _gnutls_mpi_dprint(const bigint_t a, gnutls_datum_t * dest);
int _gnutls_mpi_dprint_size(const bigint_t a, gnutls_datum_t * dest,
			    size_t size);

#define _gnutls_mpi_generate_group( gg, bits) _gnutls_mpi_ops.bigint_generate_group( gg, bits)

#endif
