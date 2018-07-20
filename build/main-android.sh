#!/bin/bash

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo -e "(*) ANDROID_NDK_ROOT not defined\n"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo -e "(*) ARCH not defined\n"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo -e "(*) API not defined\n"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo -e "(*) BASEDIR not defined\n"
    exit 1
fi

check_if_dependency_rebuilt() {
    case $1 in
        expat)
            set_dependency_rebuilt_flag "fontconfig"
            set_dependency_rebuilt_flag "libass"
        ;;
        fontconfig)
            set_dependency_rebuilt_flag "libass"
        ;;
        freetype)
            set_dependency_rebuilt_flag "fontconfig"
            set_dependency_rebuilt_flag "libass"
        ;;
        fribidi)
            set_dependency_rebuilt_flag "libass"
        ;;
        giflib)
            set_dependency_rebuilt_flag "libwebp"
        ;;
        gmp)
            set_dependency_rebuilt_flag "gnutls"
            set_dependency_rebuilt_flag "nettle"
        ;;
        jpeg)
            set_dependency_rebuilt_flag "tiff"
            set_dependency_rebuilt_flag "libwebp"
        ;;
        libiconv)
            set_dependency_rebuilt_flag "fontconfig"
            set_dependency_rebuilt_flag "gnutls"
            set_dependency_rebuilt_flag "lame"
            set_dependency_rebuilt_flag "libass"
            set_dependency_rebuilt_flag "libxml2"
        ;;
        libogg)
            set_dependency_rebuilt_flag "libvorbis"
            set_dependency_rebuilt_flag "libtheora"
        ;;
        libpng)
            set_dependency_rebuilt_flag "freetype"
            set_dependency_rebuilt_flag "libwebp"
            set_dependency_rebuilt_flag "libass"
        ;;
        libuuid)
            set_dependency_rebuilt_flag "fontconfig"
            set_dependency_rebuilt_flag "libass"
        ;;
        libvorbis)
            set_dependency_rebuilt_flag "libtheora"
        ;;
        nettle)
            set_dependency_rebuilt_flag "gnutls"
        ;;
        tiff)
            set_dependency_rebuilt_flag "libwebp"
        ;;
    esac
}

set_dependency_rebuilt_flag() {
    DEPENDENCY_REBUILT_VARIABLE=$(echo "DEPENDENCY_REBUILT_$1" | sed "s/\-/\_/g")
    export ${DEPENDENCY_REBUILT_VARIABLE}=1
}

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

echo -e "\nBuilding ${ARCH} platform on API level ${API}\n"
echo -e "\nINFO: Starting new build for ${ARCH} on API level ${API} at "$(date)"\n">> ${BASEDIR}/build.log
INSTALL_BASE="${BASEDIR}/prebuilt/android-$(get_target_build)"

# CREATING PACKAGE CONFIG DIRECTORY
PKG_CONFIG_DIRECTORY="${INSTALL_BASE}/pkgconfig"
if [ ! -d ${PKG_CONFIG_DIRECTORY} ]; then
    mkdir -p ${PKG_CONFIG_DIRECTORY} || exit 1
fi

# FILTERING WHICH EXTERNAL LIBRARIES WILL BE BUILT
# NOTE THAT BUILT-IN LIBRARIES ARE FORWARDED TO FFMPEG SCRIPT WITHOUT ANY PROCESSING
enabled_library_list=()
for library in {1..37}
do
    if [[ ${!library} -eq 1 ]]; then
        ENABLED_LIBRARY=$(get_library_name $((library - 1)))
        enabled_library_list+=(${ENABLED_LIBRARY})

        echo -e "INFO: Enabled library ${ENABLED_LIBRARY}" >> ${BASEDIR}/build.log
    fi
done

# BUILD CPU-FEATURES FIRST
build_cpufeatures

let completed=0
while [ ${#enabled_library_list[@]} -gt $completed ]; do
    for library in "${enabled_library_list[@]}"
    do
        let run=0
        case $library in
            fontconfig)
                if [[ ! -z $OK_libuuid ]] && [[ ! -z $OK_expat ]] && [[ ! -z $OK_libiconv ]] && [[ ! -z $OK_freetype ]]; then
                    run=1
                fi
            ;;
            freetype)
                if [[ ! -z $OK_libpng ]]; then
                    run=1
                fi
            ;;
            gnutls)
                if [[ ! -z $OK_nettle ]] && [[ ! -z $OK_gmp ]] && [[ ! -z $OK_libiconv ]]; then
                    run=1
                fi
            ;;
            lame)
                if [[ ! -z $OK_libiconv ]]; then
                    run=1
                fi
            ;;
            libass)
                if [[ ! -z $OK_libuuid ]] && [[ ! -z $OK_expat ]] && [[ ! -z $OK_libiconv ]] && [[ ! -z $OK_freetype ]] && [[ ! -z $OK_fribidi ]] && [[ ! -z $OK_fontconfig ]] && [[ ! -z $OK_libpng ]]; then
                    run=1
                fi
            ;;
            libtheora)
                if [[ ! -z $OK_libvorbis ]] && [[ ! -z $OK_libogg ]]; then
                    run=1
                fi
            ;;
            libvorbis)
                if [[ ! -z $OK_libogg ]]; then
                    run=1
                fi
            ;;
            libwebp)
                if [[ ! -z $OK_giflib ]] && [[ ! -z $OK_jpeg ]] && [[ ! -z $OK_libpng ]] && [[ ! -z $OK_tiff ]]; then
                    run=1
                fi
            ;;
            libxml2)
                if [[ ! -z $OK_libiconv ]]; then
                    run=1
                fi
            ;;
            nettle)
                if [[ ! -z $OK_gmp ]]; then
                    run=1
                fi
            ;;
            tiff)
                if [[ ! -z $OK_jpeg ]]; then
                    run=1
                fi
            ;;
            *)
                run=1
            ;;
        esac

        BUILD_COMPLETED_FLAG=$(echo "OK_${library}" | sed "s/\-/\_/g")
        REBUILD_FLAG=$(echo "REBUILD_${library}" | sed "s/\-/\_/g")
        DEPENDENCY_REBUILT_FLAG=$(echo "DEPENDENCY_REBUILT_${library}" | sed "s/\-/\_/g")

        if [ $run -eq 1 ] && [[ -z ${!BUILD_COMPLETED_FLAG} ]]; then
            ENABLED_LIBRARY_PATH="${INSTALL_BASE}/${library}"

            LIBRARY_IS_INSTALLED=$(library_is_installed ${INSTALL_BASE} ${library})

            echo -e "INFO: Flags detected for ${library}: already installed=${LIBRARY_IS_INSTALLED}, rebuild=${!REBUILD_FLAG}, dependency rebuilt=${!DEPENDENCY_REBUILT_FLAG}\n" >> ${BASEDIR}/build.log

            # DECIDE TO BUILD OR NOT
            if [[ ${LIBRARY_IS_INSTALLED} -ne 0 ]] || [[ ${!REBUILD_FLAG} -eq 1 ]] || [[ ${!DEPENDENCY_REBUILT_FLAG} -eq 1 ]]; then

                echo -e "----------------------------------------------------------------" >> ${BASEDIR}/build.log
                echo -e "\nINFO: Building $library with the following environment variables\n" >> ${BASEDIR}/build.log
                env >> ${BASEDIR}/build.log
                echo -e "----------------------------------------------------------------\n" >> ${BASEDIR}/build.log
                echo -e "INFO: System information\n" >> ${BASEDIR}/build.log
                uname -a >> ${BASEDIR}/build.log
                echo -e "----------------------------------------------------------------\n" >> ${BASEDIR}/build.log

                echo -n "${library}: "

                if [ -d ${ENABLED_LIBRARY_PATH} ]; then
                    rm -rf ${INSTALL_BASE}/${library} || exit 1
                fi

                SCRIPT_PATH="${BASEDIR}/build/android-${library}.sh"

                cd ${BASEDIR}

                # BUILD EACH LIBRARY ALONE FIRST
                ${SCRIPT_PATH} 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

                if [ $? -eq 0 ]; then
                    (( completed+=1 ))
                    declare "$BUILD_COMPLETED_FLAG=1"
                    check_if_dependency_rebuilt ${library}
                    echo "ok"
                else
                    echo "failed"
                    exit 1
                fi
            else
                (( completed+=1 ))
                declare "$BUILD_COMPLETED_FLAG=1"
                echo "${library}: already built"
            fi
        else
            echo -e "INFO: Skipping $library, run=$run, completed=${!BUILD_COMPLETED_FLAG}\n" >> ${BASEDIR}/build.log
        fi
    done
done

# BUILDING FFMPEG
. ${BASEDIR}/build/android-ffmpeg.sh "$@"

echo -e "\nINFO: Completed build for ${ARCH} on API level ${API} at "$(date)"\n">> ${BASEDIR}/build.log