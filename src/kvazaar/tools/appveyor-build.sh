#!/usr/bin/bash
set -e

export CC=gcc

./autogen.sh
./configure \
    --host=$MINGW_CHOST \
    --build=$MINGW_CHOST \
    --target=$MINGW_CHOST \
    --disable-shared --enable-static
make
