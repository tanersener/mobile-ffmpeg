#!/bin/bash

if [[ -z ${ARCH} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${IOS_MIN_VERSION} ]]; then
    echo "IOS_MIN_VERSION not defined"
    exit 1
fi

if [[ -z ${TARGET_SDK} ]]; then
    echo "TARGET_SDK not defined"
    exit 1
fi

if [[ -z ${SDK_PATH} ]]; then
    echo "SDK_PATH not defined"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo "BASEDIR not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/ios-common.sh

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
    -DCMAKE_SYSROOT="${SDK_PATH}" \
    -DCMAKE_FIND_ROOT_PATH="${SDK_PATH}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/ios-$(get_target_host)/libwebp" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_LINKER="$LD" \
    -DCMAKE_AR="$AR" \
    -DGIF_INCLUDE_DIR="${BASEDIR}/prebuilt/ios-$(get_target_host)/giflib/include" \
    -DGIF_LIBRARY="${BASEDIR}/prebuilt/ios-$(get_target_host)/giflib/lib" \
    -DJPEG_INCLUDE_DIR="${BASEDIR}/prebuilt/ios-$(get_target_host)/jpeg/include" \
    -DJPEG_LIBRARY="${BASEDIR}/prebuilt/ios-$(get_target_host)/jpeg/lib" \
    -DPNG_PNG_INCLUDE_DIR="${BASEDIR}/prebuilt/ios-$(get_target_host)/libpng/include" \
    -DPNG_LIBRARY="${BASEDIR}/prebuilt/ios-$(get_target_host)/libpng/lib" \
    -DTIFF_INCLUDE_DIR="${BASEDIR}/prebuilt/ios-$(get_target_host)/tiff/include" \
    -DTIFF_LIBRARY="${BASEDIR}/prebuilt/ios-$(get_target_host)/tiff/lib" \
    -DZLIB_INCLUDE_DIR="${SDK_PATH}/usr/include" \
    -DZLIB_LIBRARY="${SDK_PATH}/usr/lib" \
    -DCMAKE_SYSTEM_PROCESSOR=$(get_target_arch) \
    -DBUILD_SHARED_LIBS=0 .. || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_libwebp_package_config "0.6.1"

make install || exit 1
