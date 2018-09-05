cmake_minimum_required(VERSION 2.8.12)

if(POLICY CMP0042)
	cmake_policy(SET CMP0042 NEW)
endif()

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 OLD)
endif()

project(chromaprint C CXX)

set(chromaprint_VERSION_MAJOR 1)
set(chromaprint_VERSION_MINOR 4)
set(chromaprint_VERSION_PATCH 3)
set(chromaprint_VERSION "${chromaprint_VERSION_MAJOR}.${chromaprint_VERSION_MINOR}.${chromaprint_VERSION_PATCH}")

set(PROJECT_VERSION "${chromaprint_VERSION}")

set(chromaprint_SOVERSION 1)

set(AUDIO_PROCESSOR_LIB CACHE STRING "Library to use for audio processing")
set_property(CACHE AUDIO_PROCESSOR_LIB PROPERTY STRINGS avresample swresample)

set(FFT_LIB CACHE STRING "Library to use for FFT calculations")
set_property(CACHE FFT_LIB PROPERTY STRINGS avfft fftw3 fftw3f kissfft vdsp)

include(CMakePushCheckState)
include(CheckFunctionExists)
include(CheckCXXCompilerFlag)

find_package(Threads)

check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND BUILD_SHARED_LIBS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-D_GLIBCXX_DEBUG)
endif()

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_LIBRARIES -lm)
check_function_exists(lrintf HAVE_LRINTF)
check_function_exists(round HAVE_ROUND)
cmake_pop_check_state()

add_definitions(
	-DHAVE_CONFIG_H
	-D_SCL_SECURE_NO_WARNINGS
	-D_USE_MATH_DEFINES
	-D__STDC_LIMIT_MACROS
	-D__STDC_CONSTANT_MACROS
)

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH  "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "Installation prefix for C header files" FORCE)

if(APPLE)
	option(BUILD_FRAMEWORK "Build an OS X framework" OFF)
	set(FRAMEWORK_INSTALL_DIR "/Library/Frameworks" CACHE STRING "Directory to install frameworks to")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TOOLS "Build command line tools" OFF)
option(BUILD_TESTS "Build test suite" OFF)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

if(NOT BUILD_SHARED_LIBS)
	add_definitions(-DCHROMAPRINT_NODLL)
endif()

set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(FFmpeg)

if(FFMPEG_LIBRARIES)
	cmake_push_check_state(RESET)
	set(CMAKE_REQUIRED_LIBRARIES ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} -lm)
	check_function_exists(av_packet_unref HAVE_AV_PACKET_UNREF)
	check_function_exists(av_frame_alloc HAVE_AV_FRAME_ALLOC)
	check_function_exists(av_frame_free HAVE_AV_FRAME_FREE)
	cmake_pop_check_state()
endif()

if(NOT FFT_LIB OR FFT_LIB STREQUAL "fftw3" OR FFT_LIB STREQUAL "fftw3f")
	find_package(FFTW3)
endif()

if(APPLE)
	if(NOT FFT_LIB OR FFT_LIB STREQUAL "vdsp")
		find_library(ACCELERATE_LIBRARIES Accelerate)
	endif()
endif()

if(NOT FFT_LIB OR FFT_LIB STREQUAL "kissfft")
	find_package(KissFFT)
endif()

set(USE_VDSP OFF)
set(USE_AVFFT OFF)
set(USE_FFTW3 OFF)
set(USE_FFTW3F OFF)
set(USE_KISSFFT OFF)

if(NOT FFT_LIB)
	if(APPLE AND ACCELERATE_LIBRARIES)
		set(FFT_LIB "vdsp")
	elseif(FFMPEG_LIBAVCODEC_FFT_FOUND)
		set(FFT_LIB "avfft")
	elseif(FFTW3_LIBRARIES)
		set(FFT_LIB "fftw3")
	elseif(FFTW3_FFTWF_LIBRARY)
		set(FFT_LIB "fftw3f")
	elseif(KISSFFT_FOUND)
		set(FFT_LIB "kissfft")
	endif()
endif()

if(FFT_LIB STREQUAL "vdsp")
	if(ACCELERATE_LIBRARIES)
		set(USE_VDSP ON)
	else()
		message(FATAL_ERROR "Selected ${FFT_LIB} for FFT calculations, but the library is not found")
	endif()
elseif(FFT_LIB STREQUAL "avfft")
	if(FFMPEG_LIBAVCODEC_FFT_FOUND)
		set(USE_AVFFT ON)
	else()
		message(FATAL_ERROR "Selected ${FFT_LIB} for FFT calculations, but the library is not found")
	endif()
elseif(FFT_LIB STREQUAL "fftw3")
	if(FFTW3_LIBRARIES)
		set(USE_FFTW3 ON)
	else()
		message(FATAL_ERROR "Selected ${FFT_LIB} for FFT calculations, but the library is not found")
	endif()
elseif(FFT_LIB STREQUAL "fftw3f")
	if(FFTW3_FFTWF_LIBRARY)
		set(USE_FFTW3F ON)
	else()
		message(FATAL_ERROR "Selected ${FFT_LIB} for FFT calculations, but the library is not found")
	endif()
elseif(FFT_LIB STREQUAL "kissfft")
	if(KISSFFT_FOUND)
		set(USE_KISSFFT ON)
	else()
		message(FATAL_ERROR "Selected ${FFT_LIB} for FFT calculations, but the library is not found")
	endif()
else()
	message(FATAL_ERROR "No FFT library found")
endif()

message(STATUS "Using ${FFT_LIB} for FFT calculations")

if(NOT AUDIO_PROCESSOR_LIB)
	if(FFMPEG_LIBSWRESAMPLE_FOUND)
		set(AUDIO_PROCESSOR_LIB "swresample")
	elseif(FFMPEG_LIBAVRESAMPLE_FOUND)
		set(AUDIO_PROCESSOR_LIB "avresample")
	endif()
endif()

if(AUDIO_PROCESSOR_LIB STREQUAL "swresample")
	if(FFMPEG_LIBSWRESAMPLE_FOUND)
		set(USE_AVRESAMPLE OFF)
		set(USE_SWRESAMPLE ON)
		set(AUDIO_PROCESSOR_LIBRARIES ${FFMPEG_LIBSWRESAMPLE_LIBRARIES})
		set(AUDIO_PROCESSOR_INCLUDE_DIRS ${FFMPEG_LIBSWRESAMPLE_INCLUDE_DIRS})
	else()
		message(FATAL_ERROR "Selected ${AUDIO_PROCESSOR_LIB} for audio processing, but the library is not found")
	endif()
	message(STATUS "Using ${AUDIO_PROCESSOR_LIB} for audio conversion")
elseif(AUDIO_PROCESSOR_LIB STREQUAL "avresample")
	if(FFMPEG_LIBAVRESAMPLE_FOUND)
		set(USE_AVRESAMPLE ON)
		set(USE_SWRESAMPLE OFF)
		set(AUDIO_PROCESSOR_LIBRARIES ${FFMPEG_LIBAVRESAMPLE_LIBRARIES})
		set(AUDIO_PROCESSOR_INCLUDE_DIRS ${FFMPEG_LIBAVRESAMPLE_INCLUDE_DIRS})
	else()
		message(FATAL_ERROR "Selected ${AUDIO_PROCESSOR_LIB} for audio processing, but the library is not found")
	endif()
	message(STATUS "Using ${AUDIO_PROCESSOR_LIB} for audio conversion")
else()
	message(STATUS "Building without audio conversion support, please install FFmpeg with libswresample")
endif()

if(NOT BUILD_FRAMEWORK)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libchromaprint.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/libchromaprint.pc)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libchromaprint.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_definitions(
	-DHAVE_CONFIG_H
	-D_SCL_SECURE_NO_WARNINGS
	-D_USE_MATH_DEFINES
	-D__STDC_LIMIT_MACROS
	-D__STDC_CONSTANT_MACROS
)

if(UNIX)
	link_libraries(m)
endif()

add_subdirectory(src)

if(BUILD_TESTS)
	add_subdirectory(tests)
endif(BUILD_TESTS)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cmake" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
add_custom_target(docs doxygen)
