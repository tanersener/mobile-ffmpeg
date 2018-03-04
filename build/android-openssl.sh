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

# MANUALLY PREPARING PATHS AND TOOLS
export PATH=$PATH:${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin
export AR=ar
export AS=as
export CC=gcc
export CXX=g++
export LD=ld
export RANLIB=ranlib
export STRIP=strip

TARGET_HOST=$(android_get_target_host)
TARGET_MACHINE=$(android_get_target_machine)
COMMON_CFLAGS=$(android_get_cflags "openssl")
CXXFLAGS=$(android_get_cxxflags "openssl")
LDFLAGS=$(android_get_ldflags "openssl")

CFLAGS="${COMMON_CFLAGS} -I${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/lib/gcc/${TARGET_HOST}/4.9.x/include"

export _ANDROID_EABI="mobile-ffmpeg-${ARCH}"
export _ANDROID_ARCH="arm-${ARCH}"
export ANDROID_API="android-${API}"
export ANDROID_TOOLCHAIN="${ANDROID_NDK_ROOT}/toolchains/${_ANDROID_EABI}/bin"
export ANDROID_TOOLS="${CC} ${RANLIB} ${LD}"
export ANDROID_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/${_ANDROID_EABI}/sysroot"
export CROSS_SYSROOT="${ANDROID_SYSROOT}"
export ANDROID_NDK_SYSROOT="${ANDROID_SYSROOT}"
export SYSROOT="${ANDROID_SYSROOT}"
export NDK_SYSROOT="${ANDROID_SYSROOT}"
export ANDROID_DEV="${ANDROID_NDK_ROOT}/toolchains/${_ANDROID_EABI}/sysroot/usr"
export HOSTCC="${CC}"

export MACHINE="${TARGET_MACHINE}"
export ARCH="${ARCH}"
export SYSTEM="android"
export CROSS_COMPILE="${TARGET_HOST}-"

cd $1/src/openssl || exit 1

make clean

perl -pi -e 's/install: all install_docs install_sw/install: install_docs install_sw/g' Makefile.org

./config \
     no-shared \
     no-ssl2 \
     no-ssl3 \
     no-comp \
     no-hw \
     no-engine \
     no-err \
     no-npn \
     no-psk \
     ${CFLAGS} \
     ${LDFLAGS} \
     --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/openssl \
     --openssldir=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/openssl || exit 1

make depend || exit 1
make all || exit 1
make install || exit 1
