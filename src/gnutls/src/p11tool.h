/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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

#ifndef P11TOOL_H
#define P11TOOL_H

#include "certtool-common.h"

void pkcs11_list(FILE * outfile, const char *url, int type,
		 unsigned int flags, unsigned int detailed,
		 common_info_st *);
void pkcs11_mechanism_list(FILE * outfile, const char *url,
			   unsigned int flags, common_info_st *);
void pkcs11_get_random(FILE * outfile, const char *url,
		       unsigned bytes, common_info_st *);
void pkcs11_export(FILE * outfile, const char *pkcs11_url,
		   unsigned int flags, common_info_st *);
void
pkcs11_export_chain(FILE * outfile, const char *url, unsigned int flags,
	      common_info_st * info);

void pkcs11_token_list(FILE * outfile, unsigned int detailed,
		       common_info_st *, unsigned brief);
void pkcs11_test_sign(FILE * outfile, const char *pkcs11_url,
		  unsigned int flags, common_info_st *);
void pkcs11_write(FILE * outfile, const char *pkcs11_url,
		  const char *label, const char *id,
		  unsigned int flags, common_info_st *);
void pkcs11_delete(FILE * outfile, const char *pkcs11_url,
		   unsigned int flags, common_info_st *);
void pkcs11_init(FILE * outfile, const char *pkcs11_url, const char *label,
		 common_info_st *);
void pkcs11_set_pin(FILE * outfile, const char *pkcs11_url, common_info_st *, unsigned so);
void pkcs11_generate(FILE * outfile, const char *url,
		     gnutls_pk_algorithm_t type, unsigned int bits,
		     const char *label, const char *id, int detailed,
		     unsigned int flags, common_info_st * info);
void pkcs11_export_pubkey(FILE * outfile, const char *url, int detailed,
		     unsigned int flags, common_info_st * info);

void pkcs11_set_id(FILE * outfile, const char *url, int detailed,
		   unsigned int flags, common_info_st * info,
		   const char *id);

void pkcs11_set_label(FILE * outfile, const char *url, int detailed,
		   unsigned int flags, common_info_st * info,
		   const char *label);

#define PKCS11_TYPE_CRT_ALL 1
#define PKCS11_TYPE_TRUSTED 2
#define PKCS11_TYPE_PK 3
#define PKCS11_TYPE_ALL 4
#define PKCS11_TYPE_PRIVKEY 5
#define PKCS11_TYPE_INFO 6

#endif
