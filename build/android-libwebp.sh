#!/bin/bash

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
CFLAGS=$(get_cflags "libwebp")
CXXFLAGS=$(get_cxxflags "libwebp")
LDFLAGS=$(get_ldflags "libwebp")
PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

cd ${BASEDIR}/src/libwebp || exit 1

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
    -DCMAKE_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot" \
    -DCMAKE_FIND_ROOT_PATH="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/android-$(get_target_build)/libwebp" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$CC" \
    -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$LD" \
    -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$AR" \
    -DGIF_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/giflib/include" \
    -DGIF_LIBRARY="${BASEDIR}/prebuilt/android-$(get_target_build)/giflib/lib" \
    -DJPEG_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/jpeg/include" \
    -DJPEG_LIBRARY="${BASEDIR}/prebuilt/android-$(get_target_build)/jpeg/lib" \
    -DPNG_PNG_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/libpng/include" \
    -DPNG_LIBRARY="${BASEDIR}/prebuilt/android-$(get_target_build)/libpng/lib" \
    -DTIFF_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/tiff/include" \
    -DTIFF_LIBRARY="${BASEDIR}/prebuilt/android-$(get_target_build)/tiff/lib" \
    -DZLIB_INCLUDE_DIR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr/include" \
    -DZLIB_LIBRARY="${ANDROID_NDK_ROOT}/platform/android-${API}/arch-$(get_target_build)/usr/lib" \
    -DCMAKE_SYSTEM_PROCESSOR=$(get_target_build) \
    -DBUILD_SHARED_LIBS=0 .. || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_libwebp_package_config "1.0.0"

make install || exit 1
