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
LIB_NAME="x265"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
CFLAGS=$(get_cflags ${LIB_NAME})
CXXFLAGS=$(get_cxxflags ${LIB_NAME})
LDFLAGS=$(get_ldflags ${LIB_NAME})

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

if [ -d "cmake-build" ]; then
    rm -rf cmake-build
fi

mkdir cmake-build || exit 1
cd cmake-build || exit 1

# fix pointer array assignments
sed -i .tmp 's/parseCpuName(value, bError)/parseCpuName(value, bError, 0)/g' ${BASEDIR}/src/x265/source/common/param.cpp
sed -i .tmp '/addAvg/s/ p.pu/ *p.pu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/convert_p2s/s/ p.pu/ *p.pu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/pixelavg_pp/s/ p.pu/ *p.pu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/addAvg/s/ p.chroma/ *p.chroma/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/add_ps/s/ p.chroma/ *p.chroma/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/add_ps/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/blockfill_s/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/ssd_s/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/calcresidual/s/ p.cu/ *p.cu/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp
sed -i .tmp '/scale1D_128to64_neon/s/ p.scale/ *p.scale/g' ${BASEDIR}/src/x265/source/common/arm/asm-primitives.cpp

# fixing constant shift
sed -i .tmp 's/lsr 16/lsr #16/g' ${BASEDIR}/src/x265/source/common/arm/blockcopy8.S

rm -f ${BASEDIR}/src/${LIB_NAME}/source/CMakeLists.txt || exit 1
cp ${BASEDIR}/tools/cmake/CMakeLists.x265.ios.txt ${BASEDIR}/src/${LIB_NAME}/source/CMakeLists.txt || exit 1

cmake -Wno-dev \
    -DCMAKE_VERBOSE_MAKEFILE=1 \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SYSROOT="${SDK_PATH}" \
    -DCMAKE_FIND_ROOT_PATH="${SDK_PATH}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/ios-$(get_target_host)/${LIB_NAME}" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_LINKER="$LD" \
    -DCMAKE_AR="$AR" \
    -DCMAKE_AS="$AS" \
    -DSTATIC_LINK_CRT=1 \
    -DENABLE_PIC=1 \
    -DENABLE_ASSEMBLY=1 \
    -DCROSS_COMPILE_ARM=1 \
    -DENABLE_CLI=0 \
    -DSSE2_FOUND=0 \
    -DSSE3_FOUND=0 \
    -DSSE3_FOUND=0 \
    -DCMAKE_SYSTEM_PROCESSOR=$(get_target_arch) \
    -DENABLE_SHARED=0 ../source || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
# create_libwebp_package_config "1.0.0"

make install || exit 1
