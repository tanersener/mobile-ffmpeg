/*
 * Copyright (C) 2017 Free Software Foundation, Inc.
 *
 * Author: Ander Juaristi
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

#ifndef GNUTLS_LIB_TLS13_PSK_EXT_PARSER_H
#define GNUTLS_LIB_TLS13_PSK_EXT_PARSER_H

struct psk_ext_parser_st {
	const unsigned char *identities_data;
	size_t identities_len;

	const unsigned char *binders_data;
	size_t binders_len;
};

typedef struct psk_ext_parser_st psk_ext_parser_st;
typedef struct psk_ext_parser_st psk_ext_iter_st;

struct psk_st {
	/* constant values */
	gnutls_datum_t identity;
	uint32_t ob_ticket_age;
};

int _gnutls13_psk_ext_parser_init(psk_ext_parser_st *p,
				  const unsigned char *data, size_t len);

inline static
void _gnutls13_psk_ext_iter_init(psk_ext_iter_st *iter,
				 const psk_ext_parser_st *p)
{
	memcpy(iter, p, sizeof(*p));
}

int _gnutls13_psk_ext_iter_next_identity(psk_ext_iter_st *iter,
					 struct psk_st *psk);
int _gnutls13_psk_ext_iter_next_binder(psk_ext_iter_st *iter,
				       gnutls_datum_t *binder);

#endif /* GNUTLS_LIB_TLS13_PSK_EXT_PARSER_H */
