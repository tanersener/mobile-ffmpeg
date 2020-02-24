#!/bin/bash

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo -e "(*) ANDROID_NDK_ROOT not defined\n"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo -e "(*) ARCH not defined\n"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo -e "(*) API not defined\n"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo -e "(*) BASEDIR not defined\n"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
LIB_NAME="libwebp"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
BUILD_HOST=$(get_build_host)
CFLAGS=$(get_cflags ${LIB_NAME})
CXXFLAGS=$(get_cxxflags ${LIB_NAME})
LDFLAGS=$(get_ldflags ${LIB_NAME})

ARCH_OPTIONS=""
case ${ARCH} in
    arm-v7a)
        ARCH_OPTIONS="-DWEBP_ENABLE_SIMD=OFF"
    ;;
    *)
        ARCH_OPTIONS="-DWEBP_ENABLE_SIMD=ON"
    ;;
esac

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

if [ -d "build" ]; then
    rm -rf build
fi

mkdir build;
cd build

# OVERRIDING INCLUDE PATH ORDER
CFLAGS="-I${BASEDIR}/prebuilt/android-$(get_target_build)/giflib/include \
-I${BASEDIR}/prebuilt/android-$(get_target_build)/jpeg/include \
-I${BASEDIR}/prebuilt/android-$(get_target_build)/libpng/include \
-I${BASEDIR}/prebuilt/android-$(get_target_build)/tiff/include $CFLAGS"

cmake -Wno-dev \
    -DCMAKE_VERBOSE_MAKEFILE=0 \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot" \
    -DCMAKE_FIND_ROOT_PATH="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME}" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$CC" \
    -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$LD" \
    -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$AR" \
    -DCMAKE_AS="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$AS" \
    -DGIF_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/giflib/include" \
    -DJPEG_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/jpeg/include" \
    -DJPEG_LIBRARY="${BASEDIR}/prebuilt/android-$(get_target_build)/jpeg/lib" \
    -DPNG_PNG_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/libpng/include" \
    -DPNG_LIBRARY="${BASEDIR}/prebuilt/android-$(get_target_build)/libpng/lib" \
    -DTIFF_INCLUDE_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/tiff/include" \
    -DTIFF_LIBRARY="${BASEDIR}/prebuilt/android-$(get_target_build)/tiff/lib" \
    -DZLIB_INCLUDE_DIR="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr/include" \
    -DZLIB_LIBRARY="${ANDROID_NDK_ROOT}/platform/android-${API}/arch-$(get_target_build)/usr/lib" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=1 \
    -DGLUT_INCLUDE_DIR= \
    -DGLUT_cocoa_LIBRARY= \
    -DGLUT_glut_LIBRARY= \
    -DOPENGL_INCLUDE_DIR= \
    -DSDLMAIN_LIBRARY= \
    -DSDL_INCLUDE_DIR= \
    -DWEBP_BUILD_CWEBP=0 \
    -DWEBP_BUILD_DWEBP=0 \
    -DWEBP_BUILD_EXTRAS=0 \
    -DWEBP_BUILD_GIF2WEBP=0 \
    -DWEBP_BUILD_IMG2WEBP=0 \
    -DWEBP_BUILD_WEBPMUX=0 \
    -DWEBP_BUILD_WEBPINFO=0 \
    ${ARCH_OPTIONS} \
    -DCMAKE_SYSTEM_PROCESSOR=$(get_cmake_target_processor) \
    -DBUILD_SHARED_LIBS=0 .. || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_libwebp_package_config "1.1.0"

make install || exit 1
