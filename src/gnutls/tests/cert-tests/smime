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
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi

OUTFILE=out-pkcs7.$$.tmp

. ${srcdir}/../scripts/common.sh

check_for_datefudge

# test the --smime-to-p7 functionality
${VAGRLIND} "${CERTTOOL}" --smime-to-p7 --infile "${srcdir}/data/pkcs7.smime" --outfile ${OUTFILE}
rc=$?
if test "${rc}" != "0"; then
	echo "SMIME to pkcs7 transformation failed"
	exit ${rc}
fi


datefudge -s "2017-4-6" \
${VALGRIND} "${CERTTOOL}" --p7-verify --load-certificate "${srcdir}/../../doc/credentials/x509/cert-rsa.pem" <"${OUTFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "PKCS7 verification failed"
	exit ${rc}
fi

rm -f "${OUTFILE}"

exit 0
