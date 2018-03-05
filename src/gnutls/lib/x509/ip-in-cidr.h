/*
 * Copyright (C) 2014-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Authors: Nikos Mavrogiannopoulos, Daiki Ueno, Martin Ukrop
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

/*-
 * ip_in_cidr:
 * @ip: IP datum (IPv4 or IPv6)
 * @cidr: CIDR datum (IPv4 or IPv6)
 *
 * Check if @ip lies in the given @cidr range.
 * The @ip version must match the @cidr version (v4/v6),
 * (this is not checked).
 *
 * Returns: 1 if @ip lies withing @cidr, 0 otherwise
 -*/
static unsigned ip_in_cidr(const gnutls_datum_t *ip, const gnutls_datum_t *cidr)
{
	char str_ip[48];
	char str_cidr[97];
	unsigned byte;

	_gnutls_hard_log("matching %.*s with CIDR constraint %.*s\n",
					 (int) sizeof(str_ip),
					 _gnutls_ip_to_string(ip->data, ip->size, str_ip, sizeof(str_ip)),
					 (int) sizeof(str_cidr),
					 _gnutls_cidr_to_string(cidr->data, cidr->size, str_cidr, sizeof(str_cidr)));

	unsigned ipsize = ip->size;
	for (byte = 0; byte < ipsize; byte++)
		if (((ip->data[byte] ^ cidr->data[byte]) & cidr->data[ipsize+byte]) != 0)
			return 0;

	return 1; /* match */
}
