#!/bin/bash

android_get_target_host() {
    case ${ARCH} in
        arm)
            echo "arm-linux-androideabi"
        ;;
        arm64)
            echo "aarch64-linux-android"
        ;;
        x86)
            echo "i686-linux-android"
        ;;
        x86_64)
            echo "x86_64-linux-android"
        ;;
    esac
}

android_get_common_includes() {
    echo "-I${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot/usr/include -I${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot/usr/local/include"
}

android_get_common_cflags() {
    echo "-fstrict-aliasing -fPIE -fPIC -DANDROID -D__ANDROID_API__=${API}"
}

android_get_arch_specific_cflags() {
    case ${ARCH} in
        arm)
            echo "-march=armv7-a -mfpu=neon -mfloat-abi=softfp"
        ;;
        arm64)
            echo "-march=armv8-a -mfpu=neon -mfloat-abi=softfp"
        ;;
        x86)
            echo "-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32"
        ;;
        x86_64)
            echo "-march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel"
        ;;
    esac
}

android_get_size_optimization_cflags() {

    ARCH_OPTIMIZATION=""
    case ${ARCH} in
        arm | arm64)
            if [[ $1 -eq libwebp ]]; then
                ARCH_OPTIMIZATION="-Os"
            else
                ARCH_OPTIMIZATION="-Os -finline-limit=64"
            fi
        ;;
        x86 | x86_64)
            ARCH_OPTIMIZATION="-O2 -finline-limit=300"
        ;;
    esac

    LIB_OPTIMIZATION=""

    echo "${ARCH_OPTIMIZATION} ${LIB_OPTIMIZATION}"
}

android_get_app_specific_cflags() {

    APP_FLAGS=""
    case $1 in
        libwebp | openssl)
            APP_FLAGS=""
        ;;
        ffmpeg | shine)
            APP_FLAGS="-Wno-psabi -Wno-unused-but-set-variable -Wno-unused-function"
        ;;
        tiff)
            APP_FLAGS="-std=c99"
        ;;
        *)
            APP_FLAGS="-std=c99 -Wno-psabi -Wno-unused-but-set-variable -Wno-unused-function"
        ;;
    esac

    echo "${APP_FLAGS}"
}

android_get_cflags() {
    ARCH_FLAGS=$(android_get_arch_specific_cflags);
    APP_FLAGS=$(android_get_app_specific_cflags $1);
    COMMON_FLAGS=$(android_get_common_cflags);
    OPTIMIZATION_FLAGS=$(android_get_size_optimization_cflags $1);
    COMMON_INCLUDES=$(android_get_common_includes);

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${COMMON_INCLUDES}"
}

android_get_cxxflags() {
    case $1 in
        gnutls)
            echo "-nostdi -std=c++11 -fno-exceptions -fno-rtti"
        ;;
        opencore-amr)
            echo ""
        ;;
        *)
            echo "-std=c++11 -fno-exceptions -fno-rtti"
        ;;
    esac
}

android_get_common_linked_libraries() {
    echo "-lc -lm -ldl -llog -pie -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot/usr/lib -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/lib -L${ANDROID_NDK_ROOT}/sources/cxx-stl/llvm-libc++/lib"
}

android_get_size_optimization_ldflags() {
    echo "-Wl,--gc-sections,--icf=safe"
}

android_get_arch_specific_ldflags() {
    case ${ARCH} in
        arm)
            echo "-march=armv7-a -Wl,--fix-cortex-a8"
        ;;
        arm64)
            echo "-march=armv8-a -Wl"
        ;;
        x86)
            echo "-march=i686 -Wl"
        ;;
        x86_64)
            echo "-march=x86-64 -Wl"
        ;;
    esac
}

android_get_ldflags() {
    ARCH_FLAGS=$(android_get_arch_specific_ldflags);
    OPTIMIZATION_FLAGS=$(android_get_size_optimization_ldflags $1);
    COMMON_LINKED_LIBS=$(android_get_common_linked_libraries);

    echo "${ARCH_FLAGS} ${OPTIMIZATION_FLAGS} ${COMMON_LINKED_LIBS}"
}

create_zlib_package_config() {
    ZLIB_VERSION=$(grep '#define ZLIB_VERSION' ${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot/usr/include/zlib.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    cat > "${ZLIB_PACKAGE_CONFIG_PATH}" << EOF
prefix=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot/usr
exec_prefix=\${prefix}
libdir=${ANDROID_NDK_ROOT}/platforms/android-${API}/arch-${ARCH}/usr/lib
includedir=\${prefix}/include

Name: zlib
Description: zlib compression library
Version: ${ZLIB_VERSION}

Requires:
Libs: -L\${libdir} -lz
Cflags: -I\${includedir}
EOF
}

android_prepare_toolchain_paths() {
    export PATH=$PATH:${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/bin

    TARGET_HOST=$(android_get_target_host)
    
    export AR=${TARGET_HOST}-ar
    export AS=${TARGET_HOST}-as
    export CC=${TARGET_HOST}-gcc
    export CXX=${TARGET_HOST}-g++
    export LD=${TARGET_HOST}-ld
    export RANLIB=${TARGET_HOST}-ranlib
    export STRIP=${TARGET_HOST}-strip

    export INSTALL_PKG_CONFIG_DIR="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/pkgconfig"
    export ZLIB_PACKAGE_CONFIG_PATH="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/pkgconfig/zlib.pc"

    if [ ! -d ${INSTALL_PKG_CONFIG_DIR} ]; then
        mkdir ${INSTALL_PKG_CONFIG_DIR}
    fi

    if [ ! -f ${ZLIB_PACKAGE_CONFIG_PATH} ]; then
        create_zlib_package_config
    fi

}

