/*
 * Copyright (C) 2007-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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
  A self-test of the RFC 2818 hostname matching algorithm.  Used to
  detect regressions of the bug reported in:
  http://lists.gnupg.org/pipermail/gnutls-dev/2007-February/001385.html
*/

/* CN="*.com"
 * dns_name = *.org
 * dns_name = .example.net
 * dns_name = .example.edu.gr
*/
char wildcards[] = "-----BEGIN CERTIFICATE-----"
"MIICwDCCAimgAwIBAgICPd8wDQYJKoZIhvcNAQELBQAwVTEOMAwGA1UEAwwFKi5j"
"b20xETAPBgNVBAsTCENBIGRlcHQuMRIwEAYDVQQKEwlLb2tvIGluYy4xDzANBgNV"
"BAgTBkF0dGlraTELMAkGA1UEBhMCR1IwIhgPMjAxNDAzMTkxMzI4MDhaGA85OTk5"
"MTIzMTIzNTk1OVowVTEOMAwGA1UEAwwFKi5jb20xETAPBgNVBAsTCENBIGRlcHQu"
"MRIwEAYDVQQKEwlLb2tvIGluYy4xDzANBgNVBAgTBkF0dGlraTELMAkGA1UEBhMC"
"R1IwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAKXGznVDhL9kngInE/EDWfd5"
"LZLtfC9QpAPxLXm5hosFfjq7RKqvhM8TmB4cSjj3My16n3LUa20msDE3cBD7QunY"
"nRhlfhlJ/AWWBGiDHneGv+315RI7E/4zGJwaeh1pr0cCYHofuejP28g0MFGWPYyW"
"XAC8Yd4ID7E2IX+pAOMFAgMBAAGjgZowgZcwDAYDVR0TAQH/BAIwADBCBgNVHREE"
"OzA5gg93d3cuZXhhbXBsZS5jb22CBSoub3Jngg0qLmV4YW1wbGUubmV0ghAqLmV4"
"YW1wbGUuZWR1LmdyMBMGA1UdJQQMMAoGCCsGAQUFBwMBMA8GA1UdDwEB/wQFAwMH"
"oAAwHQYDVR0OBBYEFF1ArfDOlECVi36ZlB2SVCLKcjZfMA0GCSqGSIb3DQEBCwUA"
"A4GBAGcDnJIJFqjaDMk806xkfz7/FtbHYkj18ma3l7wgp27jeO/QDYunns5pqbqV"
"sxaKuPKLdWQdfIG7l4+TUnm/Hue6h2PFgbAyZtZbHlAtpEmLoSCmYlFqbRNqux0z"
"F5H1ocGzmbu1WQYXMlY1FYBvRDrAk7Wxt09WLdajH00S/fPT"
"-----END CERTIFICATE-----";

/* Certificate with no SAN nor CN. */
char pem1[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 00\n"
    "	Issuer: O=GnuTLS hostname check test CA\n"
    "	Validity:\n"
    "		Not Before: Fri Feb 16 12:59:09 UTC 2007\n"
    "		Not After: Fri Mar 30 12:59:13 UTC 2007\n"
    "	Subject: O=GnuTLS hostname check test CA\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			be:ec:98:7a:1d:6f:7e:6b:25:9e:e8:20:78:42:a0:64\n"
    "			05:66:43:99:6d:49:d5:18:ec:7d:b9:58:64:b2:80:a3\n"
    "			14:61:9d:0a:4f:be:2f:f0:2e:fc:d2:ab:5c:36:df:53\n"
    "			ec:43:c7:fc:de:91:bc:1e:01:a6:b7:6c:b2:07:10:2e\n"
    "			cb:61:47:75:ca:03:ce:23:6e:38:f1:34:27:1a:1a:cd\n"
    "			f7:96:f3:b3:f0:0d:67:7f:ca:77:84:3f:9c:29:f4:62\n"
    "			91:f6:12:5b:62:5a:cc:ba:ed:08:2e:32:44:26:ac:fd\n"
    "			23:ce:53:1b:bb:f2:87:fe:dc:78:93:7c:59:bf:a1:75\n"
    "		Exponent:\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Key Identifier (not critical):\n"
    "			e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "	Signature Algorithm: RSA-SHA\n"
    "	Signature:\n"
    "		7b:e8:11:6c:15:3f:f9:01:a0:f1:28:0c:62:50:58:f8\n"
    "		92:44:fb:bf:ab:20:8a:3b:81:ca:e5:68:60:71:df:2b\n"
    "		e8:50:58:82:32:ef:fb:6e:4a:72:2c:c9:37:4f:88:1d\n"
    "		d7:1b:68:5b:db:83:1b:1a:f3:b4:8e:e0:88:03:e2:43\n"
    "		91:be:d8:b1:ca:f2:62:ec:a1:fd:1a:c8:41:8c:fe:53\n"
    "		1b:be:03:c9:a1:3d:f4:ae:57:fc:44:a6:34:bb:2c:2e\n"
    "		a7:56:14:1f:89:e9:3a:ec:1f:a3:da:d7:a1:94:3b:72\n"
    "		1d:12:71:b9:65:a1:85:a2:4c:3a:d1:2c:e9:e9:ea:1c\n"
    "Other Information:\n"
    "	MD5 fingerprint:\n"
    "		fd845ded8c28ba5e78d6c1844ceafd24\n"
    "	SHA-1 fingerprint:\n"
    "		0bae431dda3cae76012b82276e4cd92ad7961798\n"
    "	Public Key ID:\n"
    "		e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB8TCCAVygAwIBAgIBADALBgkqhkiG9w0BAQUwKDEmMCQGA1UEChMdR251VExT\n"
    "IGhvc3RuYW1lIGNoZWNrIHRlc3QgQ0EwHhcNMDcwMjE2MTI1OTA5WhcNMDcwMzMw\n"
    "MTI1OTEzWjAoMSYwJAYDVQQKEx1HbnVUTFMgaG9zdG5hbWUgY2hlY2sgdGVzdCBD\n"
    "QTCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGAvuyYeh1vfmslnuggeEKgZAVmQ5lt\n"
    "SdUY7H25WGSygKMUYZ0KT74v8C780qtcNt9T7EPH/N6RvB4BprdssgcQLsthR3XK\n"
    "A84jbjjxNCcaGs33lvOz8A1nf8p3hD+cKfRikfYSW2JazLrtCC4yRCas/SPOUxu7\n"
    "8of+3HiTfFm/oXUCAwEAAaMyMDAwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQU\n"
    "6Twc+62SbuYGpFYsouHAUyfI8pUwCwYJKoZIhvcNAQEFA4GBAHvoEWwVP/kBoPEo\n"
    "DGJQWPiSRPu/qyCKO4HK5Whgcd8r6FBYgjLv+25KcizJN0+IHdcbaFvbgxsa87SO\n"
    "4IgD4kORvtixyvJi7KH9GshBjP5TG74DyaE99K5X/ESmNLssLqdWFB+J6TrsH6Pa\n"
    "16GUO3IdEnG5ZaGFokw60Szp6eoc\n" "-----END CERTIFICATE-----\n";

/* Certificate with CN but no SAN. */
char pem2[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 00\n"
    "	Issuer: CN=www.example.org\n"
    "	Validity:\n"
    "		Not Before: Fri Feb 16 13:30:30 UTC 2007\n"
    "		Not After: Fri Mar 30 13:30:32 UTC 2007\n"
    "	Subject: CN=www.example.org\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			be:ec:98:7a:1d:6f:7e:6b:25:9e:e8:20:78:42:a0:64\n"
    "			05:66:43:99:6d:49:d5:18:ec:7d:b9:58:64:b2:80:a3\n"
    "			14:61:9d:0a:4f:be:2f:f0:2e:fc:d2:ab:5c:36:df:53\n"
    "			ec:43:c7:fc:de:91:bc:1e:01:a6:b7:6c:b2:07:10:2e\n"
    "			cb:61:47:75:ca:03:ce:23:6e:38:f1:34:27:1a:1a:cd\n"
    "			f7:96:f3:b3:f0:0d:67:7f:ca:77:84:3f:9c:29:f4:62\n"
    "			91:f6:12:5b:62:5a:cc:ba:ed:08:2e:32:44:26:ac:fd\n"
    "			23:ce:53:1b:bb:f2:87:fe:dc:78:93:7c:59:bf:a1:75\n"
    "		Exponent:\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Key Identifier (not critical):\n"
    "			e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "	Signature Algorithm: RSA-SHA\n"
    "	Signature:\n"
    "		b0:4e:ac:fb:89:12:36:27:f3:72:b8:1a:57:dc:bf:f3\n"
    "		a9:27:de:15:75:94:4f:65:cc:3a:59:12:4b:91:0e:28\n"
    "		b9:8d:d3:6e:ac:5d:a8:3e:b9:35:81:0c:8f:c7:95:72\n"
    "		d9:51:61:06:00:c6:aa:68:54:c8:52:3f:b6:1f:21:92\n"
    "		c8:fd:15:50:15:ac:d4:18:29:a1:ff:c9:25:5a:ce:5e\n"
    "		11:7f:82:b2:94:8c:44:3c:3f:de:d7:3b:ff:1c:da:9c\n"
    "		81:fa:63:e1:a7:67:ee:aa:fa:d0:c9:2f:66:1b:5e:af\n"
    "		46:8c:f9:53:55:e7:80:7e:74:95:98:d4:2d:5f:94:ab\n"
    "Other Information:\n"
    "	MD5 fingerprint:\n"
    "		30cda7de4f0360892547974f45111ac1\n"
    "	SHA-1 fingerprint:\n"
    "		39e3f8fec6a8d842390b6536998a957c1a6b7322\n"
    "	Public Key ID:\n"
    "		e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB1TCCAUCgAwIBAgIBADALBgkqhkiG9w0BAQUwGjEYMBYGA1UEAxMPd3d3LmV4\n"
    "YW1wbGUub3JnMB4XDTA3MDIxNjEzMzAzMFoXDTA3MDMzMDEzMzAzMlowGjEYMBYG\n"
    "A1UEAxMPd3d3LmV4YW1wbGUub3JnMIGcMAsGCSqGSIb3DQEBAQOBjAAwgYgCgYC+\n"
    "7Jh6HW9+ayWe6CB4QqBkBWZDmW1J1RjsfblYZLKAoxRhnQpPvi/wLvzSq1w231Ps\n"
    "Q8f83pG8HgGmt2yyBxAuy2FHdcoDziNuOPE0JxoazfeW87PwDWd/yneEP5wp9GKR\n"
    "9hJbYlrMuu0ILjJEJqz9I85TG7vyh/7ceJN8Wb+hdQIDAQABozIwMDAPBgNVHRMB\n"
    "Af8EBTADAQH/MB0GA1UdDgQWBBTpPBz7rZJu5gakViyi4cBTJ8jylTALBgkqhkiG\n"
    "9w0BAQUDgYEAsE6s+4kSNifzcrgaV9y/86kn3hV1lE9lzDpZEkuRDii5jdNurF2o\n"
    "Prk1gQyPx5Vy2VFhBgDGqmhUyFI/th8hksj9FVAVrNQYKaH/ySVazl4Rf4KylIxE\n"
    "PD/e1zv/HNqcgfpj4adn7qr60MkvZhter0aM+VNV54B+dJWY1C1flKs=\n"
    "-----END CERTIFICATE-----\n";

/* Certificate with SAN but no CN. */
char pem3[] =
    "X.509 Certificate Information:"
    "	Version: 3\n"
    "	Serial Number (hex): 00\n"
    "	Issuer: O=GnuTLS hostname check test CA\n"
    "	Validity:\n"
    "		Not Before: Fri Feb 16 13:36:27 UTC 2007\n"
    "		Not After: Fri Mar 30 13:36:29 UTC 2007\n"
    "	Subject: O=GnuTLS hostname check test CA\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			be:ec:98:7a:1d:6f:7e:6b:25:9e:e8:20:78:42:a0:64\n"
    "			05:66:43:99:6d:49:d5:18:ec:7d:b9:58:64:b2:80:a3\n"
    "			14:61:9d:0a:4f:be:2f:f0:2e:fc:d2:ab:5c:36:df:53\n"
    "			ec:43:c7:fc:de:91:bc:1e:01:a6:b7:6c:b2:07:10:2e\n"
    "			cb:61:47:75:ca:03:ce:23:6e:38:f1:34:27:1a:1a:cd\n"
    "			f7:96:f3:b3:f0:0d:67:7f:ca:77:84:3f:9c:29:f4:62\n"
    "			91:f6:12:5b:62:5a:cc:ba:ed:08:2e:32:44:26:ac:fd\n"
    "			23:ce:53:1b:bb:f2:87:fe:dc:78:93:7c:59:bf:a1:75\n"
    "		Exponent:\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Alternative Name (not critical):\n"
    "			DNSname: www.example.org\n"
    "		Key Purpose (not critical):\n"
    "			TLS WWW Server.\n"
    "		Subject Key Identifier (not critical):\n"
    "			e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "	Signature Algorithm: RSA-SHA\n"
    "	Signature:\n"
    "		a1:30:bc:01:b3:0f:98:7f:8e:76:7d:23:87:34:15:7f\n"
    "		a6:ae:a1:fb:87:75:e3:e8:1a:e5:5e:03:5d:bf:44:75\n"
    "		46:4f:d2:a1:28:50:84:49:6d:3b:e0:bc:4e:de:79:85\n"
    "		fa:e1:07:b7:6e:0c:14:04:4a:82:b9:f3:22:6a:bc:99\n"
    "		14:20:3b:49:1f:e4:97:d9:ea:eb:73:9a:83:a6:cc:b8\n"
    "		55:fb:52:8e:5f:86:7c:9d:fa:af:03:76:ae:97:e0:64\n"
    "		50:59:73:22:99:55:cf:da:59:31:0a:e8:6d:a0:53:bc\n"
    "		39:63:2e:ac:92:4a:e9:8b:1e:d0:03:df:33:bb:4e:88\n"
    "Other Information:\n"
    "	MD5 fingerprint:\n"
    "		df3f57d00c8149bd826b177d6ea4f369\n"
    "	SHA-1 fingerprint:\n"
    "		e95e56e2acac305f72ea6f698c11624663a595bd\n"
    "	Public Key ID:\n"
    "		e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIICIjCCAY2gAwIBAgIBADALBgkqhkiG9w0BAQUwKDEmMCQGA1UEChMdR251VExT\n"
    "IGhvc3RuYW1lIGNoZWNrIHRlc3QgQ0EwHhcNMDcwMjE2MTMzNjI3WhcNMDcwMzMw\n"
    "MTMzNjI5WjAoMSYwJAYDVQQKEx1HbnVUTFMgaG9zdG5hbWUgY2hlY2sgdGVzdCBD\n"
    "QTCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGAvuyYeh1vfmslnuggeEKgZAVmQ5lt\n"
    "SdUY7H25WGSygKMUYZ0KT74v8C780qtcNt9T7EPH/N6RvB4BprdssgcQLsthR3XK\n"
    "A84jbjjxNCcaGs33lvOz8A1nf8p3hD+cKfRikfYSW2JazLrtCC4yRCas/SPOUxu7\n"
    "8of+3HiTfFm/oXUCAwEAAaNjMGEwDwYDVR0TAQH/BAUwAwEB/zAaBgNVHREEEzAR\n"
    "gg93d3cuZXhhbXBsZS5vcmcwEwYDVR0lBAwwCgYIKwYBBQUHAwEwHQYDVR0OBBYE\n"
    "FOk8HPutkm7mBqRWLKLhwFMnyPKVMAsGCSqGSIb3DQEBBQOBgQChMLwBsw+Yf452\n"
    "fSOHNBV/pq6h+4d14+ga5V4DXb9EdUZP0qEoUIRJbTvgvE7eeYX64Qe3bgwUBEqC\n"
    "ufMiaryZFCA7SR/kl9nq63Oag6bMuFX7Uo5fhnyd+q8Ddq6X4GRQWXMimVXP2lkx\n"
    "CuhtoFO8OWMurJJK6Yse0APfM7tOiA==\n" "-----END CERTIFICATE-----\n";

/* Certificate with wildcard SAN but no CN. */
char pem4[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 00\n"
    "	Issuer:\n"
    "	Validity:\n"
    "		Not Before: Fri Feb 16 13:40:10 UTC 2007\n"
    "		Not After: Fri Mar 30 13:40:12 UTC 2007\n"
    "	Subject:\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			be:ec:98:7a:1d:6f:7e:6b:25:9e:e8:20:78:42:a0:64\n"
    "			05:66:43:99:6d:49:d5:18:ec:7d:b9:58:64:b2:80:a3\n"
    "			14:61:9d:0a:4f:be:2f:f0:2e:fc:d2:ab:5c:36:df:53\n"
    "			ec:43:c7:fc:de:91:bc:1e:01:a6:b7:6c:b2:07:10:2e\n"
    "			cb:61:47:75:ca:03:ce:23:6e:38:f1:34:27:1a:1a:cd\n"
    "			f7:96:f3:b3:f0:0d:67:7f:ca:77:84:3f:9c:29:f4:62\n"
    "			91:f6:12:5b:62:5a:cc:ba:ed:08:2e:32:44:26:ac:fd\n"
    "			23:ce:53:1b:bb:f2:87:fe:dc:78:93:7c:59:bf:a1:75\n"
    "		Exponent:\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Alternative Name (not critical):\n"
    "			DNSname: *.example.org\n"
    "		Key Purpose (not critical):\n"
    "			TLS WWW Server.\n"
    "		Subject Key Identifier (not critical):\n"
    "			e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "	Signature Algorithm: RSA-SHA\n"
    "	Signature:\n"
    "		b1:62:e5:e3:0b:a5:99:58:b0:1c:5c:f5:d1:3f:7c:bb\n"
    "		67:e1:43:c5:d7:a2:5c:db:f2:5a:f3:03:fc:76:e4:4d\n"
    "		c1:a0:89:36:24:82:a4:a1:ad:f5:83:e3:96:75:f4:c4\n"
    "		f3:eb:ff:3a:9b:da:d2:2c:58:d4:10:37:50:33:d1:39\n"
    "		53:71:9e:48:2d:b2:5b:27:ce:1e:d9:d5:36:59:ac:17\n"
    "		3a:83:cc:59:6b:8f:6a:24:b8:9f:f0:e6:14:03:23:5a\n"
    "		87:e7:33:10:32:11:58:a2:bb:f1:e5:5a:88:87:bb:80\n"
    "		1b:b6:bb:12:18:cb:15:d5:3a:fc:99:e4:42:5a:ba:45\n"
    "Other Information:\n"
    "	MD5 fingerprint:\n"
    "		a411da7b0fa064d214116d5f94e06c24\n"
    "	SHA-1 fingerprint:\n"
    "		3596e796c73ed096d762ab3d440a9ab55a386b3b\n"
    "	Public Key ID:\n"
    "		e93c1cfbad926ee606a4562ca2e1c05327c8f295\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB0DCCATugAwIBAgIBADALBgkqhkiG9w0BAQUwADAeFw0wNzAyMTYxMzQwMTBa\n"
    "Fw0wNzAzMzAxMzQwMTJaMAAwgZwwCwYJKoZIhvcNAQEBA4GMADCBiAKBgL7smHod\n"
    "b35rJZ7oIHhCoGQFZkOZbUnVGOx9uVhksoCjFGGdCk++L/Au/NKrXDbfU+xDx/ze\n"
    "kbweAaa3bLIHEC7LYUd1ygPOI2448TQnGhrN95bzs/ANZ3/Kd4Q/nCn0YpH2Elti\n"
    "Wsy67QguMkQmrP0jzlMbu/KH/tx4k3xZv6F1AgMBAAGjYTBfMA8GA1UdEwEB/wQF\n"
    "MAMBAf8wGAYDVR0RBBEwD4INKi5leGFtcGxlLm9yZzATBgNVHSUEDDAKBggrBgEF\n"
    "BQcDATAdBgNVHQ4EFgQU6Twc+62SbuYGpFYsouHAUyfI8pUwCwYJKoZIhvcNAQEF\n"
    "A4GBALFi5eMLpZlYsBxc9dE/fLtn4UPF16Jc2/Ja8wP8duRNwaCJNiSCpKGt9YPj\n"
    "lnX0xPPr/zqb2tIsWNQQN1Az0TlTcZ5ILbJbJ84e2dU2WawXOoPMWWuPaiS4n/Dm\n"
    "FAMjWofnMxAyEViiu/HlWoiHu4AbtrsSGMsV1Tr8meRCWrpF\n"
    "-----END CERTIFICATE-----\n";

#ifdef SUPPORT_COMPLEX_WILDCARDS
/* Certificate with multiple wildcards SAN but no CN. */
char pem6[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 00\n"
    "	Validity:\n"
    "		Not Before: Sat May  3 11:00:51 UTC 2008\n"
    "		Not After: Sat May 17 11:00:54 UTC 2008\n"
    "	Subject: O=GnuTLS hostname check test CA\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			d2:05:c1:65:cb:bd:1e:2e:eb:7b:87:07:94:4c:93:33\n"
    "			f3:81:83:7d:32:1b:71:4e:4e:7f:c7:bc:bf:4b:2f:f2\n"
    "			49:b5:cf:bf:c0:b8:e8:29:cc:f3:61:bd:2e:1d:e4:e8\n"
    "			19:dd:c5:bd:2e:f0:35:b1:fd:30:d7:f5:a8:7c:83:9a\n"
    "			13:9e:bf:25:ed:08:a6:05:9e:7b:4e:23:59:c3:0e:5a\n"
    "			f3:bf:54:c7:dc:d4:13:57:a1:0f:a2:9e:c8:ab:75:66\n"
    "			de:07:84:8d:68:ad:71:04:e0:9c:bd:cb:f6:08:7a:97\n"
    "			42:f8:10:94:29:01:4a:7e:61:d7:04:21:05:4c:f1:07\n"
    "		Exponent:\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Alternative Name (not critical):\n"
    "			DNSname: *.*.example.org\n"
    "		Key Purpose (not critical):\n"
    "			TLS WWW Server.\n"
    "		Subject Key Identifier (not critical):\n"
    "			5493e6599b283b4529378818aef9a4abbf4d9918\n"
    "Other Information:\n"
    "	Public Key ID:\n"
    "		5493e6599b283b4529378818aef9a4abbf4d9918\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIICIjCCAY2gAwIBAgIBADALBgkqhkiG9w0BAQUwKDEmMCQGA1UEChMdR251VExT\n"
    "IGhvc3RuYW1lIGNoZWNrIHRlc3QgQ0EwHhcNMDgwNTAzMTEwMDUxWhcNMDgwNTE3\n"
    "MTEwMDU0WjAoMSYwJAYDVQQKEx1HbnVUTFMgaG9zdG5hbWUgY2hlY2sgdGVzdCBD\n"
    "QTCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA0gXBZcu9Hi7re4cHlEyTM/OBg30y\n"
    "G3FOTn/HvL9LL/JJtc+/wLjoKczzYb0uHeToGd3FvS7wNbH9MNf1qHyDmhOevyXt\n"
    "CKYFnntOI1nDDlrzv1TH3NQTV6EPop7Iq3Vm3geEjWitcQTgnL3L9gh6l0L4EJQp\n"
    "AUp+YdcEIQVM8QcCAwEAAaNjMGEwDwYDVR0TAQH/BAUwAwEB/zAaBgNVHREEEzAR\n"
    "gg8qLiouZXhhbXBsZS5vcmcwEwYDVR0lBAwwCgYIKwYBBQUHAwEwHQYDVR0OBBYE\n"
    "FFST5lmbKDtFKTeIGK75pKu/TZkYMAsGCSqGSIb3DQEBBQOBgQAQ9PStleVvfmlK\n"
    "wRs8RE/oOO+ouC3qLdnumNEITMRFh8Q12/X4yMLD3CH0aQ/hvHcP26PxAWzpNutk\n"
    "swNx7AzsCu6pN1t1aI3jLgo8e4/zZi57e8QcRuXZPDJxtJxVhJZX/C4pSz802WhS\n"
    "64NgtpHEMu9JUHFhtRwPcvVGYqPUUA==\n" "-----END CERTIFICATE-----\n";

/* Certificate with prefixed and suffixed wildcard SAN but no CN. */
char pem7[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 00\n"
    "	Validity:\n"
    "		Not Before: Sat May  3 11:02:43 UTC 2008\n"
    "		Not After: Sat May 17 11:02:45 UTC 2008\n"
    "	Subject: O=GnuTLS hostname check test CA\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			d2:05:c1:65:cb:bd:1e:2e:eb:7b:87:07:94:4c:93:33\n"
    "			f3:81:83:7d:32:1b:71:4e:4e:7f:c7:bc:bf:4b:2f:f2\n"
    "			49:b5:cf:bf:c0:b8:e8:29:cc:f3:61:bd:2e:1d:e4:e8\n"
    "			19:dd:c5:bd:2e:f0:35:b1:fd:30:d7:f5:a8:7c:83:9a\n"
    "			13:9e:bf:25:ed:08:a6:05:9e:7b:4e:23:59:c3:0e:5a\n"
    "			f3:bf:54:c7:dc:d4:13:57:a1:0f:a2:9e:c8:ab:75:66\n"
    "			de:07:84:8d:68:ad:71:04:e0:9c:bd:cb:f6:08:7a:97\n"
    "			42:f8:10:94:29:01:4a:7e:61:d7:04:21:05:4c:f1:07\n"
    "		Exponent:\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Alternative Name (not critical):\n"
    "			DNSname: foo*bar.example.org\n"
    "		Key Purpose (not critical):\n"
    "			TLS WWW Server.\n"
    "		Subject Key Identifier (not critical):\n"
    "			5493e6599b283b4529378818aef9a4abbf4d9918\n"
    "Other Information:\n"
    "	Public Key ID:\n"
    "		5493e6599b283b4529378818aef9a4abbf4d9918\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIICJjCCAZGgAwIBAgIBADALBgkqhkiG9w0BAQUwKDEmMCQGA1UEChMdR251VExT\n"
    "IGhvc3RuYW1lIGNoZWNrIHRlc3QgQ0EwHhcNMDgwNTAzMTEwMjQzWhcNMDgwNTE3\n"
    "MTEwMjQ1WjAoMSYwJAYDVQQKEx1HbnVUTFMgaG9zdG5hbWUgY2hlY2sgdGVzdCBD\n"
    "QTCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA0gXBZcu9Hi7re4cHlEyTM/OBg30y\n"
    "G3FOTn/HvL9LL/JJtc+/wLjoKczzYb0uHeToGd3FvS7wNbH9MNf1qHyDmhOevyXt\n"
    "CKYFnntOI1nDDlrzv1TH3NQTV6EPop7Iq3Vm3geEjWitcQTgnL3L9gh6l0L4EJQp\n"
    "AUp+YdcEIQVM8QcCAwEAAaNnMGUwDwYDVR0TAQH/BAUwAwEB/zAeBgNVHREEFzAV\n"
    "ghNmb28qYmFyLmV4YW1wbGUub3JnMBMGA1UdJQQMMAoGCCsGAQUFBwMBMB0GA1Ud\n"
    "DgQWBBRUk+ZZmyg7RSk3iBiu+aSrv02ZGDALBgkqhkiG9w0BAQUDgYEAPPNe38jc\n"
    "8NsZQVKKLYc1Y4y8LRPhvnxkSnlcGa1RzYZY1s12BZ6OVIfyxD1Z9BcNdqRSq7bQ\n"
    "kEicsGp5ugGQTNq6aSlzYOUD9/fUP3jDsH7HVb36aCF3waGCQWj+pLqK0LYcW2p/\n"
    "xnr5+z4YevFBhn7l/fMhg8TzKejxYm7TECg=\n" "-----END CERTIFICATE-----\n";
#endif

/* Certificate with ending wildcard SAN but no CN. */
char pem8[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 00\n"
    "	Validity:\n"
    "		Not Before: Sat May  3 11:24:38 UTC 2008\n"
    "		Not After: Sat May 17 11:24:40 UTC 2008\n"
    "	Subject: O=GnuTLS hostname check test CA\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			d2:05:c1:65:cb:bd:1e:2e:eb:7b:87:07:94:4c:93:33\n"
    "			f3:81:83:7d:32:1b:71:4e:4e:7f:c7:bc:bf:4b:2f:f2\n"
    "			49:b5:cf:bf:c0:b8:e8:29:cc:f3:61:bd:2e:1d:e4:e8\n"
    "			19:dd:c5:bd:2e:f0:35:b1:fd:30:d7:f5:a8:7c:83:9a\n"
    "			13:9e:bf:25:ed:08:a6:05:9e:7b:4e:23:59:c3:0e:5a\n"
    "			f3:bf:54:c7:dc:d4:13:57:a1:0f:a2:9e:c8:ab:75:66\n"
    "			de:07:84:8d:68:ad:71:04:e0:9c:bd:cb:f6:08:7a:97\n"
    "			42:f8:10:94:29:01:4a:7e:61:d7:04:21:05:4c:f1:07\n"
    "		Exponent:\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Alternative Name (not critical):\n"
    "			DNSname: www.example.*\n"
    "		Key Purpose (not critical):\n"
    "			TLS WWW Server.\n"
    "		Subject Key Identifier (not critical):\n"
    "			5493e6599b283b4529378818aef9a4abbf4d9918\n"
    "Other Information:\n"
    "	Public Key ID:\n"
    "		5493e6599b283b4529378818aef9a4abbf4d9918\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIICIDCCAYugAwIBAgIBADALBgkqhkiG9w0BAQUwKDEmMCQGA1UEChMdR251VExT\n"
    "IGhvc3RuYW1lIGNoZWNrIHRlc3QgQ0EwHhcNMDgwNTAzMTEyNDM4WhcNMDgwNTE3\n"
    "MTEyNDQwWjAoMSYwJAYDVQQKEx1HbnVUTFMgaG9zdG5hbWUgY2hlY2sgdGVzdCBD\n"
    "QTCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA0gXBZcu9Hi7re4cHlEyTM/OBg30y\n"
    "G3FOTn/HvL9LL/JJtc+/wLjoKczzYb0uHeToGd3FvS7wNbH9MNf1qHyDmhOevyXt\n"
    "CKYFnntOI1nDDlrzv1TH3NQTV6EPop7Iq3Vm3geEjWitcQTgnL3L9gh6l0L4EJQp\n"
    "AUp+YdcEIQVM8QcCAwEAAaNhMF8wDwYDVR0TAQH/BAUwAwEB/zAYBgNVHREEETAP\n"
    "gg13d3cuZXhhbXBsZS4qMBMGA1UdJQQMMAoGCCsGAQUFBwMBMB0GA1UdDgQWBBRU\n"
    "k+ZZmyg7RSk3iBiu+aSrv02ZGDALBgkqhkiG9w0BAQUDgYEAZ7gLXtXwFW61dSAM\n"
    "0Qt6IN68WBH7LCzetSF8ofG1WVUImCUU3pqXhXYtPGTrswOh2AavWTRbzVTtrFvf\n"
    "WJg09Z7H6I70RPvAYGsK9t9qJ/4TPoYTGYQgsTbVpkv13O54O6jzemd8Zws/xMH5\n"
    "7/q6C7P5OUmGOtfVe7UVDY0taQM=\n" "-----END CERTIFICATE-----\n";

/* Certificate with SAN and CN but for different names. */
char pem9[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 4a827d5c\n"
    "	Issuer: O=GnuTLS hostname check test CA,CN=foo.example.org\n"
    "	Validity:\n"
    "		Not Before: Wed Aug 12 08:29:17 UTC 2009\n"
    "		Not After: Thu Aug 13 08:29:23 UTC 2009\n"
    "	Subject: O=GnuTLS hostname check test CA,CN=foo.example.org\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			bb:66:43:f5:f2:c5:d7:b6:8c:cc:c5:df:f5:88:3b:b1\n"
    "			c9:4b:6a:0e:a1:ad:20:50:40:08:80:a1:4f:5c:a3:d0\n"
    "			f8:6c:cf:e6:3c:f7:ec:04:76:13:17:8b:64:89:22:5b\n"
    "			c0:dd:53:7c:3b:ed:7c:04:bb:80:b9:28:be:8e:9b:c6\n"
    "			8e:a0:a5:12:cb:f5:57:1e:a2:e7:bb:b7:33:49:9f:e3\n"
    "			bb:4a:ae:6a:4d:68:ff:c9:11:e2:32:8d:ce:3d:80:0b\n"
    "			8d:75:ef:d8:00:81:8f:28:04:03:a0:22:8d:61:04:07\n"
    "			fa:b6:37:7d:21:07:49:d2:09:61:69:98:90:a3:58:a9\n"
    "		Exponent (bits 24):\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): TRUE\n"
    "		Subject Alternative Name (not critical):\n"
    "			DNSname: bar.example.org\n"
    "		Key Purpose (not critical):\n"
    "			TLS WWW Server.\n"
    "		Subject Key Identifier (not critical):\n"
    "			4cb90a9bfa1d34e37edecbd20715fea1dacb6891\n"
    "	Signature Algorithm: RSA-SHA\n"
    "	Signature:\n"
    "		a2:1f:d2:90:5f:c9:1c:6f:92:1d:c5:0b:ac:b0:17:23\n"
    "		c5:67:46:94:6f:0f:62:7d:66:4c:28:ff:b7:10:73:60\n"
    "		ae:0e:a2:47:82:83:bb:89:0d:f1:16:5e:f9:5b:35:4b\n"
    "		ce:ee:5e:d0:ad:b5:8b:cc:37:b3:ac:4d:1b:58:c2:4f\n"
    "		1c:7f:c6:ac:3d:25:18:67:37:f0:27:11:9b:2c:20:b6\n"
    "		78:24:21:a6:77:44:e7:1a:e5:f6:bf:45:84:32:81:67\n"
    "		af:8d:96:26:f7:39:31:6b:63:c5:15:9d:e0:a0:9a:1e\n"
    "		96:12:cb:ad:85:cb:a7:d4:86:ac:d8:f5:e9:a4:2b:20\n"
    "Other Information:\n"
    "	MD5 fingerprint:\n"
    "		f27b18092c7497f206e70f504eee0f8e\n"
    "	SHA-1 fingerprint:\n"
    "		bebdac9d0dd54e8f044642e0f065fae5d75ca6e5\n"
    "	Public Key ID:\n"
    "		4cb90a9bfa1d34e37edecbd20715fea1dacb6891\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIICWTCCAcSgAwIBAgIESoJ9XDALBgkqhkiG9w0BAQUwQjEmMCQGA1UEChMdR251\n"
    "VExTIGhvc3RuYW1lIGNoZWNrIHRlc3QgQ0ExGDAWBgNVBAMTD2Zvby5leGFtcGxl\n"
    "Lm9yZzAeFw0wOTA4MTIwODI5MTdaFw0wOTA4MTMwODI5MjNaMEIxJjAkBgNVBAoT\n"
    "HUdudVRMUyBob3N0bmFtZSBjaGVjayB0ZXN0IENBMRgwFgYDVQQDEw9mb28uZXhh\n"
    "bXBsZS5vcmcwgZwwCwYJKoZIhvcNAQEBA4GMADCBiAKBgLtmQ/Xyxde2jMzF3/WI\n"
    "O7HJS2oOoa0gUEAIgKFPXKPQ+GzP5jz37AR2ExeLZIkiW8DdU3w77XwEu4C5KL6O\n"
    "m8aOoKUSy/VXHqLnu7czSZ/ju0quak1o/8kR4jKNzj2AC41179gAgY8oBAOgIo1h\n"
    "BAf6tjd9IQdJ0glhaZiQo1ipAgMBAAGjYzBhMA8GA1UdEwEB/wQFMAMBAf8wGgYD\n"
    "VR0RBBMwEYIPYmFyLmV4YW1wbGUub3JnMBMGA1UdJQQMMAoGCCsGAQUFBwMBMB0G\n"
    "A1UdDgQWBBRMuQqb+h00437ey9IHFf6h2stokTALBgkqhkiG9w0BAQUDgYEAoh/S\n"
    "kF/JHG+SHcULrLAXI8VnRpRvD2J9Zkwo/7cQc2CuDqJHgoO7iQ3xFl75WzVLzu5e\n"
    "0K21i8w3s6xNG1jCTxx/xqw9JRhnN/AnEZssILZ4JCGmd0TnGuX2v0WEMoFnr42W\n"
    "Jvc5MWtjxRWd4KCaHpYSy62Fy6fUhqzY9emkKyA=\n"
    "-----END CERTIFICATE-----\n";

/* Certificate with SAN and CN that match iff you truncate the SAN to
   the embedded NUL.
   See <http://thread.gmane.org/gmane.network.gnutls.general/1735>. */
char pem10[] =
    "X.509 Certificate Information:\n"
    "	Version: 3\n"
    "	Serial Number (hex): 0b5d0a870d09\n"
    "	Issuer: C=NN,O=Edel Curl Arctic Illudium Research Cloud,CN=Nothern Nowhere Trust Anchor\n"
    "	Validity:\n"
    "		Not Before: Tue Aug 04 22:07:33 UTC 2009\n"
    "		Not After: Sat Oct 21 22:07:33 UTC 2017\n"
    "	Subject: C=NN,O=Edel Curl Arctic Illudium Research Cloud,CN=localhost\n"
    "	Subject Public Key Algorithm: RSA\n"
    "		Modulus (bits 1024):\n"
    "			be:67:3b:b4:ea:c0:85:b4:c3:56:c1:a4:96:23:36:f5\n"
    "			c6:77:aa:ad:e5:c1:dd:ce:c1:9a:97:07:dd:16:90:eb\n"
    "			f0:38:b5:95:6b:a6:0f:b9:73:4e:7d:82:57:ab:5f:b5\n"
    "			ba:5c:a0:48:8c:82:77:fd:67:d8:53:44:61:86:a5:06\n"
    "			19:bf:73:51:68:2e:1a:0a:c5:05:39:ca:3d:ca:83:ed\n"
    "			07:fe:ae:b7:73:1d:60:dd:ab:9e:0e:7e:02:f3:68:42\n"
    "			93:27:c8:5f:c5:fa:cb:a9:84:06:2f:f3:66:bd:de:7d\n"
    "			29:82:57:47:e4:a9:df:bf:8b:bc:c0:46:33:5a:7b:87\n"
    "		Exponent (bits 24):\n"
    "			01:00:01\n"
    "	Extensions:\n"
    "		Subject Alternative Name (not critical):\n"
    "warning: SAN contains an embedded NUL, replacing with '!'\n"
    "			DNSname: localhost!h\n"
    "		Key Usage (not critical):\n"
    "			Key encipherment.\n"
    "		Key Purpose (not critical):\n"
    "			TLS WWW Server.\n"
    "		Subject Key Identifier (not critical):\n"
    "			0c37a3db0f73b3388a69d36eb3a7d6d8774eda67\n"
    "		Authority Key Identifier (not critical):\n"
    "			126b24d24a68b7a1b01ccdbfd64ccc405b7fe040\n"
    "		Basic Constraints (critical):\n"
    "			Certificate Authority (CA): FALSE\n"
    "	Signature Algorithm: RSA-SHA\n"
    "	Signature:\n"
    "		88:a0:17:77:77:bf:c1:8a:18:4e:a3:94:6e:45:18:31\n"
    "		fa:2f:7b:1f:ee:95:20:d1:cd:40:df:ee:f0:45:2e:e9\n"
    "		e6:cf:c8:77:bd:85:16:d7:9f:18:52:78:3f:ea:9c:86\n"
    "		62:6e:db:90:b0:cd:f1:c1:6f:2d:87:4a:a0:be:b3:dc\n"
    "		6d:e4:6b:d1:da:b9:10:25:7e:35:1f:1b:aa:a7:09:2f\n"
    "		84:77:27:b0:48:a8:6d:54:57:38:35:22:34:03:0f:d4\n"
    "		5d:ab:1c:72:15:b1:d9:89:56:10:12:fb:7d:0d:18:12\n"
    "		a9:0a:38:dc:93:cf:69:ff:75:86:9e:e3:6b:eb:92:6c\n"
    "		55:16:d5:65:8b:d7:9c:5e:4b:82:c8:92:6c:8b:e6:18\n"
    "		a2:f8:8c:65:aa:b6:eb:23:ed:cb:99:db:fc:8b:8e:1d\n"
    "		7a:39:c9:f5:7b:7f:58:7b:ed:01:6c:3c:40:ec:e3:a9\n"
    "		5f:c4:3d:cb:81:17:03:6d:2d:d7:bd:00:5f:c4:79:f2\n"
    "		fb:ab:c6:0e:a2:01:8b:a1:42:73:de:96:29:3e:bf:d7\n"
    "		d9:51:a7:d4:98:07:7f:f0:f4:cd:00:a1:e1:ac:6c:05\n"
    "		ac:ab:93:1b:b0:5c:2c:13:ad:ff:27:dc:80:99:34:66\n"
    "		bd:e3:31:54:d5:b6:3f:ce:d4:08:a3:52:28:61:5e:bd\n"
    "Other Information:\n"
    "	MD5 fingerprint:\n"
    "		0b4d6d944200cdd1639008b24dc0fe0a\n"
    "	SHA-1 fingerprint:\n"
    "		ce85660f5451b0cc12f525577f0eb9411a20c76b\n"
    "	Public Key ID:\n"
    "		a1d18c15e65c7c4935512eeea7ca5d3e6baad4e1\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQzCCAiugAwIBAgIGC10Khw0JMA0GCSqGSIb3DQEBBQUAMGcxCzAJBgNVBAYT\n"
    "Ak5OMTEwLwYDVQQKDChFZGVsIEN1cmwgQXJjdGljIElsbHVkaXVtIFJlc2VhcmNo\n"
    "IENsb3VkMSUwIwYDVQQDDBxOb3RoZXJuIE5vd2hlcmUgVHJ1c3QgQW5jaG9yMB4X\n"
    "DTA5MDgwNDIyMDczM1oXDTE3MTAyMTIyMDczM1owVDELMAkGA1UEBhMCTk4xMTAv\n"
    "BgNVBAoMKEVkZWwgQ3VybCBBcmN0aWMgSWxsdWRpdW0gUmVzZWFyY2ggQ2xvdWQx\n"
    "EjAQBgNVBAMMCWxvY2FsaG9zdDCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA\n"
    "vmc7tOrAhbTDVsGkliM29cZ3qq3lwd3OwZqXB90WkOvwOLWVa6YPuXNOfYJXq1+1\n"
    "ulygSIyCd/1n2FNEYYalBhm/c1FoLhoKxQU5yj3Kg+0H/q63cx1g3aueDn4C82hC\n"
    "kyfIX8X6y6mEBi/zZr3efSmCV0fkqd+/i7zARjNae4cCAwEAAaOBizCBiDAWBgNV\n"
    "HREEDzANggtsb2NhbGhvc3QAaDALBgNVHQ8EBAMCBSAwEwYDVR0lBAwwCgYIKwYB\n"
    "BQUHAwEwHQYDVR0OBBYEFAw3o9sPc7M4imnTbrOn1th3TtpnMB8GA1UdIwQYMBaA\n"
    "FBJrJNJKaLehsBzNv9ZMzEBbf+BAMAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQEF\n"
    "BQADggEBAIigF3d3v8GKGE6jlG5FGDH6L3sf7pUg0c1A3+7wRS7p5s/Id72FFtef\n"
    "GFJ4P+qchmJu25CwzfHBby2HSqC+s9xt5GvR2rkQJX41HxuqpwkvhHcnsEiobVRX\n"
    "ODUiNAMP1F2rHHIVsdmJVhAS+30NGBKpCjjck89p/3WGnuNr65JsVRbVZYvXnF5L\n"
    "gsiSbIvmGKL4jGWqtusj7cuZ2/yLjh16Ocn1e39Ye+0BbDxA7OOpX8Q9y4EXA20t\n"
    "170AX8R58vurxg6iAYuhQnPelik+v9fZUafUmAd/8PTNAKHhrGwFrKuTG7BcLBOt\n"
    "/yfcgJk0Zr3jMVTVtj/O1AijUihhXr0=\n" "-----END CERTIFICATE-----\n";

char pem_too_many[] = "\n"
    "      Subject: C=BE,CN=******************.gnutls.org\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDljCCAk6gAwIBAgIETcMNdjANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJC\n"
    "RTErMCkGA1UEAxMiKioqKioqKioqKioqKioqKioqKioqKiouZ251dGxzLm9yZzAe\n"
    "Fw0xMTA1MDUyMDQ5NTlaFw02NDAxMTUyMDUwMDJaMDoxCzAJBgNVBAYTAkJFMSsw\n"
    "KQYDVQQDEyIqKioqKioqKioqKioqKioqKioqKioqKi5nbnV0bHMub3JnMIIBUjAN\n"
    "BgkqhkiG9w0BAQEFAAOCAT8AMIIBOgKCATEA3c+X0qUdld2GGNjEua2mDLSdttz6\n"
    "3CHhOmI0B+gzsuiX7ixB0hLxX+3kdv9lJh4Mx0EVaV8N+a2JFI3q1xZSmkfBuwAC\n"
    "5IhFc3ikrts4w8YH0mQOh+10jGvEwAJQfE6m0Vjp5RMJqdta6usPBoBcCe+UyOn7\n"
    "Ny514ayTrZs3E0tmOnYz2MTXTPthyJIhB/zfqYhU5KOpR9JsuOM5iRGIOC2i3D5e\n"
    "SqmkjtUfstDdQTzaEGieRxtlAqLFKHMCgwMJ/fUpfpfcKk5LqnlGRnCGG5u49oq+\n"
    "KYd9X9qll2vvyEMJQ+IfihZ+HVBd9doC7vLDKkjmazDqAtfvrIsMuMGF2L98hage\n"
    "g75cJi55e0f1Sj9mYpL9QSC2LADwUsomBi18z3pQfQ/L3ZcgyG/k4FD04wIDAQAB\n"
    "o0QwQjAMBgNVHRMBAf8EAjAAMBMGA1UdJQQMMAoGCCsGAQUFBwMBMB0GA1UdDgQW\n"
    "BBSSU9ZxufhoqrNT9o31OUVmnKflMTANBgkqhkiG9w0BAQsFAAOCATEAUMK435LP\n"
    "0XpmpWLBBuC6VLLIsAGmXRv7odw8sG9fOctalsbK3zd9pDOaoFI/128GOmlTp1aC\n"
    "n4a/pZ9G5wTKRvdxVqecdYkozDtAS35uwCSQPU/P12Oug6kA4NNJDxF3FGm5eov6\n"
    "SnZDL0Qlhat9y0yOakaOkVNwESAwgUEYClZeR45htvH5oP48XEgwqHQ9jPS2MXAe\n"
    "QLBjqqeYzIvWqwT4z14tIkN0VWWqqVo/dzV+lfNwQy0UL8iWVYnks8wKs2SBkVHx\n"
    "41wBR3uCgCDwlYGDLIG1cm0n7mXrnE7KNcrwQKXL8WGNRAVvx5MVO1vDoWPyQ1Y4\n"
    "sDdnQiVER9ee/KxO6IgCTGh+nCBTSSYgLX2E/m789quPvzyi9Hf/go28he6E3dSK\n"
    "q7/LRSxaZenB/Q==\n" "-----END CERTIFICATE-----\n";

#ifdef ENABLE_OPENPGP
/* Check basic OpenPGP comparison too.
   <http://thread.gmane.org/gmane.comp.encryption.gpg.gnutls.devel/3812>. */
char pem11[] =
    "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
    "Version: GnuPG v1.4.6 (GNU/Linux)\n"
    "\n"
    "mQGiBEXInlgRBAD0teb6ohIlchkHcFlmmvtVW1KXexlDfXExf8T+fOz5z354GPOX\n"
    "sDq98ztCEE3hnPEOFj4NT0X3nEtrvLkhmZqrDHSbuJACB4qxeHwEbGFx7OIDW8+u\n"
    "4sKxpaza1GVf1NQ7VIaQiXaGHy8Esn9SW7oNhK6z5l4TIRlm3OBt3cxU3wCgjnnO\n"
    "jpGJeeo0OnZzSH+xsNLJQEcEAOmUc+7N9OhpT/gqddIgzYRr/FD0Ad6HBfABol6Q\n"
    "wWCapzIxggnZJ9i+lHujpcA8idtrBU/DGhkGtW95QaHwQ8d5SvetM7Wc/xoHEP3o\n"
    "HGvSGoXtfqlofastcC7eso39EBD10cpIB+gUmhe1MpaXm7A6m+KJO+2CkqE1vMkc\n"
    "tmKHBACzDRrWgkV+AtGWKl3ge9RkYHKxAPc0FBrpzDrvmvvNMaIme2u/+WP/xa4T\n"
    "nTjgys+pfeplHVfCO/n6nKWrVepMPE0+ZeNWzY6CsfhL7VjSN99vm7qzNHswBiJS\n"
    "gCSwJXRmQcJcS9hxqLciUyVEB32zPqX24QHnsyPYaSCzEBgOnLQPdGVzdC5nbnV0\n"
    "bHMub3JniF8EExECACAFAkXInlgCGwMGCwkIBwMCBBUCCAMEFgIDAQIeAQIXgAAK\n"
    "CRCuX60+XR0U2FcfAJ9eZDmhk5a9k4K/zu+a5xFwb9SWsgCXTkDnOIQmueZPHg5U\n"
    "VgKnazckK7kCDQRFyJ51EAgAozi9Vk9R5I2AtRcqV4jLfpzh3eiBYSUt4U3ZLxff\n"
    "LAyvGMUXA7OATGGhuKphNQLux17AGpRN4nugnIWMLE9akyrxXqg/165UFKbwwVsl\n"
    "po7KzPvEXHmOYDgVEqS0sZNWmkJeMPdCVsD2wifPkocufUu2Ux8CmrvT1nEgoiVu\n"
    "kUjplJOralQBdsPkIEk8LMVtF3IW2aHCEET0yrJ2Y2q0i/u1K4bxSUi5ESrN0UNa\n"
    "WT7wtCegdwWlObwJEgwcu/8YtjMnfBI855gXVdJiRLdOJvkU+65I/jnPQG5QEIQM\n"
    "weLty/+GHkXVN2xw5OGUIryIPUHi8+EDGOGqoxqNUMTzvwADBQf/bTPc0z3oHp+X\n"
    "hsj3JP/AMCSQV87peKqFYEnRIubsN4Y4tTwVjEkRA3s5u+qTNvdypE1tvAEmdspa\n"
    "CL/EKfMCEltcW3WUwqUIULQ2Z0t9tBuVfMEH1Z1jjb68IOVwTJYz+iBtmbq5Wxoq\n"
    "lc5woOCDVL9qaKR6hOuAukTl6L3wQL+5zGBE4k5UfLf8UVJEa4ZTqsoMi3iyQAFO\n"
    "/h7WzqUATH3aQSz9tpilJ760wadDhc+Sdt2a0W6cC+SBmJaU/ym9seTd26nyWHG+\n"
    "03G+ynCHf5pBAXHhfCNhA0lMv5h3eJECNElcCh0sYGmo19jOzbnlRSGKRqrflOtO\n"
    "YwhQXK9y/ohJBBgRAgAJBQJFyJ51AhsMAAoJEK5frT5dHRTYDDgAn2bLaS5n3Xy8\n"
    "Z/V2Me1st/9pqPfZAJ4+9YBnyjCq/0vosIoZabi+s92m7g==\n"
    "=NkXV\n" "-----END PGP PUBLIC KEY BLOCK-----\n";
#endif

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

char multi_cns[] = "\n"
	"Subject: CN=www.example.com,CN=www.example2.com,CN=www.example3.com\n"
	"\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDXzCCAkegAwIBAgIMU+p6uAg2JlqRhAbAMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
	"BgNVBAMTBENBLTAwIhgPMjAxNDA4MTIyMDM2MDhaGA85OTk5MTIzMTIzNTk1OVow\n"
	"UDEYMBYGA1UEAxMPd3d3LmV4YW1wbGUuY29tMRkwFwYDVQQDExB3d3cuZXhhbXBs\n"
	"ZTIuY29tMRkwFwYDVQQDExB3d3cuZXhhbXBsZTMuY29tMIIBIjANBgkqhkiG9w0B\n"
	"AQEFAAOCAQ8AMIIBCgKCAQEAqP5QQUqIS2lquM8hYbDHljqHBDWlGtr167DDPwix\n"
	"oIlnq84Xr1zI5zpJ2t/3U5kGTbRJiVroQCh3cVhiQyGTPSJPK+CJGi3diw5Vc2rK\n"
	"oAPxaFtaxvE36mLLH2SSuc49b6hhlRpXdWE0TgnsvJojL5V20/CZI23T27fl+DjT\n"
	"MduU92qH8wdCgp7q3sHZvtvTZuFM+edYvKZjhUz8P7JwiamG0A2UH+NiyicdAOxc\n"
	"+lfwfoyetJdTHLfwxdCXT4X91xGd9eOW9lIL5BqLuAArODTcmHDmiXpXEO/sEyHq\n"
	"L96Eawjon0Gz4IRNq7/kwDjSPJOIN0GHq6DtNmXl6J0C5wIDAQABo3YwdDAMBgNV\n"
	"HRMBAf8EAjAAMBMGA1UdJQQMMAoGCCsGAQUFBwMBMA8GA1UdDwEB/wQFAwMHoAAw\n"
	"HQYDVR0OBBYEFH6NTStc4XH/M74Meat1sT2o53fUMB8GA1UdIwQYMBaAFK8aMLKE\n"
	"hAwWmkzQxRkQ1/efnumUMA0GCSqGSIb3DQEBCwUAA4IBAQBdHknM+rddB0ET+UI2\n"
	"Or8qSNjkqBHwsZqb4hJozXFS35a1CJPQuxPzY13eHpiIfmdWL2EpKnLOU8vtAW9e\n"
	"qpozMGDyrAuZhxsXUtInbF15C+Yuw9/sqCPK44b5DCtDf6J/N8m8FvdwqO803z1D\n"
	"MGcSpES5I68+N3dwSRFYNpSLA1ul5MSlnmoffml959kx9hZNcI4N/UqkO1LMCKXX\n"
	"Nf8kGFyLdPjANcIwL5sqP+Dp4HP3wdf7Ny+KFCZ6zDbpa53gb3G0naMdllK8BMfI\n"
	"AQ4Y07zSA4K1QMdxeqaMgPIcCDLoKiMXAXNa42+K04F6SOkTjsVx9b5m0oynLt0u\n"
	"MUjE\n"
	"-----END CERTIFICATE-----\n";

char txt_ip_in_names[] = 
	"Subject: CN=172.15.1.1\n"
	"Subject Alternative Name (not critical):\n"
	"	DNSname: 172.15.2.1\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIEJDCCAoygAwIBAgIMWQXA/TIEZUXpwL2dMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
	"BgNVBAMTBENBLTEwIBcNMTcwNDMwMTA0ODI5WhgPOTk5OTEyMzEyMzU5NTlaMBUx\n"
	"EzARBgNVBAMTCjE3Mi4xNS4xLjEwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGK\n"
	"AoIBgQDP3DsT65aY+fHi6FivWbypF71T9UjAGYcho7XXPUCvBr6xQbyERZjb08qn\n"
	"RPvVDaiLeDfVve44gSho70t+fxMsCYZqkf9HN4aUzuxx2fHgMBCwyrhgm9zZ/zgA\n"
	"D92oXOPem2mKNjPavXtthqvgvwu6HmpJDd+YYR7FFbkgZswrqjd+lg0z+PGt5Xee\n"
	"LW3amPZINyc5Rai+LMlYIU29YK9G+CM3XVPQ8ygsQva+4/YoU1DVQRXFYTO1ERdn\n"
	"QDV9kmJKvQOxbjchNkLLMdBWee/WpJtBDE4KcidAsbd/6eUIINVAD7Nm5uE39mDv\n"
	"2ld4vup4j4A5dQNVhUd6iIYfkkwp9NnGMNGpgvSudPSHH8sFlfxXD8ysbD2wHeXL\n"
	"S0Q4Ejypij7tEzy5KdUWqft1QqClHawc2hZ9KKnCHW3xoUsAWxcTIlsgqUUJOkXR\n"
	"Qij2N+0SKrn6M6DSOiklCCunLUCUCceM7fiwYndhNFm5YvZq+m+Afnvxk5V7RnBu\n"
	"DLoxPxkCAwEAAaN4MHYwDAYDVR0TAQH/BAIwADAVBgNVHREEDjAMggoxNzIuMTUu\n"
	"Mi4xMA8GA1UdDwEB/wQFAwMHoAAwHQYDVR0OBBYEFFqnqgPcjDWmHm0PJNxKNeEx\n"
	"Msk1MB8GA1UdIwQYMBaAFOnh1ZODb8QbrqHBHpWyyrEVTpanMA0GCSqGSIb3DQEB\n"
	"CwUAA4IBgQArsZSxJdZ1W+y3m+y6f1Me3FB/XUscpHQ9cS0wlaikeqBvIru5zp7U\n"
	"tLT8qRS7Q8fxsL6LWiOmW5Izi4A51DYJQ9bUEqSIbp9SIV78u5v0oO1bnb7d5SV+\n"
	"BZm/zYuox2uTT9PSoB+iqQXUJ7brWdKe0NdPAzRpM928CqWJLPw0gn41GOIPN6wS\n"
	"IH29CvqRABkxzIsI8IcxHb3/F+DxTnq6aICoWe2XPeL+RqB7moP6YAC9W/r+hds2\n"
	"m8Gok+rGuG3VXk2vc/j1LRnGZfpCQV2L7e7b5eLyQ2Ce46fnxkQSTt4tc0//FTfr\n"
	"6X9624hAOV6MSlkPHNBwVE42z8KsxJfPxeHX+YzFBXqBiQ/r/TvOHDt5Tsny6lXh\n"
	"TDqlJ3NwdS/K9PAlLqhDiZwwakUS9lEY6IC7biP7mxNM8npzlqogfS07XTJgGxgb\n"
	"FtcITJKW0NPA8cnyEAt9jcgaDWw/xbVV+pIytFuGL8pjHEQ4H9Ymu6ifLNlkyu/e\n"
	"3XYCeqo17QE=\n"
	"-----END CERTIFICATE-----\n";

char txt_ip_in_cn[] =
	"CN=172.15.1.1\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIICCDCCAXGgAwIBAgIMWQXCYQfV3T9BXL4hMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
	"BgNVBAMTBENBLTEwIBcNMTcwNDMwMTA1NDI1WhgPOTk5OTEyMzEyMzU5NTlaMBUx\n"
	"EzARBgNVBAMTCjE3Mi4xNS4xLjEwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGB\n"
	"AM5hibrtVPezTZ018YR3DG+r71pTmfxVD1hWMAywQTMdut11Cg16dBtU/WJ6X3YF\n"
	"b3MAtrJf7eHnaxPneY7j590eOcqiDmb0skUATuZrX4Su0QMP4ygTcXlzMAxOFYwQ\n"
	"pd3d9LQiUxCVlg7fPI7BiqyWA1igBB34OaVbV0GHuJBVAgMBAAGjYTBfMAwGA1Ud\n"
	"EwEB/wQCMAAwDwYDVR0PAQH/BAUDAwegADAdBgNVHQ4EFgQUSXWLgTdjnYj1kv1g\n"
	"TEGZep6b0MMwHwYDVR0jBBgwFoAU3rLZPebH2OG+u4iAlJ+zbDif4GYwDQYJKoZI\n"
	"hvcNAQELBQADgYEAifPWTjcErYbxCqRZW5JhwaosOFHCJVboPsLrIM8W0HEJgqet\n"
	"TwarBBiE0mzQKU3GtjGj1ZSxUI/jBg9bzC+fs25VtdlC9nIxi5tSDI/HOoBBgXNr\n"
	"f0+Un2eHAxFcRZPWdPy1/mn83NUMnjquuA/HHcju+pcoZrEwAI3PPQHgsGQ=\n"
	"-----END CERTIFICATE-----\n";


void doit(void)
{
	gnutls_x509_crt_t x509;
#ifdef ENABLE_OPENPGP
	gnutls_openpgp_crt_t pgp;
#endif
	gnutls_datum_t data;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("global_init: %d\n", ret);

	ret = gnutls_x509_crt_init(&x509);
	if (ret < 0)
		fail("gnutls_x509_crt_init: %d\n", ret);

#ifdef ENABLE_OPENPGP
	ret = gnutls_openpgp_crt_init(&pgp);
	if (ret < 0)
		fail("gnutls_openpgp_crt_init: %d\n", ret);
#endif
	if (debug)
		success("Testing wildcards...\n");
	data.data = (unsigned char *) wildcards;
	data.size = strlen(wildcards);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example.net");
	if (ret==0)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem1...\n");
	data.data = (unsigned char *) pem1;
	data.size = strlen(pem1);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem2...\n");
	data.data = (unsigned char *) pem2;
	data.size = strlen(pem2);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "*.example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem3...\n");
	data.data = (unsigned char *) pem3;
	data.size = strlen(pem3);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "*.example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem4...\n");
	data.data = (unsigned char *) pem4;
	data.size = strlen(pem4);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname2(x509, "www.example.org", GNUTLS_VERIFY_DO_NOT_ALLOW_WILDCARDS);
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo.example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

#ifdef SUPPORT_COMPLEX_WILDCARDS
	if (debug)
		success("Testing pem6...\n");
	data.data = (unsigned char *) pem6;
	data.size = strlen(pem6);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo.example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "bar.foo.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem7...\n");
	data.data = (unsigned char *) pem7;
	data.size = strlen(pem7);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo.bar.example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret =
	    gnutls_x509_crt_check_hostname(x509, "foobar.bar.example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foobar.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	ret =
	    gnutls_x509_crt_check_hostname(x509, "foobazbar.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);
#endif

	if (debug)
		success("Testing pem8...\n");
	data.data = (unsigned char *) pem8;
	data.size = strlen(pem8);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	/* this was passing in old gnutls versions, but that was not a
	 * good idea. See http://permalink.gmane.org/gmane.comp.encryption.gpg.gnutls.devel/7380
	 * for a discussion. */
	ret = gnutls_x509_crt_check_hostname(x509, "www.example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example.");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	/* this was passing in old gnutls versions, but that was not a
	 * good idea. See http://permalink.gmane.org/gmane.comp.encryption.gpg.gnutls.devel/7380
	 * for a discussion. */
	ret = gnutls_x509_crt_check_hostname(x509, "www.example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example.foo.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem9...\n");
	data.data = (unsigned char *) pem9;
	data.size = strlen(pem9);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "foo.example.org");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "bar.example.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem10...\n");
	data.data = (unsigned char *) pem10;
	data.size = strlen(pem10);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "localhost");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing pem_too_many...\n");
	data.data = (unsigned char *) pem_too_many;
	data.size = strlen(pem_too_many);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret =
	    gnutls_x509_crt_check_hostname(x509,
					   "localhost.gnutls.gnutls.org");
	if (ret)
		fail("%d: Hostname verification should have failed (too many wildcards)\n", __LINE__);

	if (debug)
		success("Testing pem-ips...\n");
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

	/* test that we don't fallback to CN matching if a supported SAN (IP addresses
	 * in that case) is found. */
	ret = gnutls_x509_crt_check_hostname(x509, "server-0");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	/* test flag GNUTLS_VERIFY_DO_NOT_ALLOW_IP_MATCHES */
	ret = gnutls_x509_crt_check_hostname2(x509, "127.0.0.1", GNUTLS_VERIFY_DO_NOT_ALLOW_IP_MATCHES);
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname2(x509, "::1", GNUTLS_VERIFY_DO_NOT_ALLOW_IP_MATCHES);
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname2(x509, "127.0.0.2", GNUTLS_VERIFY_DO_NOT_ALLOW_IP_MATCHES);
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing multi-cns...\n");
	data.data = (unsigned char *) multi_cns;
	data.size = strlen(multi_cns);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example2.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "www.example3.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing textual IPs...\n");
	data.data = (unsigned char *) txt_ip_in_names;
	data.size = strlen(txt_ip_in_names);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "172.15.1.1");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "172.15.2.1");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	if (debug)
		success("Testing textual IPs (CN)...\n");
	data.data = (unsigned char *) txt_ip_in_cn;
	data.size = strlen(txt_ip_in_cn);

	ret = gnutls_x509_crt_import(x509, &data, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("%d: gnutls_x509_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "example.com");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

	ret = gnutls_x509_crt_check_hostname(x509, "172.15.1.1");
	if (ret)
		fail("%d: Hostname incorrectly matches (%d)\n", __LINE__, ret);

#ifdef ENABLE_OPENPGP
	if (debug)
		success("Testing pem11...\n");
	data.data = (unsigned char *) pem11;
	data.size = strlen(pem11);

	ret =
	    gnutls_openpgp_crt_import(pgp, &data,
				      GNUTLS_OPENPGP_FMT_BASE64);
	if (ret < 0)
		fail("%d: gnutls_openpgp_crt_import: %d\n", __LINE__, ret);

	ret = gnutls_openpgp_crt_check_hostname(pgp, "test.gnutls.org");
	if (!ret)
		fail("%d: Hostname incorrectly does not match (%d)\n", __LINE__, ret);

	gnutls_openpgp_crt_deinit(pgp);
#endif
	gnutls_x509_crt_deinit(x509);

	gnutls_global_deinit();
}
