#!/bin/sh

# Copyright (C) 2007-2012 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

# Sets up the execution environment needed to run the test programs
# and produce the documentation.


GUILE_LOAD_PATH="@abs_top_srcdir@/guile/modules:$GUILE_LOAD_PATH"
GUILE_LOAD_PATH="@abs_top_builddir@/guile/modules:$GUILE_LOAD_PATH"
export GUILE_LOAD_PATH

GNUTLS_GUILE_EXTENSION_DIR="@abs_top_builddir@/guile/src"
export GNUTLS_GUILE_EXTENSION_DIR

exec @abs_top_builddir@/libtool --mode=execute		                    \
       -dlopen "@abs_top_builddir@/guile/src/guile-gnutls-v-2.la"        \
       @GUILE@ "$@"
