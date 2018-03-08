#!/bin/bash

# ENABLE PLATFORMS
# ENABLE LIBRARIES

display_help() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\n'"$COMMAND"' builds ffmpeg for Android platform. By default four Android ABIs (arm, arm64, x86 and x86_64) are built with all optional libraries enabled. Use options to disable unwanted ABIS and libraries.\n"

    echo -e "Usage: ./"$COMMAND" [OPTION]...\n"   

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\tdisplay this help and exit"
    echo -e "  -V, --version\t\tdisplay version information and exit"
}

enable_feature() {
    return 0
}

disable_feature() {
    return 0
}

while [ ! $# -eq 0 ]
do

    case $1 in
	--help)
            display_help
            exit 0
	;;
        --enable-*)
            DISABLED_FLAG=`echo $1 | sed -e 's/^--.*-//g'`
	;;
        --disable-*)
            DISABLED_FLAG=`echo $1 | sed -e 's/^--.*-//g'`
	;;
    esac
    shift
done;



BASEDIR=$(pwd);

#API=21 ARCH=arm build/android-gmp.sh $BASEDIR
#API=21 ARCH=arm build/android-nettle.sh $BASEDIR
#API=21 ARCH=arm build/android-gnutls.sh $BASEDIR
#API=21 ARCH=arm build/android-ffmpeg.sh $BASEDIR
#API=21 ARCH=arm build/android-opencore-amr.sh $BASEDIR
#API=21 ARCH=arm build/android-libass.sh $BASEDIR
#API=21 ARCH=arm build/android-libuuid.sh $BASEDIR
#API=21 ARCH=arm build/android-fontconfig.sh $BASEDIR
#API=21 ARCH=arm build/android-freetype.sh $BASEDIR
#API=21 ARCH=arm build/android-fribidi.sh $BASEDIR
#API=21 ARCH=arm build/android-lame.sh $BASEDIR
#API=21 ARCH=arm build/android-libtheora.sh $BASEDIR
#API=21 ARCH=arm build/android-libwebp.sh $BASEDIR
#API=21 ARCH=arm build/android-tiff.sh $BASEDIR
#API=21 ARCH=arm build/android-libpng.sh $BASEDIR
#API=21 ARCH=arm build/android-giflib.sh $BASEDIR
#API=21 ARCH=arm build/android-jpeg.sh $BASEDIR
#API=21 ARCH=arm build/android-libvorbis.sh $BASEDIR
#API=21 ARCH=arm build/android-libogg.sh $BASEDIR
#API=21 ARCH=arm build/android-libiconv.sh $BASEDIR
#API=21 ARCH=arm build/android-libxml2.sh $BASEDIR
#API=21 ARCH=arm build/android-libvpx.sh $BASEDIR
#API=21 ARCH=arm build/android-shine.sh $BASEDIR
#API=21 ARCH=arm build/android-soxr.sh $BASEDIR
#API=21 ARCH=arm build/android-speex.sh $BASEDIR
#API=21 ARCH=arm build/android-wavpack.sh $BASEDIR
