/*
 * Copyright (C) 2017-2018 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Ander Juaristi, Nikos Mavrogiannopoulos
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

#include "gnutls_int.h"
#include "tls13/psk_ext_parser.h"

/* Returns GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE when no identities
 * are present, or 0, on success.
 */
int _gnutls13_psk_ext_parser_init(psk_ext_parser_st *p,
				  const unsigned char *data, size_t _len)
{
	ssize_t len = _len;

	if (!p || !data || !len)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	memset(p, 0, sizeof(*p));

	DECR_LEN(len, 2);
	p->identities_len = _gnutls_read_uint16(data);
	data += 2;

	if (p->identities_len == 0)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	p->identities_data = (unsigned char *) data;

	DECR_LEN(len, p->identities_len);
	data += p->identities_len;

	DECR_LEN(len, 2);
	p->binders_len = _gnutls_read_uint16(data);
	data += 2;

	p->binders_data = data;
	DECR_LEN(len, p->binders_len);

	return 0;
}

/* Extract PSK identity and move to the next iteration.
 *
 * Returns GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE when no more identities
 * are present, or 0, on success.
 */
int _gnutls13_psk_ext_iter_next_identity(psk_ext_iter_st *iter,
					 struct psk_st *psk)
{
	if (iter->identities_len == 0)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

	DECR_LEN(iter->identities_len, 2);
	psk->identity.size = _gnutls_read_uint16(iter->identities_data);
	if (psk->identity.size == 0)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	iter->identities_data += 2;
	psk->identity.data = (void*)iter->identities_data;

	DECR_LEN(iter->identities_len, psk->identity.size);
	iter->identities_data += psk->identity.size;

	DECR_LEN(iter->identities_len, 4);
	psk->ob_ticket_age = _gnutls_read_uint32(iter->identities_data);
	iter->identities_data += 4;

	return 0;
}

/* Extract PSK binder and move to the next iteration.
 *
 * Returns GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE when no more identities
 * are present, or 0, on success.
 */
int _gnutls13_psk_ext_iter_next_binder(psk_ext_iter_st *iter,
				       gnutls_datum_t *binder)
{
	if (iter->binders_len == 0)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

	DECR_LEN(iter->binders_len, 1);
	binder->size = *iter->binders_data;
	if (binder->size == 0)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	iter->binders_data++;
	binder->data = (uint8_t *)iter->binders_data;
	DECR_LEN(iter->binders_len, binder->size);
	iter->binders_data += binder->size;

	return 0;
}
