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
export CFLAGS=$(android_get_cflags "nettle")
export CXXFLAGS=$(android_get_cxxflags "nettle")
export LDFLAGS=$(android_get_ldflags "nettle")
export PKG_CONFIG_PATH="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/pkgconfig"

OPTIONAL_CPU_SUPPORT=""
if [ ${ARCH} == "x86" ] || [ ${ARCH} == "x86_64" ]; then
    OPTIONAL_CPU_SUPPORT="--enable-x86-aesni"
else
    OPTIONAL_CPU_SUPPORT="--enable-arm-neon"
fi

cd $1/src/nettle || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/nettle \
    --enable-pic \
    --enable-static \
    --disable-shared \
    --disable-mini-gmp \
    --disable-openssl \
    --disable-gcov \
    --disable-documentation ${OPTIONAL_CPU_SUPPORT} \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
