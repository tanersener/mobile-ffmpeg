/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (c) 2002 Andrew McDonald <andrew@mcdonald.org.uk>
 *
 * This file is part of GnuTLS-EXTRA.
 *
 * GnuTLS-extra is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *               
 * GnuTLS-extra is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *                               
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gnutls/gnutls.h>
#include <openssl_compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/gnutls_int.h"
#include "../lib/random.h"
#include "../lib/hash_int.h"

/* In win32 X509_NAME is defined in wincrypt.h.
 * undefine it to avoid the conflict with openssl.h.
 */
#ifdef X509_NAME
#undef X509_NAME
#endif
#include <gnutls/openssl.h>

/* Gnulib re-defines shutdown on mingw.  We only use it as a variable
   name, so restore the original name. */
#undef shutdown

/* XXX: See lib/gnutls_int.h. */
#define GNUTLS_POINTER_TO_INT(_) ((int) GNUTLS_POINTER_TO_INT_CAST (_))
#define GNUTLS_INT_TO_POINTER(_) ((void*) GNUTLS_POINTER_TO_INT_CAST (_))

/* WARNING: Error functions aren't currently thread-safe */

static int last_error = 0;

/* Library initialisation functions */

int SSL_library_init(void)
{
	gnutls_global_init();
	/* NB: we haven't got anywhere to call gnutls_global_deinit() */
	return 1;
}

void OpenSSL_add_all_algorithms(void)
{
}


/* SSL_CTX structure handling */

SSL_CTX *SSL_CTX_new(SSL_METHOD * method)
{
	SSL_CTX *ctx;

	ctx = (SSL_CTX *) calloc(1, sizeof(SSL_CTX));
	ctx->method = method;

	return ctx;
}

void SSL_CTX_free(SSL_CTX * ctx)
{
	free(ctx->method);
	free(ctx);
}

int SSL_CTX_set_default_verify_paths(SSL_CTX * ctx)
{
	return 0;
}

int
SSL_CTX_use_certificate_file(SSL_CTX * ctx, const char *certfile, int type)
{
	ctx->certfile = (char *) calloc(1, strlen(certfile) + 1);
	if (!ctx->certfile)
		return -1;
	memcpy(ctx->certfile, certfile, strlen(certfile));

	ctx->certfile_type = type;

	return 1;
}

int
SSL_CTX_use_PrivateKey_file(SSL_CTX * ctx, const char *keyfile, int type)
{
	ctx->keyfile = (char *) calloc(1, strlen(keyfile) + 1);
	if (!ctx->keyfile)
		return -1;
	memcpy(ctx->keyfile, keyfile, strlen(keyfile));

	ctx->keyfile_type = type;

	return 1;

}

void
SSL_CTX_set_verify(SSL_CTX * ctx, int verify_mode,
		   int (*verify_callback) (int, X509_STORE_CTX *))
{
	ctx->verify_mode = verify_mode;
	ctx->verify_callback = verify_callback;
}

unsigned long SSL_CTX_set_options(SSL_CTX * ctx, unsigned long options)
{
	return (ctx->options |= options);
}

long SSL_CTX_set_mode(SSL_CTX * ctx, long mode)
{
	return 0;
}

int SSL_CTX_set_cipher_list(SSL_CTX * ctx, const char *list)
{
	/* FIXME: ignore this for the moment */
	/* We're going to have to parse the "list" string to do this */
	/* It is a string, which in its simplest form is something like
	   "DES-CBC3-SHA:IDEA-CBC-MD5", but can be rather more complicated
	   (see OpenSSL's ciphers(1) manpage for details) */

	return 1;
}


/* SSL_CTX statistics */

long SSL_CTX_sess_number(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_connect(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_connect_good(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_connect_renegotiate(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_accept(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_accept_good(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_accept_renegotiate(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_hits(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_misses(SSL_CTX * ctx)
{
	return 0;
}

long SSL_CTX_sess_timeouts(SSL_CTX * ctx)
{
	return 0;
}



/* SSL structure handling */

SSL *SSL_new(SSL_CTX * ctx)
{
	SSL *ssl;
	int err;

	ssl = (SSL *) calloc(1, sizeof(SSL));
	if (!ssl)
		return NULL;

	err = gnutls_certificate_allocate_credentials(&ssl->gnutls_cred);
	if (err < 0) {
		last_error = err;
		free(ssl);
		return NULL;
	}

	gnutls_init(&ssl->gnutls_state, ctx->method->connend);

	gnutls_priority_set_direct(ssl->gnutls_state,
				   ctx->method->priority_string, NULL);

	gnutls_credentials_set(ssl->gnutls_state, GNUTLS_CRD_CERTIFICATE,
			       ssl->gnutls_cred);
	if (ctx->certfile)
		gnutls_certificate_set_x509_trust_file(ssl->gnutls_cred,
						       ctx->certfile,
						       ctx->certfile_type);
	if (ctx->keyfile)
		gnutls_certificate_set_x509_key_file(ssl->gnutls_cred,
						     ctx->certfile,
						     ctx->keyfile,
						     ctx->keyfile_type);
	ssl->ctx = ctx;
	ssl->verify_mode = ctx->verify_mode;
	ssl->verify_callback = ctx->verify_callback;

	ssl->options = ctx->options;

	ssl->rfd = (gnutls_transport_ptr_t) - 1;
	ssl->wfd = (gnutls_transport_ptr_t) - 1;

	return ssl;
}

void SSL_free(SSL * ssl)
{
	gnutls_certificate_free_credentials(ssl->gnutls_cred);
	gnutls_deinit(ssl->gnutls_state);
	free(ssl);
}

void SSL_load_error_strings(void)
{
}

int SSL_get_error(SSL * ssl, int ret)
{
	if (ret > 0)
		return SSL_ERROR_NONE;

	return SSL_ERROR_ZERO_RETURN;
}

int SSL_set_fd(SSL * ssl, int fd)
{
	gnutls_transport_set_ptr(ssl->gnutls_state,
				 GNUTLS_INT_TO_POINTER(fd));
	return 1;
}

int SSL_set_rfd(SSL * ssl, int fd)
{
	ssl->rfd = GNUTLS_INT_TO_POINTER(fd);

	if (ssl->wfd != (gnutls_transport_ptr_t) - 1)
		gnutls_transport_set_ptr2(ssl->gnutls_state, ssl->rfd,
					  ssl->wfd);

	return 1;
}

int SSL_set_wfd(SSL * ssl, int fd)
{
	ssl->wfd = GNUTLS_INT_TO_POINTER(fd);

	if (ssl->rfd != (gnutls_transport_ptr_t) - 1)
		gnutls_transport_set_ptr2(ssl->gnutls_state, ssl->rfd,
					  ssl->wfd);

	return 1;
}

void SSL_set_bio(SSL * ssl, BIO * rbio, BIO * wbio)
{
	gnutls_transport_set_ptr2(ssl->gnutls_state, rbio->fd, wbio->fd);
	/*    free(BIO); ? */
}

void SSL_set_connect_state(SSL * ssl)
{
}

int SSL_pending(SSL * ssl)
{
	return gnutls_record_check_pending(ssl->gnutls_state);
}

void
SSL_set_verify(SSL * ssl, int verify_mode,
	       int (*verify_callback) (int, X509_STORE_CTX *))
{
	ssl->verify_mode = verify_mode;
	ssl->verify_callback = verify_callback;
}

const X509 *SSL_get_peer_certificate(SSL * ssl)
{
	const gnutls_datum_t *cert_list;
	unsigned int cert_list_size = 0;

	cert_list = gnutls_certificate_get_peers(ssl->gnutls_state,
						 &cert_list_size);

	return cert_list;
}

/* SSL connection open/close/read/write functions */

int SSL_connect(SSL * ssl)
{
	X509_STORE_CTX *store;
	unsigned int cert_list_size = 0;
	int err;
	char x_priority[256];
	/* take options into account before connecting */

	memset(x_priority, 0, sizeof(x_priority));
	if (ssl->options & SSL_OP_NO_TLSv1) {
		snprintf(x_priority, sizeof(x_priority), "%s:-VERS-TLS1.0",
			 ssl->ctx->method->priority_string);
		err =
		    gnutls_priority_set_direct(ssl->gnutls_state,
					       x_priority, NULL);
		if (err < 0) {
			last_error = err;
			return 0;
		}
	}

	err = gnutls_handshake(ssl->gnutls_state);
	ssl->last_error = err;

	if (err < 0) {
		last_error = err;
		return 0;
	}

	store = (X509_STORE_CTX *) calloc(1, sizeof(X509_STORE_CTX));
	store->ssl = ssl;
	store->cert_list = gnutls_certificate_get_peers(ssl->gnutls_state,
							&cert_list_size);

	if (ssl->verify_callback) {
		ssl->verify_callback(1 /*FIXME*/, store);
	}
	ssl->state = SSL_ST_OK;

	err = store->error;
	free(store);

	/* FIXME: deal with error from callback */

	return 1;
}

int SSL_accept(SSL * ssl)
{
	X509_STORE_CTX *store;
	unsigned int cert_list_size = 0;
	int err;
	char x_priority[256];
	/* take options into account before connecting */

	memset(x_priority, 0, sizeof(x_priority));
	if (ssl->options & SSL_OP_NO_TLSv1) {
		snprintf(x_priority, sizeof(x_priority), "%s:-VERS-TLS1.0",
			 ssl->ctx->method->priority_string);
		err =
		    gnutls_priority_set_direct(ssl->gnutls_state,
					       x_priority, NULL);
		if (err < 0) {
			last_error = err;
			return 0;
		}
	}

	/* FIXME: dh params, do we want client cert? */

	err = gnutls_handshake(ssl->gnutls_state);
	ssl->last_error = err;

	if (err < 0) {
		last_error = err;
		return 0;
	}

	store = (X509_STORE_CTX *) calloc(1, sizeof(X509_STORE_CTX));
	store->ssl = ssl;
	store->cert_list = gnutls_certificate_get_peers(ssl->gnutls_state,
							&cert_list_size);

	if (ssl->verify_callback) {
		ssl->verify_callback(1 /*FIXME*/, store);
	}
	ssl->state = SSL_ST_OK;

	err = store->error;
	free(store);

	/* FIXME: deal with error from callback */

	return 1;
}

int SSL_shutdown(SSL * ssl)
{
	if (!ssl->shutdown) {
		gnutls_bye(ssl->gnutls_state, GNUTLS_SHUT_WR);
		ssl->shutdown++;
	} else {
		gnutls_bye(ssl->gnutls_state, GNUTLS_SHUT_RDWR);
		ssl->shutdown++;
	}

	/* FIXME */
	return 1;
}

int SSL_read(SSL * ssl, void *buf, int len)
{
	int ret;

	ret = gnutls_record_recv(ssl->gnutls_state, buf, len);
	ssl->last_error = ret;

	if (ret < 0) {
		last_error = ret;
		return 0;
	}

	return ret;
}

int SSL_write(SSL * ssl, const void *buf, int len)
{
	int ret;

	ret = gnutls_record_send(ssl->gnutls_state, buf, len);
	ssl->last_error = ret;

	if (ret < 0) {
		last_error = ret;
		return 0;
	}

	return ret;
}

int SSL_want(SSL * ssl)
{
	return SSL_NOTHING;
}


/* SSL_METHOD functions */

SSL_METHOD *SSLv23_client_method(void)
{
	SSL_METHOD *m;
	m = (SSL_METHOD *) calloc(1, sizeof(SSL_METHOD));
	if (!m)
		return NULL;

	strcpy(m->priority_string,
	       "NORMAL");

	m->connend = GNUTLS_CLIENT;

	return m;
}

SSL_METHOD *SSLv23_server_method(void)
{
	SSL_METHOD *m;
	m = (SSL_METHOD *) calloc(1, sizeof(SSL_METHOD));
	if (!m)
		return NULL;

	strcpy(m->priority_string,
	       "NORMAL");
	m->connend = GNUTLS_SERVER;

	return m;
}

SSL_METHOD *SSLv3_client_method(void)
{
	SSL_METHOD *m;
	m = (SSL_METHOD *) calloc(1, sizeof(SSL_METHOD));
	if (!m)
		return NULL;

	strcpy(m->priority_string,
	       "NONE:+VERS-SSL3.0:+CIPHER-ALL:+COMP-ALL:+RSA:+DHE-RSA:+DHE-DSS:+MAC-ALL");
	m->connend = GNUTLS_CLIENT;

	return m;
}

SSL_METHOD *SSLv3_server_method(void)
{
	SSL_METHOD *m;
	m = (SSL_METHOD *) calloc(1, sizeof(SSL_METHOD));
	if (!m)
		return NULL;

	strcpy(m->priority_string,
	       "NONE:+VERS-SSL3.0:+CIPHER-ALL:+COMP-ALL:+RSA:+DHE-RSA:+DHE-DSS:+MAC-ALL");
	m->connend = GNUTLS_SERVER;

	return m;
}

SSL_METHOD *TLSv1_client_method(void)
{
	SSL_METHOD *m;
	m = (SSL_METHOD *) calloc(1, sizeof(SSL_METHOD));
	if (!m)
		return NULL;

	strcpy(m->priority_string,
	       "NONE:+VERS-TLS1.0:+CIPHER-ALL:+COMP-ALL:+RSA:+DHE-RSA:+DHE-DSS:+MAC-ALL");
	m->connend = GNUTLS_CLIENT;

	return m;
}

SSL_METHOD *TLSv1_server_method(void)
{
	SSL_METHOD *m;
	m = (SSL_METHOD *) calloc(1, sizeof(SSL_METHOD));
	if (!m)
		return NULL;

	strcpy(m->priority_string,
	       "NONE:+VERS-TLS1.0:+CIPHER-ALL:+COMP-ALL:+RSA:+DHE-RSA:+DHE-DSS:+MAC-ALL");
	m->connend = GNUTLS_SERVER;

	return m;
}


/* SSL_CIPHER functions */

SSL_CIPHER *SSL_get_current_cipher(SSL * ssl)
{
	if (!ssl)
		return NULL;

	ssl->ciphersuite.version =
	    gnutls_protocol_get_version(ssl->gnutls_state);
	ssl->ciphersuite.cipher = gnutls_cipher_get(ssl->gnutls_state);
	ssl->ciphersuite.kx = gnutls_kx_get(ssl->gnutls_state);
	ssl->ciphersuite.mac = gnutls_mac_get(ssl->gnutls_state);
	ssl->ciphersuite.compression =
	    gnutls_compression_get(ssl->gnutls_state);
	ssl->ciphersuite.cert =
	    gnutls_certificate_type_get(ssl->gnutls_state);

	return &(ssl->ciphersuite);
}

const char *SSL_CIPHER_get_name(SSL_CIPHER * cipher)
{
	if (!cipher)
		return ("NONE");

	return gnutls_cipher_suite_get_name(cipher->kx,
					    cipher->cipher, cipher->mac);
}

int SSL_CIPHER_get_bits(SSL_CIPHER * cipher, int *bits)
{
	int bit_result;

	if (!cipher)
		return 0;

	bit_result = (8 * gnutls_cipher_get_key_size(cipher->cipher));

	if (bits)
		*bits = bit_result;

	return bit_result;
}

const char *SSL_CIPHER_get_version(SSL_CIPHER * cipher)
{
	const char *ret;

	if (!cipher)
		return ("(NONE)");

	ret = gnutls_protocol_get_name(cipher->version);
	if (ret)
		return ret;

	return ("unknown");
}

char *SSL_CIPHER_description(SSL_CIPHER * cipher, char *buf, int size)
{
	char *tmpbuf;
	int tmpsize;
	int local_alloc;

	if (buf) {
		tmpbuf = buf;
		tmpsize = size;
		local_alloc = 0;
	} else {
		tmpbuf = (char *) malloc(128);
		tmpsize = 128;
		local_alloc = 1;
	}

	if (snprintf(tmpbuf, tmpsize, "%s %s %s %s",
		     gnutls_protocol_get_name(cipher->version),
		     gnutls_kx_get_name(cipher->kx),
		     gnutls_cipher_get_name(cipher->cipher),
		     gnutls_mac_get_name(cipher->mac)) == -1) {
		if (local_alloc)
			free(tmpbuf);
		return (char *) "Buffer too small";
	}

	return tmpbuf;
}


/* X509 functions */

X509_NAME *X509_get_subject_name(const X509 * cert)
{
	gnutls_x509_dn *dn;
	dn = (gnutls_x509_dn *) calloc(1, sizeof(gnutls_x509_dn));
	if (gnutls_x509_extract_certificate_dn(cert, dn) < 0) {
		free(dn);
		return NULL;
	}
	return dn;
}

X509_NAME *X509_get_issuer_name(const X509 * cert)
{
	gnutls_x509_dn *dn;
	dn = (gnutls_x509_dn *) calloc(1, sizeof(gnutls_x509_dn));
	if (gnutls_x509_extract_certificate_issuer_dn(cert, dn) < 0) {
		free(dn);
		return NULL;
	}
	return dn;
}

char *X509_NAME_oneline(gnutls_x509_dn * name, char *buf, int len)
{
	/* XXX openssl allocates buffer if buf == NULL */
	if (!buf)
		return NULL;
	memset(buf, 0, len);

	snprintf(buf, len - 1,
		 "C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s/Email=%s",
		 name->country, name->state_or_province_name,
		 name->locality_name, name->organization,
		 name->organizational_unit_name, name->common_name,
		 name->email);
	return buf;
}

void X509_free(const X509 * cert)
{
	/* only get certificates as const items */
}


/* BIO functions */

void BIO_get_fd(gnutls_session_t gnutls_state, int *fd)
{
	gnutls_transport_ptr_t tmp =
	    gnutls_transport_get_ptr(gnutls_state);
	*fd = GNUTLS_POINTER_TO_INT(tmp);
}

BIO *BIO_new_socket(int sock, int close_flag)
{
	BIO *bio;

	bio = (BIO *) malloc(sizeof(BIO));
	if (!bio)
		return NULL;

	bio->fd = GNUTLS_INT_TO_POINTER(sock);

	return bio;
}


/* error handling */

unsigned long ERR_get_error(void)
{
	unsigned long ret;

	ret = -1 * last_error;
	last_error = 0;

	return ret;
}

const char *ERR_error_string(unsigned long e, char *buf)
{
	return gnutls_strerror(-1 * e);
}


/* RAND functions */

int RAND_status(void)
{
	return 1;
}

void RAND_seed(const void *buf, int num)
{
}

int RAND_bytes(unsigned char *buf, int num)
{
	gnutls_rnd(GNUTLS_RND_RANDOM, buf, num);
	return 1;
}

int RAND_pseudo_bytes(unsigned char *buf, int num)
{
	gnutls_rnd(GNUTLS_RND_NONCE, buf, num);
	return 1;
}

const char *RAND_file_name(char *buf, size_t len)
{
	return "";
}

int RAND_load_file(const char *name, long maxbytes)
{
	return maxbytes;
}

int RAND_write_file(const char *name)
{
	return 0;
}

int RAND_egd_bytes(const char *path, int bytes)
{
	/* fake it */
	return bytes;
}


/* message digest functions */

void MD5_Init(MD5_CTX * ctx)
{
	int ret;
	ret = gnutls_hash_init((void *) &ctx->handle, GNUTLS_DIG_MD5);
	if (ret < 0)
		abort();
}

void MD5_Update(MD5_CTX * ctx, const void *buf, int len)
{
	gnutls_hash(ctx->handle, buf, len);
}

void MD5_Final(unsigned char *md, MD5_CTX * ctx)
{
	gnutls_hash_deinit(ctx->handle, md);
}

unsigned char *MD5(const unsigned char *buf, unsigned long len,
		   unsigned char *md)
{
	if (!md)
		return NULL;

	gnutls_hash_fast(GNUTLS_DIG_MD5, buf, len, md);

	return md;
}

void RIPEMD160_Init(RIPEMD160_CTX * ctx)
{
	int ret;
	ret = gnutls_hash_init((void *) &ctx->handle, GNUTLS_DIG_RMD160);
	if (ret < 0)
		abort();
}

void RIPEMD160_Update(RIPEMD160_CTX * ctx, const void *buf, int len)
{
	gnutls_hash(ctx->handle, buf, len);
}

void RIPEMD160_Final(unsigned char *md, RIPEMD160_CTX * ctx)
{
	gnutls_hash_deinit(ctx->handle, md);
}

unsigned char *RIPEMD160(const unsigned char *buf, unsigned long len,
			 unsigned char *md)
{
	if (!md)
		return NULL;

	gnutls_hash_fast(GNUTLS_DIG_RMD160, buf, len, md);

	return md;
}
