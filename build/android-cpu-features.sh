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

LIB_NAME="cpu-features"
set_toolchain_clang_paths ${LIB_NAME}

$(android_ndk_cmake) -DBUILD_PIC=ON || exit 1
make -C $(android_build_dir) install || exit 1

create_cpufeatures_package_config