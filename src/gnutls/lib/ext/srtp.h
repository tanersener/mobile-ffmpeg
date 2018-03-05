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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */
#ifndef EXT_SRTP_H
#define EXT_SRTP_H

#include <extensions.h>

#define MAX_SRTP_PROFILES 4

typedef struct {
	gnutls_srtp_profile_t profiles[MAX_SRTP_PROFILES];
	unsigned profiles_size;
	gnutls_srtp_profile_t selected_profile;
	uint8_t mki[256];
	unsigned mki_size;
	unsigned int mki_received;
} srtp_ext_st;

extern const extension_entry_st ext_mod_srtp;

#endif
