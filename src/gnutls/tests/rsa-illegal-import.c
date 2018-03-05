/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <nettle/version.h>

/* Checks whether the output of the import functions is the expected one,
 * on illegal key input */

static unsigned char rsa_key_pem[] =
	"-----BEGIN RSA PRIVATE KEY-----\n"
	"MIICXQIBAAKBgQCpTrErF6KeHfaSBfQXLkw2AkrteEFca/jbWk2S0df5cRrsuC+R\n"
	"nrpHnk4prJISVQZzF+s5qgzulvRaMD0vnlCDKPjDgRLkFyiT3pW5JZJqTKiILQBw\n"
	"z6rqlQO7UWWqetc/gl9SHTq/vX5CDbA5Nxc9HJLkPX5Xl3wA12PAYmraugIDAQAB\n"
	"AoGABMjQgOM+GTHHkgDREQah6LTP4T4QusfiVHCM2KVNcSMdG6tozLirkvKKSusx\n"
	"hYsZj48ReqOvkd56MUJDuGDE7aQqhsrDnTgTnoYH7dFSY6acUucj5F6yeircFth4\n"
	"lRko09HKZ5Fd1ngstPU35GsekUMq8vaHDrRzleydp+Z5lMECQQDP/cy68Jt7tMZT\n"
	"oQQLhsddyoQG+2JiWz3PT9P9d5WdkMqzOYt6ADZ2m8HpmMcv32LQHtriSxy7JqXW\n"
	"3uSnowkEAkEA0GMOXvV/8QnWKU2/byp3HVDQP57Vq/M37BhMbxoZDAHCaIz7v8k2\n"
	"D7UBQdTeiUsm6gFJ1+E6YCnmTxdPRVuN6QJBALLLOQAGL5Jy/v4K7yA9dwpgOYiK\n"
	"9rMYPhUFSXWdI+cz/Zt9vzFcF3V0RYhaRfgYLqg7retTqFoVSgBg0OxuUSMCQBtF\n"
	"q37QAGOKVwXmz/P7icVDa024OtybIyl58J7luntwy4GlWdk6uyGJHdYAxvMO69Pa\n"
	"QVDIgDxPn32gXlaEaekCQQCVhXc3zc+VX3nM4iCpXhlET2N75ULzsR+r6CdvtwSB\n"
	"vXMBcuCE1aJHZDxqRx8XFZDZl+Ij/jrBMmtI15ebDuzH\n"
	"-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t rsa_key = { rsa_key_pem,
	sizeof(rsa_key_pem)-1
};

static unsigned char p8_rsa_pem[] =
	"-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
	"MIICojAcBgoqhkiG9w0BDAEDMA4ECDxZ1/EW+8XWAgIUYASCAoBR6R3Z341vSRvs\n"
	"/LMErKcKkAQ3THTZBpmYgR2mrJUjJBivzOuRTCRpgtjuQ4ht2Q7KV943mJXsqAFI\n"
	"Jly5fuVQ5YmRGLW+LE5sv+AGwmsii/PvGfGa9al56tHLDSeXV2VH4fly45bQ7ipr\n"
	"PZBiEgBToF/jqDFWleH2GTCnSLpc4B2cKkMO2c5RYrCCGNRK/jr1xVUDVzeiXZwE\n"
	"dbdDaV2UG/Oeo7F48UmvuWgS9YSFSUJ4fKG1KLlAQMKtAQKX+B4oL6Jbeb1jwSCX\n"
	"Q1H9hHXHTXbPGaIncPugotZNArwwrhesTszFE4NFMbg3QNKL1fabJJFIcOYIktwL\n"
	"7HG3pSiU2rqUZgS59OMJgL4jJm1lipo8ruNIl/YCpZTombOAV2Wbvq/I0SbRRXbX\n"
	"12lco8bQO1dgSkhhe58Vrs+ChaNajtNi8SjLS+Pi1tYYAVQjcQdxCGh4q8aZUhDv\n"
	"5yRp/TUOMaZqkY6YzRAlERb9jzVeh97EsOURzLu8pQgVjcNDOUAZF67KSqlSGMh7\n"
	"PdqknM/j8KaWmVMAUn4+PuWohkyjd1/1QhCnEtFZ1lbIfWrKXV76U7zyy0OTvFKw\n"
	"qemHUbryOJu0dQHziWmdtJpS7abSuhoMnrByZD+jDfQoSX7BzmdmCQGinltITYY1\n"
	"3iChqWC7jY02CiKZqTcdwkImvmDtDYOBr0uQSgBa4eh7nYmmcpdY4I6V5qAdo30w\n"
	"oXNEMqM53Syx36Fp70/Vmy0KmK8+2T4UgxGVJEgTDsEhiwJtTXxdzgxc5npbTePa\n"
	"abhFyIXIpqoUYZ9GPU8UjNEuF//wPY6klBp6VP0ixO6RqQKzbwr85EXbzoceBrLo\n"
	"eng1/Czj\n"
	"-----END ENCRYPTED PRIVATE KEY-----\n";

const gnutls_datum_t p8_rsa_key = { p8_rsa_pem,
	sizeof(p8_rsa_pem)-1
};

static
int check_x509_privkey(void)
{
	gnutls_x509_privkey_t key;
	int ret;

	global_init();

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));

	ret = gnutls_x509_privkey_import(key, &rsa_key, GNUTLS_X509_FMT_PEM);
	if (ret != GNUTLS_E_PK_INVALID_PRIVKEY)
		fail("error: %s\n", gnutls_strerror(ret));

	gnutls_x509_privkey_deinit(key);

	return 0;
}

static
int check_pkcs8_privkey1(void)
{
	gnutls_x509_privkey_t key;
	int ret;

	global_init();

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));

	ret = gnutls_x509_privkey_import_pkcs8(key, &p8_rsa_key, GNUTLS_X509_FMT_PEM, "1234", 0);
	if (ret != GNUTLS_E_PK_INVALID_PRIVKEY)
		fail("error: %s\n", gnutls_strerror(ret));

	gnutls_x509_privkey_deinit(key);

	return 0;
}

static
int check_pkcs8_privkey2(void)
{
	gnutls_privkey_t key;
	int ret;

	global_init();

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));

	ret = gnutls_privkey_import_x509_raw(key, &p8_rsa_key, GNUTLS_X509_FMT_PEM, "1234", 0);
	if (ret != GNUTLS_E_PK_INVALID_PRIVKEY)
		fail("error: %s\n", gnutls_strerror(ret));

	gnutls_privkey_deinit(key);

	return 0;
}

void doit(void)
{
#if NETTLE_VERSION_MAJOR < 3 || (NETTLE_VERSION_MAJOR == 3 && NETTLE_VERSION_MINOR <= 2)
	/* These checks are enforced only on new versions of nettle */
	exit(77);
#else
	if (check_x509_privkey() != 0) {
		fail("error in privkey check\n");
		exit(1);
	}

	if (check_pkcs8_privkey1() != 0) {
		fail("error in pkcs8 privkey check 1\n");
		exit(1);
	}

	if (check_pkcs8_privkey2() != 0) {
		fail("error in pkcs8 privkey check 2\n");
		exit(1);
	}
#endif
}
