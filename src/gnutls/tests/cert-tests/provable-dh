#!/bin/sh

# Copyright (C) 2016 Red Hat, Inc.
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
OUTFILE=provable-dh$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

if test "${FIPS140}" = 1;then
SEED="30EC334F97DBC0BA9C8652A7B5D3F7B2DBBB48A4842E190D210E01DABD535981503755EE96A270A598E9D91B2254669169EBDF4599D9F72ACA"
DSAFILE=provable-dsa2048-fips.pem
else
SEED="5A0EA041779B0AB765BE2509C4DE90E5A0E7DAADAE6E49D35938F91333A8E1FE509DD2DFE1967CD0045428103497D00388C8CE36290FE9379F8003CBF8FDA4DA27"
DSAFILE=provable-dsa2048.pem
fi

#DH parameters
${VALGRIND} "${CERTTOOL}" --generate-dh-params --provable --bits 2048 --seed "$SEED" --outfile "$OUTFILE"
rc=$?

if test "${rc}" != "0"; then
	echo "test1: Could not generate a 2048-bit DSA key"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --verify-provable-privkey --load-privkey "$OUTFILE" &
PID1=$!

${VALGRIND} "${CERTTOOL}" --verify-provable-privkey --load-privkey "$OUTFILE" --seed "$SEED" &
PID2=$!

wait $PID1
rc1=$?

wait $PID2
rc2=$?

if test "${rc1}" != "0" || test "${rc2}" != "0"; then
	echo "test1: Could not verify the generated parameters"
	exit 1
fi

rm -f "$OUTFILE"

exit 0
