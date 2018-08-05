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
LIB_NAME="x265"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
CFLAGS=$(get_cflags ${LIB_NAME})
CXXFLAGS=$(get_cxxflags ${LIB_NAME})
LDFLAGS=$(get_ldflags ${LIB_NAME})

# USE CLEAN SOURCE ON EACH BUILD
cd ${BASEDIR}/src || exit 1
rm -rf ${LIB_NAME} || exit 1
DOWNLOAD_RESULT=$(download_gpl_library_source ${LIB_NAME})
if [[ ${DOWNLOAD_RESULT} -ne 0 ]]; then
    exit 1
fi
cd ${BASEDIR}/src/${LIB_NAME} || exit 1

ASM_OPTIONS=""
case ${ARCH} in
    arm-v7a | arm-v7a-neon | arm64-v8a)
        ASM_OPTIONS="-DENABLE_ASSEMBLY=1 -DCROSS_COMPILE_ARM=1 -DSSE2_FOUND=0 -DSSE3_FOUND=0"
    ;;
    x86)
        ASM_OPTIONS="-DENABLE_ASSEMBLY=1 -DCROSS_COMPILE_ARM=0 -DSSE2_FOUND=1 -DSSE3_FOUND=1"
    ;;
    x86-64)
        ASM_OPTIONS="-DENABLE_ASSEMBLY=1 -DCROSS_COMPILE_ARM=0 -DSSE2_FOUND=1 -DSSE3_FOUND=1"
    ;;
esac

if [ -d "cmake-build" ]; then
    rm -rf cmake-build
fi

mkdir cmake-build || exit 1
cd cmake-build || exit 1

# fix pointer array assignments
${SED} 's/parseCpuName(value, bError)/parseCpuName(value, bError, 0)/g' ${BASEDIR}/src/x265/source/common/param.cpp
${SED} '/addAvg/s/ p.pu/ *p.pu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/convert_p2s/s/ p.pu/ *p.pu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/pixelavg_pp/s/ p.pu/ *p.pu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/addAvg/s/ p.chroma/ *p.chroma/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/add_ps/s/ p.chroma/ *p.chroma/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/add_ps/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/blockfill_s/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/ssd_s/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/calcresidual/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
${SED} '/scale1D_128to64_neon/s/ p.scale/ *p.scale/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp

# fixing constant shift
${SED} 's/lsr 16/lsr #16/g' ${BASEDIR}/src/x265/source/common/arm/blockcopy8.S

cmake -Wno-dev \
    -DCMAKE_VERBOSE_MAKEFILE=0 \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot" \
    -DCMAKE_FIND_ROOT_PATH="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME}" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$CC" \
    -DCMAKE_CXX_COMPILER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$CXX" \
    -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$LD" \
    -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$AR" \
    -DCMAKE_AS="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin/$AS" \
    -DSTATIC_LINK_CRT=1 \
    -DENABLE_PIC=1 \
    -DENABLE_CLI=0 \
    ${ASM_OPTIONS} \
    -DCMAKE_SYSTEM_PROCESSOR=$(get_cmake_target_processor) \
    -DENABLE_SHARED=0 ../source || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_x265_package_config "2.8"

make install || exit 1
