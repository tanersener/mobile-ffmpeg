/*
 * Copyright (C) 2014 Nikos Mavrogiannopoulos
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>

#include "../utils.h"
#include "softhsm.h"

/* Tests whether gnutls_certificate_set_x509_key_file2() will utilize
 * the provided password as PIN when PKCS #11 keys are imported */

#define CONFIG_NAME "softhsm-privkey"
#define CONFIG CONFIG_NAME".config"

static unsigned char server_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIICdDCCAd2gAwIBAgIBAzANBgkqhkiG9w0BAQsFADAaMQswCQYDVQQDEwJDQTEL\n"
"MAkGA1UEBhMCQ1owIhgPMjAxMzExMTAwODI1MjdaGA8yMDIwMTIxMzA4MjUyN1ow\n"
"HjEPMA0GA1UEAxMGQ2xpZW50MQswCQYDVQQGEwJDWjCBnzANBgkqhkiG9w0BAQEF\n"
"AAOBjQAwgYkCgYEAvQRIzvKyhr3tqmB4Pe+91DWSFayaNtcrDIT597bhxugVYW8o\n"
"jB206kx5aknAMA3PQGYcGqkLrt+nsJcmOIXDZsC6P4zeOSsF1PPhDAoX3bkUr2lF\n"
"MEt374eKdg1yvyhRxt4DOR6aD4gkC7fVtaYdgV6yXpJGMHV05LBIgQ7QtykCAwEA\n"
"AaOBwTCBvjAMBgNVHRMBAf8EAjAAMBMGA1UdJQQMMAoGCCsGAQUFBwMCMBgGA1Ud\n"
"EQQRMA+BDW5vbmVAbm9uZS5vcmcwDwYDVR0PAQH/BAUDAweAADAdBgNVHQ4EFgQU\n"
"Dbinh11GaaJcTyOpmxPYuttsiGowHwYDVR0jBBgwFoAUEg7aURJAVq70HG3MobA9\n"
"KGF+MwEwLgYDVR0fBCcwJTAjoCGgH4YdaHR0cDovL3d3dy5nZXRjcmwuY3JsL2dl\n"
"dGNybC8wDQYJKoZIhvcNAQELBQADgYEAN/Henso+5zzuFQWTpJXlUsWtRQAFhRY3\n"
"WVt3xtnyPs4pF/LKBp3Ov0GLGBkz5YlyJGFNESSyUviMsH7g7rJM8i7Bph6BQTE9\n"
"XdqbZPc0opfms4EHjmlXj5HQ0f0yoxHnLk43CR+vmbn0JPuurnEKAwjznAJR8GxI\n"
"R2MRyMxdGqs=\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIICXQIBAAKBgQC9BEjO8rKGve2qYHg9773UNZIVrJo21ysMhPn3tuHG6BVhbyiM\n"
"HbTqTHlqScAwDc9AZhwaqQuu36ewlyY4hcNmwLo/jN45KwXU8+EMChfduRSvaUUw\n"
"S3fvh4p2DXK/KFHG3gM5HpoPiCQLt9W1ph2BXrJekkYwdXTksEiBDtC3KQIDAQAB\n"
"AoGBAKXrseIAB5jh9lPeNQ7heXhjwiXGiuTjAkYOIMNDRXPuXH5YLna4yQv3L4mO\n"
"zecg6DI2sCrzA29xoukP9ZweR4RUK2cS4/QggH9UgWP0QUpvj4nogyRkh7UrWyVV\n"
"xbboHcmgqWgNLR8GrEZqlpOWFiT+f+QAx783/khvP5QLNp6BAkEA3YvvqfPpepdv\n"
"UC/Uk/8LbVK0LGTSu2ynyl1fMbos9lkJNFdfPM31K6DHeqziIGSoWCSjAsN/e8V7\n"
"MU7egWtI+QJBANppSlO+PTYHWKeOWE7NkM1yVHxAiav9Oott0JywAH8RarfyTuCB\n"
"iyMJP8Rv920GsciDY4dyx0MBJF0tiH+5G7ECQQDQbU5UPbxyMPXwIo+DjHZbq2sG\n"
"OPRoj5hrsdxVFCoouSsHqwtWUQ1Otjv1FaDHiOs3wX/6oaHV97wmb2S1rRFBAkAq\n"
"prELFXVinaCkZ9m62c3TMOZqtTetTHAoVjOMxZnzNnV+omTg1qtTFjVLqQnKUqpZ\n"
"G79N7g4XeZueTov/VSihAkAwGeDXvQ8NlrBlZACCKp1sUqaJptuJ438Qwztbl3Pq\n"
"E6/8TD5yXtrLt9S2LNAFw1i7LVksUB8IbQNTuuwV7LYI\n"
"-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};


/* GnuTLS internally calls time() to find out the current time when
   verifying certificates.  To avoid a time bomb, we hard code the
   current time.  This should work fine on systems where the library
   call to time is resolved at run-time.  */
static time_t mytime(time_t * t)
{
	time_t then = 1412850586;

	if (t)
		*t = then;

	return then;
}

#define PIN "1234"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
		unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

void doit(void)
{
	char buf[128];
	int exit_val = 0;
	int ret;
	const char *lib, *bin;
	gnutls_x509_crt_t crt;
	gnutls_x509_privkey_t key;
	gnutls_certificate_credentials_t cred;
	gnutls_datum_t tmp;

	/* The overloading of time() seems to work in linux (ELF?)
	 * systems only. Disable it on windows.
	 */
#ifdef _WIN32
	exit(77);
#endif
	bin = softhsm_bin();

	lib = softhsm_lib();

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs11_set_pin_function(pin_func, NULL);
	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	set_softhsm_conf(CONFIG);
	snprintf(buf, sizeof(buf), "%s --init-token --slot 0 --label test --so-pin "PIN" --pin "PIN, bin);
	system(buf);

	ret = gnutls_pkcs11_add_provider(lib, "trusted");
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(crt, &server_cert,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_crt_import: %s\n",
			gnutls_strerror(ret));
			exit(1);
	}

	if (debug) {
		gnutls_x509_crt_print(crt,
			      GNUTLS_CRT_PRINT_ONELINE,
			      &tmp);

		printf("\tCertificate: %.*s\n",
		       tmp.size, tmp.data);
		gnutls_free(tmp.data);
	}

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_privkey_import(key, &server_key,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr,
			"gnutls_x509_privkey_import: %s\n",
			gnutls_strerror(ret));
			exit(1);
	}

	/* initialize softhsm token */
	ret = gnutls_pkcs11_token_init(SOFTHSM_URL, PIN, "test");
	if (ret < 0) {
		fail("gnutls_pkcs11_token_init: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_token_set_pin(SOFTHSM_URL, NULL, PIN, GNUTLS_PIN_USER);
	if (ret < 0) {
		fail("gnutls_pkcs11_token_set_pin: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_copy_x509_crt(SOFTHSM_URL, crt, "cert",
					  GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE|GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_crt: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_copy_x509_privkey(SOFTHSM_URL, key, "cert", GNUTLS_KEY_DIGITAL_SIGNATURE|GNUTLS_KEY_KEY_ENCIPHERMENT,
					      GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE|GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE|GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("gnutls_pkcs11_copy_x509_privkey: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_crt_deinit(crt);
	gnutls_x509_privkey_deinit(key);
	gnutls_pkcs11_set_pin_function(NULL, NULL);

	/* Test whether gnutls_certificate_set_x509_key_file2() would import the keys
	 * when the PIN is provided as parameter */

	ret = gnutls_certificate_allocate_credentials(&cred);
	if (ret < 0) {
		fail("gnutls_certificate_allocate_credentials: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_certificate_set_x509_key_file2(cred, SOFTHSM_URL";object=cert;object-type=cert", SOFTHSM_URL";object=cert;object-type=private", 0, PIN, 0);
	if (ret < 0) {
		fail("gnutls_certificate_set_x509_key_file2: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_certificate_free_credentials(cred);

	gnutls_global_deinit();

	if (debug)
		printf("Exit status...%d\n", exit_val);
	remove(CONFIG);

	exit(exit_val);
}
