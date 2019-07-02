#!/bin/sh

# Copyright (C) 2010-2016 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of GnuTLS.
#
# GnuTLS is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.
#
# GnuTLS is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GnuTLS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

srcdir="${srcdir:-.}"
SERV="${SERV:-../src/gnutls-serv${EXEEXT}}"
CLI="${CLI:-../src/gnutls-cli${EXEEXT}}"
unset RETCODE

. "${srcdir}/scripts/common.sh"
. "${srcdir}/scripts/starttls-common.sh"

SERV="${SERV} -q"

echo "Checking STARTTLS"

eval "${GETPORT}"
launch_server $$ --echo --priority "NORMAL:+ANON-ECDH"
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2:+ANON-ECDH --insecure --starttls -d 6 </dev/null >/dev/null || \
	fail ${PID} "starttls connect should have succeeded!"


kill ${PID}
wait

exit 0
