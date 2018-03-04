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
CFLAGS=$(android_get_cflags "libssh")
CXXFLAGS=$(android_get_cxxflags "libssh")
LDFLAGS=$(android_get_ldflags "libssh")

cd $1/src/libssh || exit 1

if [ -d "build" ]; then
    rm -rf build;
fi

mkdir build;
cd build

cmake -Wno-dev \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libssh" \
    -DWITH_DEBUG_CALLTRACE=0 \
    -DWITH_EXAMPLES=0 \
    -DWITH_GSSAPI=0 \
    -DWITH_PCAP=0 \
    -DWITH_SERVER=0 \
    -DWITH_SFTP=1 \
    -DWITH_STATIC_LIB=1 \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SYSTEM_PROCESSOR=${ARCH} \
    -DCMAKE_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot" \
    -DCMAKE_FIND_ROOT_PATH="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin/${CC}" \
    -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin/${LD}" \
    -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin/${AR}" \
    -DZLIB_INCLUDE_DIR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot/usr/include" \
    -DZLIB_LIBRARY="${ANDROID_NDK_ROOT}/platform/android-${API}/arch-${ARCH}/usr/lib" \
    -DOPENSSL_INCLUDE_DIR="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/openssl/include" \
    -DOPENSSL_CRYPTO_LIBRARY="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/openssl/lib" \
    .. || exit 1

make -j$(nproc) || exit 1

make install || exit 1
