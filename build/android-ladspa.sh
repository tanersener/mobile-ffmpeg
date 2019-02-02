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
LIB_NAME="ladspa"
set_toolchain_clang_paths ${LIB_NAME}

# PREPARING FLAGS
export CFLAGS=$(get_cflags ${LIB_NAME})
export CXXFLAGS=$(get_cxxflags ${LIB_NAME})
export LDFLAGS=$(get_ldflags ${LIB_NAME})
export PACKAGE_DIR=${BASEDIR}/prebuilt/android-$(get_target_build)/${LIB_NAME}

cd ${BASEDIR}/src/${LIB_NAME}/src || exit 1

make clean 2>/dev/null 1>/dev/null

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_wavpack} -eq 1 ]]; then
    autoreconf_library ${LIB_NAME}
fi

make -j$(get_cpu_count) || exit 1

create_ladspa_package_config "1.15"

make install || exit 1
