/*
 * Copyright (C) 2007-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos, Martin Ukrop
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

#include "gnutls_int.h"
#include "ip.h"
#include <gnutls/x509.h>

#ifdef HAVE_INET_NTOP
# include <arpa/inet.h>
#endif

/*-
 * _gnutls_mask_to_prefix:
 * @mask: CIDR mask
 * @mask_size: number of bytes in @mask
 *
 * Check for mask validity (form of 1*0*) and return its prefix numerically.
 *
 * Returns: Length of 1-prefix (0 -- mask_size*8), -1 in case of invalid mask
 -*/
int _gnutls_mask_to_prefix(const unsigned char *mask, unsigned mask_size)
{
	unsigned i, prefix_length = 0;
	for (i=0; i<mask_size; i++) {
		if (mask[i] == 0xFF) {
			prefix_length += 8;
		} else {
			switch(mask[i]) {
				case 0xFE: prefix_length += 7; break;
				case 0xFC: prefix_length += 6; break;
				case 0xF8: prefix_length += 5; break;
				case 0xF0: prefix_length += 4; break;
				case 0xE0: prefix_length += 3; break;
				case 0xC0: prefix_length += 2; break;
				case 0x80: prefix_length += 1; break;
				case 0x00: break;
				default:
					return -1;
			}
			break;
		}
	}
	i++;
	// mask is invalid, if there follows something else than 0x00
	for ( ; i<mask_size; i++) {
		if (mask[i] != 0)
			return -1;
	}
	return prefix_length;
}

/*-
 * _gnutls_ip_to_string:
 * @_ip: IP address (RFC5280 format)
 * @ip_size: Size of @_ip (4 or 16)
 * @out: Resulting string
 * @out_size: Size of @out
 *
 * Transform IP address into human-readable string.
 * @string must be already allocated and
 * at least 16/48 bytes for IPv4/v6 address respectively.
 *
 * Returns: Address of result string.
 -*/
const char *_gnutls_ip_to_string(const void *_ip, unsigned int ip_size, char *out, unsigned int out_size)
{

	if (ip_size != 4 && ip_size != 16) {
		gnutls_assert();
		return NULL;
	}

	if (ip_size == 4 && out_size < 16) {
		gnutls_assert();
		return NULL;
	}

	if (ip_size == 16 && out_size < 48) {
		gnutls_assert();
		return NULL;
	}

	if (ip_size == 4)
		return inet_ntop(AF_INET, _ip, out, out_size);
	else
		return inet_ntop(AF_INET6, _ip, out, out_size);
}

/*-
 * _gnutls_cidr_to_string:
 * @_ip: CIDR range (RFC5280 format)
 * @ip_size: Size of @_ip (8 or 32)
 * @out: Resulting string
 * @out_size: Size of @out
 *
 * Transform CIDR IP address range into human-readable string.
 * The input @_ip must be in RFC5280 format (IP address in network byte
 * order, followed by its network mask which is 4 bytes in IPv4 and
 * 16 bytes in IPv6). @string must be already allocated and
 * at least 33/97 bytes for IPv4/v6 address respectively.
 *
 * Returns: Address of result string.
 -*/
const char *_gnutls_cidr_to_string(const void *_ip, unsigned int ip_size, char *out, unsigned int out_size)
{
	const unsigned char *ip = _ip;
	char tmp[64];
	const char *p;

	if (ip_size != 8 && ip_size != 32) {
		gnutls_assert();
		return NULL;
	}

	if (ip_size == 8) {
		p = inet_ntop(AF_INET, ip, tmp, sizeof(tmp));

		if (p)
			snprintf(out, out_size, "%s/%d", tmp, _gnutls_mask_to_prefix(ip+4, 4));
	} else {
		p = inet_ntop(AF_INET6, ip, tmp, sizeof(tmp));

		if (p)
			snprintf(out, out_size, "%s/%d", tmp, _gnutls_mask_to_prefix(ip+16, 16));
	}

	if (p == NULL)
		return NULL;

	return out;
}

static void prefix_to_mask(unsigned prefix, unsigned char *mask, size_t mask_size)
{
	int i;
	unsigned j;
	memset(mask, 0, mask_size);

	for (i = prefix, j = 0; i > 0 && j < mask_size; i -= 8, j++) {
		if (i >= 8) {
			mask[j] = 0xff;
		} else {
			mask[j] = (unsigned long)(0xffU << (8 - i));
		}
	}
}

/*-
 * _gnutls_mask_ip:
 * @ip: IP of @ipsize bytes
 * @mask: netmask of @ipsize bytes
 * @ipsize: IP length (4 or 16)
 *
 * Mask given IP in place according to the given mask.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 -*/
int _gnutls_mask_ip(unsigned char *ip, const unsigned char *mask, unsigned ipsize)
{
	unsigned i;

	if (ipsize != 4 && ipsize != 16)
		return GNUTLS_E_MALFORMED_CIDR;
	for (i = 0; i < ipsize; i++)
		ip[i] &= mask[i];
	return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_x509_cidr_to_rfc5280:
 * @cidr: CIDR in RFC4632 format (IP/prefix), null-terminated
 * @cidr_rfc5280: CIDR range converted to RFC5280 format
 *
 * This function will convert text CIDR range with prefix (such as '10.0.0.0/8')
 * to RFC5280 (IP address in network byte order followed by its network mask).
 * Works for both IPv4 and IPv6.
 *
 * The resulting object is directly usable for IP name constraints usage,
 * for example in functions %gnutls_x509_name_constraints_add_permitted
 * or %gnutls_x509_name_constraints_add_excluded.
 *
 * The data in datum needs to be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.5.4
 */
int gnutls_x509_cidr_to_rfc5280(const char *cidr, gnutls_datum_t *cidr_rfc5280)
{
	unsigned iplength, prefix;
	int ret;
	char *p;
	char *p_end = NULL;
	char *cidr_tmp;

	p = strchr(cidr, '/');
	if (p != NULL) {
		prefix = strtol(p+1, &p_end, 10);
		if (prefix == 0 && p_end == p+1) {
			_gnutls_debug_log("Cannot parse prefix given in CIDR %s\n", cidr);
			gnutls_assert();
			return GNUTLS_E_MALFORMED_CIDR;
		}
		unsigned length = p-cidr+1;
		cidr_tmp = gnutls_malloc(length);
		if (cidr_tmp == NULL) {
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}
		memcpy(cidr_tmp, cidr, length);
		cidr_tmp[length-1] = 0;
	} else {
		_gnutls_debug_log("No prefix given in CIDR %s\n", cidr);
		gnutls_assert();
		return GNUTLS_E_MALFORMED_CIDR;
	}

	if (strchr(cidr, ':') != 0) { /* IPv6 */
		iplength = 16;
	} else { /* IPv4 */
		iplength = 4;
	}
	cidr_rfc5280->size = 2*iplength;

	if (prefix > iplength*8) {
		_gnutls_debug_log("Invalid prefix given in CIDR %s (%d)\n", cidr, prefix);
		ret = gnutls_assert_val(GNUTLS_E_MALFORMED_CIDR);
		goto cleanup;
	}

	cidr_rfc5280->data = gnutls_malloc(cidr_rfc5280->size);
	if (cidr_rfc5280->data == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		goto cleanup;
	}

	ret = inet_pton(iplength == 4 ? AF_INET : AF_INET6, cidr_tmp, cidr_rfc5280->data);
	if (ret == 0) {
		_gnutls_debug_log("Cannot parse IP from CIDR %s\n", cidr_tmp);
		ret = gnutls_assert_val(GNUTLS_E_MALFORMED_CIDR);
		goto cleanup;
	}

	prefix_to_mask(prefix, &cidr_rfc5280->data[iplength], iplength);
	_gnutls_mask_ip(cidr_rfc5280->data, &cidr_rfc5280->data[iplength], iplength);

	ret = GNUTLS_E_SUCCESS;

cleanup:
	gnutls_free(cidr_tmp);
	return ret;
}
