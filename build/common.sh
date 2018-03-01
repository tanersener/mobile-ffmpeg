#!/bin/bash

android_get_target_host() {
    case $1 in
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

android_get_common_cppflags() {
    case $1 in
        arm)
            echo "-march=armv7-a -mfpu=neon -mfloat-abi=softfp -Wno-psabi -Wno-unused-but-set-variable -Wno-unused-function -Os -std=c99 -fstrict-aliasing -ffunction-sections -fdata-sections -fPIE -fPIC -DANDROID -D__ANDROID_API__=$API -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/local/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/include/python2.7"
        ;;
        arm64)
            echo "-march=armv8-a -mfpu=neon -mfloat-abi=softfp -Wno-psabi -Wno-unused-but-set-variable -Wno-unused-function -Os -std=c99 -fstrict-aliasing -ffunction-sections -fdata-sections -fPIE -fPIC -DANDROID -D__ANDROID_API__=$API -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/local/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/include/python2.7"
        ;;
        x86)
            echo "-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32 -Wno-psabi -Wno-unused-but-set-variable -Wno-unused-function -O2 -std=c99 -fstrict-aliasing -ffunction-sections -fdata-sections -fPIE -fPIC -DANDROID -D__ANDROID_API__=$API -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/local/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/include/python2.7"
        ;;
        x86_64)
            echo "-march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel -Wno-psabi -Wno-unused-but-set-variable -Wno-unused-function -O2 -std=c99 -fstrict-aliasing -ffunction-sections -fdata-sections -fPIE -fPIC -DANDROID -D__ANDROID_API__=$API -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/local/include -I$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/include/python2.7"
        ;;
    esac
}

android_get_common_cxxflags() {
    echo "-fno-exceptions -fno-rtti"
}

android_get_common_ldflags() {
    case $1 in
        arm)
            echo "-march=armv7-a -Wl,--fix-cortex-a8 -Wl,--gc-sections -lc -lm -ldl -llog -pie -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/lib -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/lib"
        ;;
        arm64)
            echo "-march=armv8-a -Wl -Wl,--gc-sections -lc -lm -ldl -llog -pie -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/lib -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/lib"
        ;;
        x86)
            echo "-march=i686 -Wl -Wl,--gc-sections -lc -lm -ldl -llog -pie -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/lib -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/lib"
        ;;
        x86_64)
            echo "-march=x86-64 -Wl -Wl,--gc-sections -lc -lm -ldl -llog -pie -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/sysroot/usr/lib -L$ANDROID_NDK/toolchains/mobile-ffmpeg-$ARCH/lib"
        ;;
    esac
}

android_prepare_toolchain_paths() {
    export PATH=$PATH:$ANDROID_NDK/toolchains/mobile-ffmpeg-$1/bin

    TARGET_HOST=$(android_get_target_host $1)
    
    export AR=$TARGET_HOST-ar
    export AS=$TARGET_HOST-as
    export CC=$TARGET_HOST-gcc
    export CXX=$TARGET_HOST-g++
    export LD=$TARGET_HOST-ld
    export STRIP=$TARGET_HOST-strip
}

