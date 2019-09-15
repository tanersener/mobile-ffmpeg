/*
 * Copyright (C) 2001-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include <auth/srp_kx.h>
#include <state.h>

#ifdef ENABLE_SRP

#include "srp.h"
#include <auth/srp_passwd.h>
#include <mpi.h>
#include <num.h>
#include <file.h>
#include <algorithms.h>
#include <random.h>

#include "debug.h"


/* Here functions for SRP (like g^x mod n) are defined 
 */

static int
_gnutls_srp_gx(uint8_t * text, size_t textsize, uint8_t ** result,
	       bigint_t g, bigint_t prime)
{
	bigint_t x, e = NULL;
	size_t result_size;
	int ret;

	if (_gnutls_mpi_init_scan_nz(&x, text, textsize)) {
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	ret = _gnutls_mpi_init(&e);
	if (ret < 0)
		goto cleanup;

	/* e = g^x mod prime (n) */
	ret = _gnutls_mpi_powm(e, g, x, prime);
	if (ret < 0)
		goto cleanup;

	ret = _gnutls_mpi_print(e, NULL, &result_size);
	if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER) {
		*result = gnutls_malloc(result_size);
		if ((*result) == NULL) {
			ret = GNUTLS_E_MEMORY_ERROR;
			goto cleanup;
		}

		ret = _gnutls_mpi_print(e, *result, &result_size);
		if (ret < 0)
			goto cleanup;

		ret = result_size;
	} else {
		gnutls_assert();
		ret = GNUTLS_E_MPI_PRINT_FAILED;
	}

cleanup:
	_gnutls_mpi_release(&e);
	_gnutls_mpi_release(&x);

	return ret;

}


/****************
 * Choose a random value b and calculate B = (k* v + g^b) % N.
 * where k == SHA1(N|g)
 * Return: B and if ret_b is not NULL b.
 */
bigint_t
_gnutls_calc_srp_B(bigint_t * ret_b, bigint_t g, bigint_t n, bigint_t v)
{
	bigint_t tmpB = NULL, tmpV = NULL;
	bigint_t b = NULL, B = NULL, k = NULL;
	int ret;

	/* calculate:  B = (k*v + g^b) % N 
	 */
	ret = _gnutls_mpi_init_multi(&tmpV, &tmpB, &B, &b, NULL);
	if (ret < 0)
		return NULL;

	_gnutls_mpi_random_modp(b, n, GNUTLS_RND_RANDOM);

	k = _gnutls_calc_srp_u(n, g, n);
	if (k == NULL) {
		gnutls_assert();
		goto error;
	}

	ret = _gnutls_mpi_mulm(tmpV, k, v, n);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = _gnutls_mpi_powm(tmpB, g, b, n);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = _gnutls_mpi_addm(B, tmpV, tmpB, n);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	_gnutls_mpi_release(&k);
	_gnutls_mpi_release(&tmpB);
	_gnutls_mpi_release(&tmpV);

	if (ret_b)
		*ret_b = b;
	else
		_gnutls_mpi_release(&b);

	return B;

      error:
	_gnutls_mpi_release(&b);
	_gnutls_mpi_release(&B);
	_gnutls_mpi_release(&k);
	_gnutls_mpi_release(&tmpB);
	_gnutls_mpi_release(&tmpV);
	return NULL;

}

/* This calculates the SHA1(A | B)
 * A and B will be left-padded with zeros to fill n_size.
 */
bigint_t _gnutls_calc_srp_u(bigint_t A, bigint_t B, bigint_t n)
{
	size_t b_size, a_size;
	uint8_t *holder, hd[MAX_HASH_SIZE];
	size_t holder_size, hash_size, n_size;
	int ret;
	bigint_t res;

	/* get the size of n in bytes */
	_gnutls_mpi_print(n, NULL, &n_size);

	_gnutls_mpi_print(A, NULL, &a_size);
	_gnutls_mpi_print(B, NULL, &b_size);

	if (a_size > n_size || b_size > n_size) {
		gnutls_assert();
		return NULL;	/* internal error */
	}

	holder_size = n_size + n_size;

	holder = gnutls_calloc(1, holder_size);
	if (holder == NULL)
		return NULL;

	_gnutls_mpi_print(A, &holder[n_size - a_size], &a_size);
	_gnutls_mpi_print(B, &holder[n_size + n_size - b_size], &b_size);

	ret = _gnutls_hash_fast(GNUTLS_DIG_SHA1, holder, holder_size, hd);
	if (ret < 0) {
		gnutls_free(holder);
		gnutls_assert();
		return NULL;
	}

	/* convert the bytes of hd to integer
	 */
	hash_size = 20;		/* SHA */
	ret = _gnutls_mpi_init_scan_nz(&res, hd, hash_size);
	gnutls_free(holder);

	if (ret < 0) {
		gnutls_assert();
		return NULL;
	}

	return res;
}

/* S = (A * v^u) ^ b % N 
 * this is our shared key (server premaster secret)
 */
bigint_t
_gnutls_calc_srp_S1(bigint_t A, bigint_t b, bigint_t u, bigint_t v,
		    bigint_t n)
{
	bigint_t tmp1 = NULL, tmp2 = NULL;
	bigint_t S = NULL;
	int ret;

	ret = _gnutls_mpi_init_multi(&S, &tmp1, &tmp2, NULL);
	if (ret < 0)
		return NULL;

	ret = _gnutls_mpi_powm(tmp1, v, u, n);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = _gnutls_mpi_mulm(tmp2, A, tmp1, n);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	_gnutls_mpi_powm(S, tmp2, b, n);

	_gnutls_mpi_release(&tmp1);
	_gnutls_mpi_release(&tmp2);

	return S;

error:
	_gnutls_mpi_release(&S);
	_gnutls_mpi_release(&tmp1);
	_gnutls_mpi_release(&tmp2);
	return NULL;
}

/* A = g^a % N 
 * returns A and a (which is random)
 */
bigint_t _gnutls_calc_srp_A(bigint_t * a, bigint_t g, bigint_t n)
{
	bigint_t tmpa;
	bigint_t A;
	int ret;

	ret = _gnutls_mpi_init_multi(&A, &tmpa, NULL);
	if (ret < 0) {
		gnutls_assert();
		return NULL;
	}

	_gnutls_mpi_random_modp(tmpa, n, GNUTLS_RND_RANDOM);

	ret = _gnutls_mpi_powm(A, g, tmpa, n);
	if (ret < 0)
		goto error;

	if (a != NULL)
		*a = tmpa;
	else
		_gnutls_mpi_release(&tmpa);

	return A;
error:
	_gnutls_mpi_release(&tmpa);
	_gnutls_mpi_release(&A);
	return NULL;
}

/* generate x = SHA(s | SHA(U | ":" | p))
 * The output is exactly 20 bytes
 */
static int
_gnutls_calc_srp_sha(const char *username, const char *_password,
		     uint8_t * salt, int salt_size, size_t * size,
		     void *digest, unsigned allow_invalid_pass)
{
	digest_hd_st td;
	uint8_t res[MAX_HASH_SIZE];
	int ret;
	const mac_entry_st *me = mac_to_entry(GNUTLS_MAC_SHA1);
	char *password;
	gnutls_datum_t pout;

	*size = 20;

	ret = _gnutls_utf8_password_normalize(_password, strlen(_password), &pout, allow_invalid_pass);
	if (ret < 0)
		return gnutls_assert_val(ret);
	password = (char*)pout.data;

	ret = _gnutls_hash_init(&td, me);
	if (ret < 0) {
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}
	_gnutls_hash(&td, username, strlen(username));
	_gnutls_hash(&td, ":", 1);
	_gnutls_hash(&td, password, strlen(password));

	_gnutls_hash_deinit(&td, res);

	ret = _gnutls_hash_init(&td, me);
	if (ret < 0) {
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	_gnutls_hash(&td, salt, salt_size);
	_gnutls_hash(&td, res, 20);	/* 20 bytes is the output of sha1 */

	_gnutls_hash_deinit(&td, digest);
	ret = 0;

 cleanup:
	gnutls_free(password);
	return ret;
}

int
_gnutls_calc_srp_x(char *username, char *password, uint8_t * salt,
		   size_t salt_size, size_t * size, void *digest)
{

	return _gnutls_calc_srp_sha(username, password, salt,
				    salt_size, size, digest, 1);
}


/* S = (B - k*g^x) ^ (a + u * x) % N
 * this is our shared key (client premaster secret)
 */
bigint_t
_gnutls_calc_srp_S2(bigint_t B, bigint_t g, bigint_t x, bigint_t a,
		    bigint_t u, bigint_t n)
{
	bigint_t S = NULL, tmp1 = NULL, tmp2 = NULL;
	bigint_t tmp4 = NULL, tmp3 = NULL, k = NULL;
	int ret;

	ret = _gnutls_mpi_init_multi(&S, &tmp1, &tmp2, &tmp3, &tmp4, NULL);
	if (ret < 0)
		return NULL;

	k = _gnutls_calc_srp_u(n, g, n);
	if (k == NULL) {
		gnutls_assert();
		goto freeall;
	}

	ret = _gnutls_mpi_powm(tmp1, g, x, n);	/* g^x */
	if (ret < 0) {
		gnutls_assert();
		goto freeall;
	}

	ret = _gnutls_mpi_mulm(tmp3, tmp1, k, n);	/* k*g^x mod n */
	if (ret < 0) {
		gnutls_assert();
		goto freeall;
	}

	ret = _gnutls_mpi_subm(tmp2, B, tmp3, n);
	if (ret < 0) {
		gnutls_assert();
		goto freeall;
	}

	ret = _gnutls_mpi_mul(tmp1, u, x);
	if (ret < 0) {
		gnutls_assert();
		goto freeall;
	}

	ret = _gnutls_mpi_add(tmp4, a, tmp1);
	if (ret < 0) {
		gnutls_assert();
		goto freeall;
	}

	ret = _gnutls_mpi_powm(S, tmp2, tmp4, n);
	if (ret < 0) {
		gnutls_assert();
		goto freeall;
	}

	_gnutls_mpi_release(&tmp1);
	_gnutls_mpi_release(&tmp2);
	_gnutls_mpi_release(&tmp3);
	_gnutls_mpi_release(&tmp4);
	_gnutls_mpi_release(&k);

	return S;

      freeall:
	_gnutls_mpi_release(&k);
	_gnutls_mpi_release(&tmp1);
	_gnutls_mpi_release(&tmp2);
	_gnutls_mpi_release(&tmp3);
	_gnutls_mpi_release(&tmp4);
	_gnutls_mpi_release(&S);
	return NULL;
}

/**
 * gnutls_srp_free_client_credentials:
 * @sc: is a #gnutls_srp_client_credentials_t type.
 *
 * Free a gnutls_srp_client_credentials_t structure.
 **/
void gnutls_srp_free_client_credentials(gnutls_srp_client_credentials_t sc)
{
	gnutls_free(sc->username);
	gnutls_free(sc->password);
	gnutls_free(sc);
}

/**
 * gnutls_srp_allocate_client_credentials:
 * @sc: is a pointer to a #gnutls_srp_server_credentials_t type.
 *
 * Allocate a gnutls_srp_client_credentials_t structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 *   error code.
 **/
int
gnutls_srp_allocate_client_credentials(gnutls_srp_client_credentials_t *
				       sc)
{
	*sc = gnutls_calloc(1, sizeof(srp_client_credentials_st));

	if (*sc == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	return 0;
}

/**
 * gnutls_srp_set_client_credentials:
 * @res: is a #gnutls_srp_client_credentials_t type.
 * @username: is the user's userid
 * @password: is the user's password
 *
 * This function sets the username and password, in a
 * #gnutls_srp_client_credentials_t type.  Those will be used in
 * SRP authentication.  @username should be an ASCII string or UTF-8
 * string. In case of a UTF-8 string it is recommended to be following
 * the PRECIS framework for usernames (rfc8265). The password can
 * be in ASCII format, or normalized using gnutls_utf8_password_normalize().

 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 *   error code.
 **/
int
gnutls_srp_set_client_credentials(gnutls_srp_client_credentials_t res,
				  const char *username,
				  const char *password)
{

	if (username == NULL || password == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	res->username = gnutls_strdup(username);
	if (res->username == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	res->password = gnutls_strdup(password);
	if (res->password == NULL) {
		gnutls_free(res->username);
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

/**
 * gnutls_srp_free_server_credentials:
 * @sc: is a #gnutls_srp_server_credentials_t type.
 *
 * Free a gnutls_srp_server_credentials_t structure.
 **/
void gnutls_srp_free_server_credentials(gnutls_srp_server_credentials_t sc)
{
	gnutls_free(sc->password_file);
	gnutls_free(sc->password_conf_file);

	gnutls_free(sc);
}

/* Size of the default (random) seed if
 * gnutls_srp_set_server_fake_salt_seed() is not called to set
 * a seed.
 */
#define DEFAULT_FAKE_SALT_SEED_SIZE 20

/* Size of the fake salts generated if
 * gnutls_srp_set_server_fake_salt_seed() is not called to set
 * another size.
 */
#define DEFAULT_FAKE_SALT_SIZE 16

/**
 * gnutls_srp_allocate_server_credentials:
 * @sc: is a pointer to a #gnutls_srp_server_credentials_t type.
 *
 * Allocate a gnutls_srp_server_credentials_t structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 *   error code.
 **/
int
gnutls_srp_allocate_server_credentials(gnutls_srp_server_credentials_t *
				       sc)
{
	int ret;
	*sc = gnutls_calloc(1, sizeof(srp_server_cred_st));

	if (*sc == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	(*sc)->fake_salt_seed_size = DEFAULT_FAKE_SALT_SEED_SIZE;
	ret = gnutls_rnd(GNUTLS_RND_RANDOM, (*sc)->fake_salt_seed,
			 DEFAULT_FAKE_SALT_SEED_SIZE);

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	(*sc)->fake_salt_length = DEFAULT_FAKE_SALT_SIZE;
	return 0;

cleanup:
	gnutls_free(*sc);
	return ret;
}

/**
 * gnutls_srp_set_server_credentials_file:
 * @res: is a #gnutls_srp_server_credentials_t type.
 * @password_file: is the SRP password file (tpasswd)
 * @password_conf_file: is the SRP password conf file (tpasswd.conf)
 *
 * This function sets the password files, in a
 * #gnutls_srp_server_credentials_t type.  Those password files
 * hold usernames and verifiers and will be used for SRP
 * authentication.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 *   error code.
 **/
int
gnutls_srp_set_server_credentials_file(gnutls_srp_server_credentials_t res,
				       const char *password_file,
				       const char *password_conf_file)
{

	if (password_file == NULL || password_conf_file == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the files can be opened */
	if (_gnutls_file_exists(password_file) != 0) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	if (_gnutls_file_exists(password_conf_file) != 0) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	res->password_file = gnutls_strdup(password_file);
	if (res->password_file == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	res->password_conf_file = gnutls_strdup(password_conf_file);
	if (res->password_conf_file == NULL) {
		gnutls_assert();
		gnutls_free(res->password_file);
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}


/**
 * gnutls_srp_set_server_credentials_function:
 * @cred: is a #gnutls_srp_server_credentials_t type.
 * @func: is the callback function
 *
 * This function can be used to set a callback to retrieve the user's
 * SRP credentials.  The callback's function form is:
 *
 * int (*callback)(gnutls_session_t, const char* username,
 *  gnutls_datum_t *salt, gnutls_datum_t *verifier, gnutls_datum_t *generator,
 *  gnutls_datum_t *prime);
 *
 * @username contains the actual username.
 * The @salt, @verifier, @generator and @prime must be filled
 * in using the gnutls_malloc(). For convenience @prime and @generator
 * may also be one of the static parameters defined in gnutls.h.
 *
 * Initially, the data field is NULL in every #gnutls_datum_t
 * structure that the callback has to fill in. When the
 * callback is done GnuTLS deallocates all of those buffers
 * which are non-NULL, regardless of the return value.
 *
 * In order to prevent attackers from guessing valid usernames,
 * if a user does not exist, g and n values should be filled in
 * using a random user's parameters. In that case the callback must
 * return the special value (1).
 * See #gnutls_srp_set_server_fake_salt_seed too.
 * If this is not required for your application, return a negative
 * number from the callback to abort the handshake.
 *
 * The callback function will only be called once per handshake.
 * The callback function should return 0 on success, while
 * -1 indicates an error.
 **/
void
gnutls_srp_set_server_credentials_function(gnutls_srp_server_credentials_t
					   cred,
					   gnutls_srp_server_credentials_function
					   *func)
{
	cred->pwd_callback = func;
}

/**
 * gnutls_srp_set_client_credentials_function:
 * @cred: is a #gnutls_srp_server_credentials_t type.
 * @func: is the callback function
 *
 * This function can be used to set a callback to retrieve the
 * username and password for client SRP authentication.  The
 * callback's function form is:
 *
 * int (*callback)(gnutls_session_t, char** username, char**password);
 *
 * The @username and @password must be allocated using
 * gnutls_malloc().
 *
 * The @username should be an ASCII string or UTF-8
 * string. In case of a UTF-8 string it is recommended to be following
 * the PRECIS framework for usernames (rfc8265). The password can
 * be in ASCII format, or normalized using gnutls_utf8_password_normalize().
 *
 * The callback function will be called once per handshake before the
 * initial hello message is sent.
 *
 * The callback should not return a negative error code the second
 * time called, since the handshake procedure will be aborted.
 *
 * The callback function should return 0 on success.
 * -1 indicates an error.
 **/
void
gnutls_srp_set_client_credentials_function(gnutls_srp_client_credentials_t
					   cred,
					   gnutls_srp_client_credentials_function
					   * func)
{
	cred->get_function = func;
}


/**
 * gnutls_srp_server_get_username:
 * @session: is a gnutls session
 *
 * This function will return the username of the peer.  This should
 * only be called in case of SRP authentication and in case of a
 * server.  Returns NULL in case of an error.
 *
 * Returns: SRP username of the peer, or NULL in case of error.
 **/
const char *gnutls_srp_server_get_username(gnutls_session_t session)
{
	srp_server_auth_info_t info;

	CHECK_AUTH_TYPE(GNUTLS_CRD_SRP, NULL);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_SRP);
	if (info == NULL)
		return NULL;
	return info->username;
}

/**
 * gnutls_srp_verifier:
 * @username: is the user's name
 * @password: is the user's password
 * @salt: should be some randomly generated bytes
 * @generator: is the generator of the group
 * @prime: is the group's prime
 * @res: where the verifier will be stored.
 *
 * This function will create an SRP verifier, as specified in
 * RFC2945.  The @prime and @generator should be one of the static
 * parameters defined in gnutls/gnutls.h or may be generated.
 *
 * The verifier will be allocated with @gnutls_malloc() and will be stored in
 * @res using binary format.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 *   error code.
 **/
int
gnutls_srp_verifier(const char *username, const char *password,
		    const gnutls_datum_t * salt,
		    const gnutls_datum_t * generator,
		    const gnutls_datum_t * prime, gnutls_datum_t * res)
{
	bigint_t _n, _g;
	int ret;
	size_t digest_size = 20, size;
	uint8_t digest[20];

	ret = _gnutls_calc_srp_sha(username, password, salt->data,
				   salt->size, &digest_size, digest, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	size = prime->size;
	if (_gnutls_mpi_init_scan_nz(&_n, prime->data, size)) {
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	size = generator->size;
	if (_gnutls_mpi_init_scan_nz(&_g, generator->data, size)) {
		gnutls_assert();
		_gnutls_mpi_release(&_n);
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	ret = _gnutls_srp_gx(digest, 20, &res->data, _g, _n);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_mpi_release(&_n);
		_gnutls_mpi_release(&_g);
		return ret;
	}
	res->size = ret;

	_gnutls_mpi_release(&_n);
	_gnutls_mpi_release(&_g);

	return 0;
}

/**
 * gnutls_srp_set_prime_bits:
 * @session: is a #gnutls_session_t type.
 * @bits: is the number of bits
 *
 * This function sets the minimum accepted number of bits, for use in
 * an SRP key exchange.  If zero, the default 2048 bits will be used.
 *
 * In the client side it sets the minimum accepted number of bits.  If
 * a server sends a prime with less bits than that
 * %GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER will be returned by the
 * handshake.
 *
 * This function has no effect in server side.
 *
 * Since: 2.6.0
 **/
void gnutls_srp_set_prime_bits(gnutls_session_t session, unsigned int bits)
{
	session->internals.dh_prime_bits = bits;
}

/**
 * gnutls_srp_set_server_fake_salt_seed:
 * @cred: is a #gnutls_srp_server_credentials_t type
 * @seed: is the seed data, only needs to be valid until the function
 * returns; size of the seed must be greater than zero
 * @salt_length: is the length of the generated fake salts
 *
 * This function sets the seed that is used to generate salts for
 * invalid (non-existent) usernames.
 *
 * In order to prevent attackers from guessing valid usernames,
 * when a user does not exist gnutls generates a salt and a verifier
 * and proceeds with the protocol as usual.
 * The authentication will ultimately fail, but the client cannot tell
 * whether the username is valid (exists) or invalid.
 *
 * If an attacker learns the seed, given a salt (which is part of the
 * handshake) which was generated when the seed was in use, it can tell
 * whether or not the authentication failed because of an unknown username.
 * This seed cannot be used to reveal application data or passwords.
 *
 * @salt_length should represent the salt length your application uses.
 * Generating fake salts longer than 20 bytes is not supported.
 *
 * By default the seed is a random value, different each time a
 * #gnutls_srp_server_credentials_t is allocated and fake salts are
 * 16 bytes long.
 *
 * Since: 3.3.0
 **/
void
gnutls_srp_set_server_fake_salt_seed(gnutls_srp_server_credentials_t cred,
				     const gnutls_datum_t * seed,
				     unsigned int salt_length)
{
	unsigned seed_size = seed->size;
	const unsigned char *seed_data = seed->data;

	if (seed_size > sizeof(cred->fake_salt_seed))
		seed_size = sizeof(cred->fake_salt_seed);

	memcpy(cred->fake_salt_seed, seed_data, seed_size);
	cred->fake_salt_seed_size = seed_size;

	/* Cap the salt length at the output size of the MAC algorithm
	 * we are using to generate the fake salts.
	 */
	const mac_entry_st * me = mac_to_entry(SRP_FAKE_SALT_MAC);
	const size_t mac_len = me->output_size;

	cred->fake_salt_length = (salt_length < mac_len ? salt_length : mac_len);
}

#endif				/* ENABLE_SRP */
