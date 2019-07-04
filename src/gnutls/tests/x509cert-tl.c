/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

/* gnutls_trust_list_*().
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

static unsigned char ca_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB5zCCAVKgAwIBAgIERiYdJzALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTExWhcNMDgwNDE3MTMyOTExWjAZMRcw\n"
    "FQYDVQQDEw5HbnVUTFMgdGVzdCBDQTCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA\n"
    "vuyYeh1vfmslnuggeEKgZAVmQ5ltSdUY7H25WGSygKMUYZ0KT74v8C780qtcNt9T\n"
    "7EPH/N6RvB4BprdssgcQLsthR3XKA84jbjjxNCcaGs33lvOz8A1nf8p3hD+cKfRi\n"
    "kfYSW2JazLrtCC4yRCas/SPOUxu78of+3HiTfFm/oXUCAwEAAaNDMEEwDwYDVR0T\n"
    "AQH/BAUwAwEB/zAPBgNVHQ8BAf8EBQMDBwQAMB0GA1UdDgQWBBTpPBz7rZJu5gak\n"
    "Viyi4cBTJ8jylTALBgkqhkiG9w0BAQUDgYEAiaIRqGfp1jPpNeVhABK60SU0KIAy\n"
    "njuu7kHq5peUgYn8Jd9zNzExBOEp1VOipGsf6G66oQAhDFp2o8zkz7ZH71zR4HEW\n"
    "KoX6n5Emn6DvcEH/9pAhnGxNHJAoS7czTKv/JDZJhkqHxyrE1fuLsg5Qv25DTw7+\n"
    "PfqUpIhz5Bbm7J4=\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t ca = { ca_pem, sizeof(ca_pem) };

static unsigned char cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICHjCCAYmgAwIBAgIERiYdNzALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTI3WhcNMDgwNDE3MTMyOTI3WjAdMRsw\n"
    "GQYDVQQDExJHbnVUTFMgdGVzdCBjbGllbnQwgZwwCwYJKoZIhvcNAQEBA4GMADCB\n"
    "iAKBgLtmQ/Xyxde2jMzF3/WIO7HJS2oOoa0gUEAIgKFPXKPQ+GzP5jz37AR2ExeL\n"
    "ZIkiW8DdU3w77XwEu4C5KL6Om8aOoKUSy/VXHqLnu7czSZ/ju0quak1o/8kR4jKN\n"
    "zj2AC41179gAgY8oBAOgIo1hBAf6tjd9IQdJ0glhaZiQo1ipAgMBAAGjdjB0MAwG\n"
    "A1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDwYDVR0PAQH/BAUDAweg\n"
    "ADAdBgNVHQ4EFgQUTLkKm/odNON+3svSBxX+odrLaJEwHwYDVR0jBBgwFoAU6Twc\n"
    "+62SbuYGpFYsouHAUyfI8pUwCwYJKoZIhvcNAQEFA4GBALujmBJVZnvaTXr9cFRJ\n"
    "jpfc/3X7sLUsMvumcDE01ls/cG5mIatmiyEU9qI3jbgUf82z23ON/acwJf875D3/\n"
    "U7jyOsBJ44SEQITbin2yUeJMIm1tievvdNXBDfW95AM507ShzP12sfiJkJfjjdhy\n"
    "dc8Siq5JojruiMizAf0pA7in\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t cert = { cert_pem, sizeof(cert_pem) };

static unsigned char key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXAIBAAKBgQC7ZkP18sXXtozMxd/1iDuxyUtqDqGtIFBACIChT1yj0Phsz+Y8\n"
    "9+wEdhMXi2SJIlvA3VN8O+18BLuAuSi+jpvGjqClEsv1Vx6i57u3M0mf47tKrmpN\n"
    "aP/JEeIyjc49gAuNde/YAIGPKAQDoCKNYQQH+rY3fSEHSdIJYWmYkKNYqQIDAQAB\n"
    "AoGADpmARG5CQxS+AesNkGmpauepiCz1JBF/JwnyiX6vEzUh0Ypd39SZztwrDxvF\n"
    "PJjQaKVljml1zkJpIDVsqvHdyVdse8M+Qn6hw4x2p5rogdvhhIL1mdWo7jWeVJTF\n"
    "RKB7zLdMPs3ySdtcIQaF9nUAQ2KJEvldkO3m/bRJFEp54k0CQQDYy+RlTmwRD6hy\n"
    "7UtMjR0H3CSZJeQ8svMCxHLmOluG9H1UKk55ZBYfRTsXniqUkJBZ5wuV1L+pR9EK\n"
    "ca89a+1VAkEA3UmBelwEv2u9cAU1QjKjmwju1JgXbrjEohK+3B5y0ESEXPAwNQT9\n"
    "TrDM1m9AyxYTWLxX93dI5QwNFJtmbtjeBQJARSCWXhsoaDRG8QZrCSjBxfzTCqZD\n"
    "ZXtl807ymCipgJm60LiAt0JLr4LiucAsMZz6+j+quQbSakbFCACB8SLV1QJBAKZQ\n"
    "YKf+EPNtnmta/rRKKvySsi3GQZZN+Dt3q0r094XgeTsAqrqujVNfPhTMeP4qEVBX\n"
    "/iVX2cmMTSh3w3z8MaECQEp0XJWDVKOwcTW6Ajp9SowtmiZ3YDYo1LF9igb4iaLv\n"
    "sWZGfbnU3ryjvkb6YuFjgtzbZDZHWQCo8/cOtOBmPdk=\n"
    "-----END RSA PRIVATE KEY-----\n";
const gnutls_datum_t key = { key_pem, sizeof(key_pem) };

static unsigned char server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICVjCCAcGgAwIBAgIERiYdMTALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTIxWhcNMDgwNDE3MTMyOTIxWjA3MRsw\n"
    "GQYDVQQKExJHbnVUTFMgdGVzdCBzZXJ2ZXIxGDAWBgNVBAMTD3Rlc3QuZ251dGxz\n"
    "Lm9yZzCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA17pcr6MM8C6pJ1aqU46o63+B\n"
    "dUxrmL5K6rce+EvDasTaDQC46kwTHzYWk95y78akXrJutsoKiFV1kJbtple8DDt2\n"
    "DZcevensf9Op7PuFZKBroEjOd35znDET/z3IrqVgbtm2jFqab7a+n2q9p/CgMyf1\n"
    "tx2S5Zacc1LWn9bIjrECAwEAAaOBkzCBkDAMBgNVHRMBAf8EAjAAMBoGA1UdEQQT\n"
    "MBGCD3Rlc3QuZ251dGxzLm9yZzATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8B\n"
    "Af8EBQMDB6AAMB0GA1UdDgQWBBTrx0Vu5fglyoyNgw106YbU3VW0dTAfBgNVHSME\n"
    "GDAWgBTpPBz7rZJu5gakViyi4cBTJ8jylTALBgkqhkiG9w0BAQUDgYEAaFEPTt+7\n"
    "bzvBuOf7+QmeQcn29kT6Bsyh1RHJXf8KTk5QRfwp6ogbp94JQWcNQ/S7YDFHglD1\n"
    "AwUNBRXwd3riUsMnsxgeSDxYBfJYbDLeohNBsqaPDJb7XailWbMQKfAbFQ8cnOxg\n"
    "rOKLUQRWJ0K3HyXRMhbqjdLIaQiCvQLuizo=\n" "-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXAIBAAKBgQDXulyvowzwLqknVqpTjqjrf4F1TGuYvkrqtx74S8NqxNoNALjq\n"
    "TBMfNhaT3nLvxqResm62ygqIVXWQlu2mV7wMO3YNlx696ex/06ns+4VkoGugSM53\n"
    "fnOcMRP/PciupWBu2baMWppvtr6far2n8KAzJ/W3HZLllpxzUtaf1siOsQIDAQAB\n"
    "AoGAYAFyKkAYC/PYF8e7+X+tsVCHXppp8AoP8TEZuUqOZz/AArVlle/ROrypg5kl\n"
    "8YunrvUdzH9R/KZ7saNZlAPLjZyFG9beL/am6Ai7q7Ma5HMqjGU8kTEGwD7K+lbG\n"
    "iomokKMOl+kkbY/2sI5Czmbm+/PqLXOjtVc5RAsdbgvtmvkCQQDdV5QuU8jap8Hs\n"
    "Eodv/tLJ2z4+SKCV2k/7FXSKWe0vlrq0cl2qZfoTUYRnKRBcWxc9o92DxK44wgPi\n"
    "oMQS+O7fAkEA+YG+K9e60sj1K4NYbMPAbYILbZxORDecvP8lcphvwkOVUqbmxOGh\n"
    "XRmTZUuhBrJhJKKf6u7gf3KWlPl6ShKEbwJASC118cF6nurTjuLf7YKARDjNTEws\n"
    "qZEeQbdWYINAmCMj0RH2P0mvybrsXSOD5UoDAyO7aWuqkHGcCLv6FGG+qwJAOVqq\n"
    "tXdUucl6GjOKKw5geIvRRrQMhb/m5scb+5iw8A4LEEHPgGiBaF5NtJZLALgWfo5n\n"
    "hmC8+G8F0F78znQtPwJBANexu+Tg5KfOnzSILJMo3oXiXhf5PqXIDmbN0BKyCKAQ\n"
    "LfkcEcUbVfmDaHpvzwY9VEaoMOKVLitETXdNSxVpvWM=\n"
    "-----END RSA PRIVATE KEY-----\n";

static unsigned char cert_der[602] =
    "\x30\x82\x02\x56\x30\x82\x01\xc1\xa0\x03\x02\x01\x02\x02\x04\x46"
    "\x26\x1d\x31\x30\x0b\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05"
    "\x30\x19\x31\x17\x30\x15\x06\x03\x55\x04\x03\x13\x0e\x47\x6e\x75"
    "\x54\x4c\x53\x20\x74\x65\x73\x74\x20\x43\x41\x30\x1e\x17\x0d\x30"
    "\x37\x30\x34\x31\x38\x31\x33\x32\x39\x32\x31\x5a\x17\x0d\x30\x38"
    "\x30\x34\x31\x37\x31\x33\x32\x39\x32\x31\x5a\x30\x37\x31\x1b\x30"
    "\x19\x06\x03\x55\x04\x0a\x13\x12\x47\x6e\x75\x54\x4c\x53\x20\x74"
    "\x65\x73\x74\x20\x73\x65\x72\x76\x65\x72\x31\x18\x30\x16\x06\x03"
    "\x55\x04\x03\x13\x0f\x74\x65\x73\x74\x2e\x67\x6e\x75\x74\x6c\x73"
    "\x2e\x6f\x72\x67\x30\x81\x9c\x30\x0b\x06\x09\x2a\x86\x48\x86\xf7"
    "\x0d\x01\x01\x01\x03\x81\x8c\x00\x30\x81\x88\x02\x81\x80\xd7\xba"
    "\x5c\xaf\xa3\x0c\xf0\x2e\xa9\x27\x56\xaa\x53\x8e\xa8\xeb\x7f\x81"
    "\x75\x4c\x6b\x98\xbe\x4a\xea\xb7\x1e\xf8\x4b\xc3\x6a\xc4\xda\x0d"
    "\x00\xb8\xea\x4c\x13\x1f\x36\x16\x93\xde\x72\xef\xc6\xa4\x5e\xb2"
    "\x6e\xb6\xca\x0a\x88\x55\x75\x90\x96\xed\xa6\x57\xbc\x0c\x3b\x76"
    "\x0d\x97\x1e\xbd\xe9\xec\x7f\xd3\xa9\xec\xfb\x85\x64\xa0\x6b\xa0"
    "\x48\xce\x77\x7e\x73\x9c\x31\x13\xff\x3d\xc8\xae\xa5\x60\x6e\xd9"
    "\xb6\x8c\x5a\x9a\x6f\xb6\xbe\x9f\x6a\xbd\xa7\xf0\xa0\x33\x27\xf5"
    "\xb7\x1d\x92\xe5\x96\x9c\x73\x52\xd6\x9f\xd6\xc8\x8e\xb1\x02\x03"
    "\x01\x00\x01\xa3\x81\x93\x30\x81\x90\x30\x0c\x06\x03\x55\x1d\x13"
    "\x01\x01\xff\x04\x02\x30\x00\x30\x1a\x06\x03\x55\x1d\x11\x04\x13"
    "\x30\x11\x82\x0f\x74\x65\x73\x74\x2e\x67\x6e\x75\x74\x6c\x73\x2e"
    "\x6f\x72\x67\x30\x13\x06\x03\x55\x1d\x25\x04\x0c\x30\x0a\x06\x08"
    "\x2b\x06\x01\x05\x05\x07\x03\x01\x30\x0f\x06\x03\x55\x1d\x0f\x01"
    "\x01\xff\x04\x05\x03\x03\x07\xa0\x00\x30\x1d\x06\x03\x55\x1d\x0e"
    "\x04\x16\x04\x14\xeb\xc7\x45\x6e\xe5\xf8\x25\xca\x8c\x8d\x83\x0d"
    "\x74\xe9\x86\xd4\xdd\x55\xb4\x75\x30\x1f\x06\x03\x55\x1d\x23\x04"
    "\x18\x30\x16\x80\x14\xe9\x3c\x1c\xfb\xad\x92\x6e\xe6\x06\xa4\x56"
    "\x2c\xa2\xe1\xc0\x53\x27\xc8\xf2\x95\x30\x0b\x06\x09\x2a\x86\x48"
    "\x86\xf7\x0d\x01\x01\x05\x03\x81\x81\x00\x68\x51\x0f\x4e\xdf\xbb"
    "\x6f\x3b\xc1\xb8\xe7\xfb\xf9\x09\x9e\x41\xc9\xf6\xf6\x44\xfa\x06"
    "\xcc\xa1\xd5\x11\xc9\x5d\xff\x0a\x4e\x4e\x50\x45\xfc\x29\xea\x88"
    "\x1b\xa7\xde\x09\x41\x67\x0d\x43\xf4\xbb\x60\x31\x47\x82\x50\xf5"
    "\x03\x05\x0d\x05\x15\xf0\x77\x7a\xe2\x52\xc3\x27\xb3\x18\x1e\x48"
    "\x3c\x58\x05\xf2\x58\x6c\x32\xde\xa2\x13\x41\xb2\xa6\x8f\x0c\x96"
    "\xfb\x5d\xa8\xa5\x59\xb3\x10\x29\xf0\x1b\x15\x0f\x1c\x9c\xec\x60"
    "\xac\xe2\x8b\x51\x04\x56\x27\x42\xb7\x1f\x25\xd1\x32\x16\xea\x8d"
    "\xd2\xc8\x69\x08\x82\xbd\x02\xee\x8b\x3a";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

static time_t mytime(time_t * t)
{
	time_t then = 1207000800;

	if (t)
		*t = then;

	return then;
}

static void check_stored_algos(gnutls_x509_crt_t server_crt)
{
	int ret;
	char oid[256];
	size_t oid_size;

	oid_size = sizeof(oid);
	ret = gnutls_x509_crt_get_signature_oid(server_crt, oid, &oid_size);
	if (ret < 0) {
		fail("cannot get signature algorithm: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (strcmp(oid, "1.2.840.113549.1.1.5") != 0) {
		fail("detected wrong algorithm OID: %s\n", oid);
		exit(1);
	}

	ret = gnutls_x509_crt_get_signature_algorithm(server_crt);
	if (ret != GNUTLS_SIGN_RSA_SHA1) {
		fail("detected wrong algorithm: %s\n", gnutls_sign_get_name(ret));
		exit(1);
	}

	/* PK */
	oid_size = sizeof(oid);
	ret = gnutls_x509_crt_get_pk_oid(server_crt, oid, &oid_size);
	if (ret < 0) {
		fail("cannot get PK algorithm: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (strcmp(oid, "1.2.840.113549.1.1.1") != 0) {
		fail("detected wrong PK algorithm OID: %s\n", oid);
		exit(1);
	}

	ret = gnutls_x509_crt_get_pk_algorithm(server_crt, NULL);
	if (ret != GNUTLS_PK_RSA) {
		fail("detected wrong PK algorithm: %s\n", gnutls_pk_get_name(ret));
		exit(1);
	}

}

#define NAME "localhost"
#define NAME_SIZE (sizeof(NAME)-1)
void doit(void)
{
	int ret;
	const char *path;
	gnutls_datum_t data;
	gnutls_x509_crt_t server_crt, ca_crt2;
	gnutls_x509_trust_list_t tl;
	unsigned int status;
	gnutls_typed_vdata_st vdata;
	gnutls_digest_algorithm_t hash;
	unsigned int mand;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* test for gnutls_certificate_get_issuer() */
	gnutls_x509_trust_list_init(&tl, 0);

	gnutls_x509_crt_init(&server_crt);
	gnutls_x509_crt_init(&ca_crt2);

	path = getenv("X509CERTDIR");
	if (!path)
		path = "./x509cert-dir";

	ret =
	    gnutls_x509_crt_import(server_crt, &cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("gnutls_x509_crt_import");

	check_stored_algos(server_crt);

	ret = gnutls_x509_crt_get_preferred_hash_algorithm(server_crt, &hash, &mand);
	if (ret < 0) {
		fail("error in gnutls_x509_crt_get_preferred_hash_algorithm: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (mand != 0 || hash != GNUTLS_DIG_SHA256) {
		fail("gnutls_x509_crt_get_preferred_hash_algorithm returned: %s/%d\n", gnutls_digest_get_name(hash), mand);
		exit(1);
	}

	ret = gnutls_x509_crt_import(ca_crt2, &ca, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("gnutls_x509_crt_import");

	ret =
	    gnutls_x509_trust_list_add_named_crt(tl, server_crt, NAME,
						 NAME_SIZE, 0);
	if (ret < 0)
		fail("gnutls_x509_trust_list_add_named_crt");

	ret =
	    gnutls_x509_trust_list_verify_named_crt(tl, server_crt, NAME,
						    NAME_SIZE, 0, &status,
						    NULL);
	if (ret < 0 || status != 0)
		fail("gnutls_x509_trust_list_verify_named_crt: %d\n",
		     __LINE__);

	ret =
	    gnutls_x509_trust_list_verify_named_crt(tl, server_crt, NAME,
						    NAME_SIZE - 1, 0,
						    &status, NULL);
	if (ret < 0 || status == 0)
		fail("gnutls_x509_trust_list_verify_named_crt: %d\n",
		     __LINE__);

	ret =
	    gnutls_x509_trust_list_verify_named_crt(tl, server_crt,
						    "other", 5, 0, &status,
						    NULL);
	if (ret < 0 || status == 0)
		fail("gnutls_x509_trust_list_verify_named_crt: %d\n",
		     __LINE__);

	/* check whether the name-only verification works */
	vdata.type = GNUTLS_DT_DNS_HOSTNAME;
	vdata.data = (void*)NAME;
	vdata.size = NAME_SIZE;
	ret =
	    gnutls_x509_trust_list_verify_crt2(tl, &server_crt, 1, &vdata, 1,
						GNUTLS_VERIFY_ALLOW_BROKEN, &status, NULL);
	if (ret < 0 || status != 0)
		fail("gnutls_x509_trust_list_verify_crt2 - 1: status: %x\n", status);

	vdata.type = GNUTLS_DT_DNS_HOSTNAME;
	vdata.data = (void*)NAME;
	vdata.size = NAME_SIZE-2;
	ret =
	    gnutls_x509_trust_list_verify_crt2(tl, &server_crt, 1, &vdata, 1,
						0, &status, NULL);
	if (ret < 0 || status == 0)
		fail("gnutls_x509_trust_list_verify_crt2 - 2: status: %x\n", status);


	/* check whether the key verification works */
	ret = gnutls_x509_trust_list_add_trust_dir(tl, path, NULL, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret != 1)
		fail("gnutls_x509_trust_list_add_trust_dir: %d\n", ret);

	ret =
	    gnutls_x509_trust_list_verify_crt(tl, &server_crt, 1, GNUTLS_VERIFY_ALLOW_BROKEN,
					      &status, NULL);
	if (ret < 0 || status != 0)
		fail("gnutls_x509_trust_list_verify_crt\n");



	/* test convenience functions in verify-high2.c */
	data.data = cert_pem;
	data.size = strlen((char *) cert_pem);
	ret =
	    gnutls_x509_trust_list_add_trust_mem(tl, &data, NULL,
						 GNUTLS_X509_FMT_PEM, 0,
						 0);
	if (ret < 1)
		fail("gnutls_x509_trust_list_add_trust_mem: %d (%s)\n",
		     __LINE__, gnutls_strerror(ret));

	ret =
	    gnutls_x509_trust_list_remove_trust_mem(tl, &data,
						    GNUTLS_X509_FMT_PEM);
	if (ret < 1)
		fail("gnutls_x509_trust_list_add_trust_mem: %d (%s)\n",
		     __LINE__, gnutls_strerror(ret));

	data.data = cert_der;
	data.size = sizeof(cert_der);
	ret =
	    gnutls_x509_trust_list_add_trust_mem(tl, &data, NULL,
						 GNUTLS_X509_FMT_DER, 0,
						 0);
	if (ret < 1)
		fail("gnutls_x509_trust_list_add_trust_mem: %d (%s)\n",
		     __LINE__, gnutls_strerror(ret));

	ret =
	    gnutls_x509_trust_list_remove_trust_mem(tl, &data,
						    GNUTLS_X509_FMT_DER);
	if (ret < 1)
		fail("gnutls_x509_trust_list_add_trust_mem: %d (%s)\n",
		     __LINE__, gnutls_strerror(ret));

	ret = gnutls_x509_trust_list_remove_cas(tl, &ca_crt2, 1);
	if (ret < 1)
		fail("gnutls_x509_trust_list_add_cas");

	ret =
	    gnutls_x509_trust_list_verify_crt(tl, &server_crt, 1, 0,
					      &status, NULL);
	if (ret == 0 && status == 0)
		fail("gnutls_x509_trust_list_verify_crt\n");

	gnutls_x509_trust_list_deinit(tl, 1);
	gnutls_x509_crt_deinit(ca_crt2);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
