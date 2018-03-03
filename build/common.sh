#!/bin/bash

android_get_target_host() {
    case $ARCH in
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
    echo "-I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/local/include"
}

android_get_common_cflags() {
    echo "-Wno-psabi -Wno-unused-but-set-variable -Wno-unused-function -fstrict-aliasing -fPIE -fPIC -DANDROID -D__ANDROID_API__=$API"
}

android_get_arch_specific_cflags() {
    case $ARCH in
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
    case $ARCH in
        arm | arm64)
            ARCH_OPTIMIZATION="-Os -finline-limit=64"
        ;;
        x86 | x86_64)
            ARCH_OPTIMIZATION="-O2 -finline-limit=300"
        ;;
    esac

    LIB_OPTIMIZATION=""
    case $1 in
        libiconv | libxml2 | shine | soxr | speex | wavpack | libvpx)
            LIB_OPTIMIZATION=""
        ;;
        *)
            LIB_OPTIMIZATION="-ffunction-sections -fdata-sections -fomit-frame-pointer -funswitch-loops"
        ;;
    esac

    echo "${ARCH_OPTIMIZATION} ${LIB_OPTIMIZATION}"
}

android_get_app_specific_cflags() {

    APP_FLAGS=""
    case $1 in
        shine)
            APP_FLAGS=""
        ;;
        *)
            APP_FLAGS="-std=c99"
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
    echo "-std=c++11 -fno-exceptions -fno-rtti"
}

android_get_common_linked_libraries() {
    echo "-lc -lm -ldl -llog -pie -lpthread -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/lib -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/lib"
}

android_get_size_optimization_ldflags() {
    case $1 in
        libxml2 | shine | soxr | speex | wavpack | libvpx)
            echo ""
        ;;
        *)
            echo "-Wl,--gc-sections,--icf=safe"
        ;;
    esac
}

android_get_arch_specific_ldflags() {
    case $ARCH in
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

android_prepare_toolchain_paths() {
    export PATH=$PATH:$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/bin

    TARGET_HOST=$(android_get_target_host)
    
    export AR=$TARGET_HOST-ar
    export AS=$TARGET_HOST-as
    export CC=$TARGET_HOST-gcc
    export CXX=$TARGET_HOST-g++
    export LD=$TARGET_HOST-ld
    export RANLIB=$TARGET_HOST-ranlib
    export STRIP=$TARGET_HOST-strip
}

