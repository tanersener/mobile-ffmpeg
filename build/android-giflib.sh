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
COMMON_CFLAGS=$(android_get_cflags "giflib")
CXXFLAGS=$(android_get_cxxflags "giflib")
LDFLAGS=$(android_get_ldflags "giflib")
CFLAGS="${COMMON_CFLAGS} -DS_IREAD=S_IRUSR -DS_IWRITE=S_IWUSR"

cd $1/src/giflib || exit 1

echo $CXXFLAGS
echo $CFLAGS

make clean

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/giflib \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
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
