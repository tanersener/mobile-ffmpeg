#!/bin/sh

# Copyright (C) 2015 Nikos Mavrogiannopoulos
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

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"
if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

#check whether a different PKCS #1 signature than the advertized in certificate is tolerated
${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/data/invalid-sig.pem"
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "Verification of invalid signature (1) failed"
	exit ${rc}
fi

#check whether a different tbsCertificate than the outer signature algorithm is tolerated
${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/data/invalid-sig2.pem"
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "Verification of invalid signature (2) failed"
	exit ${rc}
fi

#check whether a different tbsCertificate than the outer signature algorithm is tolerated
${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/data/invalid-sig3.pem"
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "Verification of invalid signature (3) failed"
	exit ${rc}
fi

#check whether different parameters in tbsCertificate than the outer signature is tolerated
${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/data/invalid-sig4.pem"
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "Verification of invalid signature (4) failed"
	exit ${rc}
fi

#check whether different RSA-PSS parameters in tbsCertificate than the outer signature is tolerated
${VALGRIND} "${CERTTOOL}" --verify-chain --infile "${srcdir}/data/invalid-sig5.pem"
rc=$?

# We're done.
if test "${rc}" = "0"; then
	echo "Verification of invalid signature (5) failed"
	exit ${rc}
fi

#this was causing a double free; verify that we receive the expected error code
${VALGRIND} "${CERTTOOL}" --verify-chain --infile "${srcdir}/data/cve-2019-3829.pem"
rc=$?

# We're done.
if test "${rc}" != "1"; then
	echo "Verification of invalid signature (6) failed"
	exit ${rc}
fi

exit 0
