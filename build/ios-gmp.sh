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
LIB_NAME="gmp"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
case ${ARCH} in
    i386)
        TARGET_HOST="x86-apple-darwin"
    ;;
    *)
        TARGET_HOST=$(get_target_host)
    ;;
esac
export CFLAGS=$(get_cflags ${LIB_NAME})
export CXXFLAGS=$(get_cxxflags ${LIB_NAME})
export LDFLAGS=$(get_ldflags ${LIB_NAME})

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_gmp} -eq 1 ]]; then
    autoreconf_library ${LIB_NAME}
fi

ASM_FLAGS=""
case ${ARCH} in
    armv7 | armv7s | arm64)
        ASM_FLAGS="--disable-assembly"
    ;;
    i386 | x86-64)
        ASM_FLAGS="--enable-assembly"
    ;;
esac

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/${LIB_NAME} \
    --with-pic \
    --with-sysroot=${SDK_PATH} \
    --enable-static \
    --disable-shared \
    ${ASM_FLAGS} \
    --disable-fast-install \
    --disable-maintainer-mode \
    --host=${TARGET_HOST} || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_gmp_package_config "6.1.2"

make install || exit 1
