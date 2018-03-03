#!/bin/bash

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z ${ANDROID_NDK} ]]; then
    echo "ANDROID_NDK not defined"
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
COMMON_CFLAGS=$(android_get_cflags "libtheora")
COMMON_CXXFLAGS=$(android_get_cxxflags)
COMMON_LDFLAGS=$(android_get_ldflags "libtheora")
CFLAGS="${COMMON_CFLAGS} -I${ANDROID_NDK}/prebuilt/android-${ARCH}/libogg/include -I${ANDROID_NDK}/prebuilt/android-${ARCH}/libvorbis/include"
CXXFLAGS="$COMMON_CXXFLAGS"
LDFLAGS="$COMMON_LDFLAGS -L${ANDROID_NDK}/prebuilt/android-${ARCH}/libogg/lib -L${ANDROID_NDK}/prebuilt/android-${ARCH}/libvorbis/lib"

cd $1/src/libtheora || exit 1

make clean

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
./configure \
    --prefix=${ANDROID_NDK}/prebuilt/android-${ARCH}/libtheora \
    --with-pic \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-spec \
    --disable-examples \
    --disable-telemetry \
    --enable-valgrind-testing \
    --host=${TARGET_HOST} || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make -j$(nproc) || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make install || exit 1
