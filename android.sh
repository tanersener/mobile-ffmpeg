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
LIBRARY_X264=18
LIBRARY_XVIDCORE=19
LIBRARY_X265=20
LIBRARY_LIBVIDSTAB=21
LIBRARY_LIBILBC=22
LIBRARY_OPUS=23
LIBRARY_SNAPPY=24
LIBRARY_SOXR=25
LIBRARY_LIBAOM=26
LIBRARY_CHROMAPRINT=27
LIBRARY_TWOLAME=28
LIBRARY_SDL=29
LIBRARY_TESSERACT=30
LIBRARY_GIFLIB=31
LIBRARY_JPEG=32
LIBRARY_LIBOGG=33
LIBRARY_LIBPNG=34
LIBRARY_LIBUUID=35
LIBRARY_NETTLE=36
LIBRARY_TIFF=37
LIBRARY_EXPAT=38
LIBRARY_SNDFILE=39
LIBRARY_LEPTONICA=40
LIBRARY_ZLIB=41
LIBRARY_MEDIA_CODEC=42

# ENABLE ARCH
ENABLED_ARCHITECTURES=(1 1 1 1 1)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

export BASEDIR=$(pwd)

export MOBILE_FFMPEG_TMPDIR="${BASEDIR}/.tmp"

# USING API LEVEL 24 / Android 7.0 (NOUGAT)
export API=24

get_mobile_ffmpeg_version() {
    local MOBILE_FFMPEG_VERSION=$(grep '#define MOBILE_FFMPEG_VERSION' ${BASEDIR}/android/app/src/main/cpp/mobileffmpeg.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    echo ${MOBILE_FFMPEG_VERSION}
}

display_help() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\n'"$COMMAND"' builds FFmpeg and MobileFFmpeg for Android platform. By default five Android ABIs (armeabi-v7a, armeabi-v7a-neon, arm64-v8a, x86 and x86_64) are built \
without any external libraries enabled. Options can be used to disable ABIs and/or enable external libraries. \
Please note that GPL libraries (external libraries with GPL license) need --enable-gpl flag to be set explicitly. \
When compilation ends an Android Archive (AAR) file is created with enabled platforms inside.\n"

    echo -e "Usage: ./"$COMMAND" [OPTION]...\n"   

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\t\tdisplay this help and exit"
    echo -e "  -V, --version\t\t\tdisplay version information and exit"
    echo -e "  -d, --debug\t\t\tbuild with debug information"
    echo -e "  -s, --speed\t\t\toptimize for speed instead of size"
    echo -e "  -l, --lts\t\t\tbuild lts packages to support API 21+ devices\n"

    echo -e "Licensing options:"

    echo -e "  --enable-gpl\t\t\tallow use of GPL libraries, resulting libs will be licensed under GPLv3.0 [no]\n"

    echo -e "Platforms:"

    echo -e "  --disable-arm-v7a\t\tdo not build arm-v7a platform [yes]"
    echo -e "  --disable-arm-v7a-neon\tdo not build arm-v7a-neon platform [yes]"
    echo -e "  --disable-arm64-v8a\t\tdo not build arm64-v8a platform [yes]"
    echo -e "  --disable-x86\t\t\tdo not build x86 platform [yes]"
    echo -e "  --disable-x86-64\t\tdo not build x86-64 platform [yes]\n"

    echo -e "Libraries:"

    echo -e "  --full\t\t\tenables all external libraries"
    echo -e "  --enable-android-media-codec\tbuild with built-in Android MediaCodec support[no]"
    echo -e "  --enable-android-zlib\t\tbuild with built-in zlib support[no]"
    echo -e "  --enable-chromaprint\t\tbuild with chromaprint [no]"
    echo -e "  --enable-fontconfig\t\tbuild with fontconfig [no]"
    echo -e "  --enable-freetype\t\tbuild with freetype [no]"
    echo -e "  --enable-fribidi\t\tbuild with fribidi [no]"
    echo -e "  --enable-gmp\t\t\tbuild with gmp [no]"
    echo -e "  --enable-gnutls\t\tbuild with gnutls [no]"
    echo -e "  --enable-kvazaar\t\tbuild with kvazaar [no]"
    echo -e "  --enable-lame\t\t\tbuild with lame [no]"
    echo -e "  --enable-libaom\t\tbuild with libaom [no]"
    echo -e "  --enable-libass\t\tbuild with libass [no]"
    echo -e "  --enable-libiconv\t\tbuild with libiconv [no]"
    echo -e "  --enable-libilbc\t\tbuild with libilbc [no]"
    echo -e "  --enable-libtheora\t\tbuild with libtheora [no]"
    echo -e "  --enable-libvorbis\t\tbuild with libvorbis [no]"
    echo -e "  --enable-libvpx\t\tbuild with libvpx [no]"
    echo -e "  --enable-libwebp\t\tbuild with libwebp [no]"
    echo -e "  --enable-libxml2\t\tbuild with libxml2 [no]"
    echo -e "  --enable-opencore-amr\t\tbuild with opencore-amr [no]"
    echo -e "  --enable-opus\t\t\tbuild with opus [no]"
    echo -e "  --enable-sdl\t\t\tbuild with sdl [no]"
    echo -e "  --enable-shine\t\tbuild with shine [no]"
    echo -e "  --enable-snappy\t\tbuild with snappy [no]"
    echo -e "  --enable-soxr\t\t\tbuild with soxr [no]"
    echo -e "  --enable-speex\t\tbuild with speex [no]"
    echo -e "  --enable-tesseract\t\tbuild with tesseract [no]"
    echo -e "  --enable-twolame\t\tbuild with twolame [no]"
    echo -e "  --enable-wavpack\t\tbuild with wavpack [no]\n"

    echo -e "GPL libraries:"

    echo -e "  --enable-libvidstab\t\tbuild with libvidstab [no]"
    echo -e "  --enable-x264\t\t\tbuild with x264 [no]"
    echo -e "  --enable-x265\t\t\tbuild with x265 [no]"
    echo -e "  --enable-xvidcore\t\tbuild with xvidcore [no]\n"

    echo -e "Advanced options:"

    echo -e "  --reconf-LIBRARY\t\trun autoreconf before building LIBRARY [no]"
    echo -e "  --rebuild-LIBRARY\t\tbuild LIBRARY even it is detected as already built [no]\n"
}

display_version() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\
$COMMAND v$(get_mobile_ffmpeg_version)\n\
Copyright (c) 2018 Taner Sener\n\
License LGPLv3.0: GNU LGPL version 3 or later\n\
<https://www.gnu.org/licenses/lgpl-3.0.en.html>\n\
This is free software: you can redistribute it and/or modify it under the terms of the \
GNU Lesser General Public License as published by the Free Software Foundation, \
either version 3 of the License, or (at your option) any later version."
}

skip_library() {
    SKIP_VARIABLE=$(echo "SKIP_$1" | sed "s/\-/\_/g")

    export ${SKIP_VARIABLE}=1
}

enable_debug() {
    export MOBILE_FFMPEG_DEBUG="-DDEBUG -g"

    BUILD_TYPE_ID+="debug "
}

optimize_for_speed() {
    export MOBILE_FFMPEG_OPTIMIZED_FOR_SPEED="1"
}

enable_lts_build() {
    export MOBILE_FFMPEG_LTS_BUILD="1"

    # USING API LEVEL 21 / Android 5.0 (LOLLIPOP)
    export API=21
}

reconf_library() {
    RECONF_VARIABLE=$(echo "RECONF_$1" | sed "s/\-/\_/g")

    export ${RECONF_VARIABLE}=1
}

rebuild_library() {
    REBUILD_VARIABLE=$(echo "REBUILD_$1" | sed "s/\-/\_/g")

    export ${REBUILD_VARIABLE}=1
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
        chromaprint)
            ENABLED_LIBRARIES[LIBRARY_CHROMAPRINT]=$2
        ;;
        fontconfig)
            ENABLED_LIBRARIES[LIBRARY_FONTCONFIG]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            ENABLED_LIBRARIES[LIBRARY_EXPAT]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
            set_library "freetype" $2
        ;;
        freetype)
            ENABLED_LIBRARIES[LIBRARY_FREETYPE]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "libpng" $2
        ;;
        fribidi)
            ENABLED_LIBRARIES[LIBRARY_FRIBIDI]=$2
        ;;
        gmp)
            ENABLED_LIBRARIES[LIBRARY_GMP]=$2
        ;;
        gnutls)
            ENABLED_LIBRARIES[LIBRARY_GNUTLS]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "nettle" $2
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
        libaom)
            ENABLED_LIBRARIES[LIBRARY_LIBAOM]=$2
        ;;
        libass)
            ENABLED_LIBRARIES[LIBRARY_LIBASS]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            ENABLED_LIBRARIES[LIBRARY_EXPAT]=$2
            set_library "freetype" $2
            set_library "fribidi" $2
            set_library "fontconfig" $2
            set_library "libiconv" $2
        ;;
        libiconv)
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
        ;;
        libilbc)
            ENABLED_LIBRARIES[LIBRARY_LIBILBC]=$2
        ;;
        libpng)
            ENABLED_LIBRARIES[LIBRARY_LIBPNG]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        libtheora)
            ENABLED_LIBRARIES[LIBRARY_LIBTHEORA]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBOGG]=$2
            set_library "libvorbis" $2
        ;;
        libvidstab)
            ENABLED_LIBRARIES[LIBRARY_LIBVIDSTAB]=$2
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
            set_library "tiff" $2
            set_library "libpng" $2
        ;;
        libxml2)
            ENABLED_LIBRARIES[LIBRARY_LIBXML2]=$2
            set_library "libiconv" $2
        ;;
        opencore-amr)
            ENABLED_LIBRARIES[LIBRARY_OPENCOREAMR]=$2
        ;;
        opus)
            ENABLED_LIBRARIES[LIBRARY_OPUS]=$2
        ;;
        sdl)
            ENABLED_LIBRARIES[LIBRARY_SDL]=$2
        ;;
        shine)
            ENABLED_LIBRARIES[LIBRARY_SHINE]=$2
        ;;
        snappy)
            ENABLED_LIBRARIES[LIBRARY_SNAPPY]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        soxr)
            ENABLED_LIBRARIES[LIBRARY_SOXR]=$2
        ;;
        speex)
            ENABLED_LIBRARIES[LIBRARY_SPEEX]=$2
        ;;
        tesseract)
            ENABLED_LIBRARIES[LIBRARY_TESSERACT]=$2
            ENABLED_LIBRARIES[LIBRARY_LEPTONICA]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBWEBP]=$2
            ENABLED_LIBRARIES[LIBRARY_GIFLIB]=$2
            ENABLED_LIBRARIES[LIBRARY_JPEG]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "tiff" $2
            set_library "libpng" $2
        ;;
        twolame)
            ENABLED_LIBRARIES[LIBRARY_TWOLAME]=$2
            ENABLED_LIBRARIES[LIBRARY_SNDFILE]=$2
        ;;
        wavpack)
            ENABLED_LIBRARIES[LIBRARY_WAVPACK]=$2
        ;;
        x264)
            ENABLED_LIBRARIES[LIBRARY_X264]=$2
        ;;
        x265)
            ENABLED_LIBRARIES[LIBRARY_X265]=$2
        ;;
        xvidcore)
            ENABLED_LIBRARIES[LIBRARY_XVIDCORE]=$2
        ;;
        expat | giflib | jpeg | leptonica | libogg | libpng | libsndfile | libuuid)
            # THESE LIBRARIES ARE NOT ENABLED DIRECTLY
        ;;
        nettle)
            ENABLED_LIBRARIES[LIBRARY_NETTLE]=$2
            set_library "gmp" $2
        ;;
        tiff)
            ENABLED_LIBRARIES[LIBRARY_TIFF]=$2
            ENABLED_LIBRARIES[LIBRARY_JPEG]=$2
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
    for library in {41..42}
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
    for library in {0..30}
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
    if [[ ! -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
        local LTS_BUILD__FLAG="-DMOBILE_FFMPEG_LTS "
    fi

    rm -f ${BASEDIR}/android/jni/Application.mk

    cat > "${BASEDIR}/android/jni/Application.mk" << EOF
APP_OPTIM := release

APP_ABI := ${ANDROID_ARCHITECTURES}

APP_STL := c++_shared

APP_PLATFORM := android-${API}

APP_CFLAGS := -O3 -DANDROID ${LTS_BUILD__FLAG}-Wall -Wno-deprecated-declarations -Wno-pointer-sign -Wno-switch -Wno-unused-result -Wno-unused-variable
EOF
}


# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

DETECTED_NDK_VERSION=$(grep -Eo Revision.* ${ANDROID_NDK_ROOT}/source.properties | sed 's/Revision//g;s/=//g;s/ //g')

echo -e "\nINFO: Using Android NDK v${DETECTED_NDK_VERSION} provided at ${ANDROID_NDK_ROOT}\n" 1>>${BASEDIR}/build.log 2>&1

GPL_ENABLED="no"
DISPLAY_HELP=""
BUILD_LTS=""
BUILD_TYPE_ID=""

while [ ! $# -eq 0 ]
do

    case $1 in
	    -h | --help)
            DISPLAY_HELP="1"
	    ;;
	    -V | --version)
            display_version
            exit 0
	    ;;
        --skip-*)
            SKIP_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            skip_library ${SKIP_LIBRARY}
	    ;;
        -d | --debug)
            enable_debug
	    ;;
        -s | --speed)
	        optimize_for_speed
	    ;;
        -l | --lts)
	        BUILD_LTS="1"
	    ;;
        --reconf-*)
            CONF_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            reconf_library ${CONF_LIBRARY}
	    ;;
        --rebuild-*)
            BUILD_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            rebuild_library ${BUILD_LIBRARY}
	    ;;
	    --full)
            for library in {0..42}
            do
                if [[ $library -ne 18 ]] && [[ $library -ne 19 ]] && [[ $library -ne 20 ]] && [[ $library -ne 21 ]]; then
                    enable_library $(get_library_name $library)
                fi
            done
	    ;;
        --enable-gpl)
            GPL_ENABLED="yes"
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

# DETECT BUILD TYPE
if [[ ! -z ${BUILD_LTS} ]]; then
    enable_lts_build
    BUILD_TYPE_ID+="LTS "
fi

if [[ ! -z ${DISPLAY_HELP} ]]; then
    display_help
    exit 0
fi

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

echo -e "Building mobile-ffmpeg ${BUILD_TYPE_ID}library for Android\n"
echo -e -n "INFO: Building mobile-ffmpeg ${BUILD_TYPE_ID}library for Android: " 1>>${BASEDIR}/build.log 2>&1
echo -e `date` 1>>${BASEDIR}/build.log 2>&1

# PERFORM THIS CHECK ONLY ON LTS
if [[ ! -z ${MOBILE_FFMPEG_LTS_BUILD} ]] && [[ ${ENABLED_ARCHITECTURES[0]} -eq 0 ]] && [[ ${ENABLED_ARCHITECTURES[1]} -eq 1 ]]; then
    ENABLED_ARCHITECTURES[0]=1

    echo -e "(*) arm-v7a architecture enabled since arm-v7a-neon will be built\n"
    echo -e "(*) arm-v7a architecture enabled since arm-v7a-neon will be built\n" 1>>${BASEDIR}/build.log 2>&1
fi

print_enabled_architectures
print_enabled_libraries

# CHECKING GPL LIBRARIES
for gpl_library in {18..21}
do
    if [[ ${ENABLED_LIBRARIES[$gpl_library]} -eq 1 ]]; then
        library_name=$(get_library_name ${gpl_library})

        if  [ ${GPL_ENABLED} != "yes" ]; then
            echo -e "\n(*) Invalid configuration detected. GPL library ${library_name} enabled without --enable-gpl flag.\n"
            echo -e "\n(*) Invalid configuration detected. GPL library ${library_name} enabled without --enable-gpl flag.\n" 1>>${BASEDIR}/build.log 2>&1
            exit 1
        else
            DOWNLOAD_RESULT=$(download_gpl_library_source ${library_name})
            if [[ ${DOWNLOAD_RESULT} -ne 0 ]]; then
                echo -e "\n(*) Failed to download GPL library ${library_name} source. Please check build.log file for details. If the problem persists refer to offline building instructions.\n"
                echo -e "\n(*) Failed to download GPL library ${library_name} source.\n" 1>>${BASEDIR}/build.log 2>&1
                exit 1
            fi
        fi
    fi
done

# CLEANING PREVIOUSLY COPIED c++_shared.so FILES
rm -rf ${BASEDIR}/prebuilt/android-cpp-shared 1>>${BASEDIR}/build.log 2>&1

for run_arch in {0..4}
do
    if [[ ${ENABLED_ARCHITECTURES[$run_arch]} -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TOOLCHAIN=$(get_toolchain)
        export TOOLCHAIN_ARCH=$(get_toolchain_arch)

        create_toolchain || exit 1

        # COPY libc++_shared.so FROM EACH TOOLCHAIN
        mkdir -p ${BASEDIR}/prebuilt/android-cpp-shared/${TOOLCHAIN_ARCH}

        if [[ $run_arch -eq 0 ]] || [[ $run_arch -eq 1 ]]; then
            cp ${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-api-${API}-${TOOLCHAIN}/$(get_target_host)/lib/armv7-a/libc++_shared.so ${BASEDIR}/prebuilt/android-cpp-shared/${TOOLCHAIN_ARCH} 1>>${BASEDIR}/build.log 2>&1
        elif [[ $run_arch -eq 4 ]]; then
            cp ${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-api-${API}-${TOOLCHAIN}/$(get_target_host)/lib64/libc++_shared.so ${BASEDIR}/prebuilt/android-cpp-shared/${TOOLCHAIN_ARCH} 1>>${BASEDIR}/build.log 2>&1
        else
            cp ${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-api-${API}-${TOOLCHAIN}/$(get_target_host)/lib/libc++_shared.so ${BASEDIR}/prebuilt/android-cpp-shared/${TOOLCHAIN_ARCH} 1>>${BASEDIR}/build.log 2>&1
        fi

        . ${BASEDIR}/build/main-android.sh "${ENABLED_LIBRARIES[@]}" || exit 1

        # CLEAR FLAGS
        for library in {1..43}
        do
            library_name=$(get_library_name $((library - 1)))
            unset $(echo "OK_${library_name}" | sed "s/\-/\_/g")
            unset $(echo "DEPENDENCY_REBUILT_${library_name}" | sed "s/\-/\_/g")
        done
    fi
done

rm -f ${BASEDIR}/android/build/.neon 1>>${BASEDIR}/build.log 2>&1
ANDROID_ARCHITECTURES=""
if [[ ${ENABLED_ARCHITECTURES[1]} -eq 1 ]]; then
    ANDROID_ARCHITECTURES+="$(get_android_arch 0) "
    mkdir -p ${BASEDIR}/android/build 1>>${BASEDIR}/build.log 2>&1
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

    echo -n -e "\nmobile-ffmpeg: "

    build_application_mk

    MOBILE_FFMPEG_AAR=${BASEDIR}/prebuilt/android-aar/mobile-ffmpeg

    # BUILDING ANDROID ARCHIVE LIBRARY
    rm -rf ${BASEDIR}/android/libs 1>>${BASEDIR}/build.log 2>&1

    mkdir -p ${MOBILE_FFMPEG_AAR} 1>>${BASEDIR}/build.log 2>&1

    cd ${BASEDIR}/android 1>>${BASEDIR}/build.log 2>&1

    ${ANDROID_NDK_ROOT}/ndk-build -B 1>>${BASEDIR}/build.log 2>&1

    if [ $? -eq 0 ]; then
        echo "ok"
    else
        echo "failed"
        exit 1
    fi

    echo -e -n "\n\nCreating Android archive under prebuilt/android-aar: "

    ./gradlew clean build 1>>${BASEDIR}/build.log 2>&1

    if [ $? -ne 0 ]; then
        echo -e "failed\n"
        exit 1
    fi

    cp ${BASEDIR}/android/app/build/outputs/aar/mobile-ffmpeg-*.aar ${MOBILE_FFMPEG_AAR}/mobile-ffmpeg.aar 1>>${BASEDIR}/build.log 2>&1

    if [ $? -ne 0 ]; then
        echo -e "failed\n"
        exit 1
    fi

    echo -e "Created mobile-ffmpeg Android archive successfully.\n" 1>>${BASEDIR}/build.log 2>&1

    echo -e "ok\n"
fi
