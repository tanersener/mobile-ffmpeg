#!/bin/bash
# 
# Creates a new android release from current branch
#

export BASEDIR=$(pwd)
export PACKAGE_DIRECTORY="${BASEDIR}/../../prebuilt/android-aar/mobile-ffmpeg"

create_package() {
    local NEW_PACKAGE="${PACKAGE_DIRECTORY}/mobile-ffmpeg-$1-$2.aar"

    local CURRENT_PACKAGE="${PACKAGE_DIRECTORY}/mobile-ffmpeg.aar"
    rm -f ${NEW_PACKAGE}

    mv ${CURRENT_PACKAGE} ${NEW_PACKAGE} || exit 1
}

if [ $# -ne 2 ]; then
    echo "Usage: android.sh <version code> <version name>"
    exit 1
fi

# VALIDATE VERSIONS
MOBILE_FFMPEG_VERSION=$(grep '#define MOBILE_FFMPEG_VERSION' ${BASEDIR}/../../android/app/src/main/cpp/mobileffmpeg.h | grep -Eo '\".*\"' | sed -e 's/\"//g')
if [ "${MOBILE_FFMPEG_VERSION}" != "$2" ]; then
    echo "Error: version mismatch. v$1 requested but v${MOBILE_FFMPEG_VERSION} found. Please perform the following updates and try again."
    echo "1. Update docs"
    echo "2. Update android/app/build.gradle file versions"
    echo "3. Update tools/release scripts' descriptions"
    echo "4. Update mobileffmpeg.h versions for both android and ios"
    exit 1
fi

# MIN RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-min -PreleaseProjectDescription='Includes FFmpeg v4.0.2 without any external libraries enabled.' clean install bintrayUpload || exit 1
create_package "min" "$2" || exit 1

# MIN-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gpl --enable-libvidstab --enable-x264 --enable-x265 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-min-gpl -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with libvid.stab v1.1.0, x264 20180829-2245-stable, x265 v2.8 and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
create_package "min-gpl" "$2" || exit 1

# HTTPS RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gnutls || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-https -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with gnutls v3.5.19 library enabled.' clean install bintrayUpload || exit 1
create_package "https" "$2" || exit 1

# HTTPS-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gnutls --enable-gpl --enable-libvidstab --enable-x264 --enable-x265 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-https-gpl -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with gnutls v3.5.19, libvid.stab v1.1.0, x264 20180829-2245-stable, x265 v2.8 and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
create_package "https-gpl" "$2" || exit 1

# AUDIO RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-chromaprint --enable-lame --enable-libilbc --enable-libvorbis --enable-opencore-amr --enable-opus --enable-shine --enable-soxr --enable-speex --enable-wavpack || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-audio -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with chromaprint v1.4.3, lame v3.100, libilbc v2.0.2, libvorbis v1.3.6, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, soxr v0.1.3, speex v1.2.0 and wavpack v5.1.0 libraries enabled.' clean install bintrayUpload || exit 1
create_package "audio" "$2" || exit 1

# VIDEO RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-fontconfig --enable-freetype --enable-fribidi --enable-kvazaar --enable-libaom --enable-libass --enable-libiconv --enable-libtheora --enable-libvpx --enable-snappy || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-video -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with fontconfig v2.13.0, freetype v2.9.1, fribidi v1.0.5, kvazaar v1.2.0, libaom v2018.08.29-snapshot, libass v0.14.0, libiconv v1.15, libtheora v1.1.1, libvpx v1.7.0 and snappy v1.1.7 libraries enabled.' clean install bintrayUpload || exit 1
create_package "video" "$2" || exit 1

# VIDEO-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gpl --enable-libvidstab --enable-x264 --enable-x265 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-video-gpl -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with libvid.stab v1.1.0, x264 20180829-2245-stable, x265 v2.8 and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
create_package "video-gpl" "$2" || exit 1

# FULL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --full || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-full -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with chromaprint v1.4.3, fontconfig v2.13.0, freetype v2.9.1, fribidi v1.0.5, gmp v6.1.2, gnutls v3.5.19, kvazaar v1.2.0, lame v3.100, libaom v2018.08.29-snapshot, libass v0.14.0, libiconv v1.15, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.6, libvpx v1.7.0, libwebp v1.0.0, libxml2 v2.9.8, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, snappy v1.1.7, soxr v0.1.3, speex v1.2.0 and wavpack v5.1.0 libraries enabled.' clean install bintrayUpload || exit 1
create_package "full" "$2" || exit 1

# FULL-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --full --enable-gpl --enable-libvidstab --enable-x264 --enable-x265 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-full-gpl -PreleaseProjectDescription='Includes FFmpeg v4.0.2 with chromaprint v1.4.3, fontconfig v2.13.0, freetype v2.9.1, fribidi v1.0.5, gmp v6.1.2, gnutls v3.5.19, kvazaar v1.2.0, lame v3.100, libaom v2018.08.29-snapshot, libass v0.14.0, libiconv v1.15, libilbc v2.0.2, libtheora v1.1.1, libvid.stab v1.1.0, libvorbis v1.3.6, libvpx v1.7.0, libwebp v1.0.0, libxml2 v2.9.8, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, snappy v1.1.7, soxr v0.1.3, speex v1.2.0, wavpack v5.1.0, x264 20180829-2245-stable, x265 v2.8 and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
create_package "full-gpl" "$2" || exit 1
