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
export CFLAGS=$(get_cflags "libxml2")
export CXXFLAGS=$(get_cxxflags "libxml2")
export LDFLAGS=$(get_ldflags "libxml2")
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

cd ${BASEDIR}/src/libxml2 || exit 1

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURE ALWAYS
autoreconf --force --install || exit 1

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libxml2 \
    --with-pic \
    --with-sysroot=${SDK_PATH} \
    --with-zlib \
    --with-iconv=${BASEDIR}/prebuilt/ios-$(get_target_host)/libiconv \
    --with-sax1 \
    --without-python \
    --without-debug \
    --without-lzma \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --host=${TARGET_HOST} || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_libxml2_package_config "2.9.8"

make install || exit 1
