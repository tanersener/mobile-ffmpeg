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
export CFLAGS=$(android_get_cflags "gmp")
export CXXFLAGS=$(android_get_cxxflags "gmp")
export LDFLAGS=$(android_get_ldflags "gmp")

cd $1/src/gmp || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/gmp \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

make install || exit 1
