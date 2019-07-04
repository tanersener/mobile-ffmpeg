/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Daiki Ueno
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

#include <dlfcn.h>
#include <p11-kit/pkcs11.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "softhsm.h"

/* This provides a mock PKCS #11 module that delegates all the
 * operations to SoftHSM except that it filters out CKM_RSA_PKCS_PSS
 * mechanism.
 */

static void *dl;
static CK_C_GetMechanismInfo base_C_GetMechanismInfo;
static CK_FUNCTION_LIST override_funcs;

#ifdef __sun
# pragma fini(mock_deinit)
# pragma init(mock_init)
# define _CONSTRUCTOR
# define _DESTRUCTOR
#else
# define _CONSTRUCTOR __attribute__((constructor))
# define _DESTRUCTOR __attribute__((destructor))
#endif

static CK_RV
override_C_GetMechanismInfo(CK_SLOT_ID slot_id,
			    CK_MECHANISM_TYPE type,
			    CK_MECHANISM_INFO *info)
{
	if (type == CKM_RSA_PKCS_PSS)
		return CKR_MECHANISM_INVALID;

	return base_C_GetMechanismInfo(slot_id, type, info);
}

CK_RV
C_GetFunctionList(CK_FUNCTION_LIST **function_list)
{
	CK_C_GetFunctionList func;
	CK_FUNCTION_LIST *funcs;

	assert(dl);

	func = dlsym(dl, "C_GetFunctionList");
	if (func == NULL) {
		return CKR_GENERAL_ERROR;
	}

	func(&funcs);
	base_C_GetMechanismInfo = funcs->C_GetMechanismInfo;

	memcpy(&override_funcs, funcs, sizeof(CK_FUNCTION_LIST));
	override_funcs.C_GetMechanismInfo = override_C_GetMechanismInfo;
	*function_list = &override_funcs;

	return CKR_OK;
}

static _CONSTRUCTOR void
mock_init(void)
{
	const char *lib;

	/* suppress compiler warning */
	(void) set_softhsm_conf;

	lib = softhsm_lib();

	dl = dlopen(lib, RTLD_NOW);
	if (dl == NULL)
		exit(77);
}

static _DESTRUCTOR void
mock_deinit(void)
{
	dlclose(dl);
}
