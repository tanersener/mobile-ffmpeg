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

echo "Checking chain with insecure leaf"
datefudge -s "2019-12-19" \
${VALGRIND} "${CERTTOOL}" --verify-chain --verify-profile=medium --infile "${srcdir}/data/chain-512-leaf.pem" >${OUTFILE}
rc=$?

if test "${rc}" != "1"; then
	echo "insecure chain succeeded verification (1)"
	cat $OUTFILE
	exit ${rc}
fi

echo "Checking chain with insecure subca"
datefudge -s "2019-12-19" \
${VALGRIND} "${CERTTOOL}" --verify-chain --verify-profile=medium --infile "${srcdir}/data/chain-512-subca.pem" >${OUTFILE}
rc=$?

if test "${rc}" != "1"; then
	echo "insecure chain succeeded verification (2)"
	cat $OUTFILE
	exit ${rc}
fi


echo "Checking chain with insecure ca"
datefudge -s "2019-12-19" \
${VALGRIND} "${CERTTOOL}" --verify-chain --verify-profile=medium --infile "${srcdir}/data/chain-512-ca.pem" >${OUTFILE}
rc=$?

if test "${rc}" != "1"; then
	echo "insecure chain succeeded verification (3)"
	cat $OUTFILE
	exit ${rc}
fi


rm -f "${OUTFILE}"

exit 0
