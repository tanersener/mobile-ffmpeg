/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/pkcs7.h>
#include <gnutls/abstract.h>
#include <assert.h>

#include "utils.h"

const char pkcs7_data[] = 
	"-----BEGIN PKCS7-----\n"
	"MIIHSwYJKoZIhvcNAQcCoIIHPDCCBzgCAQExCzAJBgUrDgMCGgUAMIICNwYJKwYB\n"
	"BAGCNwoBoIICKDCCAiQwDAYKKwYBBAGCNwwBAQQQu/ZNvyszUkS6h2Pwl4hELRcN\n"
	"MTYxMDExMTcxMzAyWjAOBgorBgEEAYI3DAECBQAwggGVMIIBkQRSRQA1ADIAMgAx\n"
	"ADUANAAwAEQAQwA0AEIAOQA3ADQARgA1ADQARABCADQARQAzADkAMABCAEYARgA0\n"
	"ADEAMwAyADMAOQA5AEMAOAAwADMANwAAADGCATkwQAYKKwYBBAGCNwwCATEyMDAe\n"
	"CABGAGkAbABlAgQQAQABBB5zAGEAbQBiAGEAcAAxADAAMAAwAC4AaQBuAGYAAAAw\n"
	"RQYKKwYBBAGCNwIBBDE3MDUwEAYKKwYBBAGCNwIBGaICgAAwITAJBgUrDgMCGgUA\n"
	"BBTlIhVA3EuXT1TbTjkL/0EyOZyANzBKBgorBgEEAYI3DAIBMTwwOh4MAE8AUwBB\n"
	"AHQAdAByAgQQAQABBCQyADoANgAuADAALAAyADoANgAuADEALAAyADoANgAuADQA\n"
	"AAAwYgYKKwYBBAGCNwwCAjFUMFIeTAB7AEQARQAzADUAMQBBADQAMgAtADgARQA1\n"
	"ADkALQAxADEARAAwAC0AOABDADQANwAtADAAMABDADAANABGAEMAMgA5ADUARQBF\n"
	"AH0CAgIAoEowSDBGBgorBgEEAYI3DAIBBDgwNh4EAE8AUwIEEAEAAQQoVgBpAHMA\n"
	"dABhAFgAOAA2ACwANwBYADgANgAsADEAMABYADgANgAAAKCCAwwwggMIMIIB8KAD\n"
	"AgECAhAWVsiyv5uzsk5vNBHNz/C1MA0GCSqGSIb3DQEBBQUAMC0xKzApBgNVBAMT\n"
	"IldES1Rlc3RDZXJ0IGFzbiwxMzEyMDY3OTU0ODA0ODM0NTMwHhcNMTYxMDExMTcx\n"
	"MjI4WhcNMjYxMDExMDAwMDAwWjAtMSswKQYDVQQDEyJXREtUZXN0Q2VydCBhc24s\n"
	"MTMxMjA2Nzk1NDgwNDgzNDUzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n"
	"AQEApD6uPRvnduA8nsL3gd/OdTZzk+p0P9vAQ9kVbRFG39/UuSIIm7nyNO47Tu7h\n"
	"CBuK8q5zwY31naKaOkLJMwTpUonI/rwFEhrt7EwFNi2aRVeyEbqLlwCzFK5rJGzP\n"
	"wDp4vcKpWPsqD5mOKBOXOIbQt5l8MiKM91iRqvwEEg1Eba8hKF3P/MHT2ZaxMy4O\n"
	"QdJdgjovSQfqihA5qG1wwXXTQvWeQHvt1TO+vUNTcnbO0YnIuG+c0WDljn4UVLYo\n"
	"2HFk1c7MkTfYX3OzdUbxXpMsHbbQun2XU2v+yQRgViHUDe4G6pGz4ur/aN52DEFk\n"
	"qIUCAeJWBhG4pQvMCl20L/19DwIDAQABoyQwIjALBgNVHQ8EBAMCBDAwEwYDVR0l\n"
	"BAwwCgYIKwYBBQUHAwMwDQYJKoZIhvcNAQEFBQADggEBAE5t7t5lXUYJGh8xu412\n"
	"yREBlUxQT4Uid9Kc/GmmwiQvinKMWwjdowxtfnRR/ZzrbD5AVVQIaM6JSgzLEH3x\n"
	"0geN9FqMxcaJVksnUcx9iqWm94bznoPz9FXlgQ+e6lx9vCEP1butyUhj7m8yi0pk\n"
	"D8nXwf8cszaPY2tjqMa8o77/W6pDUjIGJHNIsZJwIN/qJT3Sxs9Nb8qwLfjKB7Fp\n"
	"aLgC9BAb73rWdW2uQSGtWO9Bvf7/fcgOk2Su1CFZTf/ZoqFbtTQ+Qwl92buUFmTl\n"
	"yo9gVmPHXZWfeYaIDwTen2FI43WmLEsge8Xlfv+TpFLTby2BWnKgtxBsHA6L9Fem\n"
	"xrwxggHZMIIB1QIBATBBMC0xKzApBgNVBAMTIldES1Rlc3RDZXJ0IGFzbiwxMzEy\n"
	"MDY3OTU0ODA0ODM0NTMCEBZWyLK/m7OyTm80Ec3P8LUwCQYFKw4DAhoFAKBvMBAG\n"
	"CisGAQQBgjcCAQwxAjAAMBgGCSqGSIb3DQEJAzELBgkrBgEEAYI3CgEwHAYKKwYB\n"
	"BAGCNwIBCzEOMAwGCisGAQQBgjcCARUwIwYJKoZIhvcNAQkEMRYEFJBgjwiqs2u+\n"
	"74y1Cb725gOFBYr6MA0GCSqGSIb3DQEBAQUABIIBAI4vlVYFKOLdIfs/7kx9ADl5\n"
	"zaniHZMgjKiLAljglGCzkfO46IMdOP9/KfmTTTwWBtaP9s7fv9O0XGyOl2qH8Ufg\n"
	"2d+0iS7CI8CqwF1Q8NLPYrSl2peKAPNibfIVbLR2+RUJ7zHxevdVou9Dt36A59mW\n"
	"BZ78THyix0mVJ1ZivfzFwarChq5S4YI2fpbugTFftlr8YkRB78ki5J2sXICkcWtU\n"
	"JRBZqhvsFlsghRWbUKyp20YyPNTgaGelumFj57OLGCVGAejxme/iF8EkmrUV8zs/\n"
	"FKuAqJdZ8QPdLD5sKyOL8a19md0tYpCV2ThOWD8okm8PrSMfz4fWlIKpTOi/KE0=\n"
	"-----END PKCS7-----\n";

const unsigned char der_content[] = "\x30\x82\x02\x24\x30\x0c\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x0c\x01\x01\x04\x10\xbb\xf6\x4d\xbf\x2b\x33\x52\x44\xba\x87\x63\xf0\x97\x88\x44\x2d\x17\x0d\x31\x36\x31\x30\x31\x31\x31\x37\x31\x33\x30\x32\x5a\x30\x0e\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x0c\x01\x02\x05\x00\x30\x82\x01\x95\x30\x82\x01\x91\x04\x52\x45\x00\x35\x00\x32\x00\x32\x00\x31\x00\x35\x00\x34\x00\x30\x00\x44\x00\x43\x00\x34\x00\x42\x00\x39\x00\x37\x00\x34\x00\x46\x00\x35\x00\x34\x00\x44\x00\x42\x00\x34\x00\x45\x00\x33\x00\x39\x00\x30\x00\x42\x00\x46\x00\x46\x00\x34\x00\x31\x00\x33\x00\x32\x00\x33\x00\x39\x00\x39\x00\x43\x00\x38\x00\x30\x00\x33\x00\x37\x00\x00\x00\x31\x82\x01\x39\x30\x40\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x0c\x02\x01\x31\x32\x30\x30\x1e\x08\x00\x46\x00\x69\x00\x6c\x00\x65\x02\x04\x10\x01\x00\x01\x04\x1e\x73\x00\x61\x00\x6d\x00\x62\x00\x61\x00\x70\x00\x31\x00\x30\x00\x30\x00\x30\x00\x2e\x00\x69\x00\x6e\x00\x66\x00\x00\x00\x30\x45\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x02\x01\x04\x31\x37\x30\x35\x30\x10\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x02\x01\x19\xa2\x02\x80\x00\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14\xe5\x22\x15\x40\xdc\x4b\x97\x4f\x54\xdb\x4e\x39\x0b\xff\x41\x32\x39\x9c\x80\x37\x30\x4a\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x0c\x02\x01\x31\x3c\x30\x3a\x1e\x0c\x00\x4f\x00\x53\x00\x41\x00\x74\x00\x74\x00\x72\x02\x04\x10\x01\x00\x01\x04\x24\x32\x00\x3a\x00\x36\x00\x2e\x00\x30\x00\x2c\x00\x32\x00\x3a\x00\x36\x00\x2e\x00\x31\x00\x2c\x00\x32\x00\x3a\x00\x36\x00\x2e\x00\x34\x00\x00\x00\x30\x62\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x0c\x02\x02\x31\x54\x30\x52\x1e\x4c\x00\x7b\x00\x44\x00\x45\x00\x33\x00\x35\x00\x31\x00\x41\x00\x34\x00\x32\x00\x2d\x00\x38\x00\x45\x00\x35\x00\x39\x00\x2d\x00\x31\x00\x31\x00\x44\x00\x30\x00\x2d\x00\x38\x00\x43\x00\x34\x00\x37\x00\x2d\x00\x30\x00\x30\x00\x43\x00\x30\x00\x34\x00\x46\x00\x43\x00\x32\x00\x39\x00\x35\x00\x45\x00\x45\x00\x7d\x02\x02\x02\x00\xa0\x4a\x30\x48\x30\x46\x06\x0a\x2b\x06\x01\x04\x01\x82\x37\x0c\x02\x01\x04\x38\x30\x36\x1e\x04\x00\x4f\x00\x53\x02\x04\x10\x01\x00\x01\x04\x28\x56\x00\x69\x00\x73\x00\x74\x00\x61\x00\x58\x00\x38\x00\x36\x00\x2c\x00\x37\x00\x58\x00\x38\x00\x36\x00\x2c\x00\x31\x00\x30\x00\x58\x00\x38\x00\x36\x00\x00\x00";
#define der_content_size (sizeof(der_content)-1)
const gnutls_datum_t pkcs7_pem = {(void *) pkcs7_data, sizeof(pkcs7_data)-1};

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", "err", level, str);
}

void doit(void)
{
	gnutls_pkcs7_t pkcs7;
	const char *oid;
	gnutls_datum_t data;
	int ret;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* generate a PKCS #7 structure */
	ret = gnutls_pkcs7_init(&pkcs7);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}


	ret = gnutls_pkcs7_import(pkcs7, &pkcs7_pem, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	oid = gnutls_pkcs7_get_embedded_data_oid(pkcs7);
	if (oid == NULL) {
		fail("error in gnutls_pkcs7_get_embedded_data_oid\n");
		exit(1);
	}

	assert(strcmp(oid, "1.3.6.1.4.1.311.10.1") == 0);

	ret = gnutls_pkcs7_get_embedded_data(pkcs7, GNUTLS_PKCS7_EDATA_GET_RAW, &data);
	if (ret < 0) {
		fail("error in gnutls_pkcs7_get_embedded_data: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	assert(data.size == der_content_size);
	assert(memcmp(data.data, der_content, data.size) == 0);

	gnutls_pkcs7_deinit(pkcs7);
	gnutls_free(data.data);
}
