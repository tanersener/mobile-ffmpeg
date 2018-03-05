/*
 * Copyright (C) 2007-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos, Martin Ukrop
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

#ifndef IP_H
#define IP_H

// for documentation, see the definition
int _gnutls_mask_to_prefix(const unsigned char *mask, unsigned mask_size);

// for documentation, see the definition
const char *_gnutls_ip_to_string(const void *_ip, unsigned int ip_size, char *out, unsigned int out_size);

// for documentation, see the definition
const char *_gnutls_cidr_to_string(const void *_ip, unsigned int ip_size, char *out, unsigned int out_size);

// for documentation, see the definition
int _gnutls_mask_ip(unsigned char *ip, const unsigned char *mask, unsigned ipsize);

#endif // IP_H
