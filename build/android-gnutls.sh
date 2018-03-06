#!/bin/bash

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo "API not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS
android_prepare_toolchain_paths

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
COMMON_CFLAGS=$(android_get_cflags "gnutls")
COMMON_CXXFLAGS=$(android_get_cxxflags "gnutls")
COMMON_LDFLAGS=$(android_get_ldflags "gnutls")

export CFLAGS="${COMMON_CFLAGS} -I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/include -I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/gmp/include"
export CXXFLAGS="${COMMON_CXXFLAGS}"
export LDFLAGS="${COMMON_LDFLAGS} -L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/lib -L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/gmp/lib"
export PKG_CONFIG_PATH="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/pkgconfig"

cd $1/src/gnutls || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/gnutls \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --with-included-libtasn1 \
    --with-included-unistring \
    --without-idn \
    --without-libidn2 \
    --without-p11-kit \
    --enable-openssl-compatibility \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-code-coverage \
    --disable-doc \
    --disable-manpages \
    --disable-tests \
    --disable-tools \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./lib/gnutls.pc ${INSTALL_PKG_CONFIG_DIR}
cp ./libdane/gnutls-dane.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
