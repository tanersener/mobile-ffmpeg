cmake_minimum_required(VERSION 2.8.7)

project(libwebp C)

# Options for coder / decoder executables.
option(WEBP_ENABLE_SIMD "Enable any SIMD optimization." ON)
option(WEBP_BUILD_CWEBP "Build the cwebp command line tool." OFF)
option(WEBP_BUILD_DWEBP "Build the dwebp command line tool." OFF)
option(WEBP_BUILD_GIF2WEBP "Build the gif2webp conversion tool." OFF)
option(WEBP_BUILD_IMG2WEBP "Build the img2webp animation tool." OFF)
option(WEBP_BUILD_WEBPINFO "Build the webpinfo command line tool." OFF)
option(WEBP_BUILD_WEBP_JS "Emscripten build of webp.js." OFF)
option(WEBP_NEAR_LOSSLESS "Enable near-lossless encoding" ON)
option(WEBP_ENABLE_SWAP_16BIT_CSP "Enable byte swap for 16 bit colorspaces." OFF)

if(WEBP_BUILD_WEBP_JS)
  set(WEBP_ENABLE_SIMD OFF)
endif()

set(WEBP_DEP_LIBRARIES)
set(WEBP_DEP_INCLUDE_DIRS)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE
    "Build type: Release, Debug or RelWithDebInfo" STRING FORCE
  )
endif()

# Include dependencies.
include(cmake/deps.cmake)

################################################################################
# Options.
if(WEBP_ENABLE_SWAP_16BIT_CSP)
  add_definitions(-DWEBP_SWAP_16BIT_CSP=1)
endif()

################################################################################
# Android only.
if(ANDROID)
  include_directories(${ANDROID_NDK}/sources/android/cpufeatures)
  add_library(cpufeatures STATIC
    ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c
  )
  target_link_libraries(cpufeatures dl)
  set(WEBP_DEP_LIBRARIES ${WEBP_DEP_LIBRARIES} cpufeatures)
  set(WEBP_DEP_INCLUDE_DIRS ${WEBP_DEP_INCLUDE_DIRS}
    ${ANDROID_NDK}/sources/android/cpufeatures
  )
  add_definitions(-DHAVE_CPU_FEATURES_H=1)
  set(HAVE_CPU_FEATURES_H 1)
else()
  set(HAVE_CPU_FEATURES_H 0)
endif()

################################################################################
# WebP source files.
# Read the Makefile.am to get the source files.

# We expect the Makefiles to define the sources as defined in
# the first regex. E.g.:
# libimagedec_la_SOURCES  = image_dec.c image_dec.h
function(parse_Makefile_am FOLDER VAR SRC_REGEX)
  file(READ ${FOLDER}/Makefile.am MAKEFILE_AM)
  string(REGEX MATCHALL "${SRC_REGEX}_SOURCES[ ]*\\+?=[ ]+[0-9a-z\\._ ]*"
    FILES_PER_LINE ${MAKEFILE_AM}
  )
  set(SRCS ${${VAR}})
  foreach(FILES ${FILES_PER_LINE})
    string(FIND ${FILES} "=" OFFSET)
    math(EXPR OFFSET "${OFFSET} + 2")
    string(SUBSTRING ${FILES} ${OFFSET} -1 FILES)
    if(FILES)
      string(REGEX MATCHALL "[0-9a-z\\._]+"
        FILES ${FILES}
      )
      foreach(FILE ${FILES})
        list(APPEND SRCS ${FOLDER}/${FILE})
      endforeach()
    endif()
  endforeach()
  set(${VAR} ${SRCS} PARENT_SCOPE)
endfunction()

set(WEBP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
parse_Makefile_am(${WEBP_SRC_DIR}/dec "WEBP_DEC_SRCS" "")
parse_Makefile_am(${WEBP_SRC_DIR}/demux "WEBP_DEMUX_SRCS" "")
parse_Makefile_am(${WEBP_SRC_DIR}/dsp "WEBP_DSP_COMMON_SRCS" "COMMON")
parse_Makefile_am(${WEBP_SRC_DIR}/dsp "WEBP_DSP_ENC_SRCS" "ENC")
parse_Makefile_am(${WEBP_SRC_DIR}/dsp "WEBP_DSP_ENC_SRCS" "dsp_[^ ]*")
parse_Makefile_am(${WEBP_SRC_DIR}/dsp "WEBP_DSP_DEC_SRCS" "decode_[^ ]*")
parse_Makefile_am(${WEBP_SRC_DIR}/enc "WEBP_ENC_SRCS" "")
parse_Makefile_am(${WEBP_SRC_DIR}/utils "WEBP_UTILS_COMMON_SRCS" "COMMON")
parse_Makefile_am(${WEBP_SRC_DIR}/utils "WEBP_UTILS_ENC_SRCS" "ENC")
parse_Makefile_am(${WEBP_SRC_DIR}/utils "WEBP_UTILS_DEC_SRCS" "decode_[^ ]*")

# Remove the files specific to SIMD we don't use.
foreach(FILE ${WEBP_SIMD_FILES_NOT_TO_INCLUDE})
  list(REMOVE_ITEM WEBP_DSP_ENC_SRCS ${FILE})
  list(REMOVE_ITEM WEBP_DSP_DEC_SRCS ${FILE})
endforeach()

### Define the mandatory libraries.
# Build the webpdecoder library.
if(MSVC)
  # avoid security warnings for e.g., fopen() used in the examples.
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  add_definitions(-Wall)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${WEBP_DEP_INCLUDE_DIRS})
add_library(webpdecode OBJECT ${WEBP_DEC_SRCS})
add_library(webpdspdecode OBJECT ${WEBP_DSP_COMMON_SRCS} ${WEBP_DSP_DEC_SRCS})
add_library(webputilsdecode OBJECT ${WEBP_UTILS_COMMON_SRCS}
  ${WEBP_UTILS_DEC_SRCS})
add_library(webpdecoder $<TARGET_OBJECTS:webpdecode>
  $<TARGET_OBJECTS:webpdspdecode> $<TARGET_OBJECTS:webputilsdecode>)
target_link_libraries(webpdecoder ${WEBP_DEP_LIBRARIES})

# Build the webp library.
add_library(webpencode OBJECT ${WEBP_ENC_SRCS})
add_library(webpdsp OBJECT ${WEBP_DSP_COMMON_SRCS} ${WEBP_DSP_DEC_SRCS}
  ${WEBP_DSP_ENC_SRCS})
add_library(webputils OBJECT ${WEBP_UTILS_COMMON_SRCS} ${WEBP_UTILS_DEC_SRCS}
  ${WEBP_UTILS_ENC_SRCS})
add_library(webp $<TARGET_OBJECTS:webpdecode> $<TARGET_OBJECTS:webpdsp>
  $<TARGET_OBJECTS:webpencode> $<TARGET_OBJECTS:webputils>)
target_link_libraries(webp ${WEBP_DEP_LIBRARIES})

# Make sure the OBJECT libraries are built with position independent code
# (it is not ON by default).
set_target_properties(webpdecode webpdspdecode webputilsdecode
  webpencode webpdsp webputils PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Build the webp demux library.
add_library(webpdemux ${WEBP_DEMUX_SRCS})
target_link_libraries(webpdemux webp)

# Set the version numbers.
function(parse_version FILE NAME VAR)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/${FILE} SOURCE_FILE)
  string(REGEX MATCH "${NAME}_la_LDFLAGS[^\n]* -version-info [0-9:]+" TMP
    ${SOURCE_FILE})
  string(REGEX MATCH "[0-9:]+" TMP ${TMP})
  string(REGEX REPLACE ":" "." VERSION ${TMP})
  set(${VAR} "${VERSION}" PARENT_SCOPE)
endfunction()
parse_version(Makefile.am webp WEBP_WEBP_SOVERSION)
set_target_properties(webp PROPERTIES VERSION ${PACKAGE_VERSION}
  SOVERSION ${WEBP_WEBP_SOVERSION})
parse_version(Makefile.am webpdecoder WEBP_DECODER_SOVERSION)
set_target_properties(webpdecoder PROPERTIES VERSION ${PACKAGE_VERSION}
  SOVERSION ${WEBP_DECODER_SOVERSION})
parse_version(demux/Makefile.am webpdemux WEBP_DEMUX_SOVERSION)
set_target_properties(webpdemux PROPERTIES VERSION ${PACKAGE_VERSION}
  SOVERSION ${WEBP_DEMUX_SOVERSION})

# Define the libraries to install.
set(INSTALLED_LIBRARIES webpdecoder webp webpdemux)

### Deal with SIMD.
# Change the compile flags for SIMD files we use.
list(LENGTH WEBP_SIMD_FILES_TO_INCLUDE WEBP_SIMD_FILES_TO_INCLUDE_LENGTH)
math(EXPR WEBP_SIMD_FILES_TO_INCLUDE_RANGE
  "${WEBP_SIMD_FILES_TO_INCLUDE_LENGTH}-1"
)

foreach(I_FILE RANGE ${WEBP_SIMD_FILES_TO_INCLUDE_RANGE})
  list(GET WEBP_SIMD_FILES_TO_INCLUDE ${I_FILE} FILE)
  list(GET WEBP_SIMD_FLAGS_TO_INCLUDE ${I_FILE} SIMD_COMPILE_FLAG)
  set_source_files_properties(${FILE} PROPERTIES
    COMPILE_FLAGS ${SIMD_COMPILE_FLAG}
  )
endforeach()

# Build the executables if asked for.
if(WEBP_BUILD_CWEBP OR WEBP_BUILD_DWEBP OR
   WEBP_BUILD_GIF2WEBP OR WEBP_BUILD_IMG2WEBP OR WEBP_BUILD_WEBP_JS)
  # Example utility library.
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/examples "EXAMPLEUTIL_SRCS"
    "example_util_[^ ]*")
  list(APPEND EXAMPLEUTIL_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/stopwatch.h)
  add_library(exampleutil ${EXAMPLEUTIL_SRCS})

  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/imageio "IMAGEIOUTILS_SRCS"
    "imageio_util_[^ ]*")
  add_library(imageioutil ${IMAGEIOUTILS_SRCS})
  target_link_libraries(imageioutil webp)

  # Image-decoding utility library.
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/imageio "IMAGEDEC_SRCS"
    "imagedec_[^ ]*")
  add_library(imagedec ${IMAGEDEC_SRCS})
  target_link_libraries(imagedec imageioutil webpdemux webp
    ${WEBP_DEP_IMG_LIBRARIES})

  # Image-encoding utility library.
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/imageio "IMAGEENC_SRCS"
    "imageenc_[^ ]*")
  add_library(imageenc ${IMAGEENC_SRCS})
  target_link_libraries(imageenc webp)

  set_property(TARGET exampleutil imageioutil imagedec imageenc
    PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)
endif()

if(WEBP_BUILD_DWEBP)
  # dwebp
  include_directories(${WEBP_DEP_IMG_INCLUDE_DIRS})
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/examples "DWEBP_SRCS"
    "dwebp")
  add_executable(dwebp ${DWEBP_SRCS})
  target_link_libraries(dwebp exampleutil imagedec imageenc webpdecoder)
  install(TARGETS dwebp RUNTIME DESTINATION bin)
  set_property(TARGET dwebp PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)
endif()

if(WEBP_BUILD_CWEBP)
  # cwebp
  include_directories(${WEBP_DEP_IMG_INCLUDE_DIRS})
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/examples "CWEBP_SRCS"
    "cwebp")
  add_executable(cwebp ${CWEBP_SRCS})
  target_link_libraries(cwebp exampleutil imagedec webp)
  install(TARGETS cwebp RUNTIME DESTINATION bin)
  set_property(TARGET cwebp PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)
endif()

if(WEBP_BUILD_GIF2WEBP AND NOT GIF_FOUND)
  unset(WEBP_BUILD_GIF2WEBP CACHE)
endif()

if(WEBP_BUILD_GIF2WEBP OR WEBP_BUILD_IMG2WEBP)
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/src/mux "WEBP_MUX_SRCS"
    "")
  add_library(webpmux ${WEBP_MUX_SRCS})
  target_link_libraries(webpmux webp)
  parse_version(mux/Makefile.am webpmux WEBP_MUX_SOVERSION)
  set_target_properties(webpmux PROPERTIES VERSION ${PACKAGE_VERSION}
    SOVERSION ${WEBP_MUX_SOVERSION})
  list(APPEND INSTALLED_LIBRARIES webpmux)
endif()

if(WEBP_BUILD_GIF2WEBP)
  # gif2webp
  include_directories(${WEBP_DEP_GIF_INCLUDE_DIRS})
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/examples "GIF2WEBP_SRCS"
    "gif2webp")
  add_executable(gif2webp ${GIF2WEBP_SRCS})
  target_link_libraries(gif2webp exampleutil imageioutil webp webpmux
    ${WEBP_DEP_GIF_LIBRARIES})
  install(TARGETS gif2webp RUNTIME DESTINATION bin)
  set_property(TARGET gif2webp PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)
endif()

if(WEBP_BUILD_IMG2WEBP)
  # img2webp
  include_directories(${WEBP_DEP_IMG_INCLUDE_DIRS})
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/examples "IMG2WEBP_SRCS"
    "img2webp")
  add_executable(img2webp ${IMG2WEBP_SRCS})
  target_link_libraries(img2webp exampleutil imagedec imageioutil webp webpmux)
  install(TARGETS img2webp RUNTIME DESTINATION bin)
  set_property(TARGET img2webp PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)
endif()

if (WEBP_BUILD_WEBPINFO)
  # webpinfo
  include_directories(${WEBP_DEP_IMG_INCLUDE_DIRS})
  parse_Makefile_am(${CMAKE_CURRENT_SOURCE_DIR}/examples "WEBPINFO_SRCS"
    "webpinfo")
  add_executable(webpinfo ${WEBPINFO_SRCS})
  target_link_libraries(webpinfo exampleutil imageioutil)
  install(TARGETS webpinfo RUNTIME DESTINATION bin)
  set_property(TARGET webpinfo PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)
endif()

if(WEBP_BUILD_WEBP_JS)
  # JavaScript version
  add_executable(webp_js
                 ${CMAKE_CURRENT_SOURCE_DIR}/extras/webp_to_sdl.c)
  target_link_libraries(webp_js webpdecoder SDL)
  set(WEBP_HAVE_SDL 1)
  set_target_properties(webp_js PROPERTIES LINK_FLAGS
      "-s EXPORTED_FUNCTIONS='[\"_WebpToSDL\"]' -s INVOKE_RUN=0 \
       -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"cwrap\"]'")
  set_target_properties(webp_js PROPERTIES OUTPUT_NAME webp)
  target_compile_definitions(webp_js PUBLIC EMSCRIPTEN WEBP_HAVE_SDL)

  # WASM version
  add_executable(webp_wasm
                 ${CMAKE_CURRENT_SOURCE_DIR}/extras/webp_to_sdl.c)
  target_link_libraries(webp_wasm webpdecoder SDL)
  set_target_properties(webp_wasm PROPERTIES LINK_FLAGS
      "-s WASM=1 -s 'BINARYEN_METHOD=\"native-wasm\"' \
       -s EXPORTED_FUNCTIONS='[\"_WebpToSDL\"]' -s INVOKE_RUN=0 \
       -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"cwrap\"]'")
  target_compile_definitions(webp_wasm PUBLIC EMSCRIPTEN WEBP_HAVE_SDL)

  target_compile_definitions(webpdecoder PUBLIC EMSCRIPTEN)
endif()

# Generate the config.h file.
configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/webp/config.h)
add_definitions(-DHAVE_CONFIG_H)
# The webp folder is included as we reference config.h as
# ../webp/config.h or webp/config.h
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Install the different headers and libraries.
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/webp/decode.h
              ${CMAKE_CURRENT_SOURCE_DIR}/src/webp/demux.h
              ${CMAKE_CURRENT_SOURCE_DIR}/src/webp/encode.h
              ${CMAKE_CURRENT_SOURCE_DIR}/src/webp/mux.h
              ${CMAKE_CURRENT_SOURCE_DIR}/src/webp/mux_types.h
              ${CMAKE_CURRENT_SOURCE_DIR}/src/webp/types.h
        DESTINATION include/webp)
install(TARGETS ${INSTALLED_LIBRARIES}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Create the CMake version file.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/WebPConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Create the Config file.
include(CMakePackageConfigHelpers)
set(ConfigPackageLocation share/WebP/cmake/)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/WebPConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/WebPConfig.cmake
  INSTALL_DESTINATION ${ConfigPackageLocation}
)

# Install the generated CMake files.
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/WebPConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/WebPConfig.cmake"
  DESTINATION ${ConfigPackageLocation}
)

# Install the man pages.
set(MAN_PAGES cwebp.1 dwebp.1 gif2webp.1 img2webp.1 vwebp.1 webpmux.1
  webpinfo.1)
set(EXEC_BUILDS "CWEBP" "DWEBP" "GIF2WEBP" "IMG2WEBP" "VWEBP" "WEBPMUX"
  "WEBPINFO")
list(LENGTH MAN_PAGES MAN_PAGES_LENGTH)
math(EXPR MAN_PAGES_RANGE "${MAN_PAGES_LENGTH} - 1")

foreach(I_MAN RANGE ${MAN_PAGES_RANGE})
  list(GET EXEC_BUILDS ${I_MAN} EXEC_BUILD)
  if(WEBP_BUILD_${EXEC_BUILD})
    list(GET MAN_PAGES ${I_MAN} MAN_PAGE)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/man/${MAN_PAGE}
      DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
      COMPONENT doc
    )
  endif()
endforeach()
