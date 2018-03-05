/*
 * Copyright (C) 2014-2016 Free Software Foundation
 * Copyright (C) 2014-2016 Red Hat, Inc.
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

#ifndef X509_EXT_INT_H
#define X509_EXT_INT_H

#include "gnutls_int.h"
struct name_st {
	unsigned int type;
	gnutls_datum_t san;
	gnutls_datum_t othername_oid;
};

int _gnutls_alt_name_process(gnutls_datum_t *out, unsigned type, const gnutls_datum_t *san, unsigned raw);

#endif
