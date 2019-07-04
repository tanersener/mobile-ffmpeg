#!/bin/sh

# Copyright (C) 2017 Nikos Mavrogiannopoulos
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

OLD_CA_FILE="${srcdir}/data/alt-chain-old-ca.pem"
NEW_CA_FILE="${srcdir}/data/alt-chain-new-ca.pem"

echo ""
datefudge -s "2017-5-10" \
${VALGRIND} "${CERTTOOL}" --load-ca-certificate ${OLD_CA_FILE} --verify-hostname www.google.com --verify --infile "${srcdir}/data/alt-chain.pem" >${OUTFILE}
rc=$?

if test "${rc}" != "1"; then
	echo "alt chain failed verification (1)"
	cat $OUTFILE
	exit ${rc}
fi

echo ""
datefudge -s "2017-5-10" \
${VALGRIND} "${CERTTOOL}" --load-ca-certificate ${NEW_CA_FILE} --verify-hostname www.google.com --verify --infile "${srcdir}/data/alt-chain.pem" >${OUTFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "alt chain failed verification (2)"
	cat $OUTFILE
	exit ${rc}
fi

rm -f "${OUTFILE}"

exit 0
