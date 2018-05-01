#!/bin/bash

if [[ -z ${ARCH} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${IOS_MIN_VERSION} ]]; then
    echo "IOS_MIN_VERSION not defined"
    exit 1
fi

if [[ -z ${TARGET_SDK} ]]; then
    echo "TARGET_SDK not defined"
    exit 1
fi

if [[ -z ${SDK_PATH} ]]; then
    echo "SDK_PATH not defined"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo "BASEDIR not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/ios-common.sh

echo -e "\nBuilding ${ARCH} platform\n"
echo -e "\nINFO: Starting new build for ${ARCH} at "$(date)"\n">> ${BASEDIR}/build.log
INSTALL_BASE="${BASEDIR}/prebuilt/ios-$(get_target_host)"

# CLEANING EXISTING PACKAGE CONFIG DIRECTORY
PKG_CONFIG_DIRECTORY="${INSTALL_BASE}/pkgconfig"
if [ -d ${PKG_CONFIG_DIRECTORY} ]; then
    cd ${PKG_CONFIG_DIRECTORY} || exit 1
    rm -f *.pc || exit 1
else
    mkdir -p ${PKG_CONFIG_DIRECTORY} || exit 1
fi

# BUILDING EXTERNAL LIBRARIES
enabled_library_list=()
for library in {1..25}
do
    if [[ ${!library} -eq 1 ]]; then
        ENABLED_LIBRARY=$(get_library_name $((library - 1)))
        enabled_library_list+=(${ENABLED_LIBRARY})

        echo -e "INFO: Enabled library ${ENABLED_LIBRARY}" >> ${BASEDIR}/build.log
    fi
done

let completed=0
while [ ${#enabled_library_list[@]} -gt $completed ]; do
    for library in "${enabled_library_list[@]}"
    do
        let run=0
        case $library in
            fontconfig)
                if [ ! -z $OK_libuuid ] && [ ! -z $OK_libxml2 ] && [ ! -z $OK_libiconv ] && [ ! -z $OK_freetype ]; then
                    run=1
                fi
            ;;
            freetype)
                if [ ! -z $OK_libpng ]; then
                    run=1
                fi
            ;;
            gnutls)
                if [ ! -z $OK_nettle ] && [ ! -z $OK_gmp ] && [ ! -z $OK_libiconv ]; then
                    run=1
                fi
            ;;
            lame)
                if [ ! -z $OK_libiconv ]; then
                    run=1
                fi
            ;;
            libass)
                if [ ! -z $OK_libuuid ] && [ ! -z $OK_libxml2 ] && [ ! -z $OK_libiconv ] && [ ! -z $OK_freetype ] && [ ! -z $OK_fribidi ] && [ ! -z $OK_fontconfig ]; then
                    run=1
                fi
            ;;
            libtheora)
                if [ ! -z $OK_libvorbis ] && [ ! -z $OK_libogg ]; then
                    run=1
                fi
            ;;
            libvorbis)
                if [ ! -z $OK_libogg ]; then
                    run=1
                fi
            ;;
            libwebp)
                if [ ! -z $OK_giflib ] && [ ! -z $OK_jpeg ] && [ ! -z $OK_libpng ] && [ ! -z $OK_tiff ]; then
                    run=1
                fi
            ;;
            libxml2)
                if [ ! -z $OK_libiconv ]; then
                    run=1
                fi
            ;;
            *)
                run=1
            ;;
        esac

        CONTROL=$(echo "OK_${library}" | sed "s/\-/\_/g")

        if [[ $run -eq 1 ]] && [ -z ${!CONTROL} ]; then
            ENABLED_LIBRARY_PATH="${INSTALL_BASE}/${library}"

            echo -e "\nINFO: Building $library with the following environment variables\n" >> ${BASEDIR}/build.log
            echo -e "----------------------------------------------------------------" >> ${BASEDIR}/build.log
            env >> ${BASEDIR}/build.log
            echo -e "----------------------------------------------------------------\n" >> ${BASEDIR}/build.log

            echo -n "${library}: "

            if [ -d ${ENABLED_LIBRARY_PATH} ]; then
                rm -rf ${INSTALL_BASE}/${library} || exit 1
            fi

            SCRIPT_PATH="${BASEDIR}/build/ios-${library}.sh"

            cd ${BASEDIR}

            # BUILD EACH LIBRARY ALONE FIRST
            ${SCRIPT_PATH} 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

            if [ $? -eq 0 ]; then
                (( completed+=1 ))
                declare "$CONTROL=1"
                echo "ok"
            else
                echo "failed"
                exit 1
            fi
        else
            echo -e "\nINFO: Skipping $library, run=$run, completed=${!CONTROL}\n" >> ${BASEDIR}/build.log
        fi
    done
done

# BUILDING FFMPEG
. ${BASEDIR}/build/ios-ffmpeg.sh "$@"

# BUILDING MOBILE FFMPEG
. ${BASEDIR}/build/ios-mobile-ffmpeg.sh

echo -e "\nINFO: Completed build for ${ARCH} at "$(date)"\n">> ${BASEDIR}/build.log