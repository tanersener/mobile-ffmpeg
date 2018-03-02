#!/bin/bash

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z $ANDROID_NDK ]]; then
    echo "ANDROID_NDK not defined"
    exit 1
fi

if [[ -z $ARCH ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z $API ]]; then
    echo "API not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS
android_prepare_toolchain_paths $ARCH

TARGET_HOST=$(android_get_target_host $ARCH)
COMMON_CPPFLAGS=$(android_get_common_cppflags $ARCH)
COMMON_CXXFLAGS=$(android_get_common_cxxflags $ARCH)
COMMON_LDFLAGS=$(android_get_common_ldflags $ARCH)
CPPFLAGS="$COMMON_CPPFLAGS"
CXXFLAGS="$COMMON_CXXFLAGS"
LDFLAGS="$COMMON_LDFLAGS"

cd $1/src/wavpack || exit 1

make clean

CPPFLAGS=$CPPFLAGS \
CXXFLAGS=$CXXFLAGS \
LDFLAGS=$LDFLAGS \
./configure \
    --prefix=$ANDROID_NDK/prebuilt/android-$ARCH/wavpack \
    --with-pic \
    --with-sysroot=$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot \
    --without-iconv \
    --enable-static \
    --disable-shared \
    --disable-apps \
    --disable-fast-install \
    --disable-tests \
    --host=$TARGET_HOST || exit 1

CPPFLAGS=$CPPFLAGS \
CXXFLAGS=$CXXFLAGS \
LDFLAGS=$LDFLAGS \
make -j$(nproc) || exit 1

CPPFLAGS=$CPPFLAGS \
CXXFLAGS=$CXXFLAGS \
LDFLAGS=$LDFLAGS \
make install || exit 1
