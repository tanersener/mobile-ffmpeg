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
LIB_NAME="gnutls"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
COMMON_CFLAGS=$(get_cflags ${LIB_NAME})
COMMON_CXXFLAGS=$(get_cxxflags ${LIB_NAME})
COMMON_LDFLAGS=$(get_ldflags ${LIB_NAME})

export CFLAGS="${COMMON_CFLAGS} -I${BASEDIR}/prebuilt/ios-$(get_target_host)/libiconv/include"
export CXXFLAGS="${COMMON_CXXFLAGS}"
export LDFLAGS="${COMMON_LDFLAGS} -L${BASEDIR}/prebuilt/ios-$(get_target_host)/libiconv/lib"

export NETTLE_CFLAGS="-I${BASEDIR}/prebuilt/ios-$(get_target_host)/nettle/include"
export NETTLE_LIBS="-L${BASEDIR}/prebuilt/ios-$(get_target_host)/nettle/lib"
export HOGWEED_CFLAGS="-I${BASEDIR}/prebuilt/ios-$(get_target_host)/nettle/include"
export HOGWEED_LIBS="-L${BASEDIR}/prebuilt/ios-$(get_target_host)/nettle/lib"
export GMP_CFLAGS="-I${BASEDIR}/prebuilt/ios-$(get_target_host)/gmp/include"
export GMP_LIBS="-L${BASEDIR}/prebuilt/ios-$(get_target_host)/gmp/lib"

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

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_gnutls} -eq 1 ]]; then
    autoreconf_library ${LIB_NAME}
fi

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/${LIB_NAME} \
    --with-pic \
    --with-sysroot=${SDK_PATH} \
    --with-included-libtasn1 \
    --with-included-unistring \
    --without-idn \
    --without-libidn2 \
    --without-p11-kit \
    ${HARDWARE_ACCELERATION} \
    --enable-static \
    --disable-openssl-compatibility \
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
create_gnutls_package_config "3.5.19"

make install || exit 1
