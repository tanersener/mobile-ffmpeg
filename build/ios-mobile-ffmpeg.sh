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

export CFLAGS="${COMMON_CFLAGS} -I${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/ffmpeg/include"
export CXXFLAGS=$(get_cxxflags ${LIB_NAME})
export LDFLAGS="${COMMON_LDFLAGS} -L${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/ffmpeg/lib -framework Foundation -framework CoreVideo -lavdevice"
export PKG_CONFIG_LIBDIR="${INSTALL_PKG_CONFIG_DIR}"

# BUILD SHARED (DEFAULT) OR STATIC LIBRARIES
if [[ -z ${MOBILE_FFMPEG_STATIC} ]]; then
    BUILD_LIBRARY_OPTIONS="--enable-shared --disable-static";
else
    BUILD_LIBRARY_OPTIONS="--enable-static --disable-shared";
fi

cd ${BASEDIR}/ios || exit 1

echo -n -e "\n${LIB_NAME}: "

make distclean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_mobile_ffmpeg} -eq 1 ]]; then
    autoreconf_library ${LIB_NAME}
fi

VIDEOTOOLBOX_SUPPORT_FLAG=""
if [[ ${46} -eq 1 ]]; then
    VIDEOTOOLBOX_SUPPORT_FLAG="--enable-videotoolbox"
fi

# REMOVING OPTIONS FROM CONFIGURE TO FIX THE FOLLOWING ERROR
# ld: -flat_namespace and -bitcode_bundle (Xcode setting ENABLE_BITCODE=YES) cannot be used together
${SED_INLINE} 's/$wl-flat_namespace //g' configure
${SED_INLINE} 's/$wl-undefined //g' configure
${SED_INLINE} 's/${wl}suppress//g' configure

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/${LIB_NAME} \
    --with-pic \
    --with-sysroot=${SDK_PATH} \
    ${BUILD_LIBRARY_OPTIONS} \
    ${VIDEOTOOLBOX_SUPPORT_FLAG} \
    --disable-fast-install \
    --disable-maintainer-mode \
    --host=${TARGET_HOST} 1>>${BASEDIR}/build.log 2>&1

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make -j$(get_cpu_count) 1>>${BASEDIR}/build.log 2>&1

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make install 1>>${BASEDIR}/build.log 2>&1

if [ $? -eq 0 ]; then
    echo "ok"
else
    echo "failed"
    exit 1
fi
