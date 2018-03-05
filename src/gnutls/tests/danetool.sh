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

srcdir="${srcdir:-.}"
DANETOOL="${DANETOOL:-../src/danetool${EXEEXT}}"

if test "${WINDIR}" != ""; then
	exit 77
fi

if ! test -x "${DANETOOL}"; then
	exit 77
fi

. "${srcdir}/scripts/common.sh"

# Check local generation
OUT=$(${DANETOOL} --tlsa-rr --host www.example.com --load-certificate ${srcdir}/certs/cert-ecc256.pem)
if test $? != 0;then
	echo "error in test 1"
	exit 1
fi

if test "$OUT" != '_443._tcp.www.example.com. IN TLSA ( 03 01 01 5978dd1d2d23e992075dc359d5dd14f7ef79748af97f2b7809c9ebfd6016c433 )';then
	echo "error in test 2"
	exit 1
fi

OUT=$(${DANETOOL} --tlsa-rr --host www.example.com --load-certificate ${srcdir}/certs/cert-rsa-2432.pem)
if test $? != 0;then
	echo "error in test 3"
	exit 1
fi

if test "$OUT" != '_443._tcp.www.example.com. IN TLSA ( 03 01 01 671b40d05b28c85e9b2a52912abcdce38c0384cc5a7c693ed3148ca1e97632c9 )';then
	echo "error in test 4"
	exit 1
fi

# Check CA signed certificate generation
OUT=$(${DANETOOL} --tlsa-rr --no-domain --host www.example.com --load-certificate ${srcdir}/certs/cert-rsa-2432.pem)
if test $? != 0;then
	echo "error in test 5"
	exit 1
fi

if test "$OUT" != '_443._tcp.www.example.com. IN TLSA ( 01 01 01 671b40d05b28c85e9b2a52912abcdce38c0384cc5a7c693ed3148ca1e97632c9 )';then
	echo "error in test 6"
	exit 1
fi

# Check CA signer's certificate generation
OUT=$(${DANETOOL} --tlsa-rr --ca --no-domain --host www.example.com --load-certificate ${srcdir}/certs/cert-rsa-2432.pem)
if test $? != 0;then
	echo "error in test 7"
	exit 1
fi

if test "$OUT" != '_443._tcp.www.example.com. IN TLSA ( 00 01 01 671b40d05b28c85e9b2a52912abcdce38c0384cc5a7c693ed3148ca1e97632c9 )';then
	echo "error in test 8"
	exit 1
fi

exit 0
