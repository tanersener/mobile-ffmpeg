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
OCSPTOOL="${OCSPTOOL:-../src/ocsptool${EXEEXT}}"
DIFF="${DIFF:-diff}"

if ! test -x "${OCSPTOOL}"; then
	exit 77
fi

export TZ="UTC"

. "${srcdir}/scripts/common.sh"

check_for_datefudge

datefudge -s "2017-06-19" \
	"${OCSPTOOL}" -e --load-chain "${srcdir}/ocsp-tests/certs/chain-amazon.com.pem" --infile "${srcdir}/ocsp-tests/certs/ocsp-amazon.com.der" --verify-allow-broken
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 1 - Amazon OCSP response verification - failed"
	exit ${rc}
fi

datefudge -s "2017-06-19" \
	"${OCSPTOOL}" -e --load-chain "${srcdir}/ocsp-tests/certs/chain-amazon.com-unsorted.pem" --infile "${srcdir}/ocsp-tests/certs/ocsp-amazon.com.der" --verify-allow-broken
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 2 - Amazon OCSP response verification - failed"
	exit ${rc}
fi

# verify an OCSP response using ECDSA
datefudge -s "2017-06-29" \
	"${OCSPTOOL}" -d 6 -e --load-chain "${srcdir}/ocsp-tests/certs/chain-akamai.com.pem" --infile "${srcdir}/ocsp-tests/certs/ocsp-akamai.com.der"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 3 - Akamai (ECDSA) OCSP response verification - failed"
	exit ${rc}
fi

exit 0
