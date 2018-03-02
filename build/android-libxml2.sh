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
CPPFLAGS="$COMMON_CPPFLAGS -I$ANDROID_NDK/prebuilt/android-$ARCH/libiconv/include"
CXXFLAGS="$COMMON_CXXFLAGS"
LDFLAGS="$COMMON_LDFLAGS -L$ANDROID_NDK/prebuilt/android-$ARCH/libiconv/lib"

cd $1/src/libxml2 || exit 1

make clean

# NOTE THAT PYTHON IS DISABLED DUE TO THE FOLLOWING ERROR
#
# .../android-sdk/ndk-bundle/toolchains/mobile-ffmpeg-arm/include/python2.7/pyport.h:1029:2: error: #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
# #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
#

CPPFLAGS=$CPPFLAGS \
CXXFLAGS=$CXXFLAGS \
LDFLAGS=$LDFLAGS \
./configure \
    --prefix=$ANDROID_NDK/prebuilt/android-$ARCH/libxml2 \
    --with-pic \
    --with-sysroot=$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot \
    --with-zlib \
    --with-iconv \
    --without-python \
    --without-debug \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --host=$TARGET_HOST || exit 1

CPPFLAGS=$CPPFLAGS \
CXXFLAGS=$CXXFLAGS \
LDFLAGS=$LDFLAGS \
make -j$(nproc) || exit 1

CPPFLAGS=$CPPFLAGS \
CXXFLAGS=$CXXFLAGS \
LDFLAGS=$LDFLAGS \
make install || exit 1
