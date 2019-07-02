/*
 * Copyright (C) 2016 Nikos Mavrogiannopoulos
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

#ifndef GNUTLS_TESTS_COMMON_CERT_KEY_EXCHANGE_H
#define GNUTLS_TESTS_COMMON_CERT_KEY_EXCHANGE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnutls/gnutls.h>

#define USE_CERT 1
#define ASK_CERT 2

extern const char *server_priority;

#define try_x509(name, client_prio, client_kx, server_sign_algo, client_sign_algo) \
	try_with_key(name, client_prio, client_kx, server_sign_algo, client_sign_algo, \
		&server_ca3_localhost_cert, &server_ca3_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN)

#define try_rawpk(name, client_prio, client_kx, server_sign_algo, client_sign_algo) \
	try_with_key(name, client_prio, client_kx, server_sign_algo, client_sign_algo, \
		&rawpk_public_key1, &rawpk_private_key1, NULL, NULL, 0, GNUTLS_CRT_RAWPK, GNUTLS_CRT_UNKNOWN)

#define try_x509_ks(name, client_prio, client_kx, group) \
	try_with_key_ks(name, client_prio, client_kx, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, GNUTLS_SIGN_UNKNOWN, \
		&server_ca3_localhost_cert, &server_ca3_key, NULL, NULL, 0, group, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN)

#define try_x509_cli(name, client_prio, client_kx, server_sign_algo, client_sign_algo, client_cert) \
	try_with_key(name, client_prio, client_kx, server_sign_algo, client_sign_algo, \
		&server_ca3_localhost_cert, &server_ca3_key, &cli_ca3_cert, &cli_ca3_key, client_cert, GNUTLS_CRT_X509, GNUTLS_CRT_X509)

#define try_rawpk_cli(name, client_prio, client_kx, server_sign_algo, client_sign_algo, client_cert) \
	try_with_key(name, client_prio, client_kx, server_sign_algo, client_sign_algo, \
		&rawpk_public_key1, &rawpk_private_key1, &rawpk_public_key2, &rawpk_private_key2, client_cert, GNUTLS_CRT_RAWPK, GNUTLS_CRT_RAWPK)

void try_with_rawpk_key_fail(const char *name, const char *client_prio,
			     int server_err, int client_err,
			     const gnutls_datum_t *serv_cert,
			     const gnutls_datum_t *serv_key,
			     unsigned server_ku,
			     const gnutls_datum_t *cli_cert,
			     const gnutls_datum_t *cli_key,
			     unsigned client_ku);

void try_with_key_ks(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
		gnutls_sign_algorithm_t server_sign_algo,
		gnutls_sign_algorithm_t client_sign_algo,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *cli_cert,
		const gnutls_datum_t *cli_key,
		unsigned client_cert,
		unsigned exp_group,
		gnutls_certificate_type_t server_ctype,
		gnutls_certificate_type_t client_ctype);

inline static
void try_with_key(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
		gnutls_sign_algorithm_t server_sign_algo,
		gnutls_sign_algorithm_t client_sign_algo,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *cli_cert,
		const gnutls_datum_t *cli_key,
		unsigned client_cert,
		gnutls_certificate_type_t server_ctype,
		gnutls_certificate_type_t client_ctype)
{
	return try_with_key_ks(name, client_prio, client_kx, server_sign_algo, client_sign_algo,
			       serv_cert, serv_key, cli_cert, cli_key, client_cert, 0, server_ctype, client_ctype);
}

void try_with_key_fail(const char *name, const char *client_prio,
			int server_err, int client_err,
			const gnutls_datum_t *serv_cert,
			const gnutls_datum_t *serv_key,
			const gnutls_datum_t *cli_cert,
			const gnutls_datum_t *cli_key);

#define dtls_try(name, client_prio, client_kx, server_sign_algo, client_sign_algo) \
	dtls_try_with_key(name, client_prio, client_kx, server_sign_algo, client_sign_algo, \
		&server_ca3_localhost_cert, &server_ca3_key, NULL, NULL, 0)

#define dtls_try_cli(name, client_prio, client_kx, server_sign_algo, client_sign_algo, client_cert) \
	dtls_try_with_key(name, client_prio, client_kx, server_sign_algo, client_sign_algo, \
		&server_ca3_localhost_cert, &server_ca3_key, &cli_ca3_cert, &cli_ca3_key, client_cert)

#define dtls_try_with_key(name, client_prio, client_kx, server_sign_algo, client_sign_algo, serv_cert, serv_key, cli_cert, cli_key, client_cert) \
	dtls_try_with_key_mtu(name, client_prio, client_kx, server_sign_algo, client_sign_algo, serv_cert, serv_key, cli_cert, cli_key, client_cert, 0)

void dtls_try_with_key_mtu(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
		gnutls_sign_algorithm_t server_sign_algo,
		gnutls_sign_algorithm_t client_sign_algo,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *cli_cert,
		const gnutls_datum_t *cli_key,
		unsigned client_cert, unsigned mtu);

#endif /* GNUTLS_TESTS_COMMON_CERT_KEY_EXCHANGE_H */
