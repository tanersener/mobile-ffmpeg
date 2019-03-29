#
# leptonica
#

###############################################################################
#
# cmake settings
#
###############################################################################

cmake_minimum_required(VERSION 2.8.11)

# In-source builds are disabled.
if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    message(FATAL_ERROR
        "CMake generation is not possible within the source directory!"
        "\n Remove the CMakeCache.txt file and try again from another folder, e.g.:"
        "\n "
        "\n rm CMakeCache.txt"
        "\n mkdir build"
        "\n cd build"
        "\n cmake .."
    )
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

###############################################################################
#
# project settings
#
###############################################################################

project(leptonica C CXX)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 78)
set(VERSION_PATCH 0)
set(VERSION_PLAIN ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

option(BUILD_PROG "Build utility programs" OFF)

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/.cppan)
    find_package(GIF)
    find_package(JPEG)
    find_package(PNG)
    find_package(TIFF)
    find_package(ZLIB)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(WEBP libwebp)
        pkg_check_modules(JP2K libopenjp2)
    endif()
    if(NOT WEBP)
        message(STATUS "Looking for WEBP")
        find_path(WEBP_INCLUDE_DIR /webp/decode.h)
        find_library(WEBP_LIBRARY NAMES webp)
        if (WEBP_INCLUDE_DIR AND WEBP_LIBRARY)
            set(WEBP 1)
            set(WEBP_FOUND TRUE)
            set(WEBP_LIBRARIES ${WEBP_LIBRARY})
            set(WEBP_INCLUDE_DIRS ${WEBP_INCLUDE_DIR})
            message(STATUS "Found WEBP library: ${WEBP_LIBRARY}")
        endif()
    endif()
    if(NOT JP2K)
        find_path(JP2K_INCLUDE_DIR /openjpeg-2.3/openjpeg.h)
        find_library(JP2K_LIBRARY NAMES openjp2)
        # we need to prepend subdirs (on windows only?)
        if (JP2K_INCLUDE_DIR)
            set(JP2K_HEADER "openjpeg-2.3/openjpeg.h")
        else()
            set(JP2K_HEADER "openjpeg.h")
        endif()
        if (JP2K_INCLUDE_DIR AND JP2K_LIBRARY)
             set(JP2K 1)
             set(JP2K_FOUND TRUE)
             set(JP2K_LIBRARIES ${JP2K_LIBRARY})
             set(JP2K_INCLUDE_DIRS ${JP2K_INCLUDE_DIR})
             set(HAVE_LIBJP2K 1)
             add_definitions( -DLIBJP2K_HEADER="${JP2K_HEADER}")
             message(STATUS "Found openjpeg library: ${JP2K_LIBRARIES}")
        endif()
    endif()

else()
    add_subdirectory(.cppan)

    add_definitions(
        -DHAVE_LIBGIF=1
        -DHAVE_LIBJPEG=1
        -DHAVE_LIBPNG=1
        -DHAVE_LIBTIFF=1
        -DHAVE_LIBWEBP=1
        -DHAVE_LIBZ=1

        -DHAVE_LIBJP2K=1
        -DLIBJP2K_HEADER="openjpeg.h"
    )
endif()

###############################################################################
#
# compiler and linker
#
###############################################################################

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CLANG 1)
endif()

set(LIBRARY_TYPE SHARED)
if (STATIC)
    set(LIBRARY_TYPE)
endif()

if (WIN32)
    if (MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W1")
        if (NOT CLANG)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        endif()
    endif()
endif()

###############################################################################
#
# configure
#
###############################################################################

set(AUTOCONFIG_SRC ${CMAKE_CURRENT_BINARY_DIR}/config_auto.h.in)
set(AUTOCONFIG ${CMAKE_CURRENT_BINARY_DIR}/src/config_auto.h)

include(Configure)

configure_file(${AUTOCONFIG_SRC} ${AUTOCONFIG} @ONLY)

set(INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include" "${CMAKE_INSTALL_PREFIX}/include/leptonica")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/LeptonicaConfig-version.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LeptonicaConfig-version.cmake @ONLY)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/LeptonicaConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LeptonicaConfig.cmake @ONLY)

###############################################################################
#
# build
#
###############################################################################

add_definitions(-DHAVE_CONFIG_H)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

if (BUILD_PROG)
add_subdirectory(prog)
endif()

add_subdirectory(src)

get_target_property(leptonica_NAME leptonica NAME)
get_target_property(leptonica_VERSION leptonica VERSION)
get_target_property(leptonica_OUTPUT_NAME leptonica OUTPUT_NAME)
configure_file(lept.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/lept.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lept.pc DESTINATION lib/pkgconfig)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LeptonicaConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LeptonicaConfig-version.cmake
    DESTINATION cmake)

###############################################################################
