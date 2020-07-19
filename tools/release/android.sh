#!/bin/bash
# 
# Creates a new android release from current branch
#

export BASEDIR=$(pwd)
export PACKAGE_DIRECTORY="${BASEDIR}/../../prebuilt/android-aar/mobile-ffmpeg"
export CUSTOM_OPTIONS="--disable-arm-v7a --enable-android-zlib --enable-android-media-codec"
export GPL_PACKAGES="--enable-gpl --enable-libvidstab --enable-x264 --enable-x265 --enable-xvidcore"
export FULL_PACKAGES="--enable-fontconfig --enable-freetype --enable-fribidi --enable-gmp --enable-gnutls --enable-kvazaar --enable-lame --enable-libaom --enable-libass --enable-libiconv --enable-libilbc --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libwebp --enable-libxml2 --enable-opencore-amr --enable-opus --enable-shine --enable-snappy --enable-soxr --enable-speex --enable-twolame --enable-wavpack"

create_package() {
    local NEW_PACKAGE="${PACKAGE_DIRECTORY}/mobile-ffmpeg-$1-$2.aar"

    local CURRENT_PACKAGE="${PACKAGE_DIRECTORY}/mobile-ffmpeg.aar"
    rm -f ${NEW_PACKAGE}

    mv ${CURRENT_PACKAGE} ${NEW_PACKAGE} || exit 1
}

enable_gradle_build() {
    rm -f ${BASEDIR}/../../android/app/build.gradle || exit 1
    cp ${BASEDIR}/android/build.gradle ${BASEDIR}/../../android/app/build.gradle || exit 1
}

enable_gradle_release() {
    rm -f ${BASEDIR}/../../android/app/build.gradle || exit 1
    cp ${BASEDIR}/android/release.template.gradle ${BASEDIR}/../../android/app/build.gradle || exit 1
}

if [ $# -ne 2 ]; then
    echo "Usage: android.sh <version code> <version name>"
    exit 1
fi

# VALIDATE VERSIONS
MOBILE_FFMPEG_VERSION=$(grep '#define MOBILE_FFMPEG_VERSION' ${BASEDIR}/../../android/app/src/main/cpp/mobileffmpeg.h | grep -Eo '\".*\"' | sed -e 's/\"//g')
if [[ "${MOBILE_FFMPEG_VERSION}" != "$2" ]]; then
    echo "Error: version mismatch. v$2 requested but v${MOBILE_FFMPEG_VERSION} found. Please perform the following updates and try again."
    echo "1. Update docs"
    echo "2. Update android/app/build.gradle file versions"
    echo "3. Update tools/release scripts' descriptions"
    echo "4. Update mobileffmpeg.h versions for both android and ios"
    echo "5. Update versions in Doxyfile"
    exit 1
fi

# MIN RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-min -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 without any external libraries enabled.' clean install || exit 1
create_package "min" "$2" || exit 1

# MIN-GPL RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} ${GPL_PACKAGES} || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-min-gpl -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 with libvid.stab v1.1.0, x264 v20200630-stable, x265 v3.4 and xvidcore v1.3.7 libraries enabled.' -PreleaseGPL=1 clean install || exit 1
create_package "min-gpl" "$2" || exit 1

# HTTPS RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} --enable-gnutls --enable-gmp || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-https -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 with gmp v6.2.0 and gnutls v3.6.13 library enabled.' clean install || exit 1
create_package "https" "$2" || exit 1

# HTTPS-GPL RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} --enable-gnutls --enable-gmp ${GPL_PACKAGES} || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-https-gpl -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 with gmp v6.2.0, gnutls v3.6.13, libvid.stab v1.1.0, x264 v20200630-stable, x265 v3.4 and xvidcore v1.3.7 libraries enabled.' -PreleaseGPL=1 clean install || exit 1
create_package "https-gpl" "$2" || exit 1

# AUDIO RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} --enable-lame --enable-libilbc --enable-libvorbis --enable-opencore-amr --enable-opus --enable-shine --enable-soxr --enable-speex --enable-twolame --enable-wavpack || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-audio -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 with lame v3.100, libilbc v2.0.2, libvorbis v1.3.7, opencore-amr v0.1.5, opus v1.3.1, shine v3.1.1, soxr v0.1.3, speex v1.2.0, twolame v0.4 and wavpack v5.3.0 libraries enabled.' clean install || exit 1
create_package "audio" "$2" || exit 1

# VIDEO RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} --enable-fontconfig --enable-freetype --enable-fribidi --enable-kvazaar --enable-libaom --enable-libass --enable-libiconv --enable-libtheora --enable-libvpx --enable-snappy --enable-libwebp || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-video -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 with fontconfig v2.13.92, freetype v2.10.2, fribidi v1.0.9, kvazaar v2.0.0, libaom v1.0.0-errata1-avif-110, libass v0.14.0, libiconv v1.16, libtheora v1.1.1, libvpx v1.8.2, snappy v1.1.8 and libwebp v1.1.0 libraries enabled.' clean install || exit 1
create_package "video" "$2" || exit 1

# FULL RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} ${FULL_PACKAGES} || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-full -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 with fontconfig v2.13.92, freetype v2.10.2, fribidi v1.0.9, gmp v6.2.0, gnutls v3.6.13, kvazaar v2.0.0, lame v3.100, libaom v1.0.0-errata1-avif-110, libass v0.14.0, libiconv v1.16, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.7, libvpx v1.8.2, libwebp v1.1.0, libxml2 v2.9.10, opencore-amr v0.1.5, opus v1.3.1, shine v3.1.1, snappy v1.1.8, soxr v0.1.3, speex v1.2.0, twolame v0.4 and wavpack v5.3.0 libraries enabled.' clean install || exit 1
create_package "full" "$2" || exit 1

# FULL-GPL RELEASE
enable_gradle_build
cd ${BASEDIR}/../.. || exit 1
./android.sh ${CUSTOM_OPTIONS} ${FULL_PACKAGES} ${GPL_PACKAGES} || exit 1
cd ${BASEDIR}/../../android/app || exit 1
enable_gradle_release
gradle -p ${BASEDIR}/../../android/app -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseMinSdk=24 -PreleaseTargetSdk=29 -PreleaseProject=mobile-ffmpeg-full-gpl -PreleaseProjectDescription='Includes FFmpeg v4.4-dev-416 with fontconfig v2.13.92, freetype v2.10.2, fribidi v1.0.9, gmp v6.2.0, gnutls v3.6.13, kvazaar v2.0.0, lame v3.100, libaom v1.0.0-errata1-avif-110, libass v0.14.0, libiconv v1.16, libilbc v2.0.2, libtheora v1.1.1, libvid.stab v1.1.0, libvorbis v1.3.7, libvpx v1.8.2, libwebp v1.1.0, libxml2 v2.9.10, opencore-amr v0.1.5, opus v1.3.1, shine v3.1.1, snappy v1.1.8, soxr v0.1.3, speex v1.2.0, twolame v0.4, wavpack v5.3.0, x264 v20200630-stable, x265 v3.4 and xvidcore v1.3.7 libraries enabled.' -PreleaseGPL=1 clean install || exit 1
create_package "full-gpl" "$2" || exit 1
enable_gradle_build