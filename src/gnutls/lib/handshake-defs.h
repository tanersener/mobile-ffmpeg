/*
 * Copyright (C) 2019 Red Hat, Inc.
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
#ifndef HANDSHAKE_DEFS_H
#define HANDSHAKE_DEFS_H

#define EARLY_TRAFFIC_LABEL "c e traffic"
#define EXT_BINDER_LABEL "ext binder"
#define RES_BINDER_LABEL "res binder"
#define EARLY_EXPORTER_MASTER_LABEL "e exp master"
#define HANDSHAKE_CLIENT_TRAFFIC_LABEL "c hs traffic"
#define HANDSHAKE_SERVER_TRAFFIC_LABEL "s hs traffic"
#define DERIVED_LABEL "derived"
#define APPLICATION_CLIENT_TRAFFIC_LABEL "c ap traffic"
#define APPLICATION_SERVER_TRAFFIC_LABEL "s ap traffic"
#define APPLICATION_TRAFFIC_UPDATE "traffic upd"
#define EXPORTER_MASTER_LABEL "exp master"
#define RMS_MASTER_LABEL "res master"
#define EXPORTER_LABEL "exporter"
#define RESUMPTION_LABEL "resumption"

#define HRR_RANDOM \
	 "\xCF\x21\xAD\x74\xE5\x9A\x61\x11\xBE\x1D\x8C\x02\x1E\x65\xB8\x91" \
	 "\xC2\xA2\x11\x16\x7A\xBB\x8C\x5E\x07\x9E\x09\xE2\xC8\xA8\x33\x9C"

#define TLS13_TICKETS_TO_SEND 2

/* Enable: Appendix D4.  Middlebox Compatibility Mode */
#define TLS13_APPENDIX_D4 1

#endif /* HANDSHAKE_DEFS_H */
