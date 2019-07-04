/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/ocsp.h>
#include <gnutls/x509.h>

#include "utils.h"

/* Tests the OCSP request and response parsing in gnutls.
 * If this test fails, you most probably need to update your
 * libtasn1 */

static time_t _then = 1332548220;

static time_t mytime(time_t * t)
{

	if (t)
		*t = _then;

	return _then;
}

/* sample request */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

#define REQ1 "\x30\x67\x30\x65\x30\x3e\x30\x3c\x30\x3a\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14\x13\x9d\xa0\x9e\xf4\x32\xab\x8f\xe2\x89\x56\x67\xfa\xd0\xd4\xe3\x35\x86\x71\xb9\x04\x14\x5d\xa7\xdd\x70\x06\x51\x32\x7e\xe7\xb6\x6d\xb3\xb5\xe5\xe0\x60\xea\x2e\x4d\xef\x02\x01\x1d\xa2\x23\x30\x21\x30\x1f\x06\x09\x2b\x06\x01\x05\x05\x07\x30\x01\x02\x04\x12\x04\x10\x35\xc5\xe3\x50\xc3\xcf\x04\x33\xcc\x9e\x06\x3a\x9a\x18\x80\xcc"

static const gnutls_datum_t req1 =
    { (unsigned char *) REQ1, sizeof(REQ1) - 1 };

#define REQ1INFO							\
  "OCSP Request Information:\n"						\
  "	Version: 1\n"							\
  "	Request List:\n"						\
  "		Certificate ID:\n"					\
  "			Hash Algorithm: SHA1\n"				\
  "			Issuer Name Hash: 139da09ef432ab8fe2895667fad0d4e3358671b9\n" \
  "			Issuer Key Hash: 5da7dd700651327ee7b66db3b5e5e060ea2e4def\n" \
  "			Serial Number: 1d\n"				\
  "	Extensions:\n"							\
  "		Nonce: 35c5e350c3cf0433cc9e063a9a1880cc\n"

#define REQ1NONCE "\x04\x10\x35\xc5\xe3\x50\xc3\xcf\x04\x33\xcc\x9e\x06\x3a\x9a\x18\x80\xcc"

#define REQ1INH "\x13\x9d\xa0\x9e\xf4\x32\xab\x8f\xe2\x89\x56\x67\xfa\xd0\xd4\xe3\x35\x86\x71\xb9"
#define REQ1IKH "\x5d\xa7\xdd\x70\x06\x51\x32\x7e\xe7\xb6\x6d\xb3\xb5\xe5\xe0\x60\xea\x2e\x4d\xef"
#define REQ1SN "\x1d"

/* sample response */

#define RESP1 "\x30\x03\x0a\x01\x01"

static const gnutls_datum_t resp1 =
    { (unsigned char *) RESP1, sizeof(RESP1) - 1 };

#define RESP1INFO							\
  "OCSP Response Information:\n"					\
  "	Response Status: malformedRequest\n"

#define RESP2 "\x30\x82\x06\x8C\x0A\x01\x00\xA0\x82\x06\x85\x30\x82\x06\x81\x06\x09\x2B\x06\x01\x05\x05\x07\x30\x01\x01\x04\x82\x06\x72\x30\x82\x06\x6E\x30\x82\x01\x07\xA1\x69\x30\x67\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x43\x48\x31\x19\x30\x17\x06\x03\x55\x04\x0A\x13\x10\x4C\x69\x6E\x75\x78\x20\x73\x74\x72\x6F\x6E\x67\x53\x77\x61\x6E\x31\x1F\x30\x1D\x06\x03\x55\x04\x0B\x13\x16\x4F\x43\x53\x50\x20\x53\x69\x67\x6E\x69\x6E\x67\x20\x41\x75\x74\x68\x6F\x72\x69\x74\x79\x31\x1C\x30\x1A\x06\x03\x55\x04\x03\x13\x13\x6F\x63\x73\x70\x2E\x73\x74\x72\x6F\x6E\x67\x73\x77\x61\x6E\x2E\x6F\x72\x67\x18\x0F\x32\x30\x31\x31\x30\x39\x32\x37\x30\x39\x35\x34\x32\x38\x5A\x30\x64\x30\x62\x30\x3A\x30\x09\x06\x05\x2B\x0E\x03\x02\x1A\x05\x00\x04\x14\x13\x9D\xA0\x9E\xF4\x32\xAB\x8F\xE2\x89\x56\x67\xFA\xD0\xD4\xE3\x35\x86\x71\xB9\x04\x14\x5D\xA7\xDD\x70\x06\x51\x32\x7E\xE7\xB6\x6D\xB3\xB5\xE5\xE0\x60\xEA\x2E\x4D\xEF\x02\x01\x1D\x80\x00\x18\x0F\x32\x30\x31\x31\x30\x39\x32\x37\x30\x39\x35\x34\x32\x38\x5A\xA0\x11\x18\x0F\x32\x30\x31\x31\x30\x39\x32\x37\x30\x39\x35\x39\x32\x38\x5A\xA1\x23\x30\x21\x30\x1F\x06\x09\x2B\x06\x01\x05\x05\x07\x30\x01\x02\x04\x12\x04\x10\x16\x89\x7D\x91\x3A\xB5\x25\xA4\x45\xFE\xC9\xFD\xC2\xE5\x08\xA4\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05\x05\x00\x03\x82\x01\x01\x00\x4E\xAD\x6B\x2B\xF7\xF2\xBF\xA9\x23\x1E\x3A\x0B\x06\xDB\x55\x53\x2B\x64\x54\x11\x32\xBF\x60\xF7\x4F\xE0\x8E\x9B\xA0\xA2\x4C\x79\xC3\x2A\xE0\x43\xF7\x40\x1A\xDC\xB9\xB4\x25\xEF\x48\x01\x97\x8C\xF5\x1E\xDB\xD1\x30\x37\x73\x69\xD6\xA7\x7A\x2D\x8E\xDE\x5C\xAA\xEA\x39\xB9\x52\xAA\x25\x1E\x74\x7D\xF9\x78\x95\x8A\x92\x1F\x98\x21\xF4\x60\x7F\xD3\x28\xEE\x47\x9C\xBF\xE2\x5D\xF6\x3F\x68\x0A\xD6\xFF\x08\xC1\xDC\x95\x1E\x29\xD7\x3E\x85\xD5\x65\xA4\x4B\xC0\xAF\xC3\x78\xAB\x06\x98\x88\x19\x8A\x64\xA6\x83\x91\x87\x13\xDB\x17\xCC\x46\xBD\xAB\x4E\xC7\x16\xD1\xF8\x35\xFD\x27\xC8\xF6\x6B\xEB\x37\xB8\x08\x6F\xE2\x6F\xB4\x7E\xD5\x68\xDB\x7F\x5D\x5E\x36\x38\xF2\x77\x59\x13\xE7\x3E\x4D\x67\x5F\xDB\xA2\xF5\x5D\x7C\xBF\xBD\xB5\x37\x33\x51\x36\x63\xF8\x21\x1E\xFC\x73\x8F\x32\x69\xBB\x97\xA7\xBD\xF1\xB6\xE0\x40\x09\x68\xEA\xD5\x93\xB8\xBB\x39\x8D\xA8\x16\x1B\xBF\x04\x7A\xBC\x18\x43\x01\xE9\x3C\x19\x5C\x4D\x4B\x98\xD8\x23\x37\x39\xA4\xC4\xDD\xED\x9C\xEC\x37\xAB\x66\x44\x9B\xE7\x5B\x5D\x32\xA2\xDB\xA6\x0B\x3B\x8C\xE1\xF5\xDB\xCB\x7D\x58\xA0\x82\x04\x4B\x30\x82\x04\x47\x30\x82\x04\x43\x30\x82\x03\x2B\xA0\x03\x02\x01\x02\x02\x01\x1E\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x0B\x05\x00\x30\x45\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x43\x48\x31\x19\x30\x17\x06\x03\x55\x04\x0A\x13\x10\x4C\x69\x6E\x75\x78\x20\x73\x74\x72\x6F\x6E\x67\x53\x77\x61\x6E\x31\x1B\x30\x19\x06\x03\x55\x04\x03\x13\x12\x73\x74\x72\x6F\x6E\x67\x53\x77\x61\x6E\x20\x52\x6F\x6F\x74\x20\x43\x41\x30\x1E\x17\x0D\x30\x39\x31\x31\x32\x34\x31\x32\x35\x31\x35\x33\x5A\x17\x0D\x31\x34\x31\x31\x32\x33\x31\x32\x35\x31\x35\x33\x5A\x30\x67\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x43\x48\x31\x19\x30\x17\x06\x03\x55\x04\x0A\x13\x10\x4C\x69\x6E\x75\x78\x20\x73\x74\x72\x6F\x6E\x67\x53\x77\x61\x6E\x31\x1F\x30\x1D\x06\x03\x55\x04\x0B\x13\x16\x4F\x43\x53\x50\x20\x53\x69\x67\x6E\x69\x6E\x67\x20\x41\x75\x74\x68\x6F\x72\x69\x74\x79\x31\x1C\x30\x1A\x06\x03\x55\x04\x03\x13\x13\x6F\x63\x73\x70\x2E\x73\x74\x72\x6F\x6E\x67\x73\x77\x61\x6E\x2E\x6F\x72\x67\x30\x82\x01\x22\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01\x05\x00\x03\x82\x01\x0F\x00\x30\x82\x01\x0A\x02\x82\x01\x01\x00\xBC\x05\x3E\x4B\xBE\xC6\xB1\x33\x48\x0E\xC3\xD4\x0C\xEF\x83\x0B\xBD\xBC\x57\x5F\x14\xEF\xF5\x6D\x0B\xFF\xFA\x01\x9C\xFA\x21\x6D\x5C\xAE\x79\x29\x74\xFE\xBD\xAB\x70\x87\x98\x6B\x48\x35\x79\xE3\xE0\xC1\x14\x41\x1F\x0A\xF7\xE7\xA3\xA6\xDA\x6B\xFF\xCD\x74\xE9\x95\x00\x38\xAA\xD6\x3A\x60\xC6\x64\xA1\xE6\x02\x39\x58\x4E\xFD\xF2\x78\x08\x63\xB6\xD7\x7A\x96\x79\x62\x18\x39\xEE\x27\x8D\x3B\xA2\x3D\x48\x88\xDB\x43\xD6\x6A\x77\x20\x6A\x27\x39\x50\xE0\x02\x50\x19\xF2\x7A\xCF\x78\x23\x99\x01\xD4\xE5\xB1\xD1\x31\xE6\x6B\x84\xAF\xD0\x77\x41\x46\x85\xB0\x3B\xE6\x6A\x00\x0F\x3B\x7E\x95\x7F\x59\xA8\x22\xE8\x49\x49\x05\xC8\xCB\x6C\xEE\x47\xA7\x2D\xC9\x74\x5B\xEB\x8C\xD5\x99\xC2\xE2\x70\xDB\xEA\x87\x43\x84\x0E\x4F\x83\x1C\xA6\xEB\x1F\x22\x38\x17\x69\x9B\x72\x12\x95\x48\x71\xB2\x7B\x92\x73\x52\xAB\xE3\x1A\xA5\xD3\xF4\x44\x14\xBA\xC3\x35\xDA\x91\x6C\x7D\xB4\xC2\x00\x07\xD8\x0A\x51\xF1\x0D\x4C\xD9\x7A\xD1\x99\xE6\xA8\x8D\x0A\x80\xA8\x91\xDD\x8A\xA2\x6B\xF6\xDB\xB0\x3E\xC9\x71\xA9\xE0\x39\xC3\xA3\x58\x0D\x87\xD0\xB2\xA7\x9C\xB7\x69\x02\x03\x01\x00\x01\xA3\x82\x01\x1A\x30\x82\x01\x16\x30\x09\x06\x03\x55\x1D\x13\x04\x02\x30\x00\x30\x0B\x06\x03\x55\x1D\x0F\x04\x04\x03\x02\x03\xA8\x30\x1D\x06\x03\x55\x1D\x0E\x04\x16\x04\x14\x34\x91\x6E\x91\x32\xBF\x35\x25\x43\xCC\x28\x74\xEF\x82\xC2\x57\x92\x79\x13\x73\x30\x6D\x06\x03\x55\x1D\x23\x04\x66\x30\x64\x80\x14\x5D\xA7\xDD\x70\x06\x51\x32\x7E\xE7\xB6\x6D\xB3\xB5\xE5\xE0\x60\xEA\x2E\x4D\xEF\xA1\x49\xA4\x47\x30\x45\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x43\x48\x31\x19\x30\x17\x06\x03\x55\x04\x0A\x13\x10\x4C\x69\x6E\x75\x78\x20\x73\x74\x72\x6F\x6E\x67\x53\x77\x61\x6E\x31\x1B\x30\x19\x06\x03\x55\x04\x03\x13\x12\x73\x74\x72\x6F\x6E\x67\x53\x77\x61\x6E\x20\x52\x6F\x6F\x74\x20\x43\x41\x82\x01\x00\x30\x1E\x06\x03\x55\x1D\x11\x04\x17\x30\x15\x82\x13\x6F\x63\x73\x70\x2E\x73\x74\x72\x6F\x6E\x67\x73\x77\x61\x6E\x2E\x6F\x72\x67\x30\x13\x06\x03\x55\x1D\x25\x04\x0C\x30\x0A\x06\x08\x2B\x06\x01\x05\x05\x07\x03\x09\x30\x39\x06\x03\x55\x1D\x1F\x04\x32\x30\x30\x30\x2E\xA0\x2C\xA0\x2A\x86\x28\x68\x74\x74\x70\x3A\x2F\x2F\x63\x72\x6C\x2E\x73\x74\x72\x6F\x6E\x67\x73\x77\x61\x6E\x2E\x6F\x72\x67\x2F\x73\x74\x72\x6F\x6E\x67\x73\x77\x61\x6E\x2E\x63\x72\x6C\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x0B\x05\x00\x03\x82\x01\x01\x00\x6D\x78\xD7\x66\x90\xA6\xEB\xDD\xB5\x09\x48\xA4\xDA\x27\xFA\xAC\xB1\xBC\x8F\x8C\xBE\xCC\x8C\x09\xA2\x40\x0D\x6C\x4A\xAE\x72\x22\x1E\xC8\xAF\x6D\xF1\x12\xAF\xD7\x40\x51\x79\xD4\xDD\xB2\x0C\xDB\x97\x84\xB6\x24\xD5\xF5\xA8\xBB\xC0\x4B\xF9\x7F\x71\xF7\xB0\x65\x42\x4A\x7D\xFE\x76\x7E\x05\xD2\x46\xB8\x7D\xB3\x39\x4C\x5C\xB1\xFA\xB9\xEE\x3B\x70\x33\x39\x57\x1A\xB9\x95\x51\x33\x00\x25\x1B\x4C\xAA\xB4\xA7\x55\xAF\x63\x6D\x6F\x88\x17\x6A\x7F\xB0\x97\xDE\x49\x14\x6A\x27\x6A\xB0\x42\x80\xD6\xA6\x9B\xEF\x04\x5E\x11\x7D\xD5\x8E\x54\x20\xA2\x76\xD4\x66\x58\xAC\x9C\x12\xD3\xF5\xCA\x54\x98\xCA\x21\xEC\xC1\x55\xA1\x2F\x68\x0B\x5D\x04\x50\xD2\x5E\x70\x25\xD8\x13\xD9\x44\x51\x0E\x8A\x42\x08\x18\x84\xE6\x61\xCE\x5A\x7D\x7B\x81\x35\x90\xC3\xD4\x9D\x19\xB6\x37\xEE\x8F\x63\x5C\xDA\xD8\xF0\x64\x60\x39\xEB\x9B\x1C\x54\x66\x75\x76\xB5\x0A\x58\xB9\x3F\x91\xE1\x21\x9C\xA0\x50\x15\x97\xB6\x7E\x41\xBC\xD0\xC4\x21\x4C\xF5\xD7\xF0\x13\xF8\x77\xE9\x74\xC4\x8A\x0E\x20\x17\x32\xAE\x38\xC2\xA5\xA8\x62\x85\x17\xB1\xA2\xD3\x22\x9F\x95\xB7\xA3\x4C"

#define RESP2INFO				\
  "OCSP Response Information:\n"		\
  "	Response Status: Successful\n"		\
  "	Response Type: Basic OCSP Response\n"	\
  "	Version: 1\n" \
  "	Responder ID: CN=ocsp.strongswan.org,OU=OCSP Signing Authority,O=Linux strongSwan,C=CH\n" \
  "	Produced At: Tue Sep 27 09:54:28 UTC 2011\n" \
  "	Responses:\n" \
  "		Certificate ID:\n" \
  "			Hash Algorithm: SHA1\n" \
  "			Issuer Name Hash: 139da09ef432ab8fe2895667fad0d4e3358671b9\n" \
  "			Issuer Key Hash: 5da7dd700651327ee7b66db3b5e5e060ea2e4def\n" \
  "			Serial Number: 1d\n" \
  "		Certificate Status: good\n" \
  "		This Update: Tue Sep 27 09:54:28 UTC 2011\n" \
  "		Next Update: Tue Sep 27 09:59:28 UTC 2011\n" \
  "	Extensions:\n" \
  "		Nonce: 16897d913ab525a445fec9fdc2e508a4\n" \
  "	Signature Algorithm: RSA-SHA1\n" \
  "	Signature:\n" \
  "		4e:ad:6b:2b:f7:f2:bf:a9:23:1e:3a:0b:06:db:55:53\n" \
  "		2b:64:54:11:32:bf:60:f7:4f:e0:8e:9b:a0:a2:4c:79\n" \
  "		c3:2a:e0:43:f7:40:1a:dc:b9:b4:25:ef:48:01:97:8c\n" \
  "		f5:1e:db:d1:30:37:73:69:d6:a7:7a:2d:8e:de:5c:aa\n" \
  "		ea:39:b9:52:aa:25:1e:74:7d:f9:78:95:8a:92:1f:98\n" \
  "		21:f4:60:7f:d3:28:ee:47:9c:bf:e2:5d:f6:3f:68:0a\n" \
  "		d6:ff:08:c1:dc:95:1e:29:d7:3e:85:d5:65:a4:4b:c0\n" \
  "		af:c3:78:ab:06:98:88:19:8a:64:a6:83:91:87:13:db\n" \
  "		17:cc:46:bd:ab:4e:c7:16:d1:f8:35:fd:27:c8:f6:6b\n" \
  "		eb:37:b8:08:6f:e2:6f:b4:7e:d5:68:db:7f:5d:5e:36\n" \
  "		38:f2:77:59:13:e7:3e:4d:67:5f:db:a2:f5:5d:7c:bf\n" \
  "		bd:b5:37:33:51:36:63:f8:21:1e:fc:73:8f:32:69:bb\n" \
  "		97:a7:bd:f1:b6:e0:40:09:68:ea:d5:93:b8:bb:39:8d\n" \
  "		a8:16:1b:bf:04:7a:bc:18:43:01:e9:3c:19:5c:4d:4b\n" \
  "		98:d8:23:37:39:a4:c4:dd:ed:9c:ec:37:ab:66:44:9b\n" \
  "		e7:5b:5d:32:a2:db:a6:0b:3b:8c:e1:f5:db:cb:7d:58\n"
  /* cut */

static const gnutls_datum_t resp2 =
    { (unsigned char *) RESP2, sizeof(RESP2) - 1 };

#define RESP3 "\x30\x82\x01\xd3\x0a\x01\x00\xa0\x82\x01\xcc\x30\x82\x01\xc8\x06\x09\x2b\x06\x01\x05\x05\x07\x30\x01\x01\x04\x82\x01\xb9\x30\x82\x01\xb5\x30\x81\x9e\xa2\x16\x04\x14\x50\xea\x73\x89\xdb\x29\xfb\x10\x8f\x9e\xe5\x01\x20\xd4\xde\x79\x99\x48\x83\xf7\x18\x0f\x32\x30\x31\x34\x30\x39\x30\x34\x30\x35\x34\x39\x30\x30\x5a\x30\x73\x30\x71\x30\x49\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14\xed\x48\xad\xdd\xcb\x7b\x00\xe2\x0e\x84\x2a\xa9\xb4\x09\xf1\xac\x30\x34\xcf\x96\x04\x14\x50\xea\x73\x89\xdb\x29\xfb\x10\x8f\x9e\xe5\x01\x20\xd4\xde\x79\x99\x48\x83\xf7\x02\x10\x02\x01\x48\x91\x5d\xfd\x5e\xb6\xe0\x02\x90\xa9\x67\xb0\xe4\x64\x80\x00\x18\x0f\x32\x30\x31\x34\x30\x39\x30\x34\x30\x35\x34\x39\x30\x30\x5a\xa0\x11\x18\x0f\x32\x30\x31\x34\x30\x39\x31\x31\x30\x36\x30\x34\x30\x30\x5a\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05\x05\x00\x03\x82\x01\x01\x00\x6e\x5e\x5e\x81\xff\x3f\x4d\xc7\x53\xc7\x1b\xf3\xd3\x1d\xdc\x9a\xc7\xce\x77\x2c\x67\x56\x13\x98\x91\x02\x01\x76\xdc\x48\xb2\x1f\x9b\x17\xea\xbf\x2c\x0a\xf5\x1d\x98\x90\x3c\x5f\x55\xc2\xff\x4b\x9a\xbc\xa6\x83\x9e\xab\x2b\xeb\x9d\x01\xea\x3b\x5f\xbe\x03\x29\x70\x63\x2a\xa4\x1d\xa8\xab\x69\xb2\x64\xba\x5d\x73\x91\x5c\x92\xf3\x69\xd4\xc9\x39\x9c\x7c\x7d\xa2\x47\x92\xc2\x56\xfe\xa1\x0d\x4a\x69\xff\xda\x48\xc5\x5e\xd8\xab\x39\x88\x6a\x06\xfa\x07\x57\xd6\x48\xb5\xce\xc9\x5f\xa5\x96\xfe\x37\x18\x5e\x7f\x35\x51\xc1\x9e\x79\x5a\x26\xba\x67\x67\x38\x2a\x80\x75\x42\x99\x68\x3e\xec\x2f\x7e\x2d\xa1\xa6\xbe\x9f\x01\x51\x22\x88\x3a\xc9\x9c\xed\x51\xef\x21\x66\x7e\xa9\xd0\x3f\x13\x9c\xbb\xd2\x94\x14\x6f\x4b\xd9\xc4\xf5\x2c\xf5\x7d\x07\x68\xf3\x51\xac\xda\xc2\x09\x66\xa9\x3d\xed\xad\x02\x4d\x9c\x11\x29\x1a\x54\xfb\x1e\x7e\x36\xf4\xbb\x0d\x08\x8c\x6a\x42\x08\x10\x29\x08\x7c\x56\x0b\x18\x47\xff\x87\x11\xfd\xb2\xfb\xc9\x22\x7f\xe3\x1f\x7b\xf9\x98\xaa\x3a\x32\xb6\x2f\x02\xba\xb6\xc1\xdc\xc3\x5d\xb5\x4b\xae\x5d\x29\x6a\x31\xde\xcd"

#define RESP3INFO "OCSP Response Information:\n" \
"	Response Status: Successful\n" \
"	Response Type: Basic OCSP Response\n" \
"	Version: 1\n" \
"	Responder Key ID: 50ea7389db29fb108f9ee50120d4de79994883f7\n" \
"	Produced At: Thu Sep 04 05:49:00 UTC 2014\n" \
"	Responses:\n" \
"		Certificate ID:\n" \
"			Hash Algorithm: SHA1\n" \
"			Issuer Name Hash: ed48adddcb7b00e20e842aa9b409f1ac3034cf96\n" \
"			Issuer Key Hash: 50ea7389db29fb108f9ee50120d4de79994883f7\n" \
"			Serial Number: 020148915dfd5eb6e00290a967b0e464\n" \
"		Certificate Status: good\n" \
"		This Update: Thu Sep 04 05:49:00 UTC 2014\n" \
"		Next Update: Thu Sep 11 06:04:00 UTC 2014\n" \
"	Extensions:\n" \
"	Signature Algorithm: RSA-SHA1\n" \
"	Signature:\n" \
"		6e:5e:5e:81:ff:3f:4d:c7:53:c7:1b:f3:d3:1d:dc:9a\n" \
"		c7:ce:77:2c:67:56:13:98:91:02:01:76:dc:48:b2:1f\n" \
"		9b:17:ea:bf:2c:0a:f5:1d:98:90:3c:5f:55:c2:ff:4b\n" \
"		9a:bc:a6:83:9e:ab:2b:eb:9d:01:ea:3b:5f:be:03:29\n" \
"		70:63:2a:a4:1d:a8:ab:69:b2:64:ba:5d:73:91:5c:92\n" \
"		f3:69:d4:c9:39:9c:7c:7d:a2:47:92:c2:56:fe:a1:0d\n" \
"		4a:69:ff:da:48:c5:5e:d8:ab:39:88:6a:06:fa:07:57\n" \
"		d6:48:b5:ce:c9:5f:a5:96:fe:37:18:5e:7f:35:51:c1\n" \
"		9e:79:5a:26:ba:67:67:38:2a:80:75:42:99:68:3e:ec\n" \
"		2f:7e:2d:a1:a6:be:9f:01:51:22:88:3a:c9:9c:ed:51\n" \
"		ef:21:66:7e:a9:d0:3f:13:9c:bb:d2:94:14:6f:4b:d9\n" \
"		c4:f5:2c:f5:7d:07:68:f3:51:ac:da:c2:09:66:a9:3d\n" \
"		ed:ad:02:4d:9c:11:29:1a:54:fb:1e:7e:36:f4:bb:0d\n" \
"		08:8c:6a:42:08:10:29:08:7c:56:0b:18:47:ff:87:11\n" \
"		fd:b2:fb:c9:22:7f:e3:1f:7b:f9:98:aa:3a:32:b6:2f\n" \
"		02:ba:b6:c1:dc:c3:5d:b5:4b:ae:5d:29:6a:31:de:cd\n"

static const gnutls_datum_t resp3 =
    { (unsigned char *) RESP3, sizeof(RESP3) - 1 };

static unsigned char issuer_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDuDCCAqCgAwIBAgIBADANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJDSDEZ\n"
    "MBcGA1UEChMQTGludXggc3Ryb25nU3dhbjEbMBkGA1UEAxMSc3Ryb25nU3dhbiBS\n"
    "b290IENBMB4XDTA0MDkxMDEwMDExOFoXDTE5MDkwNzEwMDExOFowRTELMAkGA1UE\n"
    "BhMCQ0gxGTAXBgNVBAoTEExpbnV4IHN0cm9uZ1N3YW4xGzAZBgNVBAMTEnN0cm9u\n"
    "Z1N3YW4gUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL/y\n"
    "X2LqPVZuWLPIeknK86xhz6ljd3NNhC2z+P1uoCP3sBMuZiZQEjFzhnKcbXxCeo2f\n"
    "FnvhOOjrrisSuVkzuu82oxXD3fIkzuS7m9V4E10EZzgmKWIf+WuNRfbgAuUINmLc\n"
    "4YGAXBQLPyzpP4Ou48hhz/YQo58Bics6PHy5v34qCVROIXDvqhj91P8g+pS+F21/\n"
    "7P+CH2jRcVIEHZtG8M/PweTPQ95dPzpYd2Ov6SZ/U7EWmbMmT8VcUYn1aChxFmy5\n"
    "gweVBWlkH6MP+1DeE0/tL5c87xo5KCeGK8Tdqpe7sBRC4pPEEHDQciTUvkeuJ1Pr\n"
    "K+1LwdqRxo7HgMRiDw8CAwEAAaOBsjCBrzASBgNVHRMBAf8ECDAGAQH/AgEBMAsG\n"
    "A1UdDwQEAwIBBjAdBgNVHQ4EFgQUXafdcAZRMn7ntm2zteXgYOouTe8wbQYDVR0j\n"
    "BGYwZIAUXafdcAZRMn7ntm2zteXgYOouTe+hSaRHMEUxCzAJBgNVBAYTAkNIMRkw\n"
    "FwYDVQQKExBMaW51eCBzdHJvbmdTd2FuMRswGQYDVQQDExJzdHJvbmdTd2FuIFJv\n"
    "b3QgQ0GCAQAwDQYJKoZIhvcNAQELBQADggEBACOSmqEBtBLR9aV3UyCI8gmzR5in\n"
    "Lte9aUXXS+qis6F2h2Stf4sN+Nl6Gj7REC6SpfEH4wWdwiUL5J0CJhyoOjQuDl3n\n"
    "1Dw3dE4/zqMZdyDKEYTU75TmvusNJBdGsLkrf7EATAjoi/nrTOYPPhSUZvPp/D+Y\n"
    "vORJ9Ej51GXlK1nwEB5iA8+tDYniNQn6BD1MEgIejzK+fbiy7braZB1kqhoEr2Si\n"
    "7luBSnU912sw494E88a2EWbmMvg2TVHPNzCpVkpNk7kifCiwmw9VldkqYy9y/lCa\n"
    "Epyp7lTfKw7cbD04Vk8QJW782L6Csuxkl346b17wmOqn8AZips3tFsuAY3w=\n"
    "-----END CERTIFICATE-----\n";
const gnutls_datum_t issuer_data = { issuer_pem, sizeof(issuer_pem) };

static unsigned char subject_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEIjCCAwqgAwIBAgIBHTANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJDSDEZ\n"
    "MBcGA1UEChMQTGludXggc3Ryb25nU3dhbjEbMBkGA1UEAxMSc3Ryb25nU3dhbiBS\n"
    "b290IENBMB4XDTA5MDgyNzEwNDQ1MVoXDTE0MDgyNjEwNDQ1MVowWjELMAkGA1UE\n"
    "BhMCQ0gxGTAXBgNVBAoTEExpbnV4IHN0cm9uZ1N3YW4xETAPBgNVBAsTCFJlc2Vh\n"
    "cmNoMR0wGwYDVQQDFBRjYXJvbEBzdHJvbmdzd2FuLm9yZzCCASIwDQYJKoZIhvcN\n"
    "AQEBBQADggEPADCCAQoCggEBANBdWU+BF7x4lyo+xHnr4UAOU89yQQuT5vdPoXzx\n"
    "6kRPsjYAuuktgXR+SaLkQHw/YRgDPSKj5nzmmlOQf/rWRr+8O2q+C92aUICmkNvZ\n"
    "Gamo5w2WlOMZ6T5dk2Hv+QM6xT/GzWyVr1dMYu/7tywD1Bw7aW/HqkRESDu6q95V\n"
    "Wu+Lzg6XlxCNEez0YsZrN/fC6BL2qzKAqMBbIHFW8OOnh+nEY4IF5AzkZnFrw12G\n"
    "I72Z882pw97lyKwZhSz/GMQFBJx+rnNdw5P1IJwTlG5PUdoDCte/Mcr1iiA+zOov\n"
    "x55x1GoGxduoXWU5egrf1MtalRf9Pc8Xr4q3WEKTAmsZrVECAwEAAaOCAQYwggEC\n"
    "MAkGA1UdEwQCMAAwCwYDVR0PBAQDAgOoMB0GA1UdDgQWBBQfoamI2WSMtaCiVGQ5\n"
    "tPI9dF1ufDBtBgNVHSMEZjBkgBRdp91wBlEyfue2bbO15eBg6i5N76FJpEcwRTEL\n"
    "MAkGA1UEBhMCQ0gxGTAXBgNVBAoTEExpbnV4IHN0cm9uZ1N3YW4xGzAZBgNVBAMT\n"
    "EnN0cm9uZ1N3YW4gUm9vdCBDQYIBADAfBgNVHREEGDAWgRRjYXJvbEBzdHJvbmdz\n"
    "d2FuLm9yZzA5BgNVHR8EMjAwMC6gLKAqhihodHRwOi8vY3JsLnN0cm9uZ3N3YW4u\n"
    "b3JnL3N0cm9uZ3N3YW4uY3JsMA0GCSqGSIb3DQEBCwUAA4IBAQC8pqX3KrSzKeul\n"
    "GdzydAV4hGwYB3WiB02oJ2nh5MJBu7J0Kn4IVkvLUHSSZhSRxx55tQZfdYqtXVS7\n"
    "ZuyG+6rV7sb595SIRwfkLAdjbvv0yZIl4xx8j50K3yMR+9aXW1NSGPEkb8BjBUMr\n"
    "F2kjGTOqomo8OIzyI369z9kJrtEhnS37nHcdpewZC1wHcWfJ6wd9wxmz2dVXmgVQ\n"
    "L2BjXd/BcpLFaIC4h7jMXQ5FURjnU7K9xSa4T8PpR6FrQhOcIYBXAp94GiM8JqmK\n"
    "ZBGUpeP+3cy4i3DV18Kyr64Q4XZlzhZClNE43sgMqiX88dc3znpDzT7T51j+d+9k\n"
    "Rf5Z0GOR\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t subject_data = { subject_pem, sizeof(subject_pem) };

/* For testing verify functions. */

#define BLOG_RESP "\x30\x82\x06\xF8\x0A\x01\x00\xA0\x82\x06\xF1\x30\x82\x06\xED\x06\x09\x2B\x06\x01\x05\x05\x07\x30\x01\x01\x04\x82\x06\xDE\x30\x82\x06\xDA\x30\x82\x01\x25\xA1\x7E\x30\x7C\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x41\x55\x31\x0C\x30\x0A\x06\x03\x55\x04\x08\x13\x03\x4E\x53\x57\x31\x0F\x30\x0D\x06\x03\x55\x04\x07\x13\x06\x53\x79\x64\x6E\x65\x79\x31\x14\x30\x12\x06\x03\x55\x04\x0A\x13\x0B\x43\x41\x63\x65\x72\x74\x20\x49\x6E\x63\x2E\x31\x1E\x30\x1C\x06\x03\x55\x04\x0B\x13\x15\x53\x65\x72\x76\x65\x72\x20\x41\x64\x6D\x69\x6E\x69\x73\x74\x72\x61\x74\x69\x6F\x6E\x31\x18\x30\x16\x06\x03\x55\x04\x03\x13\x0F\x6F\x63\x73\x70\x2E\x63\x61\x63\x65\x72\x74\x2E\x6F\x72\x67\x18\x0F\x32\x30\x31\x32\x30\x31\x31\x33\x30\x38\x35\x30\x34\x32\x5A\x30\x66\x30\x64\x30\x3C\x30\x09\x06\x05\x2B\x0E\x03\x02\x1A\x05\x00\x04\x14\xF2\x2A\x62\x16\x93\xA6\xDA\x5A\xD0\xB9\x8D\x3A\x13\x5E\x35\xD1\xEB\x18\x36\x61\x04\x14\x75\xA8\x71\x60\x4C\x88\x13\xF0\x78\xD9\x89\x77\xB5\x6D\xC5\x89\xDF\xBC\xB1\x7A\x02\x03\x00\xBC\xE0\x80\x00\x18\x0F\x32\x30\x31\x32\x30\x31\x31\x33\x30\x37\x32\x30\x34\x39\x5A\xA0\x11\x18\x0F\x32\x30\x31\x32\x30\x31\x31\x35\x30\x38\x35\x30\x34\x32\x5A\xA1\x2A\x30\x28\x30\x26\x06\x09\x2B\x06\x01\x05\x05\x07\x30\x01\x02\x04\x19\x04\x17\x73\x69\xD2\xC5\x6F\xC7\x7E\x2E\xB0\x2F\xCC\xC3\xE2\x80\xD6\x2A\xCE\xD3\xDE\x8F\x27\x1B\xB2\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05\x05\x00\x03\x82\x01\x01\x00\x3E\x50\x9D\xE9\xA2\xE0\xCA\x33\x88\x9B\x28\x7E\xE7\xA4\xAF\xDA\xBB\x75\x2D\xD9\x66\xA6\xD5\xFA\x17\x56\xC0\x3B\xDD\x74\xB6\x7E\x42\x2C\x28\xD0\x73\x91\x54\x69\xFA\xCF\xD8\xC7\x74\x1C\x5D\xBC\x8E\xCD\xE3\x0E\xD5\x3F\x80\x71\x9C\x95\x53\xC4\xD1\x95\x63\x5D\x72\xCE\xCC\x77\x9D\x7C\xAD\x47\x3F\x34\xDA\x90\x80\xC5\x15\xE1\x2B\xEE\x98\x57\xA3\xA7\x9F\xA2\xC3\xF5\x5E\xF7\x13\x26\x52\xDA\x09\x38\x5B\x18\x91\x07\x38\xCF\x09\xDA\x08\xED\x80\x4F\x26\x3A\xB9\xBE\xF6\xED\x65\x3F\xB1\x3A\x6D\xA3\x87\x22\xA3\x2A\xA5\x99\xCC\x06\xF3\x5A\xD5\x34\xFB\x9E\x32\x28\xC3\x3E\xF4\xAF\x33\x02\xCF\x6A\x74\x73\x17\x24\x17\x41\x0D\x7E\x86\x79\x83\x34\xE8\x82\x0A\x0D\x21\xED\xCB\x3B\xB7\x31\x64\xC9\xB6\x1E\xC7\x0C\x75\xCE\xBA\xB7\xDC\xB2\x67\x96\x2B\xAD\xBF\x86\x22\x81\x54\x66\xBA\x68\x89\xD7\x7E\x35\x60\x93\xEC\x6B\xD8\x59\x23\xA0\xD0\x95\x55\x8F\x93\x52\x48\x4E\x48\xCB\x92\xE9\x67\x71\x60\x07\xC9\xA3\x3B\xAC\xD1\xEA\x5B\x71\xDB\xC1\x94\x79\x85\x55\x8C\x03\x61\x9E\xC7\xD6\x32\x40\xFA\xDD\xF6\xC9\xF8\xE0\xFF\x4D\xAC\x54\xED\x61\xFE\xB2\xA0\x82\x04\x99\x30\x82\x04\x95\x30\x82\x04\x91\x30\x82\x02\x79\xA0\x03\x02\x01\x02\x02\x03\x00\xDC\xA6\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05\x05\x00\x30\x54\x31\x14\x30\x12\x06\x03\x55\x04\x0A\x13\x0B\x43\x41\x63\x65\x72\x74\x20\x49\x6E\x63\x2E\x31\x1E\x30\x1C\x06\x03\x55\x04\x0B\x13\x15\x68\x74\x74\x70\x3A\x2F\x2F\x77\x77\x77\x2E\x43\x41\x63\x65\x72\x74\x2E\x6F\x72\x67\x31\x1C\x30\x1A\x06\x03\x55\x04\x03\x13\x13\x43\x41\x63\x65\x72\x74\x20\x43\x6C\x61\x73\x73\x20\x33\x20\x52\x6F\x6F\x74\x30\x1E\x17\x0D\x31\x31\x30\x38\x32\x33\x30\x30\x30\x38\x33\x37\x5A\x17\x0D\x31\x33\x30\x38\x32\x32\x30\x30\x30\x38\x33\x37\x5A\x30\x7C\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x41\x55\x31\x0C\x30\x0A\x06\x03\x55\x04\x08\x13\x03\x4E\x53\x57\x31\x0F\x30\x0D\x06\x03\x55\x04\x07\x13\x06\x53\x79\x64\x6E\x65\x79\x31\x14\x30\x12\x06\x03\x55\x04\x0A\x13\x0B\x43\x41\x63\x65\x72\x74\x20\x49\x6E\x63\x2E\x31\x1E\x30\x1C\x06\x03\x55\x04\x0B\x13\x15\x53\x65\x72\x76\x65\x72\x20\x41\x64\x6D\x69\x6E\x69\x73\x74\x72\x61\x74\x69\x6F\x6E\x31\x18\x30\x16\x06\x03\x55\x04\x03\x13\x0F\x6F\x63\x73\x70\x2E\x63\x61\x63\x65\x72\x74\x2E\x6F\x72\x67\x30\x82\x01\x22\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01\x05\x00\x03\x82\x01\x0F\x00\x30\x82\x01\x0A\x02\x82\x01\x01\x00\x9C\xC6\xD4\x6F\xE4\x23\xC7\xC3\x70\x4B\x75\x1F\xE4\xFC\xAE\xF6\x62\xC4\x60\xA1\xD6\xCF\xF9\x47\x40\x38\xD9\xAF\x06\xF5\xB3\x87\x09\xBA\x07\xC8\x7A\x3B\xE3\x3A\xE2\xC1\x6B\xDB\x0E\x9B\x7B\xB4\x98\x04\x40\x88\xC8\xE4\x20\x34\x9D\x5F\x94\xAE\x0C\xA0\x05\xA1\x74\x10\x3F\x1F\x93\x6D\xC5\xA0\xCE\x29\xB0\x2A\x03\x6E\xED\x3B\xD1\x9A\x7A\xF7\x0F\xA7\xB7\x39\xD7\xC3\xB4\xDE\x15\x67\x94\xF2\xEF\xB0\xDD\x5F\xE3\xC9\xD8\xD2\x34\x0E\x5D\x44\xDF\xBF\x99\xD8\x5E\x60\xF4\x39\x24\x8A\xFD\x5D\xC8\x46\x8D\x0A\xB1\x60\x7A\x4F\xD5\x27\x30\x60\x9E\x13\x06\xF8\x3A\xAA\xB3\xBB\x33\x34\x6F\x84\x81\x7E\x5C\xCC\x12\x89\xF2\xFE\x6E\x93\x83\xFA\x8B\xEE\xAB\x36\x4C\xB6\x40\xA9\xEE\xFB\xF8\x16\x5A\x55\xD1\x64\x0D\x49\xDA\x04\xDE\xD1\xC8\xCA\xEE\x5F\x24\xB1\x79\x78\xB3\x9A\x88\x13\xDD\x68\x51\x39\xE9\x68\x31\xAF\xD7\xF8\x4D\x35\x6D\x60\x58\x04\x42\xBB\x55\x92\x18\xF6\x98\x01\xA5\x74\x3B\xBC\x36\xDB\x20\x68\x18\xB8\x85\xD4\x8B\x6D\x30\x87\x4D\xD6\x33\x2D\x7A\x54\x36\x1D\x57\x42\x14\x5C\x7A\x62\x74\xD5\x1E\x2B\xD5\xBF\x04\xF3\xFF\xEC\x03\xC1\x02\x03\x01\x00\x01\xA3\x44\x30\x42\x30\x0C\x06\x03\x55\x1D\x13\x01\x01\xFF\x04\x02\x30\x00\x30\x27\x06\x03\x55\x1D\x25\x04\x20\x30\x1E\x06\x08\x2B\x06\x01\x05\x05\x07\x03\x02\x06\x08\x2B\x06\x01\x05\x05\x07\x03\x01\x06\x08\x2B\x06\x01\x05\x05\x07\x03\x09\x30\x09\x06\x03\x55\x1D\x11\x04\x02\x30\x00\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05\x05\x00\x03\x82\x02\x01\x00\x50\xDD\x63\xB7\x1A\x6F\x91\x4C\xE8\x7F\x82\x1A\x27\x04\x81\x05\xBB\xA6\x69\xAC\x41\x7B\x62\xFC\x4B\x08\xDC\x60\xCF\xB2\x5A\xF1\xB4\xB5\x27\x69\x6B\x12\xE4\x07\xC8\x16\xCE\x3B\x42\xCC\x02\x90\x66\x0E\x79\xB8\x6C\x4B\x90\x00\xC5\x66\x64\x92\x2B\x2B\x48\x0E\x84\xC2\x6D\xBF\xA5\xDE\x16\xE3\xBD\x19\xF5\x5C\x93\xA1\x86\x7F\xD9\x89\x78\x6A\x3F\x83\xF0\xAA\xF8\xEA\x1D\xA4\x13\xF7\x2A\x15\x4C\x51\x9C\xC4\xB0\xBE\x58\x66\xCF\x4C\x6C\x3D\x31\xE5\xF9\x54\x21\xCD\xA1\x30\x01\x6A\xB3\x1A\x48\x85\x34\x93\xB8\xF9\x15\x19\x48\x34\x8D\x73\xE7\x03\x50\xAF\xDE\x50\xC7\x62\xAF\x25\x22\x2B\xF6\xE8\x37\x2E\xE4\x71\xA9\x5C\x26\xEA\x79\xCB\x04\x29\x73\x6B\x8F\xDF\x1F\x5C\x41\x52\xC0\x36\xAA\xD7\x7D\x8E\x44\x54\x98\x06\x4C\x63\xA6\x0B\x01\x94\x5D\x0C\x5C\xD4\xCF\xCB\x0B\x7B\x2D\x56\xCC\xBF\x97\x7F\x15\x24\x1D\xBA\xEA\xB7\x97\xB0\x32\xAD\xFC\xEA\x6D\x94\x39\x7A\xE3\x25\x54\xFC\x4A\xF5\x3D\xBD\x2E\xD5\x31\x07\x49\x24\xCC\x92\x69\x0E\x79\xB9\xDF\xDB\x36\xBF\x04\x44\x15\xD0\x46\x99\x8C\xD2\x4C\x94\x38\x0E\x10\x64\x13\xAB\xD9\x1B\x54\x02\x31\x56\x20\xEE\x69\x95\xDF\x39\xBB\xE9\xA7\x6D\xC3\x23\x86\x0B\xD6\x34\x40\x37\xC3\xD4\x41\xA8\x2E\x71\x1D\x6E\x5B\xD7\xC5\x9F\x2A\xE6\x02\x80\xAE\x0A\x28\x69\x63\x4B\x89\x2E\xBD\x4F\x42\x58\xFB\x86\x9A\xA2\x18\xDC\xC6\x32\xC1\x46\xBA\x28\xD2\x8B\xCE\x56\x63\x04\x80\x51\x51\x39\x00\x3B\x00\xB9\x5F\x67\xFA\x90\x1E\xDA\x76\xB5\x31\xA5\xBD\x11\xD2\x5F\xDA\x5D\xD5\xF7\xEE\xAB\xC0\x62\x74\x60\x47\x32\x42\xFD\xB2\x2E\x04\x3A\x2E\xF2\xC8\xB3\x41\xA3\xBD\xFE\x94\x5F\xEF\x6E\xD7\x92\x7C\x1D\x04\xF0\xC6\x53\x8E\x46\xDC\x30\x3A\x35\x5F\x1A\x4B\xEA\x3B\x00\x8B\x97\xB5\xB9\xCE\x71\x6E\x5C\xD5\xA0\x0B\xB1\x33\x08\x89\x61\x23\xCF\x97\x9F\x8F\x9A\x50\xB5\xEC\xCE\x40\x8D\x82\x95\x8B\x79\x26\x66\xF3\xF4\x70\xD8\xEE\x58\xDD\x75\x29\xD5\x6A\x91\x51\x7A\x17\xBC\x4F\xD4\xA3\x45\x7B\x84\xE7\xBE\x69\x53\xC1\xE2\x5C\xC8\x45\xA0\x3A\xEC\xDF\x8A\x1E\xC1\x18\x84\x8B\x7A\x4E\x4E\x9E\x3A\x26\xFE\x5D\x22\xD4\xC5\x14\xBE\xEE\x06\xEB\x05\x4A\x66\xC9\xA4\xB3\x68\x04\xB0\x5D\x25\x54\xB3\x05\xED\x41\xF0\x65\x69\x6D\xA5\x4E\xB7\x97\xD8\xD8\xF5"

static const gnutls_datum_t blog_resp =
    { (unsigned char *) BLOG_RESP, sizeof(BLOG_RESP) - 1 };

static unsigned char blog_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIE8DCCAtigAwIBAgIDALzgMA0GCSqGSIb3DQEBBQUAMFQxFDASBgNVBAoTC0NB\n"
    "Y2VydCBJbmMuMR4wHAYDVQQLExVodHRwOi8vd3d3LkNBY2VydC5vcmcxHDAaBgNV\n"
    "BAMTE0NBY2VydCBDbGFzcyAzIFJvb3QwHhcNMTAxMTE2MjI1MjMzWhcNMTIxMTE1\n"
    "MjI1MjMzWjAdMRswGQYDVQQDExJibG9nLmpvc2Vmc3Nvbi5vcmcwggEiMA0GCSqG\n"
    "SIb3DQEBAQUAA4IBDwAwggEKAoIBAQDBKA6bm/Kip0i00vU+BOmUF2MBDTwps41c\n"
    "xKN5bDn7usWZj8loi6BHRPE2WzCVPnPRD1FJXBc4rXL8zZWrCRe1b4A+l8NjPN2o\n"
    "uUgJvYLXYQ2hXkvxlPBQPKNOudaOAVsahpyxk6g6Z3mskOfqPhxvjutHvMC4fOsJ\n"
    "1+FstMzvg5SpDd4uYM9m0UK8pbEUSuwW+fxyWqhciSi7kJtdrD6bwx3ub3t9GFkM\n"
    "9uTzImIslTq19w8AHQsTICNnmNwfUGF5XMUIuxun0HlFt2KUP5G3Qg9Cd18wZFql\n"
    "RQJvLA3nbVFtmN3M3yKXnGSsEn38ZJvC+UxFuSfYJN9UwgoG6gwhAgMBAAGjggEA\n"
    "MIH9MAwGA1UdEwEB/wQCMAAwNAYDVR0lBC0wKwYIKwYBBQUHAwIGCCsGAQUFBwMB\n"
    "BglghkgBhvhCBAEGCisGAQQBgjcKAwMwCwYDVR0PBAQDAgWgMDMGCCsGAQUFBwEB\n"
    "BCcwJTAjBggrBgEFBQcwAYYXaHR0cDovL29jc3AuY2FjZXJ0Lm9yZy8wdQYDVR0R\n"
    "BG4wbIISYmxvZy5qb3NlZnNzb24ub3JnoCAGCCsGAQUFBwgFoBQMEmJsb2cuam9z\n"
    "ZWZzc29uLm9yZ4ISYmxvZy5qb3NlZnNzb24ub3JnoCAGCCsGAQUFBwgFoBQMEmJs\n"
    "b2cuam9zZWZzc29uLm9yZzANBgkqhkiG9w0BAQUFAAOCAgEACQX0KziT81G0XJ4C\n"
    "SlVumGN0KcVPDjtiUYskMpUvyLF951Q4Uuih0Aa9c0LynyZq8yqr6sW5OTmnRfSU\n"
    "DuUK5IH+IPq5PU7qteQSIy+63yjMQ+1wye1zfCWI+MyaS54AOn6uZObsr4grq41i\n"
    "sTwnX8OF/z15dQBjDR18WoehsnbuMz3Ld7+w5UcVWRGDzTyZ7JrYisEywQ7TXcoK\n"
    "1IlhD1TqwFucH7lIr4mPWNjL7Nw0sw11HN0Syt9H3upcq6lqyEI0ygfNZ9cdxvmX\n"
    "WqOBxxLc6G/87G4nGW4jw3WrCX7LqSmChlR3SbEC1UhWpaQMQ+mOU5+vXon7blRV\n"
    "zGJ/1wK8mKu3fKw9rm5TQ1xfJuRABbzsD3BrrUaHlREQQ+i6SCPVFGer6oeAaxyv\n"
    "so0NCbmBQkcpmUUl0COIR/Lh/YT78PjIEfxaUnUlaZXvCbKPKP2cM8LY7ltEaTgJ\n"
    "4W6sZi3QNFySzd4sz7J/YhY/jGjqku7TfpN/GOheW8AzKTBlm3WLps1YXys4TKrB\n"
    "0RStfaPfRJI1PeSlrWl6+kQu/5O8WA8NK0JZ/0Jc4d5LNrtUXo4VU9XCthrxLkgL\n"
    "3XWgZKFrqJd1UeJJ7OvkRYfI1c5i4oAP5ksuF0SHTpqnXE8K39kUnUx3B+ItJlZP\n"
    "VXTFhXRc06QwYqYXuYSAmj7/GJk=\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t blog_cert_data = { blog_cert_pem,
	sizeof(blog_cert_pem)
};

static unsigned char blog_issuer_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIHWTCCBUGgAwIBAgIDCkGKMA0GCSqGSIb3DQEBCwUAMHkxEDAOBgNVBAoTB1Jv\n"
    "b3QgQ0ExHjAcBgNVBAsTFWh0dHA6Ly93d3cuY2FjZXJ0Lm9yZzEiMCAGA1UEAxMZ\n"
    "Q0EgQ2VydCBTaWduaW5nIEF1dGhvcml0eTEhMB8GCSqGSIb3DQEJARYSc3VwcG9y\n"
    "dEBjYWNlcnQub3JnMB4XDTExMDUyMzE3NDgwMloXDTIxMDUyMDE3NDgwMlowVDEU\n"
    "MBIGA1UEChMLQ0FjZXJ0IEluYy4xHjAcBgNVBAsTFWh0dHA6Ly93d3cuQ0FjZXJ0\n"
    "Lm9yZzEcMBoGA1UEAxMTQ0FjZXJ0IENsYXNzIDMgUm9vdDCCAiIwDQYJKoZIhvcN\n"
    "AQEBBQADggIPADCCAgoCggIBAKtJNRFIfNImflOUz0Op3SjXQiqL84d4GVh8D57a\n"
    "iX3h++tykA10oZZkq5+gJJlz2uJVdscXe/UErEa4w75/ZI0QbCTzYZzA8pD6Ueb1\n"
    "aQFjww9W4kpCz+JEjCUoqMV5CX1GuYrz6fM0KQhF5Byfy5QEHIGoFLOYZcRD7E6C\n"
    "jQnRvapbjZLQ7N6QxX8KwuPr5jFaXnQ+lzNZ6MMDPWAzv/fRb0fEze5ig1JuLgia\n"
    "pNkVGJGmhZJHsK5I6223IeyFGmhyNav/8BBdwPSUp2rVO5J+TJAFfpPBLIukjmJ0\n"
    "FXFuC3ED6q8VOJrU0gVyb4z5K+taciX5OUbjchs+BMNkJyIQKopPWKcDrb60LhPt\n"
    "XapI19V91Cp7XPpGBFDkzA5CW4zt2/LP/JaT4NsRNlRiNDiPDGCbO5dWOK3z0luL\n"
    "oFvqTpa4fNfVoIZwQNORKbeiPK31jLvPGpKK5DR7wNhsX+kKwsOnIJpa3yxdUly6\n"
    "R9Wb7yQocDggL9V/KcCyQQNokszgnMyXS0XvOhAKq3A6mJVwrTWx6oUrpByAITGp\n"
    "rmB6gCZIALgBwJNjVSKRPFbnr9s6JfOPMVTqJouBWfmh0VMRxXudA/Z0EeBtsSw/\n"
    "LIaRmXGapneLNGDRFLQsrJ2vjBDTn8Rq+G8T/HNZ92ZCdB6K4/jc0m+YnMtHmJVA\n"
    "BfvpAgMBAAGjggINMIICCTAdBgNVHQ4EFgQUdahxYEyIE/B42Yl3tW3Fid+8sXow\n"
    "gaMGA1UdIwSBmzCBmIAUFrUyG9TH8+DmjvO90rA67rI5GNGhfaR7MHkxEDAOBgNV\n"
    "BAoTB1Jvb3QgQ0ExHjAcBgNVBAsTFWh0dHA6Ly93d3cuY2FjZXJ0Lm9yZzEiMCAG\n"
    "A1UEAxMZQ0EgQ2VydCBTaWduaW5nIEF1dGhvcml0eTEhMB8GCSqGSIb3DQEJARYS\n"
    "c3VwcG9ydEBjYWNlcnQub3JnggEAMA8GA1UdEwEB/wQFMAMBAf8wXQYIKwYBBQUH\n"
    "AQEEUTBPMCMGCCsGAQUFBzABhhdodHRwOi8vb2NzcC5DQWNlcnQub3JnLzAoBggr\n"
    "BgEFBQcwAoYcaHR0cDovL3d3dy5DQWNlcnQub3JnL2NhLmNydDBKBgNVHSAEQzBB\n"
    "MD8GCCsGAQQBgZBKMDMwMQYIKwYBBQUHAgEWJWh0dHA6Ly93d3cuQ0FjZXJ0Lm9y\n"
    "Zy9pbmRleC5waHA/aWQ9MTAwNAYJYIZIAYb4QgEIBCcWJWh0dHA6Ly93d3cuQ0Fj\n"
    "ZXJ0Lm9yZy9pbmRleC5waHA/aWQ9MTAwUAYJYIZIAYb4QgENBEMWQVRvIGdldCB5\n"
    "b3VyIG93biBjZXJ0aWZpY2F0ZSBmb3IgRlJFRSwgZ28gdG8gaHR0cDovL3d3dy5D\n"
    "QWNlcnQub3JnMA0GCSqGSIb3DQEBCwUAA4ICAQApKIWuRKm5r6R5E/CooyuXYPNc\n"
    "7uMvwfbiZqARrjY3OnYVBFPqQvX56sAV2KaC2eRhrnILKVyQQ+hBsuF32wITRHhH\n"
    "Va9Y/MyY9kW50SD42CEH/m2qc9SzxgfpCYXMO/K2viwcJdVxjDm1Luq+GIG6sJO4\n"
    "D+Pm1yaMMVpyA4RS5qb1MyJFCsgLDYq4Nm+QCaGrvdfVTi5xotSu+qdUK+s1jVq3\n"
    "VIgv7nSf7UgWyg1I0JTTrKSi9iTfkuO960NAkW4cGI5WtIIS86mTn9S8nK2cde5a\n"
    "lxuV53QtHA+wLJef+6kzOXrnAzqSjiL2jA3k2X4Ndhj3AfnvlpaiVXPAPHG0HRpW\n"
    "Q7fDCo1y/OIQCQtBzoyUoPkD/XFzS4pXM+WOdH4VAQDmzEoc53+VGS3FpQyLu7Xt\n"
    "hbNc09+4ufLKxw0BFKxwWMWMjTPUnWajGlCVI/xI4AZDEtnNp4Y5LzZyo4AQ5OHz\n"
    "0ctbGsDkgJp8E3MGT9ujayQKurMcvEp4u+XjdTilSKeiHq921F73OIZWWonO1sOn\n"
    "ebJSoMbxhbQljPI/lrMQ2Y1sVzufb4Y6GIIiNsiwkTjbKqGTqoQ/9SdlrnPVyNXT\n"
    "d+pLncdBu8fA46A/5H2kjXPmEkvfoXNzczqA6NXLji/L6hOn1kGLrPo8idck9U60\n"
    "4GGSt/M3mMS+lqO3ig==\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t blog_issuer_data = { blog_issuer_pem,
	sizeof(blog_issuer_pem)
};

static unsigned char blog_signer_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEkTCCAnmgAwIBAgIDANymMA0GCSqGSIb3DQEBBQUAMFQxFDASBgNVBAoTC0NB\n"
    "Y2VydCBJbmMuMR4wHAYDVQQLExVodHRwOi8vd3d3LkNBY2VydC5vcmcxHDAaBgNV\n"
    "BAMTE0NBY2VydCBDbGFzcyAzIFJvb3QwHhcNMTEwODIzMDAwODM3WhcNMTMwODIy\n"
    "MDAwODM3WjB8MQswCQYDVQQGEwJBVTEMMAoGA1UECBMDTlNXMQ8wDQYDVQQHEwZT\n"
    "eWRuZXkxFDASBgNVBAoTC0NBY2VydCBJbmMuMR4wHAYDVQQLExVTZXJ2ZXIgQWRt\n"
    "aW5pc3RyYXRpb24xGDAWBgNVBAMTD29jc3AuY2FjZXJ0Lm9yZzCCASIwDQYJKoZI\n"
    "hvcNAQEBBQADggEPADCCAQoCggEBAJzG1G/kI8fDcEt1H+T8rvZixGCh1s/5R0A4\n"
    "2a8G9bOHCboHyHo74zriwWvbDpt7tJgEQIjI5CA0nV+UrgygBaF0ED8fk23FoM4p\n"
    "sCoDbu070Zp69w+ntznXw7TeFWeU8u+w3V/jydjSNA5dRN+/mdheYPQ5JIr9XchG\n"
    "jQqxYHpP1ScwYJ4TBvg6qrO7MzRvhIF+XMwSifL+bpOD+ovuqzZMtkCp7vv4FlpV\n"
    "0WQNSdoE3tHIyu5fJLF5eLOaiBPdaFE56Wgxr9f4TTVtYFgEQrtVkhj2mAGldDu8\n"
    "NtsgaBi4hdSLbTCHTdYzLXpUNh1XQhRcemJ01R4r1b8E8//sA8ECAwEAAaNEMEIw\n"
    "DAYDVR0TAQH/BAIwADAnBgNVHSUEIDAeBggrBgEFBQcDAgYIKwYBBQUHAwEGCCsG\n"
    "AQUFBwMJMAkGA1UdEQQCMAAwDQYJKoZIhvcNAQEFBQADggIBAFDdY7cab5FM6H+C\n"
    "GicEgQW7pmmsQXti/EsI3GDPslrxtLUnaWsS5AfIFs47QswCkGYOebhsS5AAxWZk\n"
    "kisrSA6Ewm2/pd4W470Z9VyToYZ/2Yl4aj+D8Kr46h2kE/cqFUxRnMSwvlhmz0xs\n"
    "PTHl+VQhzaEwAWqzGkiFNJO4+RUZSDSNc+cDUK/eUMdiryUiK/boNy7kcalcJup5\n"
    "ywQpc2uP3x9cQVLANqrXfY5EVJgGTGOmCwGUXQxc1M/LC3stVsy/l38VJB266reX\n"
    "sDKt/OptlDl64yVU/Er1Pb0u1TEHSSTMkmkOebnf2za/BEQV0EaZjNJMlDgOEGQT\n"
    "q9kbVAIxViDuaZXfObvpp23DI4YL1jRAN8PUQagucR1uW9fFnyrmAoCuCihpY0uJ\n"
    "Lr1PQlj7hpqiGNzGMsFGuijSi85WYwSAUVE5ADsAuV9n+pAe2na1MaW9EdJf2l3V\n"
    "9+6rwGJ0YEcyQv2yLgQ6LvLIs0Gjvf6UX+9u15J8HQTwxlOORtwwOjVfGkvqOwCL\n"
    "l7W5znFuXNWgC7EzCIlhI8+Xn4+aULXszkCNgpWLeSZm8/Rw2O5Y3XUp1WqRUXoX\n"
    "vE/Uo0V7hOe+aVPB4lzIRaA67N+KHsEYhIt6Tk6eOib+XSLUxRS+7gbrBUpmyaSz\n"
    "aASwXSVUswXtQfBlaW2lTreX2Nj1\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t blog_signer_data = { blog_signer_pem,
	sizeof(blog_signer_pem)
};


static unsigned char long_resp_signer_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIE3zCCA8egAwIBAgIQPZqC0NHDL2/ghF+ZEe5TQjANBgkqhkiG9w0BAQUFADCB\n"
    "tTELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2Ug\n"
    "YXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykxMDEvMC0GA1UEAxMm\n"
    "VmVyaVNpZ24gQ2xhc3MgMyBTZWN1cmUgU2VydmVyIENBIC0gRzMwHhcNMTQwOTEy\n"
    "MDAwMDAwWhcNMTQxMjExMjM1OTU5WjCBhzELMAkGA1UEBhMCVVMxFzAVBgNVBAoT\n"
    "DlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZWZXJpU2lnbiBUcnVzdCBOZXR3b3Jr\n"
    "MT4wPAYDVQQDEzVWZXJpU2lnbiBDbGFzcyAzIFNlY3VyZSBTZXJ2ZXIgQ0EgLSBH\n"
    "MyBPQ1NQIFJlc3BvbmRlcjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
    "ALbm3lapt756SdAuFIelnhbtZvin09w/iS38GoMpR9A326dcSiKwmk7LYnLEkgQI\n"
    "mb5g4Nx6+nkQuhZoJiSxvqoPTgwt1nZtQ4v1weitmI1elgOSin+YrrjxPCpiYrFp\n"
    "j3qwbMz2K5ktlXl/2FeY5XWYuzz4ZscfxPF1mb1Nd5C7I+rZOE7nj7m9aQPEczgp\n"
    "hfZbMBb5kceeuskBkGyv05PwYbSkPTA4bzNA5dKT2ZsXzp+XC92EssV2smRiR/A1\n"
    "ai0uLUZeB4bJgICs6PNxPUaLt1Sn2gBgi+iw3039/8aAbx52FJm1yVv3MRDtaVqR\n"
    "l1kWCnyG/VLEhP1YcyeAC0cCAwEAAaOCARUwggERMAkGA1UdEwQCMAAwgawGA1Ud\n"
    "IASBpDCBoTCBngYLYIZIAYb4RQEHFwMwgY4wKAYIKwYBBQUHAgEWHGh0dHBzOi8v\n"
    "d3d3LnZlcmlzaWduLmNvbS9DUFMwYgYIKwYBBQUHAgIwVjAVFg5WZXJpU2lnbiwg\n"
    "SW5jLjADAgEBGj1WZXJpU2lnbidzIENQUyBpbmNvcnAuIGJ5IHJlZmVyZW5jZSBs\n"
    "aWFiLiBsdGQuIChjKTk3IFZlcmlTaWduMBMGA1UdJQQMMAoGCCsGAQUFBwMJMAsG\n"
    "A1UdDwQEAwIHgDAPBgkrBgEFBQcwAQUEAgUAMCIGA1UdEQQbMBmkFzAVMRMwEQYD\n"
    "VQQDEwpUR1YtQi0xNzk4MA0GCSqGSIb3DQEBBQUAA4IBAQCM8gJSyR4O8S5m52za\n"
    "FSzMfAcai+j5AqoRhYmY/+n/Hs/2bAdPy/6a+ukWGwhWZQRYLNr7SSSBkuSuVk/W\n"
    "zZX9VJmxAt1WzFRrXvgFyjSDtnqtg89LJbUOz5hG95d/scgb3ndv5Ey5193H/b8T\n"
    "O6GZ933J0O3X6qk4bnMBDUPFXgyn0Xfv0jeYzOa/Tu2IPpcf0ugogbrZscsIZWFy\n"
    "jFlwHnFGpd2k1GXaFRPqxk+qtLAxJtjN+DfkmxGNoIAv1hHXpBhDhuzTpnmXVf32\n"
    "YfFIyYfRt/x/Z4hztF/MZ41QxJdZIqvCMooi1GAgeG2jkXLx+x6ppfhkN7+zOF8A\n"
    "4W5J\n"
    "-----END CERTIFICATE-----\n";
const gnutls_datum_t long_resp_signer_data = { long_resp_signer_pem,
	sizeof(long_resp_signer_pem)
};

static unsigned char long_resp_str[] =
    "\x30\x82\x06\xbe\x0a\x01\x00\xa0\x82\x06\xb7\x30\x82\x06\xb3\x06"
    "\x09\x2b\x06\x01\x05\x05\x07\x30\x01\x01\x04\x82\x06\xa4\x30\x82"
    "\x06\xa0\x30\x81\x9e\xa2\x16\x04\x14\x81\x75\x7a\x7e\x22\xc8\xa4"
    "\x4c\xdf\x9f\x2d\x3f\x87\x61\xaf\x57\xe1\xaf\x4f\xd9\x18\x0f\x32"
    "\x30\x31\x34\x31\x31\x31\x30\x32\x30\x33\x33\x31\x37\x5a\x30\x73"
    "\x30\x71\x30\x49\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04"
    "\x14\x0c\x81\x29\x38\x74\xb2\x96\x29\x10\x7e\xd8\x35\x62\x52\x64"
    "\x04\x53\x0d\xe0\x83\x04\x14\x0d\x44\x5c\x16\x53\x44\xc1\x82\x7e"
    "\x1d\x20\xab\x25\xf4\x01\x63\xd8\xbe\x79\xa5\x02\x10\x4e\xeb\x31"
    "\x09\x63\x39\x4e\x8e\xa0\x4e\x70\x9c\xa9\x1d\xcd\xa6\x80\x00\x18"
    "\x0f\x32\x30\x31\x34\x31\x31\x31\x30\x32\x30\x33\x33\x31\x37\x5a"
    "\xa0\x11\x18\x0f\x32\x30\x31\x34\x31\x31\x31\x37\x32\x30\x33\x33"
    "\x31\x37\x5a\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05"
    "\x05\x00\x03\x82\x01\x01\x00\x67\xf8\x80\x8d\x1b\x17\x3d\xbe\x81"
    "\xf4\x3e\x74\x6d\x65\x5d\x9c\xdf\xd7\xdc\x7c\xd5\x23\x75\x24\xaa"
    "\x55\x8f\xa5\x99\xf8\x27\xd6\x69\x8e\x5a\x25\x0d\x5e\x1e\x49\xfc"
    "\x50\x98\x7b\xe7\x49\xfb\x05\xa5\x04\x46\xb7\x5e\xf6\x20\x46\x18"
    "\xd5\xdc\x70\xd8\x99\x2b\x64\x12\xae\x74\x8e\xa1\xdb\x0e\x9f\x11"
    "\x47\xdf\x87\x6e\x9d\xb9\x13\xaa\x66\x33\x8c\xf3\x3d\xed\x33\x57"
    "\x7d\x4c\x82\x21\xc6\x18\x67\x56\xbe\x46\x78\xa8\xec\xd0\x5b\xc0"
    "\x2d\xb6\xee\x5a\xd8\xbf\xc3\xea\x49\xcd\x6d\x01\x97\x6e\x3a\x81"
    "\x0f\x06\x16\xb4\x1e\x15\x08\x5c\x46\x35\x44\xa4\x06\x84\x32\xaa"
    "\x1b\xb7\xc2\x97\xbf\xfd\xc8\xe2\x6b\x7a\xa2\x40\x3b\x50\x59\xd2"
    "\xbe\xa2\x26\x09\xea\xf7\xc1\x9e\x89\x1d\x34\x79\xc3\xba\xa6\xb8"
    "\x09\x92\xc8\xee\xa4\xe2\xe2\x32\x43\x48\xc8\xf6\x69\xe5\xde\x33"
    "\x75\xe8\x38\x8a\xb0\xda\x19\x38\x75\x39\xab\xd6\x3f\x70\xcc\x4e"
    "\x45\x16\x2a\x82\x32\x8e\x48\x92\xa4\x1f\xe9\x46\x85\x18\x78\xa7"
    "\x46\xf7\x11\x9e\x37\x95\x1a\xc3\x30\x2d\x90\x6a\xc3\xfd\x95\x81"
    "\x6b\xb1\xcb\x12\x26\x9e\xe4\xd3\x2a\xc1\xdf\x82\x57\xf2\x21\xea"
    "\x6a\x16\x12\x40\x94\xe1\xc9\xa0\x82\x04\xe7\x30\x82\x04\xe3\x30"
    "\x82\x04\xdf\x30\x82\x03\xc7\xa0\x03\x02\x01\x02\x02\x10\x3d\x9a"
    "\x82\xd0\xd1\xc3\x2f\x6f\xe0\x84\x5f\x99\x11\xee\x53\x42\x30\x0d"
    "\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05\x05\x00\x30\x81\xb5"
    "\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x17\x30"
    "\x15\x06\x03\x55\x04\x0a\x13\x0e\x56\x65\x72\x69\x53\x69\x67\x6e"
    "\x2c\x20\x49\x6e\x63\x2e\x31\x1f\x30\x1d\x06\x03\x55\x04\x0b\x13"
    "\x16\x56\x65\x72\x69\x53\x69\x67\x6e\x20\x54\x72\x75\x73\x74\x20"
    "\x4e\x65\x74\x77\x6f\x72\x6b\x31\x3b\x30\x39\x06\x03\x55\x04\x0b"
    "\x13\x32\x54\x65\x72\x6d\x73\x20\x6f\x66\x20\x75\x73\x65\x20\x61"
    "\x74\x20\x68\x74\x74\x70\x73\x3a\x2f\x2f\x77\x77\x77\x2e\x76\x65"
    "\x72\x69\x73\x69\x67\x6e\x2e\x63\x6f\x6d\x2f\x72\x70\x61\x20\x28"
    "\x63\x29\x31\x30\x31\x2f\x30\x2d\x06\x03\x55\x04\x03\x13\x26\x56"
    "\x65\x72\x69\x53\x69\x67\x6e\x20\x43\x6c\x61\x73\x73\x20\x33\x20"
    "\x53\x65\x63\x75\x72\x65\x20\x53\x65\x72\x76\x65\x72\x20\x43\x41"
    "\x20\x2d\x20\x47\x33\x30\x1e\x17\x0d\x31\x34\x30\x39\x31\x32\x30"
    "\x30\x30\x30\x30\x30\x5a\x17\x0d\x31\x34\x31\x32\x31\x31\x32\x33"
    "\x35\x39\x35\x39\x5a\x30\x81\x87\x31\x0b\x30\x09\x06\x03\x55\x04"
    "\x06\x13\x02\x55\x53\x31\x17\x30\x15\x06\x03\x55\x04\x0a\x13\x0e"
    "\x56\x65\x72\x69\x53\x69\x67\x6e\x2c\x20\x49\x6e\x63\x2e\x31\x1f"
    "\x30\x1d\x06\x03\x55\x04\x0b\x13\x16\x56\x65\x72\x69\x53\x69\x67"
    "\x6e\x20\x54\x72\x75\x73\x74\x20\x4e\x65\x74\x77\x6f\x72\x6b\x31"
    "\x3e\x30\x3c\x06\x03\x55\x04\x03\x13\x35\x56\x65\x72\x69\x53\x69"
    "\x67\x6e\x20\x43\x6c\x61\x73\x73\x20\x33\x20\x53\x65\x63\x75\x72"
    "\x65\x20\x53\x65\x72\x76\x65\x72\x20\x43\x41\x20\x2d\x20\x47\x33"
    "\x20\x4f\x43\x53\x50\x20\x52\x65\x73\x70\x6f\x6e\x64\x65\x72\x30"
    "\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01"
    "\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01\x00"
    "\xb6\xe6\xde\x56\xa9\xb7\xbe\x7a\x49\xd0\x2e\x14\x87\xa5\x9e\x16"
    "\xed\x66\xf8\xa7\xd3\xdc\x3f\x89\x2d\xfc\x1a\x83\x29\x47\xd0\x37"
    "\xdb\xa7\x5c\x4a\x22\xb0\x9a\x4e\xcb\x62\x72\xc4\x92\x04\x08\x99"
    "\xbe\x60\xe0\xdc\x7a\xfa\x79\x10\xba\x16\x68\x26\x24\xb1\xbe\xaa"
    "\x0f\x4e\x0c\x2d\xd6\x76\x6d\x43\x8b\xf5\xc1\xe8\xad\x98\x8d\x5e"
    "\x96\x03\x92\x8a\x7f\x98\xae\xb8\xf1\x3c\x2a\x62\x62\xb1\x69\x8f"
    "\x7a\xb0\x6c\xcc\xf6\x2b\x99\x2d\x95\x79\x7f\xd8\x57\x98\xe5\x75"
    "\x98\xbb\x3c\xf8\x66\xc7\x1f\xc4\xf1\x75\x99\xbd\x4d\x77\x90\xbb"
    "\x23\xea\xd9\x38\x4e\xe7\x8f\xb9\xbd\x69\x03\xc4\x73\x38\x29\x85"
    "\xf6\x5b\x30\x16\xf9\x91\xc7\x9e\xba\xc9\x01\x90\x6c\xaf\xd3\x93"
    "\xf0\x61\xb4\xa4\x3d\x30\x38\x6f\x33\x40\xe5\xd2\x93\xd9\x9b\x17"
    "\xce\x9f\x97\x0b\xdd\x84\xb2\xc5\x76\xb2\x64\x62\x47\xf0\x35\x6a"
    "\x2d\x2e\x2d\x46\x5e\x07\x86\xc9\x80\x80\xac\xe8\xf3\x71\x3d\x46"
    "\x8b\xb7\x54\xa7\xda\x00\x60\x8b\xe8\xb0\xdf\x4d\xfd\xff\xc6\x80"
    "\x6f\x1e\x76\x14\x99\xb5\xc9\x5b\xf7\x31\x10\xed\x69\x5a\x91\x97"
    "\x59\x16\x0a\x7c\x86\xfd\x52\xc4\x84\xfd\x58\x73\x27\x80\x0b\x47"
    "\x02\x03\x01\x00\x01\xa3\x82\x01\x15\x30\x82\x01\x11\x30\x09\x06"
    "\x03\x55\x1d\x13\x04\x02\x30\x00\x30\x81\xac\x06\x03\x55\x1d\x20"
    "\x04\x81\xa4\x30\x81\xa1\x30\x81\x9e\x06\x0b\x60\x86\x48\x01\x86"
    "\xf8\x45\x01\x07\x17\x03\x30\x81\x8e\x30\x28\x06\x08\x2b\x06\x01"
    "\x05\x05\x07\x02\x01\x16\x1c\x68\x74\x74\x70\x73\x3a\x2f\x2f\x77"
    "\x77\x77\x2e\x76\x65\x72\x69\x73\x69\x67\x6e\x2e\x63\x6f\x6d\x2f"
    "\x43\x50\x53\x30\x62\x06\x08\x2b\x06\x01\x05\x05\x07\x02\x02\x30"
    "\x56\x30\x15\x16\x0e\x56\x65\x72\x69\x53\x69\x67\x6e\x2c\x20\x49"
    "\x6e\x63\x2e\x30\x03\x02\x01\x01\x1a\x3d\x56\x65\x72\x69\x53\x69"
    "\x67\x6e\x27\x73\x20\x43\x50\x53\x20\x69\x6e\x63\x6f\x72\x70\x2e"
    "\x20\x62\x79\x20\x72\x65\x66\x65\x72\x65\x6e\x63\x65\x20\x6c\x69"
    "\x61\x62\x2e\x20\x6c\x74\x64\x2e\x20\x28\x63\x29\x39\x37\x20\x56"
    "\x65\x72\x69\x53\x69\x67\x6e\x30\x13\x06\x03\x55\x1d\x25\x04\x0c"
    "\x30\x0a\x06\x08\x2b\x06\x01\x05\x05\x07\x03\x09\x30\x0b\x06\x03"
    "\x55\x1d\x0f\x04\x04\x03\x02\x07\x80\x30\x0f\x06\x09\x2b\x06\x01"
    "\x05\x05\x07\x30\x01\x05\x04\x02\x05\x00\x30\x22\x06\x03\x55\x1d"
    "\x11\x04\x1b\x30\x19\xa4\x17\x30\x15\x31\x13\x30\x11\x06\x03\x55"
    "\x04\x03\x13\x0a\x54\x47\x56\x2d\x42\x2d\x31\x37\x39\x38\x30\x0d"
    "\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05\x05\x00\x03\x82\x01"
    "\x01\x00\x8c\xf2\x02\x52\xc9\x1e\x0e\xf1\x2e\x66\xe7\x6c\xda\x15"
    "\x2c\xcc\x7c\x07\x1a\x8b\xe8\xf9\x02\xaa\x11\x85\x89\x98\xff\xe9"
    "\xff\x1e\xcf\xf6\x6c\x07\x4f\xcb\xfe\x9a\xfa\xe9\x16\x1b\x08\x56"
    "\x65\x04\x58\x2c\xda\xfb\x49\x24\x81\x92\xe4\xae\x56\x4f\xd6\xcd"
    "\x95\xfd\x54\x99\xb1\x02\xdd\x56\xcc\x54\x6b\x5e\xf8\x05\xca\x34"
    "\x83\xb6\x7a\xad\x83\xcf\x4b\x25\xb5\x0e\xcf\x98\x46\xf7\x97\x7f"
    "\xb1\xc8\x1b\xde\x77\x6f\xe4\x4c\xb9\xd7\xdd\xc7\xfd\xbf\x13\x3b"
    "\xa1\x99\xf7\x7d\xc9\xd0\xed\xd7\xea\xa9\x38\x6e\x73\x01\x0d\x43"
    "\xc5\x5e\x0c\xa7\xd1\x77\xef\xd2\x37\x98\xcc\xe6\xbf\x4e\xed\x88"
    "\x3e\x97\x1f\xd2\xe8\x28\x81\xba\xd9\xb1\xcb\x08\x65\x61\x72\x8c"
    "\x59\x70\x1e\x71\x46\xa5\xdd\xa4\xd4\x65\xda\x15\x13\xea\xc6\x4f"
    "\xaa\xb4\xb0\x31\x26\xd8\xcd\xf8\x37\xe4\x9b\x11\x8d\xa0\x80\x2f"
    "\xd6\x11\xd7\xa4\x18\x43\x86\xec\xd3\xa6\x79\x97\x55\xfd\xf6\x61"
    "\xf1\x48\xc9\x87\xd1\xb7\xfc\x7f\x67\x88\x73\xb4\x5f\xcc\x67\x8d"
    "\x50\xc4\x97\x59\x22\xab\xc2\x32\x8a\x22\xd4\x60\x20\x78\x6d\xa3"
    "\x91\x72\xf1\xfb\x1e\xa9\xa5\xf8\x64\x37\xbf\xb3\x38\x5f\x00\xe1"
    "\x6e\x49";

gnutls_datum_t long_resp = {long_resp_str, sizeof(long_resp_str)-1 };

static void ocsp_invalid_calls(void)
{
	gnutls_ocsp_req_t req;
	gnutls_ocsp_resp_t resp;
	gnutls_datum_t dat;
	char c = 42;
	void *p = &c;
	int rc;

	rc = gnutls_ocsp_req_init(&req);
	if (rc != GNUTLS_E_SUCCESS) {
		fail("gnutls_ocsp_req_init alloc\n");
		exit(1);
	}
	rc = gnutls_ocsp_resp_init(&resp);
	if (rc != GNUTLS_E_SUCCESS) {
		fail("gnutls_ocsp_resp_init alloc\n");
		exit(1);
	}

	gnutls_ocsp_req_deinit(NULL);
	gnutls_ocsp_resp_deinit(NULL);

	rc = gnutls_ocsp_req_import(NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_import NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_import(NULL, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_import NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_import(req, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_import NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_import(NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_import NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_import(NULL, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_import NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_import(resp, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_import NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_export(NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_export NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_export(NULL, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_export NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_export(req, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_export NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_export(NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_export NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_export(NULL, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_export NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_export(resp, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_export NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_version(NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_get_version NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_cert_id(NULL, 0, NULL, NULL, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_get_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_cert_id(req, 0, NULL, NULL, NULL, NULL);
	if (rc != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fail("gnutls_ocsp_req_get_cert_id empty\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(NULL, 0, NULL, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, 0, NULL, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1, NULL, NULL,
					 NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1, p, NULL,
					 NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1, NULL, p,
					 NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1, NULL, NULL,
					 p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1, p, p, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1, p, NULL, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1, NULL, p, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert(NULL, 0, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert(req, 0, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}


	rc = gnutls_ocsp_req_add_cert(req, 0, p, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_add_cert(req, 0, NULL, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_add_cert_id NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_extension(NULL, 0, NULL, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_get_extension NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_extension(req, 0, NULL, NULL, NULL);
	if (rc != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fail("gnutls_ocsp_req_get_extension NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_extension(req, 0, p, p, p);
	if (rc != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fail("gnutls_ocsp_req_get_extension NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_set_extension(NULL, NULL, 0, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_set_extension NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_set_extension(req, NULL, 0, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_set_extension NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_set_extension(req, p, 0, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_set_extension NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_set_extension(req, NULL, 0, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_set_extension NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_nonce(NULL, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_get_nonce NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_get_nonce(NULL, NULL, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_get_nonce NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_set_nonce(NULL, 0, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_set_nonce NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_set_nonce(req, 0, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_set_nonce NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_req_randomize_nonce(NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_req_randomize_nonce NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_status(NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_get_status NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_status(resp);
	if (rc != GNUTLS_E_ASN1_VALUE_NOT_FOUND) {
		fail("gnutls_ocsp_resp_get_status %d\n", rc);
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_response(NULL, NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_get_response NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_response(NULL, p, p);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_get_response NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_response(resp, NULL, NULL);
	if (rc != GNUTLS_E_SUCCESS) {
		fail("gnutls_ocsp_resp_get_response %d\n", rc);
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_version(NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_get_version NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_version(resp);
	if (rc != 1) {
		fail("gnutls_ocsp_resp_get_version ret %d\n", rc);
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_responder(NULL, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_get_responder NULL\n");
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_responder(resp, NULL);
	if (rc != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_ocsp_resp_get_responder 2nd %d\n", rc);
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_responder(resp, &dat);
	if (rc != 0 && dat.data != NULL) {
		fail("gnutls_ocsp_resp_get_responder %d\n", rc);
		exit(1);
	}

	rc = gnutls_ocsp_resp_get_responder_raw_id(resp, GNUTLS_OCSP_RESP_ID_KEY, &dat);
	if (rc != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fail("gnutls_ocsp_resp_get_responder_raw_id %s\n", gnutls_strerror(rc));
		exit(1);
	}

	gnutls_free(dat.data);

	gnutls_ocsp_req_deinit(req);
	gnutls_ocsp_resp_deinit(resp);
}

/* import a request, query some fields and print and export it */
static void req_parse(void)
{
	gnutls_ocsp_req_t req;
	int ret;
	gnutls_datum_t d;

	/* init request */

	ret = gnutls_ocsp_req_init(&req);
	if (ret != 0) {
		fail("gnutls_ocsp_req_init\n");
		exit(1);
	}

	/* import ocsp request */

	ret = gnutls_ocsp_req_import(req, &req1);
	if (ret != 0) {
		fail("gnutls_ocsp_req_import %d\n", ret);
		exit(1);
	}

	/* simple version query */

	ret = gnutls_ocsp_req_get_version(req);
	if (ret != 1) {
		fail("gnutls_ocsp_req_get_version %d\n", ret);
		exit(1);
	}

	/* check nonce */
	{
		gnutls_datum_t expect =
		    { (unsigned char *) REQ1NONCE + 2,
	sizeof(REQ1NONCE) - 3 };
		gnutls_datum_t got;
		unsigned int critical;

		ret = gnutls_ocsp_req_get_nonce(req, &critical, &got);
		if (ret != 0) {
			fail("gnutls_ocsp_req_get_nonce %d\n", ret);
			exit(1);
		}

		if (critical != 0) {
			fail("unexpected critical %d\n", critical);
			exit(1);
		}

		if (expect.size != got.size ||
		    memcmp(expect.data, got.data, got.size) != 0) {
			fail("ocsp request nonce memcmp failed\n");
			exit(1);
		}

		gnutls_free(got.data);
	}

	/* print request */

	ret = gnutls_ocsp_req_print(req, GNUTLS_OCSP_PRINT_FULL, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_req_print\n");
		exit(1);
	}

	if (strlen(REQ1INFO) != d.size ||
	    memcmp(REQ1INFO, d.data, strlen(REQ1INFO)) != 0) {
		printf("expected (len %ld):\n%s\ngot (len %d):\n%.*s\n",
			strlen(REQ1INFO), REQ1INFO, (int) d.size,
			(int) d.size, d.data);
		fail("ocsp request print failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* test export */
	ret = gnutls_ocsp_req_export(req, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_req_export %d\n", ret);
		exit(1);
	}

	/* compare against earlier imported bytes */

	if (req1.size != d.size || memcmp(req1.data, d.data, d.size) != 0) {
		fail("ocsp request export memcmp failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* test setting nonce */
	{
		gnutls_datum_t n1 = { (unsigned char *) "foo", 3 };
		gnutls_datum_t n2 = { (unsigned char *) "foobar", 6 };
		gnutls_datum_t got;
		unsigned critical;

		ret = gnutls_ocsp_req_set_nonce(req, 0, &n1);
		if (ret != 0) {
			fail("gnutls_ocsp_req_set_nonce %d\n", ret);
			exit(1);
		}

		ret = gnutls_ocsp_req_get_nonce(req, &critical, &got);
		if (ret != 0) {
			fail("gnutls_ocsp_req_get_nonce %d\n", ret);
			exit(1);
		}

		if (critical != 0) {
			fail("unexpected critical %d\n", critical);
			exit(1);
		}

		if (n1.size != got.size ||
		    memcmp(n1.data, got.data, got.size) != 0) {
			fail("ocsp request parse nonce memcmp failed\n");
			exit(1);
		}

		gnutls_free(got.data);

		/* set another time */

		ret = gnutls_ocsp_req_set_nonce(req, 1, &n2);
		if (ret != 0) {
			fail("gnutls_ocsp_req_set_nonce %d\n", ret);
			exit(1);
		}

		ret = gnutls_ocsp_req_get_nonce(req, &critical, &got);
		if (ret != 0) {
			fail("gnutls_ocsp_req_get_nonce %d\n", ret);
			exit(1);
		}

		if (critical != 1) {
			fail("unexpected critical %d\n", critical);
			exit(1);
		}

		if (n2.size != got.size ||
		    memcmp(n2.data, got.data, got.size) != 0) {
			fail("ocsp request parse2 nonce memcmp failed\n");
			exit(1);
		}

		gnutls_free(got.data);

		/* randomize nonce */

		ret = gnutls_ocsp_req_randomize_nonce(req);
		if (ret != 0) {
			fail("gnutls_ocsp_req_randomize_nonce %d\n", ret);
			exit(1);
		}

		ret = gnutls_ocsp_req_get_nonce(req, &critical, &n1);
		if (ret != 0) {
			fail("gnutls_ocsp_req_get_nonce %d\n", ret);
			exit(1);
		}

		if (critical != 0) {
			fail("unexpected random critical %d\n", critical);
			exit(1);
		}

		ret = gnutls_ocsp_req_randomize_nonce(req);
		if (ret != 0) {
			fail("gnutls_ocsp_req_randomize_nonce %d\n", ret);
			exit(1);
		}

		ret = gnutls_ocsp_req_get_nonce(req, &critical, &n2);
		if (ret != 0) {
			fail("gnutls_ocsp_req_get_nonce %d\n", ret);
			exit(1);
		}

		if (critical != 0) {
			fail("unexpected random critical %d\n", critical);
			exit(1);
		}

		if (n2.size == got.size
		    && memcmp(n1.data, n2.data, n1.size) == 0) {
			fail("ocsp request random nonce memcmp failed\n");
			exit(1);
		}

		gnutls_free(n1.data);
		gnutls_free(n2.data);
	}

	/* cleanup */

	gnutls_ocsp_req_deinit(req);
}

/* check that creating a request (using low-level add_cert_id) ends up
   with same DER as above. */
static void req_addcert_id(void)
{
	gnutls_ocsp_req_t req;
	int ret;
	gnutls_datum_t d;

	/* init request */

	ret = gnutls_ocsp_req_init(&req);
	if (ret != 0) {
		fail("gnutls_ocsp_req_init\n");
		exit(1);
	}

	/* add ocsp request nonce */

	{
		gnutls_datum_t nonce =
		    { (unsigned char *) REQ1NONCE, sizeof(REQ1NONCE) - 1 };

		ret =
		    gnutls_ocsp_req_set_extension(req,
						  "1.3.6.1.5.5.7.48.1.2",
						  0, &nonce);
		if (ret != 0) {
			fail("gnutls_ocsp_req_set_extension %d\n", ret);
			exit(1);
		}
	}

	/* add cert_id */
	{
		gnutls_datum_t issuer_name_hash =
		    { (unsigned char *) REQ1INH, sizeof(REQ1INH) - 1 };
		gnutls_datum_t issuer_key_hash =
		    { (unsigned char *) REQ1IKH, sizeof(REQ1IKH) - 1 };
		gnutls_datum_t serial_number =
		    { (unsigned char *) REQ1SN, sizeof(REQ1SN) - 1 };

		ret = gnutls_ocsp_req_add_cert_id(req, GNUTLS_DIG_SHA1,
						  &issuer_name_hash,
						  &issuer_key_hash,
						  &serial_number);
		if (ret != 0) {
			fail("gnutls_ocsp_add_cert_id %d\n", ret);
			exit(1);
		}
	}

	/* print request */

	ret = gnutls_ocsp_req_print(req, GNUTLS_OCSP_PRINT_FULL, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_req_print\n");
		exit(1);
	}

	if (strlen(REQ1INFO) != d.size ||
	    memcmp(REQ1INFO, d.data, strlen(REQ1INFO)) != 0) {
		printf("expected (len %ld):\n%s\ngot (len %d):\n%.*s\n",
			strlen(REQ1INFO), REQ1INFO, (int) d.size,
			(int) d.size, d.data);
		fail("ocsp request print failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* test export */
	ret = gnutls_ocsp_req_export(req, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_req_export %d\n", ret);
		exit(1);
	}

	/* compare against earlier imported bytes */

	if (req1.size != d.size || memcmp(req1.data, d.data, d.size) != 0) {
		fail("ocsp request export memcmp failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* cleanup */

	gnutls_ocsp_req_deinit(req);
}

/* check that creating a request (using high-level add_cert) ends up
   with same DER as above. */
static void req_addcert(void)
{
	gnutls_ocsp_req_t req;
	int ret;
	gnutls_datum_t d;

	/* init request */

	ret = gnutls_ocsp_req_init(&req);
	if (ret != 0) {
		fail("gnutls_ocsp_req_init\n");
		exit(1);
	}

	/* add ocsp request nonce */

	{
		gnutls_datum_t nonce =
		    { (unsigned char *) REQ1NONCE, sizeof(REQ1NONCE) - 1 };

		ret =
		    gnutls_ocsp_req_set_extension(req,
						  "1.3.6.1.5.5.7.48.1.2",
						  0, &nonce);
		if (ret != 0) {
			fail("gnutls_ocsp_req_set_extension %d\n", ret);
			exit(1);
		}
	}

	/* add cert_id */
	{
		gnutls_x509_crt_t issuer = NULL, subject = NULL;

		ret = gnutls_x509_crt_init(&issuer);
		if (ret < 0) {
			fail("gnutls_x509_crt_init (issuer) %d\n", ret);
			exit(1);
		}

		ret = gnutls_x509_crt_init(&subject);
		if (ret < 0) {
			fail("gnutls_x509_crt_init (subject) %d\n", ret);
			exit(1);
		}

		ret =
		    gnutls_x509_crt_import(issuer, &issuer_data,
					   GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fail("gnutls_x509_crt_import (issuer) %d\n", ret);
			exit(1);
		}

		ret =
		    gnutls_x509_crt_import(subject, &subject_data,
					   GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fail("gnutls_x509_crt_import (subject) %d\n", ret);
			exit(1);
		}

		ret = gnutls_ocsp_req_add_cert(req, GNUTLS_DIG_SHA1,
						issuer, subject);
		if (ret != 0) {
			fail("gnutls_ocsp_add_cert %d\n", ret);
			exit(1);
		}

		gnutls_x509_crt_deinit(subject);
		gnutls_x509_crt_deinit(issuer);
	}

	/* print request */

	ret = gnutls_ocsp_req_print(req, GNUTLS_OCSP_PRINT_FULL, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_req_print\n");
		exit(1);
	}

	if (strlen(REQ1INFO) != d.size ||
	    memcmp(REQ1INFO, d.data, strlen(REQ1INFO)) != 0) {
		printf("expected (len %ld):\n%s\ngot (len %d):\n%.*s\n",
			strlen(REQ1INFO), REQ1INFO, (int) d.size,
			(int) d.size, d.data);
		fail("ocsp request print failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* test export */
	ret = gnutls_ocsp_req_export(req, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_req_export %d\n", ret);
		exit(1);
	}

	/* compare against earlier imported bytes */

	if (req1.size != d.size || memcmp(req1.data, d.data, d.size) != 0) {
		fail("ocsp request export memcmp failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* cleanup */

	gnutls_ocsp_req_deinit(req);
}

static void check_ocsp_resp(gnutls_ocsp_resp_t resp)
{
	int ret;
	gnutls_digest_algorithm_t digest;
	gnutls_datum_t issuer_name_hash;
	gnutls_datum_t issuer_key_hash;
	gnutls_datum_t serial_number;
	unsigned cert_status;
	time_t this_update;
	time_t next_update;
	time_t revocation_time;
	unsigned revocation_reason;

	/* functionality check of gnutls_ocsp_resp_get_single(), the data
	 * sanity check is done with the gnutls_ocsp_resp_print() checks. */
	ret = gnutls_ocsp_resp_get_single(resp, 0, &digest, &issuer_name_hash,
		&issuer_key_hash, &serial_number, &cert_status, &this_update,
		&next_update, &revocation_time, &revocation_reason);
	if (ret < 0) {
		fail("error in gnutls_ocsp_resp_get_single: %s\n", gnutls_strerror(ret));
	}

	gnutls_free(issuer_key_hash.data);
	gnutls_free(issuer_name_hash.data);
	gnutls_free(serial_number.data);

	/* test if everything works with null params */
	ret = gnutls_ocsp_resp_get_single(resp, 0, &digest, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL);
	if (ret < 0) {
		fail("error in gnutls_ocsp_resp_get_single: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_ocsp_resp_get_single(resp, 0, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, &revocation_reason);
	if (ret < 0) {
		fail("error in gnutls_ocsp_resp_get_single: %s\n", gnutls_strerror(ret));
	}

	return;
}

static void resp_import(void)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	gnutls_datum_t d;

	/* init response */

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_init\n");
		exit(1);
	}

	/* import ocsp response */

	ret = gnutls_ocsp_resp_import(resp, &resp1);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_import[%d]: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	/* print response */

	ret = gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_FULL, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_print\n");
		exit(1);
	}

	if (strlen(RESP1INFO) != d.size ||
	    memcmp(RESP1INFO, d.data, strlen(RESP1INFO)) != 0) {
		printf("expected (len %ld):\n%s\ngot (len %d):\n%.*s\n",
			strlen(RESP1INFO), RESP1INFO, (int) d.size,
			(int) d.size, d.data);
		fail("ocsp response print failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* import ocsp response */

	ret = gnutls_ocsp_resp_import(resp, &resp2);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_import[%d]: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	check_ocsp_resp(resp);

	/* print response */
	ret = gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_FULL, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_print\n");
		exit(1);
	}

	if (memcmp(RESP2INFO, d.data, strlen(RESP2INFO)) != 0) {
		printf("expected (len %ld):\n%s\ngot (len %d):\n%.*s\n",
			strlen(RESP2INFO), RESP2INFO, (int) d.size,
			(int) d.size, d.data);
		fail("ocsp response print failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* cleanup */

	gnutls_ocsp_resp_deinit(resp);

	/* import ocsp response 3*/

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_init\n");
		exit(1);
	}

	ret = gnutls_ocsp_resp_import(resp, &resp3);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_import[%d]: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	/* print response */

	ret = gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_FULL, &d);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_print 3\n");
		exit(1);
	}

	if (memcmp(RESP3INFO, d.data, strlen(RESP3INFO)) != 0) {
		printf("expected (len %ld):\n%s\ngot (len %d):\n%.*s\n",
			strlen(RESP3INFO), RESP3INFO, (int) d.size,
			(int) d.size, d.data);
		fail("ocsp response 3 print failed\n");
		exit(1);
	}
	gnutls_free(d.data);

	/* cleanup */

	gnutls_ocsp_resp_deinit(resp);
}

static void resp_verify(void)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	gnutls_x509_crt_t cert = NULL, issuer = NULL, signer = NULL;
	gnutls_x509_trust_list_t list;
	unsigned verify;

	/* init response */

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_init\n");
		exit(1);
	}

	/* import ocsp response */

	ret = gnutls_ocsp_resp_import(resp, &blog_resp);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_import %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_crt_init(&cert);
	if (ret < 0) {
		fail("gnutls_x509_crt_init (cert) %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_crt_init(&issuer);
	if (ret < 0) {
		fail("gnutls_x509_crt_init (issuer) %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_crt_init(&signer);
	if (ret < 0) {
		fail("gnutls_x509_crt_init (signer) %d\n", ret);
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(cert, &blog_cert_data,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_x509_crt_import (cert) %d\n", ret);
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(issuer, &blog_issuer_data,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_x509_crt_import (issuer) %d\n", ret);
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(signer, &blog_signer_data,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_x509_crt_import (signer) %d\n", ret);
		exit(1);
	}

	/* check direct verify with signer (should succeed) */

	ret = gnutls_ocsp_resp_verify_direct(resp, signer, &verify, 0);
	if (ret < 0) {
		fail("gnutls_ocsp_resp_verify_direct (signer) %d\n", ret);
		exit(1);
	}

	if (verify != 0) {
		fail("gnutls_ocsp_resp_verify_direct %d\n", verify);
		exit(1);
	}

	/* check direct verify with cert (should fail) */

	ret = gnutls_ocsp_resp_verify_direct(resp, cert, &verify, GNUTLS_VERIFY_ALLOW_BROKEN);
	if (ret < 0) {
		fail("gnutls_ocsp_resp_verify_direct (cert) %d\n", ret);
		exit(1);
	}

	if (verify != GNUTLS_OCSP_VERIFY_UNTRUSTED_SIGNER) {
		fail("gnutls_ocsp_resp_verify_direct3 %d\n", verify);
		exit(1);
	}

	/* check trust verify with issuer (should succeed) */

	ret = gnutls_x509_trust_list_init(&list, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_init %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_trust_list_add_cas(list, &issuer, 1, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_cas %d\n", ret);
		exit(1);
	}

	ret = gnutls_ocsp_resp_verify(resp, list, &verify, GNUTLS_VERIFY_ALLOW_BROKEN);
	if (ret < 0) {
		fail("gnutls_ocsp_resp_verify (issuer) %d\n", ret);
		exit(1);
	}

	if (verify != 0) {
		fail("gnutls_ocsp_resp_verify %d\n", verify);
		exit(1);
	}

	gnutls_x509_trust_list_deinit(list, 0);

	/* check trust verify with signer (should succeed) */

	ret = gnutls_x509_trust_list_init(&list, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_init %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_trust_list_add_cas(list, &signer, 1, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_cas %d\n", ret);
		exit(1);
	}

	ret = gnutls_ocsp_resp_verify(resp, list, &verify, 0);
	if (ret < 0) {
		fail("gnutls_ocsp_resp_verify (issuer) %d\n", ret);
		exit(1);
	}

	if (verify != 0) {
		fail("gnutls_ocsp_resp_verify %d\n", verify);
		exit(1);
	}

	gnutls_x509_trust_list_deinit(list, 0);

	/* check trust verify with cert (should fail) */

	ret = gnutls_x509_trust_list_init(&list, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_init %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_trust_list_add_cas(list, &cert, 1, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_cas %d\n", ret);
		exit(1);
	}

	ret = gnutls_ocsp_resp_verify(resp, list, &verify, GNUTLS_VERIFY_ALLOW_BROKEN);
	if (ret < 0) {
		fail("gnutls_ocsp_resp_verify (issuer) %d\n", ret);
		exit(1);
	}

	if (verify != GNUTLS_OCSP_VERIFY_UNTRUSTED_SIGNER) {
		fail("gnutls_ocsp_resp_verify %d\n", verify);
		exit(1);
	}

	gnutls_x509_trust_list_deinit(list, 0);

	/* check trust verify with all certs (should succeed) */

	ret = gnutls_x509_trust_list_init(&list, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_init %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_trust_list_add_cas(list, &cert, 1, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_cas %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_trust_list_add_cas(list, &issuer, 1, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_cas %d\n", ret);
		exit(1);
	}

	ret = gnutls_x509_trust_list_add_cas(list, &signer, 1, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_cas %d\n", ret);
		exit(1);
	}

	ret = gnutls_ocsp_resp_verify(resp, list, &verify, 0);
	if (ret < 0) {
		fail("gnutls_ocsp_resp_verify (issuer) %d\n", ret);
		exit(1);
	}

	if (verify != 0) {
		fail("gnutls_ocsp_resp_verify %d\n", verify);
		exit(1);
	}

	gnutls_x509_trust_list_deinit(list, 0);

	/* cleanup */

	gnutls_ocsp_resp_deinit(resp);
	gnutls_x509_crt_deinit(cert);
	gnutls_x509_crt_deinit(issuer);
	gnutls_x509_crt_deinit(signer);
}

static void long_resp_check(void)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	gnutls_x509_crt_t signer = NULL;
	unsigned verify;

	/* init response */

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_init\n");
		exit(1);
	}

	/* import ocsp response */

	ret = gnutls_ocsp_resp_import(resp, &long_resp);
	if (ret != 0) {
		fail("gnutls_ocsp_resp_import[%d]: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_init(&signer);
	if (ret < 0) {
		fail("gnutls_x509_crt_init (signer) %d\n", ret);
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(signer, &long_resp_signer_data,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_x509_crt_import (cert) %d\n", ret);
		exit(1);
	}

	/* check direct verify with signer (should succeed) */

	ret = gnutls_ocsp_resp_verify_direct(resp, signer, &verify, 0);
	if (ret < 0) {
		fail("gnutls_ocsp_resp_verify_direct (signer) %d\n", ret);
		exit(1);
	}

	if (verify != 0) {
		fail("gnutls_ocsp_resp_verify_direct %d\n", verify);
		exit(1);
	}

	gnutls_x509_crt_deinit(signer);
	gnutls_ocsp_resp_deinit(resp);
}

void doit(void)
{
	int ret;

	ret = global_init();
	if (ret < 0) {
		fail("global_init\n");
		exit(1);
	}

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(99);

	ocsp_invalid_calls();
	req_parse();
	resp_import();
	req_addcert_id();
	req_addcert();
	resp_verify();

	_then = 1415974540;
	long_resp_check();

	/* we're done */

	gnutls_global_deinit();
}
