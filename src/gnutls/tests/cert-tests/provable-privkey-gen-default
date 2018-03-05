#!/bin/sh

# Copyright (C) 2014 Nikos Mavrogiannopoulos
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

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"
OUTFILE=provable-privkey$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

${VALGRIND} "${CERTTOOL}" --generate-privkey --provable --seed "A3:54:5C:B3:1D:70:56:1C:A0:BD:2C:C8:78:C1:9C:56:CD:69:75:50:0C:3A:FD:BF:E8:96:83:FA:52:BC:98:C5" --outfile $OUTFILE
rc=$?

if test "${rc}" != "0"; then
	echo "Could not generate a default key"
	exit 1
fi

rm -f "$OUTFILE"

exit 0
