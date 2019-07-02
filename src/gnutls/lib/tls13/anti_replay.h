/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Daiki Ueno
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

#ifndef GNUTLS_LIB_TLS13_ANTI_REPLAY_H
#define GNUTLS_LIB_TLS13_ANTI_REPLAY_H

int _gnutls_anti_replay_check(gnutls_anti_replay_t,
			      uint32_t client_ticket_age,
			      struct timespec *ticket_creation_time,
			      gnutls_datum_t *id);

#endif /* GNUTLS_LIB_TLS13_ANTI_REPLAY_H */
