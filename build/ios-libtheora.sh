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
export CFLAGS=$(get_cflags "libtheora")
export CXXFLAGS=$(get_cxxflags "libtheora")
export LDFLAGS=$(get_ldflags "libtheora")
export PKG_CONFIG_PATH=${INSTALL_PKG_CONFIG_DIR}

cd ${BASEDIR}/src/libtheora || exit 1

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_libtheora} -eq 1 ]]; then
    autoreconf --force --install
fi

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libtheora \
    --with-pic \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-spec \
    --disable-examples \
    --disable-telemetry \
    --enable-valgrind-testing \
    --host=${TARGET_HOST} || exit 1

make -j$(get_cpu_count) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp theoradec.pc ${INSTALL_PKG_CONFIG_DIR}
cp theoraenc.pc ${INSTALL_PKG_CONFIG_DIR}
cp theora.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
