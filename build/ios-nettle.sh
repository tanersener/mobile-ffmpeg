#!/bin/bash

if [[ -z ${ARCH} ]]; then
    echo -e "(*) ARCH not defined\n"
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
if [[ ${APPLE_TVOS_BUILD} -eq 1 ]]; then
    . ${BASEDIR}/build/tvos-common.sh
else
    . ${BASEDIR}/build/ios-common.sh
fi

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
LIB_NAME="nettle"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
export CFLAGS=$(get_cflags ${LIB_NAME})
export CXXFLAGS=$(get_cxxflags ${LIB_NAME})
export LDFLAGS=$(get_ldflags ${LIB_NAME})

OPTIONAL_CPU_SUPPORT=""
case ${ARCH} in
    armv7 | armv7s | arm64 | arm64e)
        OPTIONAL_CPU_SUPPORT="--enable-arm-neon"
    ;;
    i386 | x86-64)
        OPTIONAL_CPU_SUPPORT="--enable-x86-aesni"
    ;;
esac

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_nettle} -eq 1 ]]; then
    autoreconf_library ${LIB_NAME}
fi

./configure \
    --prefix=${BASEDIR}/prebuilt/$(get_target_build_directory)/${LIB_NAME} \
    --enable-pic \
    --enable-static \
    --with-include-path=${BASEDIR}/prebuilt/$(get_target_build_directory)/gmp/include \
    --with-lib-path=${BASEDIR}/prebuilt/$(get_target_build_directory)/gmp/lib \
    --disable-shared \
    --disable-mini-gmp \
    --disable-assembler \
    --disable-openssl \
    --disable-gcov \
    --disable-documentation \
    ${OPTIONAL_CPU_SUPPORT} \
    --host=${TARGET_HOST} || exit 1

make -j$(get_cpu_count) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
