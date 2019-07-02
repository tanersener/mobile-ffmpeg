/*
 * Copyright (C) 2016-2018 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <gnutls/gnutls.h>
#include <gnutls/pkcs11.h>
#ifndef CRYPTOKI_GNU
# define CRYPTOKI_GNU
#endif
#include <p11-kit/pkcs11.h>

#include "utils.h"

/* Tests whether a gnutls_pkcs11_token_get_ptr returns valid handles. */

#if defined(HAVE___REGISTER_ATFORK)

#ifdef _WIN32
# define P11LIB "libpkcs11mock1.dll"
#else
# include <dlfcn.h>
# define P11LIB "libpkcs11mock1.so"
#endif


static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

#define TOKEN_NAME "whatever"
void doit(void)
{
	int ret;
	const char *lib;
	unsigned long slot_id;
	struct ck_function_list *mod;
	struct ck_info info;
	struct ck_token_info tinfo;
	ck_rv_t rv;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	lib = getenv("P11MOCKLIB1");
	if (lib == NULL)
		lib = P11LIB;

	ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs11_add_provider(lib, NULL);
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	{
		static const char url[] = "pkcs11:token="TOKEN_NAME;

		/* Testing a too small buffer */
		size_t size = 1;
		char *buf = gnutls_malloc(size);
		assert(buf != NULL);
		ret = gnutls_pkcs11_token_get_info(url,
			GNUTLS_PKCS11_TOKEN_LABEL,
			buf, &size);
		assert(ret == GNUTLS_E_SHORT_MEMORY_BUFFER);
		assert(size == strlen(TOKEN_NAME)+1);

		/* Testing a too small buffer by one */
		size -= 1;
		buf = gnutls_realloc(buf, size);
		assert(buf != NULL);
		ret = gnutls_pkcs11_token_get_info(url,
			GNUTLS_PKCS11_TOKEN_LABEL,
			buf, &size);
		assert(ret == GNUTLS_E_SHORT_MEMORY_BUFFER);
		assert(size == strlen(TOKEN_NAME)+1);

		/* Testing an exactly fitting buffer */
		buf = gnutls_realloc(buf, size);
		assert(buf != NULL);
		ret = gnutls_pkcs11_token_get_info(url,
			GNUTLS_PKCS11_TOKEN_LABEL,
			buf, &size);
		assert(ret == 0);
		assert(strcmp(buf, TOKEN_NAME) == 0);
		assert(size == strlen(TOKEN_NAME));

		gnutls_free(buf);
	}

	ret = gnutls_pkcs11_token_get_ptr("pkcs11:token=invalid", (void**)&mod, &slot_id, 0);
	assert(ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	ret = gnutls_pkcs11_token_get_ptr("pkcs11:", (void**)&mod, &slot_id, 0);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	rv = mod->C_GetInfo(&info);
	if (rv != CKR_OK) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(info.cryptoki_version.major == 0x02);
	assert(info.cryptoki_version.minor == 0x14);
	assert(info.flags == 0);
	assert(info.library_version.major == 0x01);
	assert(info.library_version.minor == 0x00);

	rv = mod->C_GetTokenInfo(slot_id, &tinfo);
	if (rv != CKR_OK) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	assert(tinfo.session_count == 0);
	assert(tinfo.hardware_version.major == 0x01);
	assert(tinfo.firmware_version.major == 0x01);

	if (debug)
		printf("done\n\n\n");

	gnutls_pkcs11_deinit();
	gnutls_global_deinit();
}
#else
void doit(void)
{
	exit(77);
}
#endif
