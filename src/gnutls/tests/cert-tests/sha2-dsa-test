#!/bin/sh

# Copyright (C) 2006-2008, 2010, 2012 Free Software Foundation, Inc.
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

#set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
TEMPLFILE=template-dsa.$$.tmp
CAFILE=ca-dsa.$$.tmp
SUBCAFILE=subca-dsa.$$.tmp
TMPFILE=sha2-dsa.$$.tmp
USERFILE=user-dsa.$$.tmp
VERIFYFILE=verify-dsa.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

echo ca > $TEMPLFILE
echo "cn = SHA 256 CA" >> $TEMPLFILE

"${CERTTOOL}" -d 2 --generate-self-signed --template $TEMPLFILE \
	--load-privkey "${srcdir}/data/key-ca-dsa.pem" \
	--outfile $CAFILE \
	--hash sha256 >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo ca > $TEMPLFILE
echo "cn = SHA 224 Mid CA" >> $TEMPLFILE

"${CERTTOOL}" -d 2 --generate-certificate --template $TEMPLFILE \
	--load-ca-privkey "${srcdir}/data/key-ca-dsa.pem" \
	--load-ca-certificate $CAFILE \
	--load-privkey "${srcdir}/data/key-subca-dsa.pem" \
	--outfile $SUBCAFILE \
	--hash sha224 >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo "cn = End-user" > $TEMPLFILE

"${CERTTOOL}" -d 2 --generate-certificate --template $TEMPLFILE \
	--load-ca-privkey "${srcdir}/data/key-subca-dsa.pem" \
	--load-ca-certificate $SUBCAFILE \
	--load-privkey "${srcdir}/data/key-dsa.pem" \
	--outfile $USERFILE >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

cat $USERFILE $SUBCAFILE $CAFILE > $TMPFILE
"${CERTTOOL}" --verify-chain <$TMPFILE > $VERIFYFILE

if [ $? != 0 ]; then
	cat $VERIFYFILE
	exit 1
fi

rm -f $VERIFYFILE $USERFILE $CAFILE $SUBCAFILE $TEMPLFILE $TMPFILE

exit 0
