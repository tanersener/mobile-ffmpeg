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
export CFLAGS=$(android_get_cflags "libvorbis")
export CXXFLAGS=$(android_get_cxxflags "libvorbis")
export LDFLAGS=$(android_get_ldflags "libvorbis")

cd $1/src/libvorbis || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libvorbis \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --with-ogg-includes=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libogg/include \
    --with-ogg-libraries=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libogg/lib \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-docs \
    --disable-examples \
    --disable-oggtest \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp vorbisenc.pc ${INSTALL_PKG_CONFIG_DIR}
cp vorbisfile.pc ${INSTALL_PKG_CONFIG_DIR}
cp vorbis.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
