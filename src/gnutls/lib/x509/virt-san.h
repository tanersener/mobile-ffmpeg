/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
 * Copyright (C) 2015 Red Hat, Inc.
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

#ifndef VIRT_SAN_H
# define VIRT_SAN_H

#include "x509_ext_int.h"

int _gnutls_alt_name_assign_virt_type(struct name_st *name, unsigned type, gnutls_datum_t *san, const char *othername_oid, unsigned raw);

#endif
