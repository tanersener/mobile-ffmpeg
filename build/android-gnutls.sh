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
LIB_NAME="gnutls"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
COMMON_CFLAGS=$(get_cflags ${LIB_NAME})
COMMON_CXXFLAGS=$(get_cxxflags ${LIB_NAME})
COMMON_LDFLAGS=$(get_ldflags ${LIB_NAME})

export CFLAGS="${COMMON_CFLAGS} -I${BASEDIR}/prebuilt/android-$(get_target_build)/libiconv/include"
export CXXFLAGS="${COMMON_CXXFLAGS}"
export LDFLAGS="${COMMON_LDFLAGS} -L${BASEDIR}/prebuilt/android-$(get_target_build)/libiconv/lib"

export NETTLE_CFLAGS="-I${BASEDIR}/prebuilt/android-$(get_target_build)/nettle/include"
export NETTLE_LIBS="-L${BASEDIR}/prebuilt/android-$(get_target_build)/nettle/lib"
export HOGWEED_CFLAGS="-I${BASEDIR}/prebuilt/android-$(get_target_build)/nettle/include"
export HOGWEED_LIBS="-L${BASEDIR}/prebuilt/android-$(get_target_build)/nettle/lib"
export GMP_CFLAGS="-I${BASEDIR}/prebuilt/android-$(get_target_build)/gmp/include"
export GMP_LIBS="-L${BASEDIR}/prebuilt/android-$(get_target_build)/gmp/lib"

cd ${BASEDIR}/src/${LIB_NAME} || exit 1

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_gnutls} -eq 1 ]]; then
    autoreconf_library ${LIB_NAME}
fi

./configure \
    --prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME} \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot \
    --with-included-libtasn1 \
    --with-included-unistring \
    --without-idn \
    --without-libidn2 \
    --without-p11-kit \
    --enable-openssl-compatibility \
    --enable-hardware-acceleration \
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
