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
CFLAGS=$(android_get_cflags "libwebp")
CXXFLAGS=$(android_get_cxxflags "libwebp")
LDFLAGS=$(android_get_ldflags "libwebp")

cd $1/src/libwebp || exit 1

if [ -d "build" ]; then
    rm -rf build;
fi

mkdir build;
cd build

cmake -Wno-dev \
    -DCMAKE_VERBOSE_MAKEFILE=0 \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot" \
    -DCMAKE_FIND_ROOT_PATH="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libwebp" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin/$CC" \
    -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin/$LD" \
    -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin/$AR" \
    -DGIF_INCLUDE_DIR="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/giflib/include" \
    -DGIF_LIBRARY="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/giflib/lib" \
    -DJPEG_INCLUDE_DIR="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/jpeg/include" \
    -DJPEG_LIBRARY="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/jpeg/lib" \
    -DPNG_PNG_INCLUDE_DIR="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libpng/include" \
    -DPNG_LIBRARY="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libpng/lib" \
    -DTIFF_INCLUDE_DIR="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/tiff/include" \
    -DTIFF_LIBRARY="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/tiff/lib" \
    -DZLIB_INCLUDE_DIR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot/usr/include" \
    -DZLIB_LIBRARY="${ANDROID_NDK_ROOT}/platform/android-${API}/arch-${ARCH}/usr/lib" \
    -DCMAKE_SYSTEM_PROCESSOR=${ARCH} \
    -DBUILD_SHARED_LIBS=0 .. || exit 1

make -j$(nproc) || exit 1

make install || exit 1
