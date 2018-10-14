#!/bin/sh
#
#  autobuild

libtoolize -c -f || glibtoolize -c -f
aclocal
autoheader -f
autoconf
if grep -q PKG_CHECK_MODULES configure; then
  # The generated configure is invalid because pkg-config is unavailable.
  rm configure
  echo "Error, missing pkg-config. Check the build requirements."
  exit 1
fi
automake -a -c -f
