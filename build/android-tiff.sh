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

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
export CFLAGS=$(get_cflags "tiff")
export CXXFLAGS=$(get_cxxflags "tiff")
export LDFLAGS=$(get_ldflags "tiff")

# MANUALLY PREPARING PATHS AND TOOLS
export PATH=$PATH:${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin
export AR=${TARGET_HOST}-ar
export AS=${TARGET_HOST}-clang
export CC=${TARGET_HOST}-clang
export CXX=${TARGET_HOST}-clang++
export LD=${TARGET_HOST}-ld
export RANLIB=${TARGET_HOST}-ranlib
export STRIP=${TARGET_HOST}-strip
export INSTALL_PKG_CONFIG_DIR="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/pkgconfig"

cd ${BASEDIR}/src/tiff || exit 1

make distclean 2>/dev/null 1>/dev/null

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/tiff \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot \
    --with-jpeg-include-dir=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/jpeg/include \
    --with-jpeg-lib-dir=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/jpeg/lib \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-maintainer-mode \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
