#!/bin/sh

# Copyright (C) 2017 Red Hat, Inc.
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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
PKGCONFIG="${PKG_CONFIG:-$(which pkg-config)}"
DIFF="${DIFF:-diff -b -B}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

${PKGCONFIG} --version >/dev/null || exit 77

${PKGCONFIG} --atleast-version=4.12 libtasn1 || exit 77

# Check whether certificates with invalid time fields are accepted
for file in openssl-invalid-time-format.pem;do
	${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/$file"
	rc=$?

	if test "${rc}" != "0";then
		echo "file $file was not rejected"
		exit 1
	fi
done

exit 0
