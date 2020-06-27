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
LIB_NAME="sdl"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
export CFLAGS=$(get_cflags ${LIB_NAME})
export CXXFLAGS=$(get_cxxflags ${LIB_NAME})
export LDFLAGS=$(get_ldflags ${LIB_NAME})
export PKG_CONFIG_LIBDIR="${INSTALL_PKG_CONFIG_DIR}"

SIMD_OPTIONS=""
case ${ARCH} in
    arm-v7a)
        SIMD_OPTIONS="-DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a -DCMAKE_ANDROID_ARM_MODE=ON -DCMAKE_ANDROID_ARM_NEON=OFF -DCMAKE_SYSTEM_PROCESSOR=armv7-a"
    ;;
    arm-v7a-neon)
        SIMD_OPTIONS="-DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a -DCMAKE_ANDROID_ARM_MODE=ON -DCMAKE_ANDROID_ARM_NEON=ON -DCMAKE_SYSTEM_PROCESSOR=armv7-a"
    ;;
    arm64-v8a)
        SIMD_OPTIONS="-DCMAKE_ANDROID_ARCH_ABI=$(get_android_arch 2) -DCMAKE_SYSTEM_PROCESSOR=$(get_cmake_target_processor)"
    ;;
    x86)
        SIMD_OPTIONS="-DCMAKE_ANDROID_ARCH_ABI=$(get_android_arch 3) -DCMAKE_SYSTEM_PROCESSOR=i686"
    ;;
    x86-64)
        SIMD_OPTIONS="-DCMAKE_ANDROID_ARCH_ABI=$(get_android_arch 4) -DCMAKE_SYSTEM_PROCESSOR=$(get_cmake_target_processor)"
    ;;
esac

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

if [ -d "build" ]; then
    rm -rf build
fi

mkdir build;
cd build

# USE OUR OWN IMPLEMENTATION
rm -f ${BASEDIR}/src/${LIB_NAME}/src/core/android/SDL_android.c
cp ${BASEDIR}/tools/make/sdl/SDL_android.c ${BASEDIR}/src/${LIB_NAME}/src/core/android/SDL_android.c

cmake -Wno-dev \
    -DCMAKE_VERBOSE_MAKEFILE=0 \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SYSROOT="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot" \
    -DCMAKE_FIND_ROOT_PATH="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/${TARGET_HOST}/lib;${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr/lib/${TARGET_HOST}/${API};${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/lib" \
    -DCMAKE_ANDROID_NDK=${ANDROID_NDK_ROOT} \
    -DCMAKE_SYSTEM_LIBRARY_PATH="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/${TARGET_HOST}/lib;${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr/lib/${TARGET_HOST}/${API};${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/lib" \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_ANDROID_API="${API}" \
    -DCMAKE_ANDROID_NDK="${ANDROID_NDK_ROOT}" \
    ${SIMD_OPTIONS} \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME}" \
    -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$LD" \
    -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$AR" \
    -DCMAKE_AS="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$AS" \
    -DCMAKE_RANLIB="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$RANLIB" \
    -DSDL_SHARED=0 \
    -DSDL_STATIC=1 \
    -DSDL_STATIC_PIC=1 .. || exit 1

make -j$(get_cpu_count) || exit 1

# WORKAROUND FOR LINKERS THAT CAN NOT USE FULL PATHS i.e. arm64-v8a LINKER
${SED_INLINE} "s/\.so / /g" sdl2.pc
${SED_INLINE} "s/${ANDROID_NDK_ROOT//\//\\/}\/toolchains\/llvm\/prebuilt\/${TOOLCHAIN//\//\\/}\/${TARGET_HOST//\//\\/}\/lib\/lib//g" sdl2.pc
${SED_INLINE} "s/${ANDROID_NDK_ROOT//\//\\/}\/toolchains\/llvm\/prebuilt\/${TOOLCHAIN//\//\\/}\/sysroot\/usr\/lib\/${TARGET_HOST//\//\\/}\/${API}\/lib//g" sdl2.pc
${SED_INLINE} "s/${ANDROID_NDK_ROOT//\//\\/}\/toolchains\/llvm\/prebuilt\/${TOOLCHAIN//\//\\/}\/lib\/lib//g" sdl2.pc

# MANUALLY COPY PKG-CONFIG FILES
cp *.pc ${INSTALL_PKG_CONFIG_DIR} || exit 1

make install || exit 1
