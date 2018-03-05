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

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
COMMON_CFLAGS=$(android_get_cflags "fontconfig")
COMMON_LDFLAGS=$(android_get_ldflags "fontconfig")

export CFLAGS="$COMMON_CFLAGS -I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libuuid/include"
export CXXFLAGS=$(android_get_cxxflags "fontconfig")
export LDFLAGS="$COMMON_LDFLAGS -L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libuuid/lib"
export PKG_CONFIG_PATH=${INSTALL_PKG_CONFIG_DIR}

cd $1/src/fontconfig || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/fontconfig \
    --with-pic \
    --with-pkgconfigdir=${INSTALL_PKG_CONFIG_DIR} \
    --with-libiconv-includes=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/include \
    --with-libiconv-lib=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/lib \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-rpath \
    --enable-iconv \
    --enable-libxml2 \
    --disable-docs \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

make install || exit 1
