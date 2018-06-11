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
LIB_NAME="mobile-ffmpeg"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
COMMON_CFLAGS=$(get_cflags ${LIB_NAME})
COMMON_LDFLAGS=$(get_ldflags ${LIB_NAME})

export CFLAGS="${COMMON_CFLAGS} -I${BASEDIR}/prebuilt/ios-$(get_target_host)/ffmpeg/include"
export CXXFLAGS=$(get_cxxflags ${LIB_NAME})
export LDFLAGS="${COMMON_LDFLAGS} -L${BASEDIR}/prebuilt/ios-$(get_target_host)/ffmpeg/lib"
export PKG_CONFIG_LIBDIR="${INSTALL_PKG_CONFIG_DIR}"

cd ${BASEDIR}/ios || exit 1

echo -n -e "\n${LIB_NAME}: "

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_mobile_ffmpeg} -eq 1 ]]; then
    autoreconf_library ${LIB_NAME}
fi

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/${LIB_NAME} \
    --with-pic \
    --with-sysroot=${SDK_PATH} \
    --enable-shared \
    --disable-static \
    --disable-fast-install \
    --disable-maintainer-mode \
    --host=${TARGET_HOST} 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make -j$(get_cpu_count) 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make install 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -eq 0 ]; then
    echo "ok"
else
    echo "failed"
    exit 1
fi
