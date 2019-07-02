#!/bin/sh

# Copyright (C) 2007-2010, 2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
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

set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
TMPFILE=key-id.$$.tmp
TEMPLFILE=tmpl.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=1"
fi

PARAMS="--generate-certificate --load-privkey '${srcdir}/data/key-user.pem' --load-ca-privkey '${srcdir}/data/key-ca.pem' --template $TEMPLFILE"

echo "serial = 1" > $TEMPLFILE

#eval "${CERTTOOL}" ${PARAMS} --load-ca-certificate $srcdir/ca-gnutls-keyid.pem \
#    --outfile user-gnutls-keyid.pem 2> /dev/null

#eval "${CERTTOOL}" ${PARAMS} --load-ca-certificate $srcdir/ca-no-keyid.pem \
#    --outfile user-no-keyid.pem 2> /dev/null

eval ${VALGRIND} "${CERTTOOL}" ${PARAMS} --load-ca-certificate "${srcdir}/data/ca-weird-keyid.pem" \
	--outfile $TMPFILE

if ${VALGRIND} "${CERTTOOL}" -i < $TMPFILE \
	| grep '7a2c7a6097460603cbfb28e8e219df18deeb4e0d' > /dev/null; then
:
else
	echo "Could not find CA SKI in user certificate."
	exit 1;
fi

rm -f $TEMPLFILE $TMPFILE

# We're done.
exit 0
