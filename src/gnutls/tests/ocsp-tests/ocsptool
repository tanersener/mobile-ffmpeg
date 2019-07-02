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

# Sanity check program for various ocsptool options

srcdir="${srcdir:-.}"
OCSPTOOL="${OCSPTOOL:-../src/ocsptool${EXEEXT}}"
DIFF="${DIFF:-diff}"
CMP="${CMP:-cmp}"
TMPFILE=ocsp.$$.tmp

if ! test -x "${OCSPTOOL}"; then
	exit 77
fi

export TZ="UTC"

"${OCSPTOOL}" -j --infile "${srcdir}/ocsp-tests/response1.pem" --outfile "${TMPFILE}"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 1 - PEM loading failed"
	exit ${rc}
fi

${CMP} "${srcdir}/ocsp-tests/response1.der" "${TMPFILE}" >/dev/null 2>&1
rc=$?
if test "${rc}" != "0"; then
	echo "Test 1 - Comparison of DER file failed"
	exit ${rc}
fi

"${OCSPTOOL}" -j --outpem --infile "${srcdir}/ocsp-tests/response1.pem" --outfile "${TMPFILE}"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 2 - PEM loading failed"
	exit ${rc}
fi

${DIFF} -B "${srcdir}/ocsp-tests/response1.pem" "${TMPFILE}" >/dev/null 2>&1
rc=$?
if test "${rc}" != "0"; then
	echo "Test 2 - Comparison of PEM file failed $TMPFILE"
	exit ${rc}
fi


"${OCSPTOOL}" -j --infile "${srcdir}/ocsp-tests/response1.der"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 3 - Transparent (backwards compatible) DER loading failed"
	exit ${rc}
fi

"${OCSPTOOL}" -j --inder --infile "${srcdir}/ocsp-tests/response1.der"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 4 - DER loading failed"
	exit ${rc}
fi

rm -f "${TMPFILE}"

exit 0
