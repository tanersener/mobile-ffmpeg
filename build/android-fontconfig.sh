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
COMMON_CFLAGS=$(android_get_cflags "fontconfig")
CXXFLAGS=$(android_get_cxxflags "fontconfig")
COMMON_LDFLAGS=$(android_get_ldflags "fontconfig")

CFLAGS="${COMMON_CFLAGS} -I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/include -I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libxml2/include -I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/freetype/include"
LDFLAGS="$COMMON_LDFLAGS -L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/lib -L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libxml2/lib -L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/freetype/lib"

cd $1/src/fontconfig || exit 1

make clean

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
PKG_CONFIG_PATH=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/freetype/lib/pkgconfig:${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libxml2/lib/pkgconfig \
./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/fontconfig \
    --with-pic \
    --with-libiconv-prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-largefile \
    --disable-uuid \
    --enable-iconv \
    --enable-libxml2 \
    --disable-docs \
    --host=${TARGET_HOST} || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
#make -j$(nproc) || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
#make install || exit 1
