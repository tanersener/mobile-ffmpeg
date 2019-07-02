#!/bin/sh

# Copyright (C) 2014 Red Hat
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

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

. "${srcdir}/scripts/common.sh"

TMPFILE=long.$$.pem.tmp

${VALGRIND} "${CERTTOOL}" --crl-info --inder --infile "${srcdir}/data/long.crl" --outfile $TMPFILE
if test $? != 0; then
	echo "CRL decoding failed 1!"
	exit 1
fi

check_if_equal "${srcdir}/data/long.pem" $TMPFILE "^warning"
if test $? != 0; then
	echo "CRL decoding failed 2!"
	exit 1
fi

rm -f $TMPFILE

exit 0
