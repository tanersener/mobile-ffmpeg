/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#ifdef ENABLE_OPENPGP
#include <gnutls/openpgp.h>
#endif

#include "utils.h"

/*
 * A self-test of the IP matching algorithm. See 
 * name-constraints-ip.c for lower level checks.
 */

char pem_ips[] = "\n"
	"X.509 Certificate Information:\n"
	"	Version: 3\n"
	"	Serial Number (hex): 00\n"
	"	Issuer: CN=server-0\n"
	"	Validity:\n"
	"		Not Before: Fri Jun 27 09:14:36 UTC 2014\n"
	"		Not After: Fri Dec 31 23:59:59 UTC 9999\n"
	"	Subject: CN=server-0\n"
	"	Subject Public Key Algorithm: RSA\n"
	"	Algorithm Security Level: Medium (2048 bits)\n"
	"		Modulus (bits 2048):\n"
	"			00:c1:56:12:f6:c3:c7:e3:4c:7e:ff:04:4e:88:1d:67\n"
	"			a7:f3:4d:64:cc:12:a7:ff:50:aa:5c:31:b9:3c:d1:d1\n"
	"			ba:78:2c:7d:dd:54:4a:cd:5a:f2:38:8b:b2:c5:26:7e\n"
	"			25:05:36:b6:92:e6:1d:c3:00:39:a0:c5:1c:b5:63:3d\n"
	"			00:e9:b4:b5:75:a7:14:b1:ff:a0:03:9d:ba:77:da:e5\n"
	"			de:21:fb:56:da:06:9d:84:57:53:3d:08:45:45:20:fd\n"
	"			e7:60:65:2e:55:60:db:d3:91:da:64:ff:c4:42:42:54\n"
	"			77:cb:47:54:68:1e:b4:62:ad:8a:3c:0a:28:89:cb:d3\n"
	"			81:d3:15:9a:1d:67:90:51:83:90:6d:fb:a1:0e:54:6b\n"
	"			29:d7:ef:79:19:14:f6:0d:82:73:8f:79:58:0e:af:0e\n"
	"			cc:bd:17:ab:b5:a2:1f:76:a1:9f:4b:7b:e8:f9:7b:28\n"
	"			56:cc:f1:5b:0e:93:c9:e5:44:2f:2d:0a:22:7d:0b:2b\n"
	"			30:84:c3:1e:d6:4d:63:5b:41:51:83:d4:b5:09:f4:cc\n"
	"			ab:ad:51:1b:8e:a1:f6:b1:27:5b:43:3c:bc:ae:10:93\n"
	"			d4:ce:3b:10:ca:3f:22:dd:9e:a8:3f:4a:a6:a8:cd:8f\n"
	"			d0:6a:e0:40:26:28:0f:af:0e:13:e1:ac:b9:ac:41:cc\n"
	"			5d\n"
	"		Exponent (bits 24):\n"
	"			01:00:01\n"
	"	Extensions:\n"
	"		Basic Constraints (critical):\n"
	"			Certificate Authority (CA): TRUE\n"
	"		Subject Alternative Name (not critical):\n"
	"			IPAddress: 127.0.0.1\n"
	"			IPAddress: 192.168.5.1\n"
	"			IPAddress: 10.100.2.5\n"
	"			IPAddress: 0:0:0:0:0:0:0:1\n"
	"			IPAddress: fe80:0:0:0:3e97:eff:fe18:359a\n"
	"		Key Usage (critical):\n"
	"			Certificate signing.\n"
	"		Subject Key Identifier (not critical):\n"
	"			bd3d0b6cab6b33d8a8e1ed15b7ab17587cc2a09f\n"
	"	Signature Algorithm: RSA-SHA256\n"
	"	Signature:\n"
	"		02:22:52:4b:69:e5:4f:f8:17:0a:46:34:d1:ec:6b:f5\n"
	"		ae:5b:fc:e2:00:ca:1f:f0:1d:74:91:9c:85:0a:a7:06\n"
	"		3d:fa:93:0d:35:85:ea:3e:01:9f:9e:bc:52:72:95:b2\n"
	"		8a:3a:78:6e:d2:5d:4d:60:88:2b:be:6f:68:75:c7:19\n"
	"		ac:c9:ea:ab:74:f6:62:4d:30:1e:87:e4:70:1e:96:f4\n"
	"		0b:48:ef:c9:28:14:6f:fa:c1:7b:d3:ef:b3:d8:52:90\n"
	"		5d:20:d0:aa:8b:10:ab:74:86:46:be:cb:6c:93:54:60\n"
	"		bc:6e:d6:4d:b2:1e:25:65:38:52:5b:6c:b4:57:8f:0f\n"
	"		26:4f:36:ea:42:eb:71:68:93:f3:a9:7a:66:5c:b6:07\n"
	"		7d:15:b5:f4:b8:5c:7c:e0:cd:d0:fa:5b:2a:6b:fd:4c\n"
	"		71:12:45:d0:37:9e:cf:90:59:6e:fd:ba:3a:8b:ca:37\n"
	"		01:cc:6f:e0:32:c7:9e:a4:ea:61:2c:e5:ad:66:73:80\n"
	"		5c:5e:0c:44:ec:c2:74:b8:fe:6e:66:af:76:cc:30:10\n"
	"		1f:3a:ac:34:36:e6:5b:72:f3:ee:5a:68:c3:43:37:56\n"
	"		c3:08:02:3c:96:1c:27:18:d0:38:fa:d7:51:4e:82:7d\n"
	"		fc:81:a2:23:c5:05:80:0e:b4:ba:d3:19:39:74:9c:74\n"
	"Other Information:\n"
	"	SHA1 fingerprint:\n"
	"		43536dd4198f6064c117c3825020b14c108f9a34\n"
	"	SHA256 fingerprint:\n"
	"		5ab6626aa069da15650edcfff7305767ff5b8d338289f851a624ea89b50ff06a\n"
	"	Public Key ID:\n"
	"		bd3d0b6cab6b33d8a8e1ed15b7ab17587cc2a09f\n"
	"	Public key's random art:\n"
	"		+--[ RSA 2048]----+\n"
	"		|		 |\n"
	"		|	.	|\n"
	"		|	. +	|\n"
	"		|      .  .= .    |\n"
	"		|	.S+oo     |\n"
	"		|	E+.+     |\n"
	"		|    .  +. *.o    |\n"
	"		|   . oo.=..+ o   |\n"
	"		|    ooo.+Bo .    |\n"
	"		+-----------------+\n"
	"\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDKzCCAhOgAwIBAgIBADANBgkqhkiG9w0BAQsFADATMREwDwYDVQQDEwhzZXJ2\n"
	"ZXItMDAiGA8yMDE0MDYyNzA5MTQzNloYDzk5OTkxMjMxMjM1OTU5WjATMREwDwYD\n"
	"VQQDEwhzZXJ2ZXItMDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMFW\n"
	"EvbDx+NMfv8ETogdZ6fzTWTMEqf/UKpcMbk80dG6eCx93VRKzVryOIuyxSZ+JQU2\n"
	"tpLmHcMAOaDFHLVjPQDptLV1pxSx/6ADnbp32uXeIftW2gadhFdTPQhFRSD952Bl\n"
	"LlVg29OR2mT/xEJCVHfLR1RoHrRirYo8CiiJy9OB0xWaHWeQUYOQbfuhDlRrKdfv\n"
	"eRkU9g2Cc495WA6vDsy9F6u1oh92oZ9Le+j5eyhWzPFbDpPJ5UQvLQoifQsrMITD\n"
	"HtZNY1tBUYPUtQn0zKutURuOofaxJ1tDPLyuEJPUzjsQyj8i3Z6oP0qmqM2P0Grg\n"
	"QCYoD68OE+GsuaxBzF0CAwEAAaOBhTCBgjAPBgNVHRMBAf8EBTADAQH/MD8GA1Ud\n"
	"EQQ4MDaHBH8AAAGHBMCoBQGHBApkAgWHEAAAAAAAAAAAAAAAAAAAAAGHEP6AAAAA\n"
	"AAAAPpcO//4YNZowDwYDVR0PAQH/BAUDAwcEADAdBgNVHQ4EFgQUvT0LbKtrM9io\n"
	"4e0Vt6sXWHzCoJ8wDQYJKoZIhvcNAQELBQADggEBAAIiUktp5U/4FwpGNNHsa/Wu\n"
	"W/ziAMof8B10kZyFCqcGPfqTDTWF6j4Bn568UnKVsoo6eG7SXU1giCu+b2h1xxms\n"
	"yeqrdPZiTTAeh+RwHpb0C0jvySgUb/rBe9Pvs9hSkF0g0KqLEKt0hka+y2yTVGC8\n"
	"btZNsh4lZThSW2y0V48PJk826kLrcWiT86l6Zly2B30VtfS4XHzgzdD6Wypr/Uxx\n"
	"EkXQN57PkFlu/bo6i8o3Acxv4DLHnqTqYSzlrWZzgFxeDETswnS4/m5mr3bMMBAf\n"
	"Oqw0NuZbcvPuWmjDQzdWwwgCPJYcJxjQOPrXUU6CffyBoiPFBYAOtLrTGTl0nHQ=\n"
	"-----END CERTIFICATE-----\n"
	"";

void doit(void)
{
	gnutls_x509_crt_t x509;
	gnutls_datum_t data;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("global_init: %d\n", ret);

	ret = gnutls_x509_crt_init(&x509);
	if (ret < 0)
		fail("gnutls_x509_crt_init: %d\n", ret);

	data.data = (unsigned char *) pem_ips;
	data.size = strlen(pem_ips);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "127.0.0.2");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "127.0.0.1");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "192.168.5.1");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "::1");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "fe80::3e97:eff:fe18:359a");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_ip(x509, (unsigned char*)"\x7f\x00\x00\x02", 4, 0);
	if (ret)
		fail("%d: IP incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_ip(x509, (unsigned char*)"\x7f\x00\x00\x01", 4, 0);
	if (!ret)
		fail("%d: IP incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_ip(x509, (unsigned char*)"\xc0\xa8\x05\x01", 4, 0);
	if (!ret)
		fail("%d: IP incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_ip(x509, (unsigned char*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 16, 0);
	if (!ret)
		fail("%d: IP incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_ip(x509, (unsigned char*)"\xfe\x80\x00\x00\x00\x00\x00\x00\x3e\x97\x0e\xff\xfe\x18\x35\x9a", 16, 0);
	if (!ret)
		fail("%d: IP incorrectly does not match (%d)\n", __LINE__, ret);

	gnutls_x509_crt_deinit(x509);

	gnutls_global_deinit();
}
