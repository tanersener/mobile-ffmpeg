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

/* Tests whether a gnutls_pkcs11_obj_get_ptr returns valid handles. */

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

#define PIN "1234"

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
	int ret;
	const char *lib;
	unsigned long slot_id;
	unsigned char sig[256];
	unsigned long len;
	struct ck_function_list *mod;
	struct ck_info info;
	struct ck_token_info tinfo;
	ck_session_handle_t session;
	ck_object_handle_t ohandle;
	gnutls_pkcs11_obj_t obj;
	struct ck_mechanism mech;
	ck_rv_t rv;
	gnutls_datum_t data;

	data.data = (void*)"\x38\x17\x0c\x08\xcb\x45\x8f\xd4\x87\x9c\x34\xb6\xf6\x08\x29\x4c\x50\x31\x2b\xbb";
	data.size = 20;

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

	assert(gnutls_pkcs11_obj_init(&obj)>=0);

	gnutls_pkcs11_obj_set_pin_function(obj, pin_func, NULL);

	/* unknown object */
	ret = gnutls_pkcs11_obj_import_url(obj, "pkcs11:token=unknown;object=invalid;type=private", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	assert(ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	ret = gnutls_pkcs11_obj_import_url(obj, "pkcs11:object=test;type=private", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	assert(ret >= 0);

	ret = gnutls_pkcs11_obj_get_ptr(obj, (void**)&mod, (void*)&session,
					(void*)&ohandle,
					&slot_id, GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs11_obj_deinit(obj);

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

	assert(tinfo.hardware_version.major == 0x01);
	assert(tinfo.firmware_version.major == 0x01);

	mech.mechanism = CKM_RSA_PKCS;
	mech.parameter = NULL;
	mech.parameter_len = 0;

	rv = mod->C_SignInit(session, &mech, ohandle);
	if (rv != CKR_OK) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	len = sizeof(sig);
	rv = mod->C_Sign(session, data.data, data.size, sig, &len);
	if (rv != CKR_OK) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	mod->C_CloseSession(session);

	gnutls_pkcs11_deinit();
	gnutls_global_deinit();
}
#else
void doit(void)
{
	exit(77);
}
#endif
