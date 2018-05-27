#!/bin/bash

# ARCH INDEXES
ARCH_ARM_V7A=0
ARCH_ARM_V7A_NEON=1
ARCH_ARM64_V8A=2
ARCH_X86=3
ARCH_X86_64=4

# LIBRARY INDEXES
LIBRARY_FONTCONFIG=0
LIBRARY_FREETYPE=1
LIBRARY_FRIBIDI=2
LIBRARY_GMP=3
LIBRARY_GNUTLS=4
LIBRARY_LAME=5
LIBRARY_LIBASS=6
LIBRARY_LIBICONV=7
LIBRARY_LIBTHEORA=8
LIBRARY_LIBVORBIS=9
LIBRARY_LIBVPX=10
LIBRARY_LIBWEBP=11
LIBRARY_LIBXML2=12
LIBRARY_OPENCOREAMR=13
LIBRARY_SHINE=14
LIBRARY_SPEEX=15
LIBRARY_WAVPACK=16
LIBRARY_KVAZAAR=17
LIBRARY_GIFLIB=18
LIBRARY_JPEG=19
LIBRARY_LIBOGG=20
LIBRARY_LIBPNG=21
LIBRARY_LIBUUID=22
LIBRARY_NETTLE=23
LIBRARY_TIFF=24
LIBRARY_ZLIB=25
LIBRARY_MEDIA_CODEC=26

# ENABLE ARCH
ENABLED_ARCHITECTURES=(1 1 1 1 1)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

export BASEDIR=$(pwd)

# USING API LEVEL 21 / Android 5.0 (LOLLIPOP)
export API=21

get_mobile_ffmpeg_version() {
    local MOBILE_FFMPEG_VERSION=$(grep '#define MOBILE_FFMPEG_VERSION' ${BASEDIR}/android/app/src/main/cpp/mobileffmpeg.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    echo ${MOBILE_FFMPEG_VERSION}
}

display_help() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\n'"$COMMAND"' builds FFmpeg for Android platform. By default five Android ABIs (armeabi-v7a, armeabi-v7a-neon, arm64-v8a, x86 and x86_64) are built without any external libraries enabled. Options can be used to disable ABIs and/or enable external libraries.\n"

    echo -e "Usage: ./"$COMMAND" [OPTION]...\n"   

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\t\tdisplay this help and exit"
    echo -e "  -V, --version\t\t\tdisplay version information and exit\n"

    echo -e "Platforms:"

    echo -e "  --disable-arm-v7a\t\tdo not build arm-v7a platform"
    echo -e "  --disable-arm-v7a-neon\tdo not build arm-v7a-neon platform"
    echo -e "  --disable-arm64-v8a\t\tdo not build arm64-v8a platform"
    echo -e "  --disable-x86\t\t\tdo not build x86 platform"
    echo -e "  --disable-x86-64\t\tdo not build x86-64 platform\n"

    echo -e "Libraries:"

    echo -e "  --full\t\t\tenables all external libraries"
    echo -e "  --enable-android-media-codec\tbuild with built-in media codec"
    echo -e "  --enable-android-zlib\t\tbuild with built-in zlib"
    echo -e "  --enable-fontconfig\t\tbuild with fontconfig"
    echo -e "  --enable-freetype\t\tbuild with freetype"
    echo -e "  --enable-fribidi\t\tbuild with fribidi"
    echo -e "  --enable-gnutls\t\tbuild with gnutls"
    echo -e "  --enable-gmp\t\t\tbuild with gmp"
    echo -e "  --enable-kvazaar\t\tbuild with kvazaar"
    echo -e "  --enable-lame\t\t\tbuild with lame"
    echo -e "  --enable-libass\t\tbuild with libass"
    echo -e "  --enable-libiconv\t\tbuild with libiconv"
    echo -e "  --enable-libtheora\t\tbuild with libtheora"
    echo -e "  --enable-libvorbis\t\tbuild with libvorbis"
    echo -e "  --enable-libvpx\t\tbuild with libvpx"
    echo -e "  --enable-libwebp\t\tbuild with libwebp"
    echo -e "  --enable-libxml2\t\tbuild with libxml2"
    echo -e "  --enable-opencore-amr\t\tbuild with opencore-amr"
    echo -e "  --enable-shine\t\tbuild with shine"
    echo -e "  --enable-speex\t\tbuild with speex"
    echo -e "  --enable-wavpack\t\tbuild with wavpack"
    echo -e "  --reconf-LIBRARY\t\trun autoreconf before building LIBRARY\n"
}

display_version() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\
$COMMAND $(get_mobile_ffmpeg_version)\n\
Copyright (c) 2018 Taner Sener\n\
License LGPLv3.0: GNU LGPL version 3 or later\n\
<https://www.gnu.org/licenses/lgpl-3.0.en.html>\n\
This is free software: you can redistribute it and/or modify it under the terms of the \
GNU Lesser General Public License as published by the Free Software Foundation, \
either version 3 of the License, or (at your option) any later version."
}

reconf_library() {
    RECONF_VARIABLE=$(echo "RECONF_$1" | sed "s/\-/\_/g")

    export ${RECONF_VARIABLE}=1
}

enable_library() {
    set_library $1 1
}

set_library() {
    case $1 in
        android-media-codec)
            ENABLED_LIBRARIES[LIBRARY_MEDIA_CODEC]=$2
        ;;
        android-zlib)
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        fontconfig)
            ENABLED_LIBRARIES[LIBRARY_FONTCONFIG]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            set_library "libxml2" $2
            set_library "freetype" $2
        ;;
        freetype)
            ENABLED_LIBRARIES[LIBRARY_FREETYPE]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBPNG]=$2
        ;;
        fribidi)
            ENABLED_LIBRARIES[LIBRARY_FRIBIDI]=$2
        ;;
        gmp)
            ENABLED_LIBRARIES[LIBRARY_GMP]=$2
        ;;
        gnutls)
            ENABLED_LIBRARIES[LIBRARY_GNUTLS]=$2
            ENABLED_LIBRARIES[LIBRARY_NETTLE]=$2
            set_library "gmp" $2
            set_library "libiconv" $2
        ;;
        kvazaar)
            ENABLED_LIBRARIES[LIBRARY_KVAZAAR]=$2
        ;;
        lame)
            ENABLED_LIBRARIES[LIBRARY_LAME]=$2
            set_library "libiconv" $2
        ;;
        libass)
            ENABLED_LIBRARIES[LIBRARY_LIBASS]=$2
            set_library "freetype" $2
            set_library "fribidi" $2
            set_library "fontconfig" $2
            set_library "libiconv" $2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            set_library "libxml2" $2
        ;;
        libiconv)
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
        ;;
        libtheora)
            ENABLED_LIBRARIES[LIBRARY_LIBTHEORA]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBOGG]=$2
            set_library "libvorbis" $2
        ;;
        libvorbis)
            ENABLED_LIBRARIES[LIBRARY_LIBVORBIS]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBOGG]=$2
        ;;
        libvpx)
            ENABLED_LIBRARIES[LIBRARY_LIBVPX]=$2
        ;;
        libwebp)
            ENABLED_LIBRARIES[LIBRARY_LIBWEBP]=$2
            ENABLED_LIBRARIES[LIBRARY_GIFLIB]=$2
            ENABLED_LIBRARIES[LIBRARY_JPEG]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBPNG]=$2
            ENABLED_LIBRARIES[LIBRARY_TIFF]=$2
        ;;
        libxml2)
            ENABLED_LIBRARIES[LIBRARY_LIBXML2]=$2
            set_library "libiconv" $2
        ;;
        opencore-amr)
            ENABLED_LIBRARIES[LIBRARY_OPENCOREAMR]=$2
        ;;
        shine)
            ENABLED_LIBRARIES[LIBRARY_SHINE]=$2
        ;;
        speex)
            ENABLED_LIBRARIES[LIBRARY_SPEEX]=$2
        ;;
        wavpack)
            ENABLED_LIBRARIES[LIBRARY_WAVPACK]=$2
        ;;
        giflib | jpeg | libogg | libpng | libuuid | nettle | tiff)
        ;;
        *)
            print_unknown_library $1
        ;;
    esac
}

disable_arch() {
    set_arch $1 0
}

set_arch() {
    case $1 in
        arm-v7a)
            ENABLED_ARCHITECTURES[ARCH_ARM_V7A]=$2
        ;;
        arm-v7a-neon)
            ENABLED_ARCHITECTURES[ARCH_ARM_V7A_NEON]=$2
        ;;
        arm64-v8a)
            ENABLED_ARCHITECTURES[ARCH_ARM64_V8A]=$2
        ;;
        x86)
            ENABLED_ARCHITECTURES[ARCH_X86]=$2
        ;;
        x86-64)
            ENABLED_ARCHITECTURES[ARCH_X86_64]=$2
        ;;
        *)
            print_unknown_platform $1
        ;;
    esac
}

print_unknown_option() {
    echo -e "Unknown option \"$1\".\nSee $0 --help for available options."
    exit 1
}

print_unknown_library() {
    echo -e "Unknown library \"$1\".\nSee $0 --help for available libraries."
    exit 1
}

print_unknown_platform() {
    echo -e "Unknown platform \"$1\".\nSee $0 --help for available platforms."
    exit 1
}

print_enabled_architectures() {
    echo -n "Architectures: "

    let enabled=0;
    for print_arch in {0..4}
    do
        if [[ ${ENABLED_ARCHITECTURES[$print_arch]} -eq 1 ]]; then
            if [[ ${enabled} -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_arch_name $print_arch)
            enabled=$((${enabled} + 1));
        fi
    done

    if [ ${enabled} -gt 0 ]; then
        echo ""
    else
        echo "none"
    fi
}

print_enabled_libraries() {
    echo -n "Libraries: "

    let enabled=0;

    # FIRST BUILT-IN LIBRARIES
    for library in {25..26}
    do
        if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then
            if [[ ${enabled} -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_library_name $library)
            enabled=$((${enabled} + 1));
        fi
    done

    # THEN EXTERNAL LIBRARIES
    for library in {0..17}
    do
        if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then
            if [[ ${enabled} -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_library_name $library)
            enabled=$((${enabled} + 1));
        fi
    done

    if [ ${enabled} -gt 0 ]; then
        echo ""
    else
        echo "none"
    fi
}

build_application_mk() {
    rm -f ${BASEDIR}/android/jni/Application.mk

    cat > "${BASEDIR}/android/jni/Application.mk" << EOF
APP_OPTIM := release

APP_ABI := ${ANDROID_ARCHITECTURES}

APP_STL := c++_shared

APP_PLATFORM := android-21

APP_CFLAGS := -O3 -DANDROID -Wall -Wno-deprecated-declarations -Wno-pointer-sign -Wno-switch -Wno-unused-result -Wno-unused-variable
EOF
}


# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

while [ ! $# -eq 0 ]
do

    case $1 in
	    -h | --help)
            display_help
            exit 0
	    ;;
	    -V | --version)
            display_version
            exit 0
	    ;;
        --reconf-*)
            CONF_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            reconf_library ${CONF_LIBRARY}
	    ;;
	    --full)
            for library in {0..26}
            do
                enable_library $(get_library_name $library)
            done
	    ;;
        --enable-*)
            ENABLED_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            enable_library ${ENABLED_LIBRARY}
	    ;;
        --disable-*)
            DISABLED_ARCH=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            disable_arch ${DISABLED_ARCH}
	    ;;
	    *)
	        print_unknown_option $1
	    ;;
    esac
    shift
done;

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

echo -e "Building mobile-ffmpeg for Android\n"

if [[ ${ENABLED_ARCHITECTURES[0]} -eq 0 ]] && [[ ${ENABLED_ARCHITECTURES[1]} -eq 1 ]]; then
    ENABLED_ARCHITECTURES[0]=1

    echo -e "(*) arm-v7a architecture enabled since arm-v7a-neon will be built\n"
    echo -e "(*) arm-v7a architecture enabled since arm-v7a-neon will be built\n" 2>>${BASEDIR}/build.log 1>>${BASEDIR}/build.log
fi

print_enabled_architectures
print_enabled_libraries

for run_arch in {0..4}
do
    if [[ ${ENABLED_ARCHITECTURES[$run_arch]} -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TOOLCHAIN=$(get_toolchain)
        export TOOLCHAIN_ARCH=$(get_toolchain_arch)

        create_toolchain || exit 1

        . ${BASEDIR}/build/main-android.sh "${ENABLED_LIBRARIES[@]}" || exit 1

        # CLEAR FLAGS
        for library in {1..27}
        do
            library_name=$(get_library_name $((library - 1)))
            unset $(echo "OK_${library_name}" | sed "s/\-/\_/g")
        done
    fi
done

rm -f ${BASEDIR}/android/build/.neon
ANDROID_ARCHITECTURES=""
if [[ ${ENABLED_ARCHITECTURES[1]} -eq 1 ]]; then
    ANDROID_ARCHITECTURES+="$(get_android_arch 0) "
    mkdir -p ${BASEDIR}/android/build
    cat > "${BASEDIR}/android/build/.neon" << EOF
EOF
elif [[ ${ENABLED_ARCHITECTURES[0]} -eq 1 ]]; then
    ANDROID_ARCHITECTURES+="$(get_android_arch 0) "
fi
if [[ ${ENABLED_ARCHITECTURES[2]} -eq 1 ]]; then
    ANDROID_ARCHITECTURES+="$(get_android_arch 2) "
fi
if [[ ${ENABLED_ARCHITECTURES[3]} -eq 1 ]]; then
    ANDROID_ARCHITECTURES+="$(get_android_arch 3) "
fi
if [[ ${ENABLED_ARCHITECTURES[4]} -eq 1 ]]; then
    ANDROID_ARCHITECTURES+="$(get_android_arch 4) "
fi

if [[ ! -z ${ANDROID_ARCHITECTURES} ]]; then

    echo -e -n "\n\nCreating Android archive under prebuilt/android-aar: "

    build_application_mk

    MOBILE_FFMPEG_AAR=${BASEDIR}/prebuilt/android-aar/mobile-ffmpeg

    # BUILDING ANDROID ARCHIVE LIBRARY
    rm -rf ${BASEDIR}/prebuilt/android-aar
    rm -rf ${BASEDIR}/android/libs

    mkdir -p ${MOBILE_FFMPEG_AAR}

    cd ${BASEDIR}/android

    ${ANDROID_NDK_ROOT}/ndk-build -B 2>>${BASEDIR}/build.log 1>>${BASEDIR}/build.log

    if [ $? -ne 0 ]; then
        echo -e "failed\n"
        exit 1
    fi

    gradle clean build 2>>${BASEDIR}/build.log 1>>${BASEDIR}/build.log

    if [ $? -ne 0 ]; then
        echo -e "failed\n"
        exit 1
    fi

    cp ${BASEDIR}/android/app/build/outputs/aar/mobile-ffmpeg-1.0.aar ${MOBILE_FFMPEG_AAR} || exit 1

    echo -e "Created mobile-ffmpeg Android archive successfully.\n" >> ${BASEDIR}/build.log

    echo -e "ok\n"
fi
