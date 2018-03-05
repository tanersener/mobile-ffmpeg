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

#DH parameters - no seed
${VALGRIND} "${CERTTOOL}" --generate-dh-params --provable --outfile "$OUTFILE"
rc=$?

if test "${rc}" != "0"; then
	echo "test2: Could not generate DH parameters"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" --verify-provable-privkey --load-privkey "$OUTFILE"
rc=$?

if test "${rc}" != "0"; then
	echo "test2: Could not verify the generated parameters"
	exit 1
fi

rm -f "$OUTFILE"

exit 0
