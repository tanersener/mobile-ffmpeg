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
unset RETCODE
TMPFILE=c.$$.tmp.c
TMPFILE_O=c.$$.tmp.o

echo "$CFLAGS"|grep sanitize && exit 77

${PKGCONFIG} --version >/dev/null || exit 77

PKG_CONFIG_PATH=${top_builddir}/lib
export PKG_CONFIG_PATH

set -e

cat >$TMPFILE <<__EOF__
#include <gnutls/gnutls.h>

int main()
{
gnutls_global_init();
}
__EOF__

COMMON="-I${PKG_CONFIG_PATH}/includes -L${PKG_CONFIG_PATH}/.libs -I${srcdir}/../lib/includes"
echo "Trying dynamic linking with:"
echo "  * flags: $(${PKGCONFIG} --libs gnutls)"
echo "  * common: ${COMMON}"
echo "  * lib: ${CFLAGS}"
cc ${TMPFILE} -o ${TMPFILE_O} $(${PKGCONFIG} --libs gnutls) $(${PKGCONFIG} --cflags gnutls) ${COMMON}

echo ""
echo "Trying static linking with $(${PKGCONFIG} --libs --static gnutls)"
cc ${TMPFILE} -o ${TMPFILE_O} $(${PKGCONFIG} --static --libs gnutls) $(${PKGCONFIG} --cflags gnutls) ${COMMON}

rm -f ${TMPFILE} ${TMPFILE_O}

exit 0
