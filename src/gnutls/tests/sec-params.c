/*
 * Copyright (C) 2014 Red Hat
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
#include <utils.h>

#include <gnutls/gnutls.h>

int main(int argc, char *argv[])
{
int ret;
gnutls_sec_param_t p;

	ret = global_init();
	if (ret != 0) {
		printf("%d: %s\n", ret, gnutls_strerror(ret));
		return EXIT_FAILURE;
	}
	
	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_EC, 160);
	if (p != GNUTLS_SEC_PARAM_LOW) {
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_EC, 192);
	if (p != GNUTLS_SEC_PARAM_LEGACY) {
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_EC, 256);
	if (p != GNUTLS_SEC_PARAM_HIGH) {
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_EC, 384);
	if (p != GNUTLS_SEC_PARAM_ULTRA) {
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_RSA, 1024);
#ifdef ENABLE_FIPS140
	if (p != GNUTLS_SEC_PARAM_LEGACY) {
#else
	if (p != GNUTLS_SEC_PARAM_LOW) {
#endif
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_RSA, 2048);
	if (p != GNUTLS_SEC_PARAM_MEDIUM) {
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_RSA, 3072);
	if (p != GNUTLS_SEC_PARAM_HIGH) {
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_DH, 1024);
#ifdef ENABLE_FIPS140
	if (p != GNUTLS_SEC_PARAM_LEGACY) {
#else
	if (p != GNUTLS_SEC_PARAM_LOW) {
#endif
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	p = gnutls_pk_bits_to_sec_param(GNUTLS_PK_DH, 2048);
	if (p != GNUTLS_SEC_PARAM_MEDIUM) {
		fprintf(stderr, "%d: error in sec param, p:%u\n", __LINE__, (unsigned)p);
		return 1;
	}

	gnutls_global_deinit();

	return 0;
}
