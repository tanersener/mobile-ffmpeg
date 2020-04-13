/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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

#ifndef GNUTLS_LIB_X509_OCSP_H
#define GNUTLS_LIB_X509_OCSP_H

/* Online Certificate Status Protocol - RFC 2560
 */
#include <gnutls/ocsp.h>

/* fifteen days */
#define MAX_OCSP_VALIDITY_SECS (15*60*60*24)

time_t _gnutls_ocsp_get_validity(gnutls_ocsp_resp_const_t resp);
#define MAX_OCSP_MSG_SIZE 128
const char *_gnutls_ocsp_verify_status_to_str(gnutls_ocsp_verify_reason_t r, char out[MAX_OCSP_MSG_SIZE]);

#endif /* GNUTLS_LIB_X509_OCSP_H */
