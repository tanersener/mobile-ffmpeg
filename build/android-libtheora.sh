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

if [[ -z ${BASEDIR} ]]; then
    echo "BASEDIR not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
set_toolchain_clang_paths

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
export CFLAGS=$(get_cflags "libtheora")
export CXXFLAGS=$(get_cxxflags "libtheora")
export LDFLAGS=$(get_ldflags "libtheora")
export PKG_CONFIG_PATH=${INSTALL_PKG_CONFIG_DIR}

cd ${BASEDIR}/src/libtheora || exit 1

make distclean 2>/dev/null 1>/dev/null

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-$(get_target_build ${ARCH})/libtheora \
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
