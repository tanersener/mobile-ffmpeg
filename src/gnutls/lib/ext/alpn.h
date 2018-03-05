/*
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
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
#ifndef EXT_ALPN_H
#define EXT_ALPN_H

#include <extensions.h>

#define MAX_ALPN_PROTOCOLS 8
#define MAX_ALPN_PROTOCOL_NAME 32

typedef struct {
	uint8_t protocols[MAX_ALPN_PROTOCOLS][MAX_ALPN_PROTOCOL_NAME];
	unsigned protocol_size[MAX_ALPN_PROTOCOLS];
	unsigned size;
	uint8_t *selected_protocol;
	unsigned selected_protocol_size;
	unsigned flags;
} alpn_ext_st;

extern const extension_entry_st ext_mod_alpn;

#endif
