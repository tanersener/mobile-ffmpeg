#!/bin/sh

# Copyright (C) 2016 Nikos Mavrogiannopoulos
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

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
GREP="${GREP:-grep}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=1"
fi

TMPFILE=tmp-key.$$.p8

${VALGRIND} "${CERTTOOL}" -k --password 1234 --infile "${srcdir}/data/p8key-illegal.pem"
rc=$?
# We're done.
if test "${rc}" != "1"; then
	echo "Error in importing illegal PKCS#8 key"
	exit ${rc}
fi

#check invalid RSA pem key. The key has even prime factor.
${VALGRIND} "${CERTTOOL}" -k --infile "${srcdir}/data/key-illegal.pem"
rc=$?
# We're done.
if test "${rc}" != "1"; then
	echo "Error in importing illegal RSA key"
	exit ${rc}
fi

#check invalid RSA pem key. The key has too large salt.
${VALGRIND} "${CERTTOOL}" -k --infile "${srcdir}/data/key-illegal-rsa-pss.pem"
rc=$?
# We're done.
if test "${rc}" != "1"; then
	echo "Error in importing illegal RSA-PSS key"
	exit ${rc}
fi

#sanity generation
${VALGRIND} "${CERTTOOL}" --generate-privkey --key-type rsa-pss --hash sha256 --salt-size 64 --bits 2048 >/dev/null
rc=$?
# We're done.
if test "${rc}" != "0"; then
	echo "Error in generating an RSA-PSS key"
	exit ${rc}
fi

# generate illegal value
${VALGRIND} "${CERTTOOL}" --generate-privkey --key-type rsa-pss --hash sha256 --salt-size 1024 --bits 2048 >/dev/null
rc=$?
# We're done.
if test "${rc}" != "1"; then
	echo "Error: allowed generation of an illegal key"
	exit ${rc}
fi

rm -f $TMPFILE

exit 0
