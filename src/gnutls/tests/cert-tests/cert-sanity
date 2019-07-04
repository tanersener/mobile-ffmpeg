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
# You should have received a copy of the GNU General Public License
# along with GnuTLS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

# This checks whether invalid certificates are accepted

# x509-v1-with-sid.pem: X509v1 certificate with subject unique ID
# x509-v1-with-iid.pem: X509v1 certificate with issuer unique ID
# x509-v3-with-fractional-time.pem: X509v3 certificate with fractional time
# x509-with-zero-version.pem: X509 certificate with version being zero

for file in x509-v1-with-sid.pem x509-v1-with-iid.pem x509-v3-with-fractional-time.pem \
	x509-with-zero-version.pem; do

	${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/$file"
	rc=$?

	if test "${rc}" != 1; then
		echo "Illegal X509 certificate was accepted"
		exit 1
	fi
done

exit 0
