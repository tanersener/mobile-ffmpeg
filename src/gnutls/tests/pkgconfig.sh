#!/bin/sh

# Copyright (C) 2017 Nikos Mavrogiannopoulos
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
top_builddir="${top_builddir:-..}"
PKGCONFIG="${PKG_CONFIG:-$(which pkg-config)}"
CC=${CC:-cc}
unset RETCODE
TMPFILE=c.$$.tmp.c
TMPFILE_O=c.$$.tmp.o

echo "$CFLAGS"|grep sanitize && exit 77

${PKGCONFIG} --version >/dev/null || exit 77

${PKGCONFIG} --libs nettle
if test $? != 0;then
	echo "Nettle was not found in pkg-config"
	exit 77
fi

for lib in libidn2 p11-kit-1
do
	OTHER=$(${PKGCONFIG} --libs --static $lib)
	if test -n "${OTHER}" && test "${OTHER#*-R}" != "$OTHER";then
		echo "Found invalid string in $lib flags: ${OTHER}"
		exit 77
	fi
done

if ! test -r ${top_builddir}/lib/gnutls.pc ;then
	echo "gnutls.pc not present at ${top_builddir}/lib"
	exit 1
fi

PKG_CONFIG_PATH=${top_builddir}/lib:$PKG_CONFIG_PATH
export PKG_CONFIG_PATH

set -e

cat >$TMPFILE <<__EOF__
#include <gnutls/gnutls.h>

int main()
{
gnutls_global_init();
}
__EOF__

COMMON="-I${top_builddir}/lib/includes -L${top_builddir}/lib/.libs -I${srcdir}/../lib/includes"
echo "Trying dynamic linking with:"
echo "  * flags: $(${PKGCONFIG} --libs gnutls)"
echo "  * common: ${COMMON}"
echo "  * lib: ${CFLAGS}"
echo cc ${TMPFILE} -o ${TMPFILE_O} $(${PKGCONFIG} --libs gnutls) $(${PKGCONFIG} --cflags gnutls) ${COMMON}
${CC} ${TMPFILE} -o ${TMPFILE_O} $(${PKGCONFIG} --libs gnutls) $(${PKGCONFIG} --cflags gnutls) ${COMMON}

echo ""
echo "Trying static linking with $(${PKGCONFIG} --libs --static gnutls)"
echo cc ${TMPFILE} -o ${TMPFILE_O} $(${PKGCONFIG} --static --libs gnutls) $(${PKGCONFIG} --cflags gnutls) ${COMMON}
${CC} ${TMPFILE} -o ${TMPFILE_O} $(${PKGCONFIG} --static --libs gnutls) $(${PKGCONFIG} --cflags gnutls) ${COMMON}

rm -f ${TMPFILE} ${TMPFILE_O}

exit 0
