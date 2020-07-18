#!/bin/bash

export MOBILE_FFMPEG_TMPDIR="${BASEDIR}/.tmp"

autoreconf_library() {
  echo -e "\nDEBUG: Running full autoreconf for $1\n" 1>>${BASEDIR}/build.log 2>&1

  # TRY FULL RECONF
  (autoreconf --force --install)

  local EXTRACT_RC=$?
  if [ ${EXTRACT_RC} -eq 0 ]; then
    return
  fi

  echo -e "\nDEBUG: Full autoreconf failed. Running full autoreconf with include for $1\n" 1>>${BASEDIR}/build.log 2>&1

  # TRY FULL RECONF WITH m4
  (autoreconf --force --install -I m4)

  EXTRACT_RC=$?
  if [ ${EXTRACT_RC} -eq 0 ]; then
    return
  fi

  echo -e "\nDEBUG: Full autoreconf with include failed. Running autoreconf without force for $1\n" 1>>${BASEDIR}/build.log 2>&1

  # TRY RECONF WITHOUT FORCE
  (autoreconf --install)

  EXTRACT_RC=$?
  if [ ${EXTRACT_RC} -eq 0 ]; then
    return
  fi

  echo -e "\nDEBUG: Autoreconf without force failed. Running autoreconf without force with include for $1\n" 1>>${BASEDIR}/build.log 2>&1

  # TRY RECONF WITHOUT FORCE WITH m4
  (autoreconf --install -I m4)

  EXTRACT_RC=$?
  if [ ${EXTRACT_RC} -eq 0 ]; then
    return
  fi

  echo -e "\nDEBUG: Autoreconf without force with include failed. Running default autoreconf for $1\n" 1>>${BASEDIR}/build.log 2>&1

  # TRY DEFAULT RECONF
  (autoreconf)

  EXTRACT_RC=$?
  if [ ${EXTRACT_RC} -eq 0 ]; then
    return
  fi

  echo -e "\nDEBUG: Default autoreconf failed. Running default autoreconf with include for $1\n" 1>>${BASEDIR}/build.log 2>&1

  # TRY DEFAULT RECONF WITH m4
  (autoreconf -I m4)

  EXTRACT_RC=$?
  if [ ${EXTRACT_RC} -eq 0 ]; then
    return
  fi
}

#
# 1. <repo url>
# 2. <tag name>
# 3. <local folder path>
#
clone_git_repository() {
  local RC

  (mkdir -p $3 1>>${BASEDIR}/build.log 2>&1)

  RC=$?

  if [ ${RC} -ne 0 ]; then
    echo -e "\nDEBUG: Failed to create local directory $3\n" 1>>${BASEDIR}/build.log 2>&1
    rm -rf $3 1>>${BASEDIR}/build.log 2>&1
    echo ${RC}
    return
  fi

  (git clone --depth 1 --branch $2 $1 $3 1>>${BASEDIR}/build.log 2>&1)

  RC=$?

  if [ ${RC} -ne 0 ]; then
    echo -e "\nDEBUG: Failed to clone $1 -> $2\n" 1>>${BASEDIR}/build.log 2>&1
    rm -rf $3 1>>${BASEDIR}/build.log 2>&1
    echo ${RC}
    return
  fi

  echo ${RC}
}

#
# 1. <url>
# 2. <local file name>
# 3. <on error action>
#
download() {
  if [ ! -d "${MOBILE_FFMPEG_TMPDIR}" ]; then
    mkdir -p "${MOBILE_FFMPEG_TMPDIR}"
  fi

  (curl --fail --location $1 -o ${MOBILE_FFMPEG_TMPDIR}/$2 1>>${BASEDIR}/build.log 2>&1)

  local RC=$?

  if [ ${RC} -eq 0 ]; then
    echo -e "\nDEBUG: Downloaded $1 to ${MOBILE_FFMPEG_TMPDIR}/$2\n" 1>>${BASEDIR}/build.log 2>&1
  else
    rm -f ${MOBILE_FFMPEG_TMPDIR}/$2 1>>${BASEDIR}/build.log 2>&1

    echo -e -n "\nINFO: Failed to download $1 to ${MOBILE_FFMPEG_TMPDIR}/$2, rc=${RC}. " 1>>${BASEDIR}/build.log 2>&1

    if [ "$3" == "exit" ]; then
      echo -e "DEBUG: Build will now exit.\n" 1>>${BASEDIR}/build.log 2>&1
      exit 1
    else
      echo -e "DEBUG: Build will continue.\n" 1>>${BASEDIR}/build.log 2>&1
    fi
  fi

  echo ${RC}
}

download_library_source() {
  local LIB_REPO_URL=""
  local LIB_NAME="$1"
  local LIB_LOCAL_PATH=${BASEDIR}/src/${LIB_NAME}
  local LIB_TAG=""
  local LIBRARY_RC=""
  local DOWNLOAD_RC=""

  echo -e "\nDEBUG: Downloading library source: $1\n" 1>>${BASEDIR}/build.log 2>&1

  case $1 in
  cpu-features)
    LIB_REPO_URL="https://github.com/tanersener/cpu_features"
    LIB_TAG="v0.4.1.1"
    ;;
  esac

  LIBRARY_RC=$(library_is_downloaded "${LIB_NAME}")

  if [ ${LIBRARY_RC} -eq 0 ]; then
    echo -e "INFO: $1 already downloaded. Source folder found at ${LIB_LOCAL_PATH}\n" 1>>${BASEDIR}/build.log 2>&1
    echo 0
    return
  fi

  DOWNLOAD_RC=$(clone_git_repository "${LIB_REPO_URL}" "${LIB_TAG}" "${LIB_LOCAL_PATH}")

  if [ ${DOWNLOAD_RC} -ne 0 ]; then
    echo -e "INFO: Downloading library $1 failed. Can not get library from ${LIB_REPO_URL}\n" 1>>${BASEDIR}/build.log 2>&1
    echo ${DOWNLOAD_RC}
  else
    echo -e "DEBUG: $1 library downloaded\n" 1>>${BASEDIR}/build.log 2>&1
  fi
}

download_gpl_library_source() {
  local GPL_LIB_URL=""
  local GPL_LIB_FILE=""
  local GPL_LIB_ORIG_DIR=""
  local GPL_LIB_DEST_DIR="$1"
  local GPL_LIB_SOURCE_PATH="${BASEDIR}/src/${GPL_LIB_DEST_DIR}"
  local LIBRARY_RC=""
  local DOWNLOAD_RC=""

  echo -e "\nDEBUG: Downloading GPL library source: $1\n" 1>>${BASEDIR}/build.log 2>&1

  case $1 in
  libvidstab)
    GPL_LIB_URL="https://github.com/georgmartius/vid.stab/archive/v1.1.0.tar.gz"
    GPL_LIB_FILE="v1.1.0.tar.gz"
    GPL_LIB_ORIG_DIR="vid.stab-1.1.0"
    ;;
  x264)
    GPL_LIB_URL="https://code.videolan.org/videolan/x264/-/archive/296494a4011f58f32adc54304a2654627558c59a/x264-296494a4011f58f32adc54304a2654627558c59a.tar.bz2"
    GPL_LIB_FILE="x264-296494a4011f58f32adc54304a2654627558c59a.tar.bz2"
    GPL_LIB_ORIG_DIR="x264-296494a4011f58f32adc54304a2654627558c59a"
    ;;
  x265)
    GPL_LIB_URL="https://bitbucket.org/multicoreware/x265/downloads/x265_3.4.tar.gz"
    GPL_LIB_FILE="x265_3.4.tar.gz"
    GPL_LIB_ORIG_DIR="x265_3.4"
    ;;
  xvidcore)
    GPL_LIB_URL="https://downloads.xvid.com/downloads/xvidcore-1.3.7.tar.gz"
    GPL_LIB_FILE="xvidcore-1.3.7.tar.gz"
    GPL_LIB_ORIG_DIR="xvidcore"
    ;;
  rubberband)
    GPL_LIB_URL="https://breakfastquay.com/files/releases/rubberband-1.8.2.tar.bz2"
    GPL_LIB_FILE="rubberband-1.8.2.tar.bz2"
    GPL_LIB_ORIG_DIR="rubberband-1.8.2"
    ;;
  esac

  LIBRARY_RC=$(library_is_downloaded "${GPL_LIB_DEST_DIR}")

  if [ ${LIBRARY_RC} -eq 0 ]; then
    echo -e "INFO: $1 already downloaded. Source folder found at ${GPL_LIB_SOURCE_PATH}\n" 1>>${BASEDIR}/build.log 2>&1
    echo 0
    return
  fi

  local GPL_LIB_PACKAGE_PATH="${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_FILE}"

  echo -e "DEBUG: $1 source not found. Checking if library package ${GPL_LIB_FILE} is downloaded at ${GPL_LIB_PACKAGE_PATH} \n" 1>>${BASEDIR}/build.log 2>&1

  if [ ! -f "${GPL_LIB_PACKAGE_PATH}" ]; then
    echo -e "DEBUG: $1 library package not found. Downloading from ${GPL_LIB_URL}\n" 1>>${BASEDIR}/build.log 2>&1

    DOWNLOAD_RC=$(download "${GPL_LIB_URL}" "${GPL_LIB_FILE}")

    if [ ${DOWNLOAD_RC} -ne 0 ]; then
      echo -e "INFO: Downloading GPL library $1 failed. Can not get library package from ${GPL_LIB_URL}\n" 1>>${BASEDIR}/build.log 2>&1
      echo ${DOWNLOAD_RC}
      return
    else
      echo -e "DEBUG: $1 library package downloaded\n" 1>>${BASEDIR}/build.log 2>&1
    fi
  else
    echo -e "DEBUG: $1 library package already downloaded\n" 1>>${BASEDIR}/build.log 2>&1
  fi

  local EXTRACT_COMMAND=""

  if [[ ${GPL_LIB_FILE} == *bz2 ]]; then
    EXTRACT_COMMAND="tar jxf ${GPL_LIB_PACKAGE_PATH} --directory ${MOBILE_FFMPEG_TMPDIR}"
  else
    EXTRACT_COMMAND="tar zxf ${GPL_LIB_PACKAGE_PATH} --directory ${MOBILE_FFMPEG_TMPDIR}"
  fi

  echo -e "DEBUG: Extracting library package ${GPL_LIB_FILE} inside ${MOBILE_FFMPEG_TMPDIR}\n" 1>>${BASEDIR}/build.log 2>&1

  ${EXTRACT_COMMAND} 1>>${BASEDIR}/build.log 2>&1

  local EXTRACT_RC=$?

  if [ ${EXTRACT_RC} -ne 0 ]; then
    echo -e "\nINFO: Downloading GPL library $1 failed. Extract for library package ${GPL_LIB_FILE} completed with rc=${EXTRACT_RC}. Deleting failed files.\n" 1>>${BASEDIR}/build.log 2>&1
    rm -f ${GPL_LIB_PACKAGE_PATH} 1>>${BASEDIR}/build.log 2>&1
    rm -rf ${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_ORIG_DIR} 1>>${BASEDIR}/build.log 2>&1
    echo ${EXTRACT_RC}
    return
  fi

  echo -e "DEBUG: Extract completed. Copying library source to ${GPL_LIB_SOURCE_PATH}\n" 1>>${BASEDIR}/build.log 2>&1

  COPY_COMMAND="cp -r ${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_ORIG_DIR} ${GPL_LIB_SOURCE_PATH}"

  ${COPY_COMMAND} 1>>${BASEDIR}/build.log 2>&1

  local COPY_RC=$?

  if [ ${COPY_RC} -eq 0 ]; then
    echo -e "DEBUG: Downloading GPL library source $1 completed successfully\n" 1>>${BASEDIR}/build.log 2>&1
  else
    echo -e "\nINFO: Downloading GPL library $1 failed. Copying library source to ${GPL_LIB_SOURCE_PATH} completed with rc=${COPY_RC}\n" 1>>${BASEDIR}/build.log 2>&1
    rm -rf ${GPL_LIB_SOURCE_PATH} 1>>${BASEDIR}/build.log 2>&1
    echo ${COPY_RC}
    return
  fi
}

get_cpu_count() {
  if [ "$(uname)" == "Darwin" ]; then
    echo $(sysctl -n hw.physicalcpu)
  else
    echo $(nproc)
  fi
}

#
# 1. <lib name>
#
library_is_downloaded() {
  local LOCAL_PATH
  local LIB_NAME=$1
  local FILE_COUNT
  local REDOWNLOAD_VARIABLE
  REDOWNLOAD_VARIABLE=$(echo "REDOWNLOAD_$1" | sed "s/\-/\_/g")

  LOCAL_PATH=${BASEDIR}/src/${LIB_NAME}

  echo -e "DEBUG: Checking if ${LIB_NAME} is already downloaded at ${LOCAL_PATH}\n" 1>>${BASEDIR}/build.log 2>&1

  if [ ! -d ${LOCAL_PATH} ]; then
    echo -e "DEBUG: ${LOCAL_PATH} directory not found\n" 1>>${BASEDIR}/build.log 2>&1
    echo 1
    return
  fi

  FILE_COUNT=$(ls -l ${LOCAL_PATH} | wc -l)

  if [[ ${FILE_COUNT} -eq 0 ]]; then
    echo -e "DEBUG: No files found under ${LOCAL_PATH}\n" 1>>${BASEDIR}/build.log 2>&1
    echo 1
    return
  fi

  if [[ ${REDOWNLOAD_VARIABLE} -eq 1 ]]; then
    echo -e "INFO: ${LIB_NAME} library already downloaded but re-download requested\n" 1>>${BASEDIR}/build.log 2>&1
    rm -rf ${LOCAL_PATH} 1>>${BASEDIR}/build.log 2>&1
    echo 1
  else
    echo -e "INFO: ${LIB_NAME} library already downloaded\n" 1>>${BASEDIR}/build.log 2>&1
    echo 0
  fi
}

library_is_installed() {
  local INSTALL_PATH=$1
  local LIB_NAME=$2
  local HEADER_COUNT
  local LIB_COUNT

  echo -e "DEBUG: Checking if ${LIB_NAME} is already built and installed at ${INSTALL_PATH}/${LIB_NAME}\n" 1>>${BASEDIR}/build.log 2>&1

  if [ ! -d ${INSTALL_PATH}/${LIB_NAME} ]; then
    echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME} directory not found\n" 1>>${BASEDIR}/build.log 2>&1
    echo 1
    return
  fi

  if [ ! -d ${INSTALL_PATH}/${LIB_NAME}/lib ]; then
    echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME}/lib directory not found\n" 1>>${BASEDIR}/build.log 2>&1
    echo 1
    return
  fi

  if [ ! -d ${INSTALL_PATH}/${LIB_NAME}/include ]; then
    echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME}/include directory not found\n" 1>>${BASEDIR}/build.log 2>&1
    echo 1
    return
  fi

  HEADER_COUNT=$(ls -l ${INSTALL_PATH}/${LIB_NAME}/include | wc -l)
  LIB_COUNT=$(ls -l ${INSTALL_PATH}/${LIB_NAME}/lib | wc -l)

  if [[ ${HEADER_COUNT} -eq 0 ]]; then
    echo -e "DEBUG: No headers found under ${INSTALL_PATH}/${LIB_NAME}/include\n" 1>>${BASEDIR}/build.log 2>&1
    echo 1
    return
  fi

  if [[ ${LIB_COUNT} -eq 0 ]]; then
    echo -e "DEBUG: No libraries found under ${INSTALL_PATH}/${LIB_NAME}/lib\n" 1>>${BASEDIR}/build.log 2>&1
    echo 1
    return
  fi

  echo -e "INFO: ${LIB_NAME} library is already built and installed\n" 1>>${BASEDIR}/build.log 2>&1

  echo 0
}

prepare_inline_sed() {
  if [ "$(uname)" == "Darwin" ]; then
    export SED_INLINE="sed -i .tmp"
  else
    export SED_INLINE="sed -i"
  fi
}

to_capital_case() {
  echo "$(echo ${1:0:1} | tr '[a-z]' '[A-Z]')${1:1}"
}
