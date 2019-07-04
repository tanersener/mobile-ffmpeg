#!/bin/sh

# Copyright (C) 2011-2012 Free Software Foundation, Inc.
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
TMPFILE=ecdsa.$$.tmp
TMPCA=ecdsa-ca.$$.tmp
TMPCAKEY=ecdsa-ca-key.$$.tmp
TMPSUBCA=ecdsa-subca.$$.tmp
TMPSUBCAKEY=ecdsa-subca-key.$$.tmp
TMPKEY=ecdsa-key.$$.tmp
TMPTEMPL=template.$$.tmp
TMPUSER=user.$$.tmp
VERIFYOUT=verify.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

echo ca > $TMPTEMPL
echo "cn = ECDSA SHA 256 CA" >> $TMPTEMPL

"${CERTTOOL}" --generate-privkey --ecc > $TMPCAKEY 2>/dev/null

"${CERTTOOL}" -d 2 --generate-self-signed --template $TMPTEMPL \
	--load-privkey $TMPCAKEY \
	--outfile $TMPCA \
	--hash sha256 >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo ca > $TMPTEMPL
"${CERTTOOL}" --generate-privkey --ecc > $TMPSUBCAKEY 2>/dev/null
echo "cn = ECDSA SHA 224 Mid CA" >> $TMPTEMPL

"${CERTTOOL}" -d 2 --generate-certificate --template $TMPTEMPL \
	--load-ca-privkey $TMPCAKEY \
	--load-ca-certificate $TMPCA \
	--load-privkey $TMPSUBCAKEY \
	--outfile $TMPSUBCA \
	--hash sha224 >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

echo "cn = End-user" > $TMPTEMPL

"${CERTTOOL}" --generate-privkey --ecc > $TMPKEY 2>/dev/null

"${CERTTOOL}" -d 2 --generate-certificate --template $TMPTEMPL \
	--load-ca-privkey $TMPSUBCAKEY \
	--load-ca-certificate $TMPSUBCA \
	--load-privkey $TMPKEY \
	--outfile $TMPUSER >$TMPFILE 2>&1

if [ $? != 0 ]; then
	cat $TMPFILE
	exit 1
fi

cat $TMPUSER $TMPSUBCA $TMPCA > $TMPFILE
"${CERTTOOL}" --verify-chain <$TMPFILE > $VERIFYOUT

if [ $? != 0 ]; then
	cat $VERIFYOUT
	exit 1
fi

rm -f $VERIFYOUT $TMPUSER $TMPCA $TMPSUBCA $TMPTEMPL $TMPFILE
rm -f $TMPSUBCAKEY $TMPCAKEY $TMPKEY

"${CERTTOOL}" -k < "${srcdir}/data/bad-key.pem" | grep "validation failed" >/dev/null 2>&1
if [ $? != 0 ]; then
	echo "certtool didn't detect a bad ECDSA key."
	exit 1
fi

exit 0
