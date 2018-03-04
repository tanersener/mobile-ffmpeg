#!/bin/bash

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo "API not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS
android_prepare_toolchain_paths

TARGET_HOST=$(android_get_target_host)
COMMON_CFLAGS=$(android_get_cflags "libxml2")
COMMON_CXXFLAGS=$(android_get_cxxflags "libxml2")
COMMON_LDFLAGS=$(android_get_ldflags "libxml2")
CFLAGS="${COMMON_CFLAGS} -I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/include"
CXXFLAGS="$COMMON_CXXFLAGS"
LDFLAGS="$COMMON_LDFLAGS -L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/lib"

cd $1/src/libxml2 || exit 1

make clean

# NOTE THAT PYTHON IS DISABLED DUE TO THE FOLLOWING ERROR
#
# .../android-sdk/ndk-bundle/toolchains/mobile-ffmpeg-arm/include/python2.7/pyport.h:1029:2: error: #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
# #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
#

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libxml2 \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --with-zlib \
    --with-iconv \
    --without-python \
    --without-debug \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --host=${TARGET_HOST} || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make -j$(nproc) || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make install || exit 1
