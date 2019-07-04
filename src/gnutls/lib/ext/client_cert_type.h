/*
 * Copyright (C) 2016 - 2018 ARPA2 project
 *
 * Author: Tom Vrancken (dev@tomvrancken.nl)
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
 * This file is part of the client_certificate_type extension as
 * defined in RFC7250 (https://tools.ietf.org/html/rfc7250).
 *
 * The client_certificate_type extension in the client hello indicates
 * the certificate types the client is able to provide to the server,
 * when requested using a certificate_request message.
 */

#ifndef GNUTLS_LIB_EXT_CLIENT_CERT_TYPE_H
#define GNUTLS_LIB_EXT_CLIENT_CERT_TYPE_H

#include <hello_ext.h>

extern const hello_ext_entry_st ext_mod_client_cert_type;

#endif /* GNUTLS_LIB_EXT_CLIENT_CERT_TYPE_H */
