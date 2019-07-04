/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Authors: Daiki Ueno
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
#include <common.h>
#include <x509.h>
#include <x509_int.h>

/**
 * gnutls_x509_spki_init:
 * @spki: A pointer to the type to be initialized
 *
 * This function will initialize a SubjectPublicKeyInfo structure used
 * in PKIX. The structure is used to set additional parameters
 * in the public key information field of a certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.0
 *
 **/
int
gnutls_x509_spki_init(gnutls_x509_spki_t *spki)
{
	gnutls_x509_spki_t tmp;

	FAIL_IF_LIB_ERROR;

	tmp =
	    gnutls_calloc(1, sizeof(gnutls_x509_spki_st));

	if (!tmp)
		return GNUTLS_E_MEMORY_ERROR;

	*spki = tmp;

	return 0;		/* success */
}

/**
 * gnutls_x509_spki_deinit:
 * @spki: the SubjectPublicKeyInfo structure
 *
 * This function will deinitialize a SubjectPublicKeyInfo structure.
 *
 * Since: 3.6.0
 *
 **/
void
gnutls_x509_spki_deinit(gnutls_x509_spki_t spki)
{
	gnutls_free(spki);
}

/**
 * gnutls_x509_spki_set_rsa_pss_params:
 * @spki: the SubjectPublicKeyInfo structure
 * @dig: a digest algorithm of type #gnutls_digest_algorithm_t
 * @salt_size: the size of salt string
 *
 * This function will set the public key parameters for
 * an RSA-PSS algorithm, in the SubjectPublicKeyInfo structure.
 *
 * Since: 3.6.0
 *
 **/
void
gnutls_x509_spki_set_rsa_pss_params(gnutls_x509_spki_t spki,
				    gnutls_digest_algorithm_t dig,
				    unsigned int salt_size)
{
	spki->pk = GNUTLS_PK_RSA_PSS;
	spki->rsa_pss_dig = dig;
	spki->salt_size = salt_size;
}

/**
 * gnutls_x509_spki_get_rsa_pss_params:
 * @spki: the SubjectPublicKeyInfo structure
 * @dig: if non-NULL, it will hold the digest algorithm
 * @salt_size: if non-NULL, it will hold the salt size
 *
 * This function will get the public key algorithm parameters
 * of RSA-PSS type.
 *
 * Returns: zero if the parameters are present or a negative
 *     value on error.
 *
 * Since: 3.6.0
 *
 **/
int
gnutls_x509_spki_get_rsa_pss_params(gnutls_x509_spki_t spki,
				    gnutls_digest_algorithm_t *dig,
				    unsigned int *salt_size)
{
	if (spki->pk == 0)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (spki->pk != GNUTLS_PK_RSA_PSS)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (dig)
		*dig = spki->rsa_pss_dig;
	if (salt_size)
		*salt_size = spki->salt_size;

	return 0;
}
