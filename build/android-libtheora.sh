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
export CFLAGS=$(android_get_cflags "libtheora")
export CXXFLAGS=$(android_get_cxxflags "libtheora")
export LDFLAGS=$(android_get_ldflags "libtheora")
export PKG_CONFIG_PATH=${INSTALL_PKG_CONFIG_DIR}

cd $1/src/libtheora || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libtheora \
    --with-pic \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-spec \
    --disable-examples \
    --disable-telemetry \
    --enable-valgrind-testing \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp theoradec.pc ${INSTALL_PKG_CONFIG_DIR}
cp theoraenc.pc ${INSTALL_PKG_CONFIG_DIR}
cp theora.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
