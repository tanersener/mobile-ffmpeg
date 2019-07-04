/*
 * Copyright (C) 2004-2015 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
 * Copyright (C) 2015 Red Hat, Inc.
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

/* Tests whether memory input to ciphers are properly aligned */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)

/* socketpair isn't supported on Win32. */
int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <nettle/aes.h>
#include <nettle/cbc.h>
#include <nettle/gcm.h>
#include <assert.h>

#include "utils.h"

#include "ex-session-info.c"
#include "ex-x509-info.c"

pid_t child;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", child ? "server" : "client", level,
		str);
}

/* A very basic TLS client, with anonymous authentication.
 */


#define MAX_BUF 1024
#define MSG "Hello TLS"

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

struct myaes_ctx {
	struct aes_ctx aes;
	unsigned char iv[16];
	int enc;
};

static int
myaes_init(gnutls_cipher_algorithm_t algorithm, void **_ctx, int enc)
{
	/* we use key size to distinguish */
	if (algorithm != GNUTLS_CIPHER_AES_128_CBC
	    && algorithm != GNUTLS_CIPHER_AES_192_CBC
	    && algorithm != GNUTLS_CIPHER_AES_256_CBC)
		return GNUTLS_E_INVALID_REQUEST;

	*_ctx = calloc(1, sizeof(struct myaes_ctx));
	if (*_ctx == NULL) {
		return GNUTLS_E_MEMORY_ERROR;
	}

	((struct myaes_ctx *) (*_ctx))->enc = enc;

	return 0;
}

static int
myaes_setkey(void *_ctx, const void *userkey, size_t keysize)
{
	struct myaes_ctx *ctx = _ctx;

	if (ctx->enc)
		aes_set_encrypt_key(&ctx->aes, keysize, userkey);
	else
		aes_set_decrypt_key(&ctx->aes, keysize, userkey);

	return 0;
}

static int myaes_setiv(void *_ctx, const void *iv, size_t iv_size)
{
	struct myaes_ctx *ctx = _ctx;

	memcpy(ctx->iv, iv, 16);
	return 0;
}

static int
myaes_encrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct myaes_ctx *ctx = _ctx;

#if 0 /* this is under the control of the caller */
	if (((unsigned long)src)%16 != 0) {
		fail("encrypt: source is not 16-byte aligned: %lu\n", ((unsigned long)src)%16);
	}
#endif

	if (((unsigned long)dst)%16 != 0) {
		fail("encrypt: dest is not 16-byte aligned: %lu\n", ((unsigned long)dst)%16);
	}

	cbc_encrypt(&ctx->aes, (nettle_cipher_func*)aes_encrypt, 16, ctx->iv, src_size, dst, src);
	return 0;
}

static int
myaes_decrypt(void *_ctx, const void *src, size_t src_size,
	    void *dst, size_t dst_size)
{
	struct myaes_ctx *ctx = _ctx;

	if (((unsigned long)src)%16 != 0) {
		fail("decrypt: source is not 16-byte aligned: %lu\n", ((unsigned long)src)%16);
	}

#if 0 /* this is under the control of the caller */
	if (((unsigned long)dst)%16 != 0) {
		fail("decrypt: dest is not 16-byte aligned: %lu\n", ((unsigned long)dst)%16);
	}
#endif

	cbc_decrypt(&ctx->aes, (nettle_cipher_func*)aes_decrypt, 16, ctx->iv, src_size, dst, src);

	return 0;
}

static void myaes_deinit(void *_ctx)
{
	free(_ctx);
}

static void client(int sd, const char *prio)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t xcred;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_certificate_allocate_credentials(&xcred);

	/* sets the trusted cas file
	 */
	gnutls_certificate_set_x509_trust_mem(xcred, &ca,
					      GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_x509_key_mem(xcred, &cert, &key,
					    GNUTLS_X509_FMT_PEM);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	/* put the x509 credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	gnutls_transport_set_int(session, sd);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		goto end;
	} else if (debug) {
		success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* see the Getting peer's information example */
	if (debug)
		print_info(session);

	ret = gnutls_record_send(session, MSG, strlen(MSG));

	if (ret == strlen(MSG)) {
		if (debug)
			success("client: sent record.\n");
	} else {
		fail("client: failed to send record.\n");
		gnutls_perror(ret);
		goto end;
	}

	ret = gnutls_record_recv(session, buffer, MAX_BUF);

	if (debug)
		success("client: recv returned %d.\n", ret);

	if (ret == GNUTLS_E_REHANDSHAKE) {
		if (debug)
			success("client: doing handshake!\n");
		ret = gnutls_handshake(session);
		if (ret == 0) {
			if (debug)
				success
				    ("client: handshake complete, reading again.\n");
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} else {
			fail("client: handshake failed.\n");
		}
	}

	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
		goto end;
	}

	if (debug) {
		printf("- Received %d bytes: ", ret);
		for (ii = 0; ii < ret; ii++) {
			fputc(buffer[ii], stdout);
		}
		fputs("\n", stdout);
	}

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

      end:

	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(xcred);
}

/* This is a sample TLS 1.0 echo server, using X.509 authentication.
 */

#define MAX_BUF 1024

/* These are global */



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

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

static void server(int sd, const char *prio)
{
	gnutls_certificate_credentials_t x509_cred;
	int ret;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];

	/* this must be called once in the program
	 */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_trust_mem(x509_cred, &ca,
					      GNUTLS_X509_FMT_PEM);

	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	if (debug)
		success("Launched, generating DH parameters...\n");

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, sd);
	ret = gnutls_handshake(session);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug) {
		success("server: Handshake was completed\n");
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));
	}

	/* see the Getting peer's information example */
	if (debug)
		print_info(session);

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);
		ret = gnutls_record_recv(session, buffer, MAX_BUF);

		if (ret == 0) {
			if (debug)
				success
				    ("server: Peer has closed the GnuTLS connection\n");
			break;
		} else if (ret < 0) {
			fail("server: Received corrupted data(%d). Closing...\n", ret);
			break;
		} else if (ret > 0) {
			/* echo data back to the client
			 */
			gnutls_record_send(session, buffer,
					   strlen(buffer));
		}
	}
	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	if (debug)
		success("server: finished\n");
}

static
void start(const char *prio)
{
	int sockets[2];
	int err;

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		perror("socketpair");
		fail("socketpair failed\n");
		return;
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status;

		server(sockets[0], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		client(sockets[1], prio);
		exit(0);
	}
}

void doit(void)
{
	int ret;

	global_init();

	ret = gnutls_crypto_register_cipher(GNUTLS_CIPHER_AES_128_CBC, 1,
		myaes_init,
		myaes_setkey,
		myaes_setiv,
		myaes_encrypt,
		myaes_decrypt,
		myaes_deinit);
	if (ret < 0) {
		fail("%d: cannot register cipher\n", __LINE__);
	}


	start("NORMAL:-CIPHER-ALL:+AES-128-CBC:-VERS-ALL:+VERS-TLS1.1");
	start("NORMAL:-CIPHER-ALL:+AES-128-CBC:-VERS-ALL:+VERS-TLS1.2");

	gnutls_global_deinit();
}

#endif				/* _WIN32 */
