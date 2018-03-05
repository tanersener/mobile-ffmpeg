/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef CERTTOOL_COMMON_H
#define CERTTOOL_COMMON_H

#include <gnutls/x509.h>
#include <stdio.h>

#define TYPE_CRT 1
#define TYPE_CRQ 2

void certtool_version(void);

#include <gnutls/x509.h>
#include <gnutls/abstract.h>

typedef struct common_info {
	const char *secret_key;
	const char *privkey;
	const char *pubkey;
	int pkcs8;
	int incert_format;
	int outcert_format;
	const char *cert;

	const char *request;
	const char *crl;
	const char *ca;
	const char *data_file;
	const char *ca_privkey;
	unsigned bits;
	const char *sec_param;
	const char *pkcs_cipher;
	const char *password;
	int null_password;
	int empty_password;
	unsigned int crq_extensions;
	unsigned int v1_cert;
	/* for key generation */
	unsigned provable;

	unsigned char *seed;
	unsigned seed_size;

	const char *pin;
	const char *so_pin;

	int cprint;
	unsigned key_usage;

	unsigned int batch;
	/* when printing PKCS #11 objects, only print urls */
	unsigned int only_urls;
	unsigned int verbose;
} common_info_st;

int cipher_to_flags(const char *cipher);

void
print_private_key(FILE *outfile, common_info_st * cinfo, gnutls_x509_privkey_t key);
gnutls_pubkey_t load_public_key_or_import(int mand,
					  gnutls_privkey_t privkey,
					  common_info_st * info);
gnutls_privkey_t load_private_key(int mand, common_info_st * info);
gnutls_x509_privkey_t load_x509_private_key(int mand,
					    common_info_st * info);
gnutls_x509_privkey_t *load_privkey_list(int mand, size_t * privkey_size,
					 common_info_st * info);
gnutls_x509_crq_t load_request(common_info_st * info);
gnutls_privkey_t load_ca_private_key(common_info_st * info);
gnutls_x509_crt_t load_ca_cert(unsigned mand, common_info_st * info);
gnutls_x509_crt_t load_cert(int mand, common_info_st * info);
gnutls_datum_t *load_secret_key(int mand, common_info_st * info);
gnutls_pubkey_t load_pubkey(int mand, common_info_st * info);
gnutls_x509_crt_t *load_cert_list(int mand, size_t * size,
				  common_info_st * info);
gnutls_x509_crl_t *load_crl_list(int mand, size_t * size,
				  common_info_st * info);
int get_bits(gnutls_pk_algorithm_t key_type, int info_bits,
	     const char *info_sec_param, int warn);

gnutls_sec_param_t str_to_sec_param(const char *str);
gnutls_ecc_curve_t str_to_curve(const char *str);

/* prime.c */
int generate_prime(FILE * outfile, int how, common_info_st * info);
void dh_info(FILE * infile, FILE * outfile, common_info_st * ci);

gnutls_x509_privkey_t *load_privkey_list(int mand, size_t * privkey_size,
					 common_info_st * info);

void _pubkey_info(FILE * outfile, gnutls_certificate_print_formats_t,
		  gnutls_pubkey_t pubkey);
void print_ecc_pkey(FILE * outfile, gnutls_ecc_curve_t curve,
		    gnutls_datum_t * k, gnutls_datum_t * x,
		    gnutls_datum_t * y, int cprint);
void print_rsa_pkey(FILE * outfile, gnutls_datum_t * m, gnutls_datum_t * e,
		    gnutls_datum_t * d, gnutls_datum_t * p,
		    gnutls_datum_t * q, gnutls_datum_t * u,
		    gnutls_datum_t * exp1, gnutls_datum_t * exp2,
		    int cprint);
void print_dsa_pkey(FILE * outfile, gnutls_datum_t * x, gnutls_datum_t * y,
		    gnutls_datum_t * p, gnutls_datum_t * q,
		    gnutls_datum_t * g, int cprint);

FILE *safe_open_rw(const char *file, int privkey_op);

const char *get_password(common_info_st * cinfo, unsigned int *flags,
			 int confirm);

extern unsigned char *lbuffer;
extern unsigned long lbuffer_size;

void fix_lbuffer(unsigned long);

void decode_seed(gnutls_datum_t *seed, const char *hex, unsigned hex_size);

#endif
