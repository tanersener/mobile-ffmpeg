/*
 * Copyright (C) 2012 Free Software Foundation
 *
 * Author: Martin Storsjo
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

#ifndef GNUTLS_LIB_EXT_SRTP_H
#define GNUTLS_LIB_EXT_SRTP_H
#ifndef EXT_SRTP_H
#define EXT_SRTP_H

#include <hello_ext.h>

#define MAX_SRTP_PROFILES 4

typedef struct {
	gnutls_srtp_profile_t profiles[MAX_SRTP_PROFILES];
	unsigned profiles_size;
	gnutls_srtp_profile_t selected_profile;
	uint8_t mki[256];
	unsigned mki_size;
	unsigned int mki_received;
} srtp_ext_st;

extern const hello_ext_entry_st ext_mod_srtp;

#endif

#endif /* GNUTLS_LIB_EXT_SRTP_H */
