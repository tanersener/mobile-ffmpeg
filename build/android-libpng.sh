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
CFLAGS=$(android_get_cflags "libpng")
CXXFLAGS=$(android_get_cxxflags "libpng")
LDFLAGS=$(android_get_ldflags "libpng")

OPTIONAL_CPU_SUPPORT=""
if [ ${ARCH} == "x86" ] || [ ${ARCH} == "x86_64" ]; then
    OPTIONAL_CPU_SUPPORT="--enable-sse"
fi
if [ ${ARCH} == "arm" ] || [ ${ARCH} == "arm64" ]; then
    OPTIONAL_CPU_SUPPORT="--enable-arm-neon"
fi

cd $1/src/libpng || exit 1

make clean

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libpng \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-unversioned-libpng-pc \
    --disable-unversioned-libpng-config \
    --enable-hardware-optimizations \
    $OPTIONAL_CPU_SUPPORT \
    --host=${TARGET_HOST} || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make -j$(nproc) || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make install || exit 1
