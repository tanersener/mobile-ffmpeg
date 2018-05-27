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
COMMON_CFLAGS=$(get_cflags "gnutls")
COMMON_CXXFLAGS=$(get_cxxflags "gnutls")
COMMON_LDFLAGS=$(get_ldflags "gnutls")

export CFLAGS="${COMMON_CFLAGS} -I${BASEDIR}/prebuilt/ios-$(get_target_host)/libiconv/include -I${BASEDIR}/prebuilt/ios-$(get_target_host)/gmp/include"
export CXXFLAGS="${COMMON_CXXFLAGS}"
export LDFLAGS="${COMMON_LDFLAGS} -L${BASEDIR}/prebuilt/ios-$(get_target_host)/libiconv/lib -L${BASEDIR}/prebuilt/ios-$(get_target_host)/gmp/lib"
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

HARDWARE_ACCELERATION=""
case ${ARCH} in
    arm64)

        # HARDWARE ACCELERATION IS DISABLED DUE TO THE FOLLOWING ERROR
        #
        #   elf/sha1-armv8.s:1270:1: error: unknown directive
        #  .inst 0x5e280803
        HARDWARE_ACCELERATION="--disable-hardware-acceleration"
    ;;
    *)
        HARDWARE_ACCELERATION="--enable-hardware-acceleration"
    ;;
esac

cd ${BASEDIR}/src/gnutls || exit 1

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_gnutls} -eq 1 ]]; then
    autoreconf --force --install
fi

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/gnutls \
    --with-pic \
    --with-sysroot=${SDK_PATH} \
    --with-included-libtasn1 \
    --with-included-unistring \
    --without-idn \
    --without-libidn2 \
    --without-p11-kit \
    --enable-openssl-compatibility \
    ${HARDWARE_ACCELERATION} \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-code-coverage \
    --disable-doc \
    --disable-manpages \
    --disable-tests \
    --disable-tools \
    --disable-maintainer-mode \
    --host=${TARGET_HOST} || exit 1

make -j$(get_cpu_count) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_gnutls_package_config "3.5.18"

make install || exit 1
