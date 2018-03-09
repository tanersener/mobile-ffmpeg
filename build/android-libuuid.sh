#!/bin/bash

create_uuid_package_config() {
    local UUID_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/uuid.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libuuid
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: uuid
Description: Universally unique id library
Version: ${UUID_VERSION}
Requires:
Cflags: -I\${includedir}
Libs: -L\${libdir} -luuid
EOF
}

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

if [[ -z ${ARCH//-/_} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo "API not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
android_prepare_toolchain_paths

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
export CFLAGS=$(android_get_cflags "libuuid")
export CXXFLAGS=$(android_get_cxxflags "libuuid")
export LDFLAGS=$(android_get_ldflags "libuuid")

cd $1/src/libuuid || exit 1

make distclean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libuuid \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH//-/_}/sysroot \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# AUTO-GENERATED PKG-CONFIG FILE IS WRONG. CREATING IT MANUALLY
create_uuid_package_config "1.0.3"

make install || exit 1
