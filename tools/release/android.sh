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

if [ $# -ne 2 ];
then
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
    echo "5. Update README.md"
    echo "6. Run tag.sh to tag released files"
    exit 1
fi

# MIN RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-min -PreleaseProjectDescription='Includes FFmpeg v3.4.4 without any external libraries enabled.' clean install bintrayUpload || exit 1
create_package "min" "1.2" || exit 1

# MIN-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gpl --enable-x264 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-min-gpl -PreleaseProjectDescription='Includes FFmpeg v3.4.4 with x264 v20180815-2245-stable and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
create_package "min-gpl" "1.2" || exit 1

# HTTPS RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gnutls || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-https -PreleaseProjectDescription='Includes FFmpeg v3.4.4 with gnutls v3.5.19 library enabled.' clean install bintrayUpload || exit 1
create_package "https" "1.2" || exit 1

# HTTPS-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gnutls --enable-gpl --enable-x264 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-https-gpl -PreleaseProjectDescription='Includes FFmpeg v3.4.4 with gnutls v3.5.19, x264 v20180815-2245-stable and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
create_package "https-gpl" "1.2" || exit 1

# FULL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --full || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-full -PreleaseProjectDescription='Includes FFmpeg v3.4.4 with fontconfig v2.12.93, freetype v2.9.1, fribidi v1.0.5, gmp v6.1.2, gnutls v3.5.19, kvazaar v1.2.0, libiconv v1.15, lame v3.100, libass v0.14.0, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.5, libvpx v1.7.0, libwebp v0.6.1, libxml2 v2.9.7, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, snappy v1.1.7, speex v1.2.0 and wavpack v5.1.0 libraries enabled.' clean install bintrayUpload || exit 1
create_package "full" "1.2" || exit 1

# FULL-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --full --enable-gpl --enable-x264 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-full-gpl -PreleaseProjectDescription='Includes FFmpeg v3.4.4 with fontconfig v2.12.93, freetype v2.9.1, fribidi v1.0.5, gmp v6.1.2, gnutls v3.5.19, kvazaar v1.2.0, libiconv v1.15, lame v3.100, libass v0.14.0, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.5, libvpx v1.7.0, libwebp v0.6.1, libxml2 v2.9.7, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, snappy v1.1.7, speex v1.2.0, wavpack v5.1.0, x264 v20180815-2245-stable and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
create_package "full-gpl" "1.2" || exit 1
