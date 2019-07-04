#!/bin/sh

# Copyright (C) 2019 Daiki Ueno
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
#

srcdir="${srcdir:-.}"
builddir="${builddir:-.}"

. "${srcdir}/scripts/common.sh"

check_for_datefudge

datefudge -s 2019-04-12 "${builddir}/tls13/prf-early" "$@"
exit $?
