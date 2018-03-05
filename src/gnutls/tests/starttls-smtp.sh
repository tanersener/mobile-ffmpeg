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

echo "Checking STARTTLS over SMTP"

eval "${GETPORT}"
socat TCP-LISTEN:${PORT} EXEC:"$CHAT -e -S -v -f ${srcdir}/starttls-smtp.txt",pty &
PID=$!
wait_server ${PID}

${VALGRIND} "${CLI}" -p "${PORT}" 127.0.0.1 --priority NORMAL:+ANON-ECDH --insecure --starttls-proto smtp --verbose </dev/null >/dev/null
if test $? != 1;then
	fail ${PID} "connect should have failed with error code 1"
fi

kill ${PID}
wait

exit 0
