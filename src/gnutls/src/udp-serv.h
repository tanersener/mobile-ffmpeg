/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GNUTLS_SRC_UDP_SERV_H
#define GNUTLS_SRC_UDP_SERV_H

#include <gnutls/dtls.h>

void udp_server(const char *name, int port, int mtu);
gnutls_session_t initialize_session(int dtls);
const char *human_addr(const struct sockaddr *sa, socklen_t salen,
		       char *buf, size_t buflen);
int wait_for_connection(void);
int listen_socket(const char *name, int listen_port, int socktype);

#endif /* GNUTLS_SRC_UDP_SERV_H */
