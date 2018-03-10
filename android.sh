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
LIBRARY_GIFLIB=17
LIBRARY_JPEG=18
LIBRARY_LIBOGG=19
LIBRARY_LIBPNG=20
LIBRARY_LIBUUID=21
LIBRARY_NETTLE=22
LIBRARY_TIFF=23
LIBRARY_ZLIB=24
LIBRARY_MEDIA_CODEC=25

# ENABLE ARCH
ENABLED_ARCHITECTURES=(1 0 0 0 0)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

export BASEDIR=$(pwd)

# USING API LEVEL 21 / Android 5.0 (LOLLIPOP)
export API=21

display_help() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\n'"$COMMAND"' builds ffmpeg for Android platform. By default five Android ABIs (armeabi-v7a, armeabi-v7a-neon, arm64-v8a, x86 and x86_64) are built without any external libraries enabled. Options can be used to disable unwanted ABIS and/or enable needed external libraries.\n"

    echo -e "Usage: ./"$COMMAND" [OPTION]...\n"   

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\tdisplay this help and exit"
    echo -e "  -V, --version\t\tdisplay version information and exit\n"

    echo -e "Platforms:"

    echo -e "  --disable-arm-v7a\t\tdo not build arm-v7a platform"
    echo -e "  --disable-arm-v7a-neon\tdo not build arm-v7a-neon platform"
    echo -e "  --disable-arm64-v8a\t\tdo not build arm64-v8a platform"
    echo -e "  --disable-x86\t\t\tdo not build x86 platform"
    echo -e "  --disable-x86-64\t\tdo not build x86-64 platform\n"

    echo -e "Libraries:"

    echo -e "  --full\t\t\tenables all external libraries\n"

    echo -e "  --enable-android-media-codec\tbuild with built-in media codec"
    echo -e "  --disable-android-media-codec\tbuild without built-in media codec\n"

    echo -e "  --enable-android-zlib\t\tbuild with built-in zlib"
    echo -e "  --disable-android-zlib\tbuild without built-in zlib\n"

    echo -e "  --enable-fontconfig\t\tbuild with fontconfig"
    echo -e "  --disable-fontconfig\t\tbuild without fontconfig\n"

    echo -e "  --enable-freetype\t\tbuild with freetype"
    echo -e "  --disable-freetype\t\tbuild without freetype\n"

    echo -e "  --enable-fribidi\t\tbuild with fribidi"
    echo -e "  --disable-fribidi\t\tbuild without fribidi\n"

    echo -e "  --enable-gnutls\t\tbuild with gnutls"
    echo -e "  --disable-gnutls\t\tbuild without gnutls\n"

    echo -e "  --enable-gmp\t\t\tbuild with gmp"
    echo -e "  --disable-gmp\t\t\tbuild without gmp\n"

    echo -e "  --enable-lame\t\t\tbuild with lame"
    echo -e "  --disable-lame\t\tbuild without lame\n"

    echo -e "  --enable-libass\t\tbuild with libass"
    echo -e "  --disable-libass\t\tbuild without libass\n"

    echo -e "  --enable-libiconv\t\tbuild with libiconv"
    echo -e "  --disable-libiconv\t\tbuild without libiconv\n"

    echo -e "  --enable-libtheora\t\tbuild with libtheora"
    echo -e "  --disable-libtheora\t\tbuild without libtheora\n"

    echo -e "  --enable-libvorbis\t\tbuild with libvorbis"
    echo -e "  --disable-libvorbis\t\tbuild without libvorbis\n"

    echo -e "  --enable-libvpx\t\tbuild with libvpx"
    echo -e "  --disable-libvpx\t\tbuild without libvpx\n"

    echo -e "  --enable-libwebp\t\tbuild with libwebp"
    echo -e "  --disable-libwebp\t\tbuild without libwebp\n"

    echo -e "  --enable-libxml2\t\tbuild with libxml2"
    echo -e "  --disable-libxml2\t\tbuild without libxml2\n"

    echo -e "  --enable-opencore-amr\t\tbuild with opencore-amr"
    echo -e "  --disable-opencore-amr\tbuild without opencore-amr\n"

    echo -e "  --enable-shine\t\tbuild with shine"
    echo -e "  --disable-shine\t\tbuild without shine\n"

    echo -e "  --enable-speex\t\tbuild with speex"
    echo -e "  --disable-speex\t\tbuild without speex\n"

    echo -e "  --enable-wavpack\t\tbuild with wavpack"
    echo -e "  --disable-wavpack\t\tbuild without wavpack\n"
}

enable_library() {
    set_library $1 1
}

disable_library() {
        set_library $1 0
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
    esac
}

enable_arch() {
    set_arch $1 1
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
    esac
}

print_enabled_architectures() {
    echo -n "Architectures: "

    let enabled=0;
    for print_arch in {0..4}
    do
        if [[ ENABLED_ARCHITECTURES[$print_arch] -eq 1 ]]; then
            if [[ $enabled -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_arch_name $print_arch)
            enabled=$(($enabled + 1));
        fi
    done

    if [ $enabled -gt 0 ]; then
        echo ""
    else
        echo "none"
    fi
}

print_enabled_libraries() {
    echo -n "Libraries: "

    let enabled=0;

    # FIRST BUILT-IN LIBRARIES
    for library in {24..25}
    do
        if [[ ENABLED_LIBRARIES[$library] -eq 1 ]]; then
            if [[ $enabled -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_library_name $library)
            enabled=$(($enabled + 1));
        fi
    done

    # THEN EXTERNAL LIBRARIES
    for library in {0..16}
    do
        if [[ ENABLED_LIBRARIES[$library] -eq 1 ]]; then
            if [[ $enabled -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_library_name $library)
            enabled=$(($enabled + 1));
        fi
    done

    if [ $enabled -gt 0 ]; then
        echo ""
    else
        echo "none"
    fi
}

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

echo -e "Building mobile-ffmpeg for Android\n"

# USING LISTS TO PROCESS UNORDERED DISABLE/ENABLE FLAGS
enabled_library_list=()
disabled_library_list=()

while [ ! $# -eq 0 ]
do

    case $1 in
	    --help)
            display_help
            exit 0
	    ;;
	    --full)
            for library in {0..25}
            do
                enabled_library_list+=($(get_library_name $library))
            done
	    ;;
        --enable-*)
            ENABLED_FLAG=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            case ${ENABLED_FLAG} in
                arm-v7a | arm-v7a-neon | arm64-v8a | x86 | x86-64)
                    enable_arch ${ENABLED_FLAG}
                ;;
                *)
                    enabled_library_list+=(${ENABLED_FLAG})
                ;;
            esac
	    ;;
        --disable-*)
            DISABLED_FLAG=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            case ${DISABLED_FLAG} in
                arm-v7a | arm-v7a-neon | arm64-v8a | x86 | x86-64)
                    disable_arch ${DISABLED_FLAG}
                ;;
                *)
                    disabled_library_list+=(${DISABLED_FLAG})
                ;;
            esac
	    ;;
    esac
    shift
done;

# FIRST DISABLE LIBRARIES
for disabled_library in "${disabled_library_list[@]}"
do
    disable_library ${disabled_library}
done

# THEN APPLY ENABLED ONES
for enabled_library in "${enabled_library_list[@]}"
do
    enable_library ${enabled_library}
done

print_enabled_architectures
print_enabled_libraries

for run_arch in {0..4}
do
    if [[ ENABLED_ARCHITECTURES[$run_arch] -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TOOLCHAIN=$(get_toolchain)
        export TOOLCHAIN_ARCH=$(get_toolchain_arch)

        create_toolchain

        . ${BASEDIR}/build/main-android.sh "${ENABLED_LIBRARIES[@]}"
    fi
done
