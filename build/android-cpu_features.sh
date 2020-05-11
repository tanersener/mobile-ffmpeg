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

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

# PREPARE PATHS & DEFINE ${INSTALL_PKG_CONFIG_DIR}
LIB_NAME="cpu_features"
set_toolchain_clang_paths ${LIB_NAME}

PREFIX=${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME}

android_cmake() {
  local cmake=$(which cmake)
if [[ -z ${cmake} ]]; then
  cmake=$(find ${ANDROID_HOME}/cmake -path \*/bin/cmake -type f -print -quit)
fi
if [[ -z ${cmake} ]]; then
    echo -e "(*) cmake not found\n"
    exit 1
fi
echo ${cmake}
}

create_cpu_features_package_config() {
  cat > "${INSTALL_PKG_CONFIG_DIR}/${LIB_NAME}.pc" << EOF
prefix=${PREFIX}
exec_prefix=\${prefix}/bin
libdir=\${prefix}/lib
includedir=\${prefix}/include/ndk_compat

Name: cpufeatures
URL: https://github.com/google/cpu_features
Description: cpu_features Android compatibility library
Version: 1.${API}

Requires:
Libs: -L\${libdir} -lndk_compat
Cflags: -I\${includedir}
EOF
}

$(android_cmake) \
  -B${BASEDIR}/android/build/${LIB_NAME}/$(get_target_build) \
  -H${BASEDIR}/src/cpu_features \
  -DCMAKE_VERBOSE_MAKEFILE=0 \
  -DANDROID_ABI=$(get_cmake_android_abi) \
  -DANDROID_PLATFORM=android-${API} \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DBUILD_SHARED_LIBS=ON || exit 1

make -C ${BASEDIR}/android/build/cpu_features/$(get_target_build) install

create_cpu_features_package_config