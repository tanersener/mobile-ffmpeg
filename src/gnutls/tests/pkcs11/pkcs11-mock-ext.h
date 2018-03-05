/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#ifndef PKCS11_MOCK_EXT_H
# define PKCS11_MOCK_EXT_H

/* This flag instructs the module to return CKR_OK on sensitive
 * objects */
#define MOCK_FLAG_BROKEN_GET_ATTRIBUTES 1
#define MOCK_FLAG_ALWAYS_AUTH (1<<1)
/* simulate the safenet HSMs always auth behavior */
#define MOCK_FLAG_SAFENET_ALWAYS_AUTH (1<<2)

#endif
