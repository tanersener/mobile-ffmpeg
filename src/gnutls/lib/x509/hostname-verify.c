/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 * Copyright (C) 2002 Andrew McDonald
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
#include <str.h>
#include <x509_int.h>
#include <common.h>
#include "errors.h"
#include <system.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * gnutls_x509_crt_check_hostname:
 * @cert: should contain an gnutls_x509_crt_t type
 * @hostname: A null terminated string that contains a DNS name
 *
 * This function will check if the given certificate's subject matches
 * the given hostname.  This is a basic implementation of the matching
 * described in RFC6125, and takes into account wildcards,
 * and the DNSName/IPAddress subject alternative name PKIX extension.
 *
 * For details see also gnutls_x509_crt_check_hostname2().
 *
 * Returns: non-zero for a successful match, and zero on failure.
 **/
unsigned
gnutls_x509_crt_check_hostname(gnutls_x509_crt_t cert,
			       const char *hostname)
{
	return gnutls_x509_crt_check_hostname2(cert, hostname, 0);
}

static int
check_ip(gnutls_x509_crt_t cert, const void *ip, unsigned ip_size)
{
	char temp[16];
	size_t temp_size;
	unsigned i;
	int ret = 0;

	/* try matching against:
	 *  1) a IPaddress alternative name (subjectAltName) extension
	 *     in the certificate
	 */

	/* Check through all included subjectAltName extensions, comparing
	 * against all those of type IPAddress.
	 */
	for (i = 0; !(ret < 0); i++) {
		temp_size = sizeof(temp);
		ret = gnutls_x509_crt_get_subject_alt_name(cert, i,
							   temp,
							   &temp_size,
							   NULL);

		if (ret == GNUTLS_SAN_IPADDRESS) {
			if (temp_size == ip_size && memcmp(temp, ip, ip_size) == 0)
				return 1;
		} else if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER) {
			ret = 0;
		}
	}

	/* not found a matching IP
	 */
	return 0;
}

/**
 * gnutls_x509_crt_check_ip:
 * @cert: should contain an gnutls_x509_crt_t type
 * @ip: A pointer to the raw IP address
 * @ip_size: the number of bytes in ip (4 or 16)
 * @flags: should be zero
 *
 * This function will check if the IP allowed IP addresses in 
 * the certificate's subject alternative name match the provided
 * IP address.
 *
 * Returns: non-zero for a successful match, and zero on failure.
 **/
unsigned
gnutls_x509_crt_check_ip(gnutls_x509_crt_t cert,
			 const unsigned char *ip, unsigned int ip_size,
			 unsigned int flags)
{
	return check_ip(cert, ip, ip_size);
}

/* whether gnutls_x509_crt_check_hostname2() will consider these
 * alternative name types. This is to satisfy RFC6125 requirement
 * that we do not fallback to CN-ID if we encounter a supported name
 * type.
 */
#define IS_SAN_SUPPORTED(san) (san==GNUTLS_SAN_DNSNAME||san==GNUTLS_SAN_IPADDRESS)

/**
 * gnutls_x509_crt_check_hostname2:
 * @cert: should contain an gnutls_x509_crt_t type
 * @hostname: A null terminated string that contains a DNS name
 * @flags: gnutls_certificate_verify_flags
 *
 * This function will check if the given certificate's subject matches
 * the given hostname.  This is a basic implementation of the matching
 * described in RFC6125, and takes into account wildcards,
 * and the DNSName/IPAddress subject alternative name PKIX extension.
 *
 * IPv4 addresses are accepted by this function in the dotted-decimal
 * format (e.g, ddd.ddd.ddd.ddd), and IPv6 addresses in the hexadecimal
 * x:x:x:x:x:x:x:x format. For them the IPAddress subject alternative
 * name extension is consulted. Previous versions to 3.6.0 of GnuTLS
 * in case of a non-match would consult (in a non-standard extension)
 * the DNSname and CN fields. This is no longer the case.
 *
 * When the flag %GNUTLS_VERIFY_DO_NOT_ALLOW_WILDCARDS is specified no
 * wildcards are considered. Otherwise they are only considered if the
 * domain name consists of three components or more, and the wildcard
 * starts at the leftmost position.

 * When the flag %GNUTLS_VERIFY_DO_NOT_ALLOW_IP_MATCHES is specified,
 * the input will be treated as a DNS name, and matching of textual IP addresses
 * against the IPAddress part of the alternative name will not be allowed.
 *
 * The function gnutls_x509_crt_check_ip() is available for matching
 * IP addresses.
 *
 * Returns: non-zero for a successful match, and zero on failure.
 *
 * Since: 3.3.0
 **/
unsigned
gnutls_x509_crt_check_hostname2(gnutls_x509_crt_t cert,
				const char *hostname, unsigned int flags)
{
	char dnsname[MAX_CN];
	size_t dnsnamesize;
	int found_dnsname = 0;
	int ret = 0;
	int i = 0;
	struct in_addr ipv4;
	char *p = NULL;
	char *a_hostname;
	unsigned have_other_addresses = 0;
	gnutls_datum_t out;

	/* check whether @hostname is an ip address */
	if (!(flags & GNUTLS_VERIFY_DO_NOT_ALLOW_IP_MATCHES) &&
	    ((p=strchr(hostname, ':')) != NULL || inet_pton(AF_INET, hostname, &ipv4) != 0)) {

		if (p != NULL) {
			struct in6_addr ipv6;

			ret = inet_pton(AF_INET6, hostname, &ipv6);
			if (ret == 0) {
				gnutls_assert();
				goto hostname_fallback;
			}
			ret = check_ip(cert, &ipv6, 16);
		} else {
			ret = check_ip(cert, &ipv4, 4);
		}

		/* Prior to 3.6.0 we were accepting misconfigured servers, that place their IP
		 * in the DNS field of subjectAlternativeName. That is no longer the case. */
		return ret;
	}

 hostname_fallback:
	/* convert the provided hostname to ACE-Labels domain. */
	ret = gnutls_idna_map (hostname, strlen(hostname), &out, 0);
	if (ret < 0) {
		_gnutls_debug_log("unable to convert hostname %s to IDNA format\n", hostname);
		a_hostname = (char*)hostname;
	} else {
		a_hostname = (char*)out.data;
	}

	/* try matching against:
	 *  1) a DNS name as an alternative name (subjectAltName) extension
	 *     in the certificate
	 *  2) the common name (CN) in the certificate, if the certificate is acceptable for TLS_WWW_SERVER purpose
	 *
	 *  either of these may be of the form: *.domain.tld
	 *
	 *  only try (2) if there is no subjectAltName extension of
	 *  type dNSName, and there is a single CN.
	 */

	/* Check through all included subjectAltName extensions, comparing
	 * against all those of type dNSName.
	 */
	for (i = 0; !(ret < 0); i++) {

		dnsnamesize = sizeof(dnsname);
		ret = gnutls_x509_crt_get_subject_alt_name(cert, i,
							   dnsname,
							   &dnsnamesize,
							   NULL);

		if (ret == GNUTLS_SAN_DNSNAME) {
			found_dnsname = 1;

			if (_gnutls_has_embedded_null(dnsname, dnsnamesize)) {
				_gnutls_debug_log("certificate has %s with embedded null in name\n", dnsname);
				continue;
			}

			if (!_gnutls_str_is_print(dnsname, dnsnamesize)) {
				_gnutls_debug_log("invalid (non-ASCII) name in certificate %.*s\n", (int)dnsnamesize, dnsname);
				continue;
			}

			ret = _gnutls_hostname_compare(dnsname, dnsnamesize, a_hostname, flags);
			if (ret != 0) {
				ret = 1;
				goto cleanup;
			}
		} else {
			if (IS_SAN_SUPPORTED(ret))
				have_other_addresses = 1;
		}
	}

	if (!have_other_addresses && !found_dnsname && _gnutls_check_key_purpose(cert, GNUTLS_KP_TLS_WWW_SERVER, 0) != 0) {
		/* did not get the necessary extension, use CN instead, if the
		 * certificate would have been acceptable for a TLS WWW server purpose.
		 * That is because only for that purpose the CN is a valid field to
		 * store the hostname.
		 */

		/* enforce the RFC6125 (§1.8) requirement that only
		 * a single CN must be present */
		dnsnamesize = sizeof(dnsname);
		ret = gnutls_x509_crt_get_dn_by_oid
			(cert, OID_X520_COMMON_NAME, 1, 0, dnsname,
			 &dnsnamesize);
		if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			ret = 0;
			goto cleanup;
		}

		dnsnamesize = sizeof(dnsname);
		ret = gnutls_x509_crt_get_dn_by_oid
			(cert, OID_X520_COMMON_NAME, 0, 0, dnsname,
			 &dnsnamesize);
		if (ret < 0) {
			ret = 0;
			goto cleanup;
		}

		if (_gnutls_has_embedded_null(dnsname, dnsnamesize)) {
			_gnutls_debug_log("certificate has CN %s with embedded null in name\n", dnsname);
			ret = 0;
			goto cleanup;
		}

		if (!_gnutls_str_is_print(dnsname, dnsnamesize)) {
			_gnutls_debug_log("invalid (non-ASCII) name in certificate CN %.*s\n", (int)dnsnamesize, dnsname);
			ret = 0;
			goto cleanup;
		}

		ret = _gnutls_hostname_compare(dnsname, dnsnamesize, a_hostname, flags);
		if (ret != 0) {
			ret = 1;
			goto cleanup;
		}
	}

	/* not found a matching name
	 */
	ret = 0;
 cleanup:
	if (a_hostname != hostname) {
		gnutls_free(a_hostname);
	}
	return ret;
}
