#!/bin/bash

if [[ -z ${ARCH} ]]; then
    echo -e "(*) ARCH not defined\n"
    exit 1
fi

if [[ -z ${IOS_MIN_VERSION} ]]; then
    echo -e "(*) IOS_MIN_VERSION not defined\n"
    exit 1
fi

if [[ -z ${TARGET_SDK} ]]; then
    echo -e "(*) TARGET_SDK not defined\n"
    exit 1
fi

if [[ -z ${SDK_PATH} ]]; then
    echo -e "(*) SDK_PATH not defined\n"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo -e "(*) BASEDIR not defined\n"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/ios-common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
LIB_NAME="libwebp"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
CFLAGS=$(get_cflags ${LIB_NAME})
CXXFLAGS=$(get_cxxflags ${LIB_NAME})
LDFLAGS=$(get_ldflags ${LIB_NAME})

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

if [ -d "build" ]; then
    rm -rf build
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
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/ios-$(get_target_host)/${LIB_NAME}" \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_LINKER="$LD" \
    -DCMAKE_AR="$AR" \
    -DCMAKE_AS="$AS" \
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

make ${MOBILE_FFMPEG_DEBUG} -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_libwebp_package_config "1.0.0"

make install || exit 1
