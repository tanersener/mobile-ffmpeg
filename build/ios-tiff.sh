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
export CFLAGS=$(get_cflags "tiff")
export CXXFLAGS=$(get_cxxflags "tiff")
export LDFLAGS=$(get_ldflags "tiff")

echo "LDFLAGS is $LDFLAGS"

cd ${BASEDIR}/src/tiff || exit 1

make distclean 2>/dev/null 1>/dev/null

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/tiff \
    --with-pic \
    --with-sysroot=${SDK_PATH} \
    --with-jpeg-include-dir=${BASEDIR}/prebuilt/ios-$(get_target_host)/jpeg/include \
    --with-jpeg-lib-dir=${BASEDIR}/prebuilt/ios-$(get_target_host)/jpeg/lib \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-maintainer-mode \
    --disable-cxx \
    --disable-win32-io \
    --disable-lzma \
    --host=${TARGET_HOST} || exit 1

make -j$(get_cpu_count) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
